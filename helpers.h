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

#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PROGN	"aerocli" /* program name */

struct options {
	short all;
	short dump;
	char *dump_fn;
};

void print_help();
void err_die(char *msg, ...);
void err_msg(char *msg, ...);
void init_opts(struct options *opts);
void print_heading(char *text);
void parse_cmdline(struct options *opts, int argc, char *argv[]);

#endif /* HELPERS_H_ */
