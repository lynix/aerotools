/* Copyright lynix <lynix47@gmail.com>, 2010
 *
 * This file is part of aerotools.
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

#include "aerocli.h"

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
	opts->fan_rpm = 0;
	opts->fan_duty = 0;
	opts->temp = 0;

	return;
}

void parse_cmdline(struct options *opts, int argc, char *argv[])
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
			case 'a':
				opts->all = 1;
				break;
			case 's':
				n = atoi(argv[i] + 3);
				if (n < 1 || n > TEMP_NUM) {
					err_die("invalid sensor number");
				}
				opts->temp = n;
				break;
			case 'f':
				n = atoi(argv[i] + 4);
				if (n < 1 || n > FAN_NUM) {
					err_die("invalid fan number");
				}
				switch (*(argv[i]+2)) {
					case 'r':
						opts->fan_rpm = n;
						break;
					case 'd':
						opts->fan_duty = n;
						break;
					default:
						err_die("invalid fan property");
						break;
				}
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

	printf("Usage:  %s [OPTIONS]\n\n", PROGN);

	printf("Options:\n");
	printf("  -a         print all data read from the device\n");
	printf("  -fr N      print rpm of fan N (1<=N<=4)\n");
	printf("  -fd N      print duty of fan N (1<=N<=4)\n");
	printf("  -s  N      print temperature on sensor N (1<=N<=6)\n");
	printf("  -d  FILE   dump the raw data-buffer to FILE\n");
	printf("  -h         display this usage and license information\n");

	printf("\n");
	printf("This version of %s was built on %s %s.\n", PROGN, __DATE__, __TIME__);
}

void dump_data(char *file, char *buffer)
{
	int fh;

	if ((fh = open(file, O_WRONLY|O_CREAT, 0644)) < 0) {
		err_msg("failed to open %s for writing: %s", file, strerror(errno));
		return;
	}
	if (write(fh, buffer, BUFFS) != BUFFS) {
		err_msg("failed to write to %s: %s", file, strerror(errno));
	}
	close(fh);

	return;
}

int main(int argc, char *argv[])
{
	int i;
	double d;
	char buffer[BUFFS], *err;
    struct options opts;
    struct usb_device *aq_dev;
    struct usb_dev_handle *aq_handle;

    err = NULL;

    /* parse cmdline arguments */
    init_opts(&opts);
    parse_cmdline(&opts, argc, argv);

    /* setup device, read data, close device */
    if ((aq_dev = dev_find()) == NULL) {
    	err_die("no aquaero device found");
    }
    if ((aq_handle = dev_init(aq_dev, &err)) == NULL) {
    	err_die("failed to initialize device: %s", err);
    }
    if (dev_read(aq_handle, buffer) < 0) {
    	err_msg("failed to read from aquero");
    }
    if (dev_close(aq_handle) < 0) {
       	err_msg("failed to close device");
    }

    /* dump data if requested */
    if (opts.dump) {
    	dump_data(opts.dump_fn, buffer);
    }

    /* print only selected data if requested */
    if (opts.fan_duty) {
    	printf("%u%%\n", get_fan_duty(opts.fan_duty-1, buffer));
    	exit(EXIT_SUCCESS);
    }
    if (opts.fan_rpm) {
    	printf("%urpm\n", get_fan_rpm(opts.fan_rpm-1, buffer));
    	exit(EXIT_SUCCESS);
    }
    if (opts.temp) {
       	printf("%2.1f°C\n", get_temp_value(opts.temp-1, buffer));
       	exit(EXIT_SUCCESS);
    }

    /* print data */
    if (opts.all) {
    	print_heading("Device data");
    	printf("Name          %s\n", get_name(buffer));
    	printf("Firmware      %s\n", get_fw(buffer));
    	printf("OS Version    %u\n", get_os(buffer));
    	printf("Serial        %u\n", get_serial(buffer));
    	printf("Produced      20%02u-%02u\n", get_prod_year(buffer),
    			get_prod_month(buffer));
    	printf("Flash count   %u\n", get_flash_count(buffer));
    	putchar('\n');
    }
    print_heading("Fan sensors");
    for (i = 0; i < FAN_NUM; i++) {
    	/* TODO: handle disconnected ones */
    	printf("%-10s %u%% @ %u rpm\n", get_fan_name(i, buffer), get_fan_duty(i, buffer), get_fan_rpm(i, buffer));
    }
    putchar('\n');
    print_heading("Temp sensors");
    for (i = 0; i < TEMP_NUM; i++) {
    	d = get_temp_value(i, buffer);
    	if (d != TEMP_NCONN) {
    		printf("%-10s %2.1f°C\n", get_temp_name(i, buffer), d);
    	} else if (opts.all) {
    		printf("%-10s not connected\n", get_temp_name(i, buffer));
    	}
    }

    exit(EXIT_SUCCESS);
}
