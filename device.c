/* Copyright lynix <lynix47@gmail.com>, 2010
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

#include "device.h"


/*
 * globals
 */

libusb_device 	*aq_usb_dev = NULL;
uchar 			*aq_data_buffer = NULL;


/*
 * helper functions
 */

ushort aq_get_ushort(uchar *buffer, int offset)
{
	return (((uchar)buffer[offset]) << 8) + (uchar)buffer[offset + 1];
}

char *aq_trim_str(uchar *buffer, int offset, int max_length)
{
	unsigned short i;

	i = offset + max_length;
	while (i > offset) {
		if (buffer[i-1] != ' ')
			break;
		i--;
	}
	buffer[i] = '\0';

	return (char *)(buffer + offset);
}


/*
 * device-specific wrapper functions
 */

char *aq_get_name()
{
	if (aq_data_buffer == NULL)
		return NULL;

	return strdup(aq_trim_str(aq_data_buffer, AQ_DEV_NAME_OFFS,
			AQ_DEV_NAME_LEN));
}

char *aq_get_fw()
{
	if (aq_data_buffer == NULL)
		return NULL;

	return strdup(aq_trim_str(aq_data_buffer, AQ_DEV_FW_OFFS, AQ_DEV_FW_LEN));
}

uchar aq_get_prod_year()
{
	if (aq_data_buffer == NULL)
		return 0;

	return aq_data_buffer[AQ_DEV_PROD_Y_OFFS];
}

uchar aq_get_prod_month()
{
	if (aq_data_buffer == NULL)
		return 0;

	return aq_data_buffer[AQ_DEV_PROD_M_OFFS];
}

ushort aq_get_flash_count()
{
	if (aq_data_buffer == NULL)
		return 0;

	return aq_get_ushort(aq_data_buffer, AQ_DEV_FLASHC_OFFS);
}

ushort aq_get_os_version()
{
	if (aq_data_buffer == NULL)
		return 0;

	return aq_get_ushort(aq_data_buffer, AQ_DEV_OS_OFFS);
}

char *aq_get_fan_name(char n)
{
	int offset;

	if (aq_data_buffer == NULL)
		return NULL;

	offset = AQ_FAN_NAME_OFFS + (n * (AQ_FAN_NAME_LEN + 1));

	return strdup(aq_trim_str(aq_data_buffer, offset, AQ_FAN_NAME_LEN));
}

ushort aq_get_fan_rpm(char n)
{
	int offset;

	if (aq_data_buffer == NULL)
		return 0;

	offset = AQ_FAN_RPM_OFFS + (n * AQ_FAN_RPM_LEN);
	/* TODO: handle disconnected fans */

	return aq_get_ushort(aq_data_buffer, offset);
}

char aq_get_fan_duty(char n)
{
	/* TODO: simplify */
	uchar 	t;
	ushort 	s;

	if (aq_data_buffer == NULL)
		return -1;

	t = *(aq_data_buffer + AQ_FAN_PWR_OFFS + (n * AQ_FAN_PWR_LEN));
	s = (t * 100) >> 8;

	return (char)s;
}

char *aq_get_temp_name(char n)
{
	int offset;

	if (aq_data_buffer == NULL)
		return NULL;

	offset = AQ_TEMP_NAME_OFFS + (n * (AQ_TEMP_NAME_LEN + 1));

	return strdup(aq_trim_str(aq_data_buffer, offset, AQ_TEMP_NAME_LEN));
}

double aq_get_temp_value(char n)
{
	ushort 	c;
	int 	offset;

	if (aq_data_buffer == NULL)
		return -1;

	offset = AQ_TEMP_VAL_OFFS + (n * AQ_TEMP_VAL_LEN);
	c = aq_get_ushort(aq_data_buffer, offset);

	return (c != AQ_TEMP_NAN) ? (double)c / 10 : AQ_TEMP_NCONN;
}

ushort aq_get_serial()
{
	if (aq_data_buffer == NULL)
		return 0;

	return aq_get_ushort(aq_data_buffer, AQ_DEV_SERIAL_OFFS);
}


/*
 * device communication related functions
 */

libusb_device *aq_dev_find()
{
	libusb_device 	**dev_list;
	libusb_device 	*ret, *dev;
	struct 			libusb_device_descriptor desc;
	ssize_t 		n, i;

	if ((n = libusb_get_device_list(NULL, &dev_list)) < 0) {
		return NULL;
	}

	ret = NULL;
	for (i = 0; i < n; i++) {
	    dev = dev_list[i];
	    if (libusb_get_device_descriptor(dev, &desc) < 0) {
	    	break;
	    }
	    if (desc.idVendor == AQ_USB_VID &&
	    		desc.idProduct == AQ_USB_PID) {
	        ret = libusb_ref_device(dev);
	        break;
	    }
	}
	libusb_free_device_list(dev_list, 1);

	return ret;
}

int aq_dev_init(char **err)
{
	char 	kernel_active;
	int		i, n;
	struct 	libusb_device_handle *handle;

	if (libusb_open(aq_usb_dev, &handle) != 0) {
	    *err = "failed to open device";
	    return -1;
	}

	kernel_active = 0;
	/* TODO: definitely more error handling */
	if (libusb_kernel_driver_active(handle, 0)) {
		kernel_active = 1;
		i = libusb_detach_kernel_driver(handle, 0);
		if (i != 0 && i != LIBUSB_ERROR_NOT_FOUND) {
			*err = "failed to detach kernel driver";
			libusb_close(handle);
			return -1;
		}

	}

	/* TODO: simplify that mess? */
	if (libusb_get_configuration(handle, &i) != 0) {
		*err = "failed to get current configuration";
		libusb_close(handle);
		return -1;
	}
	if (i != AQ_USB_CONF || kernel_active) {
		if ((i = libusb_set_configuration(handle, AQ_USB_CONF)) < 0) {
			if (i == LIBUSB_ERROR_BUSY) {
				n = 1;
				while (n < AQ_USB_RETRIES) {
					sleep(AQ_USB_RETRY_DELAY);
					i = libusb_set_configuration(handle, AQ_USB_CONF);
					if (i != LIBUSB_ERROR_BUSY)
						break;
				}
				if (i == LIBUSB_ERROR_BUSY) {
					*err = "failed to set device configuration (device busy)";
					libusb_close(handle);
					return -1;
				} else if (i != 0) {
					*err = "failed to set device configuration";
					libusb_close(handle);
					return -1;
				}
			} else {
				*err = "failed to set device configuration";
				libusb_close(handle);
				return -1;
			}
		}
	}

	libusb_close(handle);

	return 0;
}

int aq_dev_poll(char **err)
{
	int 					i, n, transferred;
	libusb_device_handle 	*handle;

	if (aq_data_buffer == NULL)
		return -1;
	if (aq_usb_dev == NULL)
		return -1;

	/* USB device initialization and configuration */
	/* TODO: maybe some error handling */
	if (libusb_open(aq_usb_dev, &handle) != 0) {
	    *err = "failed to open device";
	    return -1;
	}

	if ((i = libusb_claim_interface(handle, 0)) < 0) {
		if (i == LIBUSB_ERROR_BUSY) {
			n = 1;
			while (n < AQ_USB_RETRIES) {
				sleep(AQ_USB_RETRY_DELAY);
				i = libusb_claim_interface(handle, 0);
				if (i != LIBUSB_ERROR_BUSY)
					break;
			}
		}
		if (i < 0) {
			if (i == LIBUSB_ERROR_BUSY)
				*err = "failed to claim interface (device busy)";
			else
				*err = "failed to claim interface";
			goto err_exit2;
		}
	}

	if ((i = libusb_interrupt_transfer(handle, AQ_USB_ENDP, aq_data_buffer,
			AQ_USB_READ_LEN, &transferred, AQ_USB_TIMEOUT)) != 0) {
		*err = "failed to read from device";
		goto err_exit1;
	}

	libusb_release_interface(handle, 0);
	libusb_close(handle);

	return 0;

err_exit1: libusb_release_interface(handle, 0);
err_exit2: libusb_close(handle);
	return -1;
}


/*
 * one-for-all functions to be used from outside, proposed order
 */

int aquaero_init(char **err_msg)
{
	/* initialize libusb */
	if (libusb_init(NULL) != 0) {
		*err_msg = "failed to initialize libusb";
		return -1;
	}
	/* allocate raw data buffer */
	if ((aq_data_buffer = malloc(AQ_USB_READ_LEN)) == NULL) {
		*err_msg = "failed to allocate data buffer";
		libusb_exit(NULL);
		return -1;
	}
	/* find device */
	if ((aq_usb_dev = aq_dev_find()) == NULL) {
		*err_msg = "no aquaero(R) device found";
		free(aq_data_buffer);
		libusb_exit(NULL);
		return -1;
	}
	/* configure device */
	if (aq_dev_init(err_msg) != 0) {
		free(aq_data_buffer);
		libusb_unref_device(aq_usb_dev);
		aq_usb_dev = NULL;
		libusb_exit(NULL);
		return -1;
	}

	return 0;
}

struct aquaero_data *aquaero_poll_data(char **err_msg)
{
	int		i;
	struct	aquaero_data *ret_data;

	if (aq_data_buffer == NULL || aq_usb_dev == NULL) {
		*err_msg = "uninitialized, call aquaero_init() first";
		return NULL;
	}

	/* prepare data structure */
	if ((ret_data = malloc(sizeof(struct aquaero_data))) == NULL) {
		*err_msg = "out-of-memory allocating data structure";
		return NULL;
	}


	/* poll raw data */
	if ((i = aq_dev_poll(err_msg)) < 0) {
		free(ret_data);
		return NULL;
	}

	/* process data, fill structure */
	ret_data->device_name = aq_get_name();
	ret_data->firmware = aq_get_fw();
	ret_data->prod_year = aq_get_prod_year();
	ret_data->prod_month = aq_get_prod_month();
	ret_data->device_serial = aq_get_serial();
	ret_data->flash_count = aq_get_flash_count();
	ret_data->os_version = aq_get_os_version();
	for (i = 0; i < AQ_FAN_NUM; i++) {
		ret_data->fan_names[i] = aq_get_fan_name(i);
		ret_data->fan_rpm[i] = aq_get_fan_rpm(i);
		ret_data->fan_duty[i] = aq_get_fan_duty(i);
	}
	for (i = 0; i < AQ_TEMP_NUM; i++) {
		ret_data->temp_names[i] = aq_get_temp_name(i);
		ret_data->temp_values[i] = aq_get_temp_value(i);
	}

	return ret_data;
}

uchar *aquaero_get_buffer()
{
	return aq_data_buffer;
}

void aquaero_exit()
{
	libusb_exit(NULL);

	if (aq_data_buffer != NULL)
		free(aq_data_buffer);

	return;
}
