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

#include "aerocli.h"


struct usb_device *dev_find(void)
{
	struct usb_bus *bus;
	struct usb_device *dev, *ret;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	ret = NULL;

	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if ((dev->descriptor.idVendor == USB_VID) &&  (dev->descriptor.idProduct == USB_PID)) {
				ret = dev;
			}
		}
	}

	if (ret == NULL) {
		err_die("couldn't find any aquaero device");
	}

	return ret;
}

struct usb_dev_handle *dev_init(struct usb_device *dev)
{
	struct usb_dev_handle *handle;

	if ((handle = usb_open(dev)) == NULL) {
	    err_die("couldn't open device");
	}
	if (usb_detach_kernel_driver_np(handle, 0) < 0) {
	 	err_msg("couldn't detach kernel driver");
	}
	if (usb_set_configuration(handle, USB_CONF) < 0) {
	   	err_msg("unable to set configuration");
	}
	if (usb_claim_interface(handle, 0) < 0) {
		usb_close(handle);
	   	err_die("couldn't claim interface");
	}

	return handle;
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

char *get_name(char *buffer)
{
	buffer[DEV_NAME_OFFS + DEV_NAME_LEN] = '\0';

	return buffer + DEV_NAME_OFFS;
}

char *get_fw(char *buffer)
{
	buffer[DEV_FW_OFFS + DEV_FW_LEN] = '\0';

	return buffer + DEV_FW_OFFS;
}

char *get_product(char *buffer)
{
	//TODO: implement
	return "not implemented yet";
}

char *get_fan_name(char n, char *buffer)
{
	buffer[FAN_NAME_OFFS + (n * (FAN_NAME_LEN+1)) + FAN_NAME_LEN] = '\0';

	return buffer + FAN_NAME_OFFS + (n * (FAN_NAME_LEN+1));
}

int get_fan_rpm(char n, char *buffer)
{
	return ((int)buffer[FAN_RPM_OFFS + (n * FAN_RPM_LEN)] << 8) + 256 + (int)buffer[FAN_RPM_OFFS + (n * FAN_RPM_LEN) + 1];
}

char get_fan_duty(char n, char *buffer)
{
	int t;
	char r;

	t = buffer[FAN_PWR_OFFS + (n * FAN_PWR_LEN)];
	t = t&0xFF;
	r = (t * 100) >> 8;

	return r;
}

char *get_temp_name(char n, char *buffer)
{
	buffer[TEMP_NAME_OFFS + (n * (TEMP_NAME_LEN+1)) + TEMP_NAME_LEN] = '\0';

	return buffer + TEMP_NAME_OFFS + (n * (TEMP_NAME_LEN+1));
}

double get_temp_value(char n, char *buffer)
{
	return (double)((buffer[TEMP_VAL_OFFS + (n * TEMP_VAL_LEN)] << 8) + buffer[TEMP_VAL_OFFS + (n * TEMP_VAL_LEN) + 1]) / 10;
}

int get_serial(char *buffer)
{
	//TODO: implement
	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	float f;
	char buffer[BUFFS];
    struct options opts;
    struct usb_device *aq_dev;
    struct usb_dev_handle *aq_handle;

	/* parse cmdline arguments */
    init_opts(&opts);
    parse_cmdline(&opts, argc, argv);

    /* setup device, read data */
    aq_dev = dev_find();
    aq_handle = dev_init(aq_dev);
    if (usb_interrupt_read(aq_handle, USB_ENDP, buffer, BUFFS, USB_TIMEOUT) < 0) {
    	err_msg("failed to read from aquero");
    }
    if (usb_close(aq_handle) < 0) {
       	err_msg("failed to close device");
    }

    /* dump data if requested */
    if (opts.dump) {
    	dump_data(opts.dump_fn, buffer);
    }

    /* print data */
    if (opts.all) {
    	print_heading("Device data");
    	printf("Name:      %s\n", get_name(buffer));
    	printf("Firmware:  %s\n", get_fw(buffer));
    	printf("Serial:    %d\n", get_serial(buffer));
    	printf("Produced:  %s\n", get_product(buffer));
    }
    print_heading("Fans");
    for (i = 0; i < FAN_NUM; i++) {
    	printf("%s: %u%% @ %d rpm\n", get_fan_name(i, buffer), get_fan_duty(i, buffer), get_fan_rpm(i, buffer));
    }
    print_heading("Temperatures");
    for (i = 0; i < TEMP_NUM; i++) {
    	if ((f = get_temp_value(i, buffer)) == TEMP_NAN) {
    		continue;
    	}
       	printf("%s: %.2f°C\n", get_temp_name(i, buffer), f);
    }

    exit(EXIT_SUCCESS);
}
