/* Copyright lynix <lynix47@gmail.com>, 2010
 *
 * This file is part of aerocli.
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

#ifndef AEROD_H_
#define AEROD_H_

/* includes */
#include "libaquaero.h"
#include <stdio.h>
#include <stdarg.h>					/* variable arguments 	*/
#include <syslog.h>					/* syslog()				*/
#include <pthread.h>				/* pthreads				*/
#include <sys/socket.h>				/* socket definitions 	*/
#include <sys/types.h>				/* socket types			*/
#include <arpa/inet.h>				/* inet funtions		*/
#include <string.h>					/* strlen()				*/
#include <getopt.h>					/* getopt()				*/
#include <errno.h>					/* errno				*/
#include <signal.h>					/* signal handling		*/

/* program name */
#define PROGN			"aerod"

/* default settings */
#define PORT			7634
#define HDDTEMP_PORT	7634
#define HDDTEMP_HOST	"127.0.0.1"
#define INTERVAL		30
#define INTERVAL_MIN	5
#define INTERVAL_MAX	65535
#define PID_FILE		"/var/run/aerod.pid"
#define Q_LENGTH		1024
#define MAX_LINE		1024

/* cmdline options structure */
struct options {
	unsigned short 	port;
	unsigned short 	interval;
	unsigned int 	fork:1;
	unsigned int	hddtemp:1;
	unsigned int	pidfile:1;
};

/* functions */
void print_help();
void init_opts();
void parse_cmdline(int argc, char *argv[]);
void log_msg(int prio, char *msg, ...);
void err_die(char *msg, ...);
void signal_handler(int signal);
void send_data();
void die();
int	 tcp_start_server();
void *tcp_handler();
int	 poll_data();
char *poll_aquaero(char **err_msg);
char *poll_hddtemp(char *host, unsigned short port);
int	 write_pidfile(int pid);

#endif /* AEROD_H_ */
