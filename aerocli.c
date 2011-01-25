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
	opts->profile = 0;

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
				if (n < 1 || n > AQ_NUM_TEMP) {
					err_die("invalid sensor number");
				}
				opts->temp = n;
				break;
			case 'f':
				n = atoi(argv[i] + 4);
				if (n < 1 || n > AQ_NUM_FAN) {
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
			case 'p':
				n = atoi(argv[i] + 3);
				if (n != 1 && n != 2)		/* TODO: doubles with library */
					err_die("invalid profile number");
				opts->profile = n;
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
	printf("%s Copyright (c) 2010-2011  lynix <lynix47@gmail.com>\n\n", PROGN);

	printf("Usage:  %s [OPTIONS]\n\n", PROGN);

	printf("Options:\n");
	printf("  -a         print all data read from the device\n");
	printf("  -fr N      print rpm of fan N (1<=N<=4)\n");
	printf("  -fd N      print duty of fan N (1<=N<=4)\n");
	printf("  -s  N      print temperature on sensor N (1<=N<=6)\n");
	printf("  -p  N      load profile N (N in {1,2})\n");
	printf("  -d  FILE   dump the raw data-buffer to FILE\n");
	printf("  -h         display usage information\n");

	printf("\n");
	printf("This %s version was built on %s %s.\n", PROGN, __DATE__, __TIME__);
}

void dump_data(char *file, char *buffer, int buffsize)
{
	int fh;

	if ((fh = open(file, O_WRONLY|O_CREAT, 0644)) < 0) {
		err_msg("failed to open %s for writing: %s", file, strerror(errno));
		return;
	}
	if (write(fh, buffer, buffsize) != buffsize) {
		err_msg("failed to write to %s: %s", file, strerror(errno));
	}
	close(fh);

	return;
}

int main(int argc, char *argv[])
{
	int		i;
	char	*err;
    struct	options opts;
    aquaero_data aq_data;

    /* parse cmdline arguments */
    init_opts(&opts);
    parse_cmdline(&opts, argc, argv);

    /* initialize device communication stuff */
    if (aquaero_init(&err) != 0)
    	err_die(err);

    /* change profile if requested */
    if (opts.profile > 0) {
    	if (aquaero_load_profile(opts.profile, &err) != 0)
    		err_msg(err);
    	aquaero_exit();
    	exit(EXIT_SUCCESS);		/* TODO: fix return value */
    }

    /* poll data from device */
    if (aquaero_poll_data(&aq_data, &err) < 0)
    	err_die(err);

    /* dump data if requested */
    if (opts.dump)
    	dump_data(opts.dump_fn, (char *)aquaero_get_buffer(), AQ_USB_READ_LEN);

    /* finalize device communication stuff */
    aquaero_exit();

    /* print only selected data if requested */
    if (opts.fan_duty) {
    	printf("%u%%\n", aq_data.fans[opts.fan_duty-1].duty);
    	exit(EXIT_SUCCESS);
    }
    if (opts.fan_rpm) {
    	printf("%urpm\n", aq_data.fans[opts.fan_rpm-1].rpm);
    	exit(EXIT_SUCCESS);
    }
    if (opts.temp) {
       	printf("%2.1f°C\n", aq_data.temps[opts.temp-1].value);
       	exit(EXIT_SUCCESS);
    }

    /* print data */
    if (opts.all) {
    	print_heading("Device data");
    	printf("Name          %s\n", aq_data.device.name);
    	printf("Firmware      %s\n", aq_data.device.fw_name);
    	printf("OS Version    %u\n", aq_data.device.os_version);
    	printf("Serial        %u\n", aq_data.device.serial);
    	printf("Produced      20%02u-%02u\n", aq_data.device.prod_year,
    			aq_data.device.prod_month);
    	printf("Flash count   %u\n", aq_data.device.flash_count);
    	putchar('\n');
    	printf("Profile       %u\n", aq_data.device.profile);
    	printf("Language      %s\n", aq_data.device.language);
    	printf("Time          %u:%u:%u\n", aq_data.device.time_h,
    			aq_data.device.time_m, aq_data.device.time_s);
    	putchar('\n');
    }
    print_heading("Fan sensors");
    for (i = 0; i < AQ_NUM_FAN; i++) {
    	if (aq_data.fans[i].rpm > 0 || opts.all)
			printf("%-10s %u%% @ %u rpm\n", aq_data.fans[i].name,
					aq_data.fans[i].duty, aq_data.fans[i].rpm);
    }
    putchar('\n');
    print_heading("Temp sensors");
    for (i = 0; i < AQ_NUM_TEMP; i++) {
    	if (aq_data.temps[i].connected)
    		printf("%-10s %2.1f°C\n", aq_data.temps[i].name,
    				aq_data.temps[i].value);
    	else if (opts.all)
    		printf("%-10s not connected\n", aq_data.temps[i].name);
    }
    if (aq_data.flow.connected || opts.all) {
    	putchar('\n');
    	print_heading("Flow sensors");
    	if (aq_data.flow.connected) {
			printf("%-10s %2.2fl/h\n", aq_data.flow.name, aq_data.flow.value);
		} else {
			printf("%-10s not connected\n", aq_data.flow.name);
		}
    }

    exit(EXIT_SUCCESS);
}
