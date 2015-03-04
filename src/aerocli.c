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

#include "aerocli.h"

int main(int argc, char *argv[])
{
	int		i, r = EXIT_SUCCESS;
	char	*err = NULL;
    aquaero_data aq_data;

    if ((argc == 2) && (strcmp(argv[1], "-h") == 0)) {
    	print_help();
    	exit(EXIT_SUCCESS);
    }

    /* poll current data */
	if (aquaero_init(&err) != 0)
		err_die(err);
	if (aquaero_poll_data(&aq_data, &err) < 0) {
		err_msg(err);
		aquaero_exit();
		exit(EXIT_FAILURE);
	}

	if (argc == 1)
    	print_summary(&aq_data);
	else
		for (i = 1; i < argc; i++) {
			if (*(argv[i]) != '-')
				continue;

			if (strcmp(argv[i], "-a") == 0)
				print_all(&aq_data);
			else if (strcmp(argv[i], "-f") == 0)
				print_fans(&aq_data);
			else if (strcmp(argv[i], "-t") == 0)
				print_temps(&aq_data, 0);
			else if (strcmp(argv[i], "-d") == 0)
				r = dump_data(argv[i+1], aquaero_get_buffer());
			else if (strcmp(argv[i], "-T") == 0)
				r = sync_time(&err);
			else if (strcmp(argv[i], "-p") == 0)
				r = aquaero_load_profile(atoi(argv[i+1]), &err);
			else if (strncmp(argv[i], "-F", 2) == 0)
				r = set_fan_duty(atoi(argv[i] + 2), atoi(argv[i+1]), &err);
			else
				err_die("unknown parameter: %s", argv[i]);
		}

	aquaero_exit();
	if (err != NULL)
		err_msg(err);

	exit(r);
}

void print_fans(aquaero_data *data)
{
	int i;

	printf(":: Fan readings\n");
	for (i = 0; i < AQ_NUM_FAN; i++)
		printf("  %-10s %u%% @ %u rpm\n", data->fans[i].name, data->fans[i].duty,
				data->fans[i].rpm);
}

void print_temps(aquaero_data *data, char verbose)
{
	int i;

	printf(":: Temperatures\n");
	for (i = 0; i < AQ_NUM_TEMP; i++)
		if (data->temps[i].connected)
			printf("  %-10s %2.1f °C\n", data->temps[i].name,
					data->temps[i].value);
		else if (verbose)
			printf("  %-10s not connected\n", data->temps[i].name);
}

void print_flow(aquaero_data *data)
{
	printf(":: Flow sensor\n");
	if (data->flow.connected)
		printf("  %-10s %2.2f l/h\n", data->flow.name, data->flow.value);
	else
		printf("  %-10s not connected\n", data->flow.name);
}

void print_device(aquaero_data *data)
{
	printf(":: Device data\n");
	printf("  Name          %s\n", data->device.name);
	printf("  Firmware      %s\n", data->device.fw_name);
	printf("  OS Version    %u\n", data->device.os_version);
	printf("  Serial        %u\n", data->device.serial);
	printf("  Produced      20%02u-%02u\n", data->device.prod_year,
			data->device.prod_month);
	printf("  Flash count   %u\n", data->device.flash_count);
	printf("  Profile       %u\n", data->device.profile);
	printf("  Language      %s\n", data->device.language);
	printf("  Time          %s %02u:%02u:%02u\n", strday(data->device.time_d),
			data->device.time_h, data->device.time_m, data->device.time_s);
}

void print_summary(aquaero_data *data)
{
	print_fans(data);
	print_temps(data, 0);
	if (data->flow.connected)
		print_flow(data);
}

void print_all(aquaero_data *data)
{
	print_device(data);
	print_fans(data);
	print_temps(data, 1);
	print_flow(data);
}

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

char *strday(aq_byte day)
{
	switch (day) {
		case 0:
			return "Sun";
		case 1:
			return "Mon";
		case 2:
			return "Tue";
		case 3:
			return "Wed";
		case 4:
			return "Thu";
		case 5:
			return "Fri";
		case 6:
			return "Sat";
		default:
			return "???";
	}
}

void print_help()
{
	printf("%s Copyright (c) 2010-2011  lynix <lynix47@gmail.com>\n\n", PROGN);

	printf("Usage:  %s [OPTIONS]\n\n", PROGN);

	printf("Without any options, %s prints a summary of sensor, fan\n", PROGN);
	printf("and flow data.\n\n");

	printf("Reading Options:\n");
	printf("  -a    print all data read from the device\n");
	printf("  -t    print temperature readings only\n");
	printf("  -f    print fan readings only\n");

	printf("Writing Options:\n");
	printf("  -p  N      load profile N (1-2)\n");
	printf("  -FX Y      set fan X (1-4) to Y%% power\n");
	printf("  -T         synchronize time\n\n");

	printf("  -d  FILE   dump raw device buffer to FILE\n");
	printf("  -h         display usage information\n");

	printf("\n");
	printf("This %s version was built on %s %s.\n", PROGN, __DATE__, __TIME__);
}

int dump_data(char *file, unsigned char *buffer)
{
	FILE *fh;

	if ((fh = fopen(file, "w")) == NULL) {
		perror(file);
		return EXIT_FAILURE;
	}
	if (fwrite(buffer, 1, AQ_USB_READ_LEN, fh) != AQ_USB_READ_LEN) {
		perror(file);
		fclose(fh);
		return EXIT_FAILURE;
	}

	fclose(fh);

	return EXIT_SUCCESS;
}

int sync_time(char **err) {
	struct tm *ts;
	time_t now;

	now = time(0);
	ts = localtime(&now);

	return aquaero_set_time(ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_wday,
			err);
}

int set_fan_duty(char num, aq_byte duty, char **err_msg)
{
	printf("debug: setting fan %d to %d%%\n", num, duty);
	if (duty < 0 || duty > 100) {
		*err_msg = "invalid duty value, must be in [0,100]";
		return EXIT_FAILURE;
	}

	if (num < 1 || num > 4) {
		*err_msg = "invalid fan number, must be in [1,4]";
		return EXIT_FAILURE;
	}

	return  aquaero_set_fan_duty(num-1, duty, err_msg);
}
