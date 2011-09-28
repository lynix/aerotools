/* Copyright 2010-2011 lynix <lynix47@gmail.com>
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
#include "libaquaero.h"
#include <stdio.h>
#include <string.h>			/* strerror() */
#include <errno.h>			/* int errno */
#include <stdarg.h>			/* err_msg(), err_die() */
#include <stdlib.h>			/* exit() */
#include <time.h>

/* program name */
#define PROGN				"aerocli"

/* functions */
void print_help();
void print_all(aquaero_data *data);
void print_summary(aquaero_data *data);
void print_device(aquaero_data *data);
void print_flow(aquaero_data *data);
void print_temps(aquaero_data *data, char verbose);
void print_fans(aquaero_data *data);
void err_die(char *msg, ...);
void err_msg(char *msg, ...);
char *strday(aq_byte day);
int  dump_data(char *file, unsigned char *buffer);
int  sync_time(char **err);
int  set_fan_duty(char num, aq_byte duty, char **err_msg);

#endif /* AEROCLI_H_ */
