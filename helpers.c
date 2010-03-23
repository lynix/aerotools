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

#include "helpers.h"

void err_die(char *msg, ...)
{
	va_list argpointer;

	va_start(argpointer, msg);
	fprintf(stderr, "%s: ", PROGN);
	vfprintf(stderr, msg, argpointer);
	fprintf(stderr, "\n");
	fflush(stderr);

	exit(EXIT_FAILURE);
}

void err_msg(char *msg, ...)
{
	va_list argpointer;

	va_start(argpointer, msg);
	fprintf(stderr, "%s: ", PROGN);
	vfprintf(stderr, msg, argpointer);
	fprintf(stderr, "\n");
	fflush(stderr);

	return;
}

void print_heading(char *text)
{
	printf(":: %s\n", text);

	return;
}

void init_opts(struct options *opts)
{
	opts->all = 0;
	opts->dump = 0;

	return;
}

void parse_cmdline(struct options *opts, int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; i++) {
		if (*(argv[i]) != '-') {
			continue;
		}
		switch (*(argv[i]+1)) {
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
			case 'a':
				opts->all = 1;
				break;
			case 'd':
				opts->dump = 1;
				opts->dump_fn = argv[i] + 3;
				break;
			default:
				err_die("invalid arguments. Try -h for help.");
				break;
		}
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

	printf("Usage:    %s [OPTIONS]\n\n", PROGN);

	printf("Options:\n");
	printf("  -a         print all data read from the device\n");
	printf("  -d FILE    dump the raw data-buffer to FILE\n");
	printf("  -h         display this usage and license information\n");

	printf("\n");
	printf("This version of %s was built on %s %s.\n", PROGN, __DATE__, __TIME__);
}
