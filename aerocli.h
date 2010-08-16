/* Copyright lynix <lynix47@gmail.com>, 2010
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

#ifndef AEROCLI_H_
#define AEROCLI_H_

/* includes */
#include "device.h"
#include <stdio.h>
#include <sys/stat.h>		/* open() */
#include <fcntl.h>			/* open() */
#include <string.h>			/* strerror() */
#include <errno.h>			/* int errno */
#include <stdarg.h>			/* err_msg(), err_die() */
#include <stdlib.h>			/* exit() */
#include <unistd.h>			/* write(), close() */

/* program name */
#define PROGN				"aerocli"

/* cmdline options structure */
struct options {
	short	all;
	short	fan_rpm;
	short	fan_duty;
	short	temp;
	short	dump;
	char	*dump_fn;
};

/* functions */
void print_help();
void err_die(char *msg, ...);
void err_msg(char *msg, ...);
void init_opts(struct options *opts);
void print_heading(char *text);
void parse_cmdline(struct options *opts, int argc, char *argv[]);
void dump_data(char *file, char *buffer, int buffsize);

#endif /* AEROCLI_H_ */
