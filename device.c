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

/* globals */
struct usb_device *aq_usb_dev = NULL;


struct usb_device *aq_dev_find()
{
	struct usb_bus *bus;
	struct usb_device *dev, *ret;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	ret = NULL;

	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if ((dev->descriptor.idVendor == AQ_USB_VID) &&
					(dev->descriptor.idProduct == AQ_USB_PID)) {
				ret = dev;
			}
		}
	}

	return ret;
}

struct usb_dev_handle *aq_dev_init(char **err)
{
	struct usb_dev_handle *handle;
	int v;

	if ((handle = usb_open(aq_usb_dev)) == NULL) {
	    *err = "failed to open device";
	    return NULL;
	}
	if ((v = usb_detach_kernel_driver_np(handle, 0)) < 0) {
		if (v != AQ_ENODATA) {
			*err = "failed to dispatch kernel driver";
			usb_close(handle);
			return NULL;
		} /* ENODATA means no kernel driver present, nothing to detach */
	}
	/* TODO: implement retrying (AQ_USB_RETRIES, AQ_USB_RETRY_DELAY */
	if (usb_set_configuration(handle, AQ_USB_CONF) < 0) {
		*err = "failed to set configuration";
		usb_close(handle);
		return NULL;
	}
	/* TODO: handle EBUSY (see libusb trac wiki page) */
	if (usb_claim_interface(handle, 0) < 0) {
		*err = "failed to claim interface";
		usb_close(handle);
		return NULL;
	}

	return handle;
}

int	aq_dev_read(struct usb_dev_handle *devh, char *buffer)
{
	return usb_interrupt_read(devh, AQ_USB_ENDP, buffer, AQ_BUFFS,
			AQ_USB_TIMEOUT);
}

int	aq_dev_close(struct usb_dev_handle *devh)
{
	return usb_close(devh);
}

ushort aq_get_short(char *buffer, int offset)
{
	return (((unsigned char)buffer[offset]) << 8) +
			(unsigned char)buffer[offset + 1];
}

char *aq_get_string(char *buffer, int offset, int max_length)
{
	unsigned short i;

	i = offset + max_length;
	while ((buffer[i-1] == ' ') && (i > offset)) {
		i--;
	}
	buffer[i] = '\0';

	return buffer + offset;
}

char *aq_get_name(char *buffer)
{
	return aq_get_string(buffer, AQ_DEV_NAME_OFFS, AQ_DEV_NAME_LEN);
}

char *aq_get_fw(char *buffer)
{
	return aq_get_string(buffer, AQ_DEV_FW_OFFS, AQ_DEV_FW_LEN);
}

char aq_get_prod_year(char *buffer)
{
	return buffer[AQ_DEV_PROD_Y_OFFS];
}

char aq_get_prod_month(char *buffer)
{
	return buffer[AQ_DEV_PROD_M_OFFS];
}

ushort aq_get_flash_count(char *buffer)
{
	return aq_get_short(buffer, AQ_DEV_FLASHC_OFFS);
}

ushort aq_get_os_version(char *buffer)
{
	return aq_get_short(buffer, AQ_DEV_OS_OFFS);
}

char *aq_get_fan_name(char n, char *buffer)
{
	return aq_get_string(buffer, AQ_FAN_NAME_OFFS + (n * (AQ_FAN_NAME_LEN + 1)),
			AQ_FAN_NAME_LEN);
}

ushort aq_get_fan_rpm(char n, char *buffer)
{
	/* TODO: handle disconnected fans */
	return aq_get_short(buffer, AQ_FAN_RPM_OFFS + (n * AQ_FAN_RPM_LEN));
}

char aq_get_fan_duty(char n, char *buffer)
{
	/* TODO: simplify */
	unsigned char t;
	unsigned short s;

	t = *(buffer + AQ_FAN_PWR_OFFS + (n * AQ_FAN_PWR_LEN));
	s = (t * 100) >> 8;

	return (char)s;
}

char *aq_get_temp_name(char n, char *buffer)
{
	return aq_get_string(buffer, AQ_TEMP_NAME_OFFS + (n *
			(AQ_TEMP_NAME_LEN + 1)), AQ_TEMP_NAME_LEN);
}

double aq_get_temp_value(char n, char *buffer)
{
	unsigned short c;

	c = aq_get_short(buffer, AQ_TEMP_VAL_OFFS + (n * AQ_TEMP_VAL_LEN));

	return (c != AQ_TEMP_NAN) ? (double)c / 10 : -1;
}

ushort aq_get_serial(char *buffer)
{
	return aq_get_short(buffer, AQ_DEV_SERIAL_OFFS);
}

struct aquaero_data *aquaero_poll_data(char *buffer, char **err_msg)
{
	int		i;
	char 	*raw_data;
	struct 	usb_dev_handle *dev_handle;
	struct	aquaero_data *ret_data;

	/* search device if not specified */
	if (aq_usb_dev == NULL) {
		if ((aq_usb_dev = aq_dev_find()) == NULL) {
			*err_msg = "no aquaero(R) device found";
			return NULL;
		}
	}
	/* initialize device */
	if ((dev_handle = aq_dev_init(err_msg)) == NULL) {
		return NULL;
	}
	/* prepare data structure */
	if ((ret_data = malloc(sizeof(struct aquaero_data))) == NULL) {
		*err_msg = "out-of-memory allocating data structure";
		return NULL;
	}
	/* prepare data buffer */
	if (buffer == NULL) {
		if ((raw_data = malloc(AQ_BUFFS)) == NULL) {
			*err_msg = "out-of-memory allocating data buffer";
			return NULL;
		}
	} else {
		raw_data = buffer;
	}

	/* poll raw data */
	if ((i = aq_dev_read(dev_handle, raw_data)) < 0) {
		if (buffer == NULL) {
			free(raw_data);
		}
		/*TODO: bring in error code */
		*err_msg = "failed to read from device";
		return NULL;
	}
	aq_dev_close(dev_handle);

	/* process data, fill structure */
	ret_data->device_name = strdup(aq_get_name(raw_data));
	ret_data->firmware = strdup(aq_get_fw(raw_data));
	ret_data->prod_year = aq_get_prod_year(raw_data);
	ret_data->prod_month = aq_get_prod_month(raw_data);
	ret_data->device_serial = aq_get_serial(raw_data);
	ret_data->flash_count = aq_get_flash_count(raw_data);
	ret_data->os_version = aq_get_os_version(raw_data);
	for (i = 0; i < AQ_FAN_NUM; i++) {
		ret_data->fan_names[i] = strdup(aq_get_fan_name(i, raw_data));
		ret_data->fan_rpm[i] = aq_get_fan_rpm(i, raw_data);
		ret_data->fan_duty[i] = aq_get_fan_duty(i, raw_data);
	}
	for (i = 0; i < AQ_TEMP_NUM; i++) {
		ret_data->temp_names[i] = strdup(aq_get_temp_name(i, raw_data));
		ret_data->temp_values[i] = aq_get_temp_value(i, raw_data);
	}

	/* our own allocated data buffer, free */
	if (buffer == NULL) {
		free(raw_data);
	}

	return ret_data;
}
