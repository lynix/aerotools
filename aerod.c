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

int main(int argc, char *argv[])
{
	int			pid, sid, one;
	char		*err;
	struct		usb_device *aq_dev;
	struct		sockaddr_in servaddr;
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
    			if (write_pidf(pid) != 0) {
    				err_msg(LOG_ERR, "failed to write pidfile (%s)", PIDF);
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
    if (pthread_mutex_init(&data_lock, NULL) != 0) {
    	err_msg(LOG_ERR, "faiuled to setup mutex, terminating");
    	exit(EXIT_FAILURE);
	}

    /* setup device, initial poll */
	err = NULL;
	if ((aq_dev = dev_find()) == NULL) {
		err_msg(LOG_ERR, "no aquaero device found, terminating");
		exit(EXIT_FAILURE);
	}
	if ((aq_handle = dev_init(aq_dev, &err)) == NULL) {
		err_msg(LOG_ERR, "failed to initialize device (%s), terminating", err);
		exit(EXIT_FAILURE);
	}

    /* start TCP server, thread */
	if ((list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		err_msg(LOG_ERR, "error creating listening socket, terminating");
		dev_close(aq_handle);
		exit(EXIT_FAILURE);
    }
	/* Allow local port reuse in TIME_WAIT TODO: check*/
	one = 1;
	setsockopt(list_s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port  = htons(opts.port);
    if (bind(list_s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    	err_msg(LOG_ERR, "error binding server socket: %s", strerror(errno));
    	die();
    }
	if (listen(list_s, LISTENQ) < 0) {
		err_msg(LOG_ERR, "error started listening, terminating");
		die();
	} else {
		err_msg(LOG_INFO, "listening on port %d", opts.port);
	}
	if (pthread_create(&tcp_thread, NULL, tcp_serve, &list_s) != 0) {
		err_msg(LOG_ERR, "error spawning listen-thread, terminating");
		die();
	}

	/* start infinite polling-loop */
	while (1) {
		if (poll_data(aq_handle) != 0) {
			err_msg(LOG_ERR, "failed to read from device, terminating");
			die();
		}
		sleep(opts.interval);
	}

    dev_close(aq_handle);
    exit(EXIT_SUCCESS);
}

int poll_data(struct usb_dev_handle *dh)
{
	char *buffer;

	if ((buffer = malloc(BUFFS)) == NULL) {
		return -1;
	}
	if (dev_read(dh, buffer) < 0) {
		return -1;
	}

	/* critical section for data */
	pthread_mutex_lock(&data_lock);
	if (data != NULL) {
		free(data);
	}
	if ((data = malloc(249)) == NULL) {
		return -1;
	}
	/*sprintf(data, "|/dev/fan1|%s|%u|X|\n|/dev/fan2|%s|%u|X|\n|/dev/fan3|%s|%u|X|\n|/dev/fan4|%s|%u|X|\n|/dev/temp1|%s|%.0f|C|\n|/dev/temp2|%s|%.0f|C|\n|/dev/temp3|%s|%.0f|C|\n|/dev/temp4|%s|%.0f|C|\n|/dev/temp5|%s|%.0f|C|\n|/dev/temp6|%s|%.0f|C|",
			get_fan_name(0, buffer), get_fan_rpm(0, buffer), get_fan_name(1, buffer), get_fan_rpm(1, buffer), get_fan_name(2, buffer), get_fan_rpm(2, buffer), get_fan_name(3, buffer), get_fan_rpm(3, buffer),
			get_temp_name(0, buffer), get_temp_value(0, buffer), get_temp_name(1, buffer), get_temp_value(1, buffer), get_temp_name(2, buffer), get_temp_value(2, buffer),
			get_temp_name(3, buffer), get_temp_value(3, buffer), get_temp_name(4, buffer), get_temp_value(4, buffer), get_temp_name(5, buffer), get_temp_value(5, buffer));*/
	/* TODO: fix this line mess, use for-loops etc. */
	sprintf(data, "|/dev/temp5|%s|%.0f|C||/dev/temp6|%s|%.0f|C|",
				get_temp_name(4, buffer), get_temp_value(4, buffer), get_temp_name(5, buffer), get_temp_value(5, buffer));
	pthread_mutex_unlock(&data_lock);
	/* end critical section */

	free(buffer);

	return 0;
}

void *tcp_serve(void *list_s)
{
	conn_s = -1;

	while (1) {
		if ((conn_s = accept(*((int *)list_s), NULL, NULL)) < 0) {
		    break;
		}
		pthread_mutex_lock(&data_lock);
		send_data();
		pthread_mutex_unlock(&data_lock);
		close(conn_s);
	}

	return NULL;
}

void send_data()
{
    size_t nleft;
    ssize_t nwritten;
    const char *buffer;

    buffer = data;
    nleft  = strlen(data);
    while (nleft > 0) {
    	if ((nwritten = write(conn_s, buffer, nleft)) <= 0) {
    		if (errno == EINTR) {
    			nwritten = 0;
    		} else {
    			return;
    		}
    	}
    	nleft -= nwritten;
    	buffer += nwritten;
    }

    return;
}

int  write_pidf(int pid)
{
	FILE *file;

	if ((file = fopen(PIDF, "w")) != NULL) {
		fprintf(file, "%d", pid);
		fclose(file);
	} else return -1;

	return 0;
}

void signal_handler(int sig)
{
	switch(sig) {
		case SIGTERM:
			err_msg(LOG_WARNING, "received SIGTERM signal, exiting");
			die();
			break;
		default:
			err_msg(LOG_WARNING, "unhandled signal %d", sig);
			break;
	}

	return;
}

void die()
{
	if (conn_s >= 0) {
		shutdown(conn_s, 2);
	}
	close(conn_s);
	shutdown(list_s, 2);
	close(list_s);
	dev_close(aq_handle);
	closelog();
	unlink(PIDF);

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
				if (n < INTER_MIN || n > INTER_MAX) {
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
	printf("This version of %s was built on %s %s.\n", PROGN, __DATE__, __TIME__);
}
