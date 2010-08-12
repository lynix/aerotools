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
#include <errno.h>

/* globals */
struct usb_device *aq_usb_dev = NULL;

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
	unsigned char 	t;
	unsigned short 	s;

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

struct usb_device *aq_dev_find()
{
	struct usb_bus 		*bus;
	struct usb_device 	*dev, *ret;

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

int aq_dev_poll(char *buffer, char **err)
{
	int 	i, n;
	struct 	usb_dev_handle *handle;

	/* search device if not yet found */
	if (aq_usb_dev == NULL) {
		if ((aq_usb_dev = aq_dev_find()) == NULL) {
			*err = "no aquaero(R) device found";
			return -1;
		}
	}

	/* USB device initialization and configuration */
	if ((handle = usb_open(aq_usb_dev)) == NULL) {
	    *err = "failed to open device";
	    return -1;
	}
	if ((i = usb_detach_kernel_driver_np(handle, 0)) < 0) {
		if (i != -ENODATA) {
			if (i == -EPERM)
				*err = "failed to detach kernel driver (permission denied)";
			else
				*err = "failed to detach kernel driver";
			goto err_exit2;
		}
	}
	if ((i = usb_set_configuration(handle, AQ_USB_CONF)) < 0) {
		*err = "failed to set device configuration";
		goto err_exit2;
	}

	if ((i = usb_claim_interface(handle, 0)) < 0) {
		if (i == -EBUSY) {
			n = 1;
			while (n < AQ_USB_RETRIES) {
				sleep(AQ_USB_RETRY_DELAY);
				i = usb_claim_interface(handle, 0);
				if (i != -EBUSY)
					break;
			}
		}
		if (i < 0) {
			if (i == -EBUSY)
				*err = "failed to claim interface (device busy)";
			else
				*err = "failed to claim interface";
			goto err_exit2;
		}
	}

	if ((i = usb_interrupt_read(handle, AQ_USB_ENDP, buffer, AQ_BUFFS,
			AQ_USB_TIMEOUT)) != AQ_BUFFS) {
		n = 0;
		while (n < AQ_USB_RETRIES) {
			i = usb_interrupt_read(handle, AQ_USB_ENDP, buffer, AQ_BUFFS,
					AQ_USB_TIMEOUT);
			if (i == AQ_BUFFS)
				break;
			else if (i < 0)
				goto err_exit1;
		}
		if (i != AQ_BUFFS)
			goto err_exit1;
	}

	usb_release_interface(handle, 0);
	usb_close(handle);

	return 0;

err_exit1: usb_release_interface(handle, 0);
err_exit2: usb_close(handle);
	return -1;
}

struct aquaero_data *aquaero_poll_data(char *buffer, char **err_msg)
{
	int		i;
	char 	*raw_data;
	struct	aquaero_data *ret_data;

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
	if ((i = aq_dev_poll(raw_data, err_msg)) < 0) {
		if (buffer == NULL)
			free(raw_data);
		return NULL;
	}

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
