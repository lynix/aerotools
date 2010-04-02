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

#ifndef AEROD_H_
#define AEROD_H_

/* includes */
#include "device.h"
#include <stdio.h>
#include <stdarg.h>			/* err_msg() */
#include <syslog.h>			/* syslog() */
#include <pthread.h>        /* pthreads */
#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <string.h>			/* strlen() */
#include <errno.h>			/* errno */
#include <signal.h>			/* signal handling */


/* program name */
#define PROGN	"aerod"

/* default settings */
#define PORT		7634
#define INTERVAL	30
#define INTER_MIN	5
#define INTER_MAX	86400
#define PIDF		"/var/run/aerod.pid"
#define LISTENQ        (1024)   /*  Backlog for listen()   */

/* cmdline options structure */
struct options {
	int port;
	int interval;
	int fork;
};

/* globals */
char *data;
pthread_mutex_t data_lock;
struct options opts;
int       list_s;                /*  listening socket          */
int       conn_s;                /*  connection socket         */
struct 	usb_dev_handle *aq_handle;


/* functions */
void print_help();
void init_opts();
void parse_cmdline(int argc, char *argv[]);
void err_msg(int prio, char *msg, ...);
void *tcp_serve(void *list_s);
void signal_handler(int sig);
void send_data();
void die();
int	 poll_data(struct usb_dev_handle *dh);
int  write_pidf(int pid);

#endif /* AEROD_H_ */
