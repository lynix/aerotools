/* Copyright lynix <lynix47@gmail.com>, 2010
 *
 * This file is part of aerocli.
 *
 * aerocli is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * aerocli is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with aerocli. If not, see <http://www.gnu.org/licenses/>.
 */

#include "aerod.h"

/* globals */
int     		server_sock;
int     		connection_sock;
char			*data_buffer;
struct			options opts;
pthread_mutex_t data_buffer_lock;

int main(int argc, char *argv[])
{
	int			pid, sid;
	const int	one = 1;
	struct		sockaddr_in server_addr;
	pthread_t	tcp_thread;

    /* parse cmdline arguments */
    init_opts();
    parse_cmdline(argc, argv);

    /* daemonize */
    if (opts.fork) {
    	pid = fork();
    	switch (pid) {
    		case -1:
				/* fork failure */
				err_msg(LOG_ERR, "failed to fork into background");
				exit(EXIT_FAILURE);
				break;
    		case 0:
    			/* child */
    			sid = setsid();
    			err_msg(LOG_INFO, "started by user %d", getuid());
    			break;
    		default:
    			/* parent */
    			if (write_pidfile(pid) != 0) {
    				err_msg(LOG_ERR, "failed to write pidfile (%s)", PID_FILE);
    			}
    			exit(EXIT_SUCCESS);
    			break;
    	}
    }

    /* register signals */
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);

    /* setup data sync mutex */
    if (pthread_mutex_init(&data_buffer_lock, NULL) != 0) {
    	err_msg(LOG_ERR, "failed to setup mutex, terminating");
    	exit(EXIT_FAILURE);
	}

    /* initial poll */
    if (poll_data() != 0) {
    	exit(EXIT_FAILURE);
    }

    /* start TCP server */
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		err_msg(LOG_ERR, "error creating server socket, terminating");
		exit(EXIT_FAILURE);
    }
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port  = htons(opts.port);
    if (bind(server_sock, (struct sockaddr *)&server_addr,
    		sizeof(server_addr)) < 0) {
    	err_msg(LOG_ERR, "error binding server socket: %s", strerror(errno));
    	die();
    }
	if (listen(server_sock, Q_LENGTH) < 0) {
		err_msg(LOG_ERR, "error started listening, terminating");
		die();
	} else {
		err_msg(LOG_INFO, "listening on port %d", opts.port);
	}
	if (pthread_create(&tcp_thread, NULL, tcp_serve, NULL) != 0) {
		err_msg(LOG_ERR, "error spawning listen-thread, terminating");
		die();
	}

	/* start infinite polling-loop */
	while (1) {
		if (poll_data() != 0) {
			err_msg(LOG_ERR, "failed to read from device, terminating");
			die();
		}
		sleep(opts.interval);
	}
}

int poll_data()
{
	char 	*raw_buffer, *temp_data, *position, *error, i;
	struct	usb_device *aq_dev;
	struct 	usb_dev_handle *aq_handle;
	double 	d;

	/* setup read buffer */
	if ((raw_buffer = malloc(BUFFS)) == NULL) {
		return -1;
	}

	/* setup device, read raw data */
	error = NULL;
	if ((aq_dev = dev_find()) == NULL) {
		err_msg(LOG_ERR, "no aquaero device found, terminating");
		return -1;
	}
	if ((aq_handle = dev_init(aq_dev, &error)) == NULL) {
		err_msg(LOG_ERR, "failed to initialize device (%s), terminating",
				error);
		return -1;
	}
	if (dev_read(aq_handle, raw_buffer) < 0) {
		dev_close(aq_handle);
		return -1;
	}
	dev_close(aq_handle);

	/* process data */
	if ((temp_data = malloc(DATA_MAX_LEN)) == NULL) {
		free(raw_buffer);
		return -1;
	}
	position = temp_data;
	for (i = 0; i < TEMP_NUM; i++) {
		if ((d = get_temp_value(i, raw_buffer)) != TEMP_NCONN) {
			sprintf(position, "|/dev/temp%d|%s|%.0f|C|", i+1,
					get_temp_name(i, raw_buffer), d);
			position = temp_data + strlen(temp_data);
		}
	}
	temp_data = realloc(temp_data, strlen(temp_data)+1);

	/* begin critical section for data_buffer */
	pthread_mutex_lock(&data_buffer_lock);
	if (data_buffer != NULL) {
		free(data_buffer);
	}
	data_buffer = temp_data;
	pthread_mutex_unlock(&data_buffer_lock);
	/* end critical section for data_buffer*/

	free(raw_buffer);

	return 0;
}

void *tcp_serve()
{
	connection_sock = -1;

	while (1) {
		if ((connection_sock = accept(server_sock, NULL, NULL)) < 0) {
		    break;
		}
		send_data();
		close(connection_sock);
	}

	return NULL;
}

void send_data()
{
    size_t 		bytes_left;
    ssize_t 	bytes_written;
    const char  *write_pointer;

    pthread_mutex_lock(&data_buffer_lock);
    write_pointer = data_buffer;
    bytes_left  = strlen(data_buffer);
    while (bytes_left > 0) {
    	if ((bytes_written = write(connection_sock, write_pointer,
    			bytes_left)) <= 0) {
    		if (errno == EINTR) {
    			bytes_written = 0;
    		} else {
    			return;
    		}
    	}
    	bytes_left -= bytes_written;
    	write_pointer += bytes_written;
    }
    pthread_mutex_unlock(&data_buffer_lock);

    return;
}

int write_pidfile(int pid)
{
	FILE *file;

	if ((file = fopen(PID_FILE, "w")) != NULL) {
		fprintf(file, "%d", pid);
		fclose(file);
	} else return -1;

	return 0;
}

void signal_handler(int signal)
{
	switch(signal) {
		case SIGTERM:
			err_msg(LOG_WARNING, "received SIGTERM signal, terminating");
			die();
			break;
		default:
			err_msg(LOG_WARNING, "unhandled signal %d", signal);
			break;
	}

	return;
}

void die()
{
	if (connection_sock >= 0) {
		shutdown(connection_sock, 2);
	}
	close(connection_sock);
	shutdown(server_sock, 2);
	close(server_sock);
	closelog();
	unlink(PID_FILE);

	exit(EXIT_SUCCESS);
}

void init_opts()
{
	opts.port = PORT;
	opts.interval = INTERVAL;
	opts.fork = 1;

	return;
}

void parse_cmdline(int argc, char *argv[])
{
	int i, n;

	for (i = 1; i < argc; i++) {
		if (*(argv[i]) != '-') {
			continue;
		}
		switch (*(argv[i]+1)) {
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
			case 'F':
				opts.fork = 0;
				break;
			case 'p':
				n = atoi(argv[i] + 3);
				if (n < 1 || n > 65535) {
					err_msg(0, "invalid port: %d", n);
					exit(EXIT_FAILURE);
				}
				opts.port = n;
				break;
			case 'i':
				n = atoi(argv[i] + 3);
				if (n < INTERVAL_MIN || n > INTERVAL_MAX) {
					err_msg(0, "invalid interval: %d", n);
					exit(EXIT_FAILURE);
				}
				opts.interval = n;
				break;
			default:
				err_msg(0, "invalid arguments. Try -h for help.");
				break;
		}
	}

	return;
}

void err_msg(int prio, char *msg, ...)
{
	va_list argpointer;

	va_start(argpointer, msg);
	if (opts.fork && (prio > 0)) {
		vsyslog(prio|LOG_DAEMON, msg, argpointer);
	} else {
		fprintf(stderr, "%s: ", PROGN);
		vfprintf(stderr, msg, argpointer);
		fprintf(stderr, "\n");
		fflush(stderr);
	}

	return;
}

void print_help()
{
	printf("%s  Copyright (c) 2010  lynix <lynix47@gmail.com>\n\n", PROGN);

	printf("This program comes with ABSOLUTELY NO WARRANTY, use at\n");
	printf("own risk. This is free software, and you are welcome to\n");
	printf("redistribute it under the terms of the GNU General\n");
	printf("Public License as published by the Free Software\n");
	printf("Foundation, either version 3 of the License, or (at your\n");
	printf("option) any later version.\n\n");

	printf("Usage:  %s [OPTIONS]\n\n", PROGN);

	printf("Options:\n");
	printf("  -p   port to listen to (default %d)\n", PORT);
	printf("  -i   interval for polling in seconds (default: %d)\n", INTERVAL);
	printf("  -F   don't daemonize, stay in foreground\n");
	printf("  -h   display this usage and license information\n");

	printf("\n");
	printf("This version of %s was built on %s %s.\n", PROGN, __DATE__,
			__TIME__);
}
