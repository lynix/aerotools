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

#include "device.h"

struct usb_device *dev_find()
{
	struct usb_bus *bus;
	struct usb_device *dev, *ret;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	ret = NULL;

	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if ((dev->descriptor.idVendor == USB_VID) &&
					(dev->descriptor.idProduct == USB_PID)) {
				ret = dev;
			}
		}
	}

	return ret;
}

struct usb_dev_handle *dev_init(struct usb_device *dev, char **err)
{
	struct usb_dev_handle *handle;
	int v;

	if ((handle = usb_open(dev)) == NULL) {
	    *err = "failed to open device";
	    return NULL;
	}
	if ((v = usb_detach_kernel_driver_np(handle, 0)) < 0) {
		if (v != ENODATA) {
			*err = "failed to dispatch kernel driver";
			usb_close(handle);
			return NULL;
		} /* ENODATA means no kernel driver present, nothing to detach */
	}
	if (usb_set_configuration(handle, USB_CONF) < 0) {
		*err = "failed to set configuration";
		usb_close(handle);
		return NULL;
	}
	if (usb_claim_interface(handle, 0) < 0) {
		*err = "failed to claim interface";
		usb_close(handle);
		return NULL;
	}

	return handle;
}

int	dev_read(struct usb_dev_handle *devh, char *buffer)
{
	return usb_interrupt_read(devh, USB_ENDP, buffer, BUFFS, USB_TIMEOUT);
}

int	dev_close(struct usb_dev_handle *devh)
{
	return usb_close(devh);
}

ushort get_short(char *buffer, int offset)
{
	return (((unsigned char)buffer[offset]) << 8) +
			(unsigned char)buffer[offset + 1];
}

char *get_string(char *buffer, int offset, int max_length)
{
	unsigned short i;

	i = offset + max_length;
	while ((buffer[i-1] == ' ') && (i > offset)) {
		i--;
	}
	buffer[i] = '\0';

	return buffer + offset;
}

char *get_name(char *buffer)
{
	return get_string(buffer, DEV_NAME_OFFS, DEV_NAME_LEN);
}

char *get_fw(char *buffer)
{
	return get_string(buffer, DEV_FW_OFFS, DEV_FW_LEN);
}

char get_prod_year(char *buffer)
{
	return buffer[DEV_PROD_Y_OFFS];
}

char get_prod_month(char *buffer)
{
	return buffer[DEV_PROD_M_OFFS];
}

ushort get_flash_count(char *buffer)
{
	return get_short(buffer, DEV_FLASHC_OFFS);
}

ushort get_os(char *buffer)
{
	return get_short(buffer, DEV_OS_OFFS);
}

char *get_fan_name(char n, char *buffer)
{
	return get_string(buffer, FAN_NAME_OFFS + (n * (FAN_NAME_LEN + 1)),
			FAN_NAME_LEN);
}

ushort get_fan_rpm(char n, char *buffer)
{
	/* TODO: handle disconnected fans */
	return get_short(buffer, FAN_RPM_OFFS + (n * FAN_RPM_LEN));
}

char get_fan_duty(char n, char *buffer)
{
	/* TODO: simplify */
	unsigned char t;
	unsigned short s;

	t = *(buffer + FAN_PWR_OFFS + (n * FAN_PWR_LEN));
	s = (t * 100) >> 8;

	return (char)s;
}

char *get_temp_name(char n, char *buffer)
{
	return get_string(buffer, TEMP_NAME_OFFS + (n * (TEMP_NAME_LEN + 1)),
			TEMP_NAME_LEN);
}

double get_temp_value(char n, char *buffer)
{
	unsigned short c;

	c = get_short(buffer, TEMP_VAL_OFFS + (n * TEMP_VAL_LEN));

	return (c != TEMP_NAN) ? (double)c / 10 : -1;
}

ushort get_serial(char *buffer)
{
	return get_short(buffer, DEV_SERIAL_OFFS);
}
