/* Copyright 2010-2012 lynix <lynix47@gmail.com>
 *
 * This file is part of aerotools.
 *
 * aerotools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * aerotools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with aerotools. If not, see <http://www.gnu.org/licenses/>.
 */

#include "aerod.h"

/* globals */
int     		server_sock;
char			*data_str;
struct			options opts;
pthread_mutex_t data_lock;

int main(int argc, char *argv[])
{
	int				pid;
	pthread_t		tcp_thread;
	char			*err_msg;

    /* parse cmdline arguments */
    parse_cmdline(argc, argv);

    /* initialize device communication */
    err_msg = NULL;
	if (aquaero_init(&err_msg) != 0) {
		log_msg(LOG_ERR, err_msg);
		exit(EXIT_FAILURE);
	}

	/* sync device clock if requested */
	if (opts.sync_clock) {
		if (sync_time(&err_msg) != 0) {
			log_msg(LOG_ERR, err_msg);
			exit(EXIT_FAILURE);
		}
	}

    /* daemonize */
    if (opts.fork) {
    	pid = fork();
    	switch (pid) {
    		case -1:
				/* fork failure */
				log_msg(LOG_ERR, "failed to fork into background: %s",
						strerror(errno));
				exit(EXIT_FAILURE);
				break;
    		case 0:
    			/* child */
    			setsid();
    			log_msg(LOG_INFO, "started by user %d", getuid());
    			break;
    		default:
    			/* parent */
    			exit(EXIT_SUCCESS);
    			break;
    	}
    }

    /* write pid-file */
    if (opts.pidfile)
		if (write_pidfile(pid) != 0)
			log_msg(LOG_WARNING, "failed to write %s: %s", PID_FILE,
					strerror(errno));

    /* register signals */
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);

    /* setup data sync mutex */
    if (pthread_mutex_init(&data_lock, NULL) != 0)
    	err_die("failed to setup mutex, terminating");

    /* initial poll */
    if (poll_data() != 0)
    	die();

    /* start tcp server, spawn handler thread */
	if (tcp_start_server() != 0)
		err_die("error opening tcp server socket, terminating");
	if (pthread_create(&tcp_thread, NULL, tcp_handler, NULL) != 0)
		err_die("error spawning listen-thread, terminating");
	log_msg(LOG_INFO, "listening on port %d", opts.port);

	/* start infinite polling-loop */
	while (1) {
		if (poll_data() != 0)
			die();
		sleep(opts.interval);
	}

	return 0;
}

int tcp_start_server()
{
	const int	one = 1;
	struct		sockaddr_in server_addr;

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port  = htons(opts.port);
	if (bind(server_sock, (struct sockaddr *)&server_addr,
			sizeof(server_addr)) < 0) {
		return -1;
	}
	if (listen(server_sock, Q_LENGTH) < 0) {
		return -1;
	}

	return 0;
}

void *tcp_handler()
{
	int connection_sock;

	while ((connection_sock = accept(server_sock, NULL, NULL)) >= 0) {
		pthread_mutex_lock(&data_lock);
		if (data_str != NULL)
			send(connection_sock, data_str, strlen(data_str)+1, 0);
		pthread_mutex_unlock(&data_lock);
		close(connection_sock);
	}

	return NULL;
}

int poll_data()
{
	/* TODO: oh my, heavy refactoring needed here */
	char *poll_str, *hddtemp_data, *err_msg;

	if ((poll_str = poll_aquaero(&err_msg)) == NULL) {
		log_msg(LOG_ERR, "error reading from aquaero(R): %s", err_msg);
		return -1;
	}

	if (opts.hddtemp) {
		if ((hddtemp_data = poll_hddtemp(HDDTEMP_HOST, HDDTEMP_PORT)) == NULL)
			log_msg(LOG_ERR, "failed to retrieve data from hddtemp");
		else {
			if (poll_str != NULL) {
				if ((poll_str = realloc(poll_str, strlen(poll_str) +
						strlen(hddtemp_data) + 1)) == NULL) {
					log_msg(LOG_ERR, "out-of-memory joining data strings");
					return -1;
				}
				strcat(poll_str, hddtemp_data);
				free(hddtemp_data);
			} else
				poll_str = hddtemp_data;
		}
	}

	pthread_mutex_lock(&data_lock);
	free(data_str);
	data_str = poll_str;
	pthread_mutex_unlock(&data_lock);

	return 0;
}

char *poll_aquaero(char **err_msg)
{
	aquaero_data aq_data;
	int		i;
	char 	*aquaero_data_str, *position;

	/* setup device, read raw data */
	if (aquaero_poll_data(&aq_data, err_msg) < 0)
		return NULL;

	if ((aquaero_data_str = malloc(AQ_DATA_BUFLEN)) == NULL)
		return NULL;
	position = aquaero_data_str;

	/* fan rpm */
	for (i=0; i<AQ_NUM_FAN; i++) {
		/* TODO: OK to use other units that F/C in hddtemp? */
		sprintf(position, "|/dev/fan%d|%s|%d|C|", i+1, aq_data.fans[i].name,
				aq_data.fans[i].rpm);
		position = aquaero_data_str + strlen(aquaero_data_str);
	}
	/* fan percentage */
	for (i=0; i<AQ_NUM_FAN; i++) {
			sprintf(position, "|/dev/fan%dp|%s|%d|C|", i+1, aq_data.fans[i].name,
					aq_data.fans[i].duty);
			position = aquaero_data_str + strlen(aquaero_data_str);
		}
	/* temperature sensors */
	for (i=0; i<AQ_NUM_TEMP; i++) {
		if (!aq_data.temps[i].connected)
			continue;
		/* TODO: extract format strings to global DEFs */
		if (opts.precision)
			sprintf(position, "|/dev/temp%d|%s|%.1f|C|", i+1,
				aq_data.temps[i].name,	aq_data.temps[i].value);
		else
			sprintf(position, "|/dev/temp%d|%s|%.0f|C|", i+1,
							aq_data.temps[i].name,	aq_data.temps[i].value);
		position = aquaero_data_str + strlen(aquaero_data_str);
	}
	/* flow sensor */
	if (aq_data.flow.connected) {
		/* TODO: OK to use other units that F/C in hddtemp? */
		/* TODO: extract format strings to global DEFs */
		if (opts.precision)
			sprintf(position, "|/dev/flow|%s|%.1f|C|", aq_data.flow.name,
				aq_data.flow.value);
		else
			sprintf(position, "|/dev/flow|%s|%.0f|C|", aq_data.flow.name,
							aq_data.flow.value);
		position = aquaero_data_str + strlen(aquaero_data_str);
	}

	return aquaero_data_str;
}

char *poll_hddtemp(char *host, unsigned short port)
{
	char 	*hddtemp_buffer;
	int		client_sock, bytes_read;
	struct 	sockaddr_in server_addr;

	if ((hddtemp_buffer = calloc(MAX_LINE, 1)) == NULL)
		return NULL;

	/* tcp connection to hddtempd */
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host);
	server_addr.sin_port = htons(port);
	if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		free(hddtemp_buffer);
		return NULL;
	}
	if ((connect(client_sock, (struct sockaddr*) &server_addr,
			sizeof(server_addr))) < 0) {
		free(hddtemp_buffer);
		return NULL;
	}

	/* receive all available data */
	bytes_read = recv(client_sock, hddtemp_buffer, MAX_LINE, MSG_WAITALL);
	shutdown(client_sock, SHUT_RD);
	close(client_sock);

	if (bytes_read < 0) {
		free(hddtemp_buffer);
		hddtemp_buffer = NULL;
	} else {
		hddtemp_buffer = realloc(hddtemp_buffer, bytes_read+1);
		hddtemp_buffer[bytes_read] = '\0';
	}

	return hddtemp_buffer;
}

int sync_time(char **err) {
	struct tm *ts;
	time_t now;

	now = time(0);
	ts = localtime(&now);

	return aquaero_set_time(ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_wday,
			err);
}

int write_pidfile(int pid)
{
	FILE *file;

	if ((file = fopen(PID_FILE, "w")) == NULL) {
		return -1;
	}
	fprintf(file, "%d", pid);
	fclose(file);

	return 0;
}

void signal_handler(int signal)
{
	switch(signal) {
		case SIGTERM:
			log_msg(LOG_WARNING, "received SIGTERM, terminating");
			die();
			break;
		case SIGHUP:
			log_msg(LOG_WARNING, "received SIGHUP, terminating");
			die();
			break;
		case SIGINT:
			log_msg(LOG_WARNING, "received SIGINT, terminating");
			die();
			break;
		default:
			log_msg(LOG_WARNING, "unhandled signal: %d", signal);
			break;
	}

	return;
}

void die()
{
	close(server_sock);
	closelog();
	unlink(PID_FILE);
	aquaero_exit();

	exit(EXIT_SUCCESS);
}

void parse_cmdline(int argc, char *argv[])
{
	char 		c;
	int			n;
	extern int 	optind, optopt, opterr;

	/* init options */
	opts.port = PORT;
	opts.interval = INTERVAL;
	opts.fork = 1;
	opts.hddtemp = 0;
	opts.pidfile = 0;
	opts.sync_clock = 0;
	opts.hddtemp_port = HDDTEMP_PORT;
	opts.precision = 0;

	/* parse cmdline */
	while ((c = getopt(argc, argv, "hFftsPp:i:T:")) != -1) {
		switch (c) {
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
			case 'F':
				opts.fork = 0;
				break;
			case 'f':
				opts.pidfile = 1;
				break;
			case 't':
				opts.hddtemp = 1;
				break;
			case 'T':
				n = atoi(optarg);
				if (n < 1 || n > 65535) {
					log_msg(LOG_ERR, "invalid hddtemp port: %d", n);
					exit(EXIT_FAILURE);
				}
				opts.hddtemp_port = n;
				break;	
			case 'p':
				n = atoi(optarg);
				if (n < 1 || n > 65535) {
					log_msg(LOG_ERR, "invalid port: %d", n);
					exit(EXIT_FAILURE);
				}
				opts.port = n;
				break;
			case 'P':
				opts.precision = 1;
				break;
			case 's':
				opts.sync_clock = 1;
				break;
			case 'i':
				n = atoi(optarg);
				if (n < INTERVAL_MIN || n > INTERVAL_MAX) {
					log_msg(LOG_ERR, "invalid interval: %d", n);
					exit(EXIT_FAILURE);
				}
				opts.interval = n;
				break;
	        case '?':
	        	if (optopt == 'p'|| optopt == 'i') {
	        		fprintf (stderr, "option -%c requires an argument.\n",
	        				optopt);
	        	} else {
	        		log_msg(LOG_ERR, "unknown option \"-%c\". Try -h for help.",
	        				optopt);
				}
				exit(EXIT_FAILURE);
			default:
				log_msg(LOG_ERR, "invalid arguments. Try -h for help.");
				exit(EXIT_FAILURE);
		}
	}

	return;
}

void log_msg(int prio, char *msg, ...)
{
	FILE *out_fp;
	va_list argpointer;

	va_start(argpointer, msg);
	if (opts.fork) {
		vsyslog(prio|LOG_DAEMON, msg, argpointer);
	} else {
		out_fp = (prio < LOG_NOTICE) ? stderr : stdout;
		fprintf(out_fp, "%s: ", PROGN);
		vfprintf(out_fp, msg, argpointer);
		fprintf(out_fp, "\n");
		fflush(out_fp);
	}

	return;
}

void err_die(char *msg, ...)
{
	va_list argpointer;

	va_start(argpointer, msg);
	log_msg(LOG_ERR, msg, argpointer);
	die();
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
	printf("  -p PORT  port to listen on (default %d)\n", PORT);
	printf("  -i INT   interval for polling in seconds (default: %d)\n", INTERVAL);
	printf("  -P       extended precision, may violate hddtemp protocol\n");
	printf("  -F       don't daemonize, stay in foreground\n");
	printf("  -f FILE  write PID file (/var/run/%s.pid)\n", PROGN);
	printf("  -t       query hddtemp and include data\n");
	printf("  -T PORT  hddtemp port to query (default 7634)\n");
	printf("  -s       sync device clock with system time\n");
	printf("  -h       display this usage and license information\n");

	printf("\n");
	printf("This version of %s was built on %s %s.\n", PROGN, __DATE__,
			__TIME__);
}
