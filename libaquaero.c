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

#include "libaquaero.h"


/*
 *	globals
 */

libusb_device 	*aq_usb_dev = NULL;
uchar 			*aq_data_buffer = NULL;


/*
 *	helper functions
 */

ushort aq_get_ushort(uchar *ptr)
{
	return (*ptr << 8) + *(ptr + 1);
}

char *aq_trim_str(uchar *start, int max_length)
{
	uchar *end = start + max_length;

	while (end > start) {
		if (*(end-1) != ' ')
			break;
		end--;
	}
	*end = '\0';

	return (char *)start;
}

char *aq_strcat(char *str1, char *str2)
{
	char *ret;

	if ((ret = malloc(strlen(str1) + strlen(str2) + 1)) == NULL)
		return NULL;
	strcpy(ret, str1);
	strcpy(ret + strlen(str1), str2);

	return ret;
}

char *aq_libusb_strerr(int err)
{
	switch (err) {
		case LIBUSB_SUCCESS:				return LIBUSB_STR_SUCCESS;
		case LIBUSB_ERROR_IO:				return LIBUSB_STR_ERR_IO;
		case LIBUSB_ERROR_INVALID_PARAM:	return LIBUSB_STR_ERR_INVALID_PARAM;
		case LIBUSB_ERROR_ACCESS:			return LIBUSB_STR_ERR_ACCESS;
		case LIBUSB_ERROR_NO_DEVICE:		return LIBUSB_STR_ERR_NO_DEVICE;
		case LIBUSB_ERROR_NOT_FOUND:		return LIBUSB_STR_ERR_NOT_FOUND;
		case LIBUSB_ERROR_BUSY:				return LIBUSB_STR_ERR_BUSY;
		case LIBUSB_ERROR_TIMEOUT:			return LIBUSB_STR_ERR_TIMEOUT;
		case LIBUSB_ERROR_OVERFLOW:			return LIBUSB_STR_ERR_OVERFLOW;
		case LIBUSB_ERROR_PIPE:				return LIBUSB_STR_ERR_PIPE;
		case LIBUSB_ERROR_INTERRUPTED:		return LIBUSB_STR_ERR_INTERRUPTED;
		case LIBUSB_ERROR_NO_MEM:			return LIBUSB_STR_ERR_NO_MEM;
		case LIBUSB_ERROR_NOT_SUPPORTED:	return LIBUSB_STR_ERR_NOT_SUPPORTED;
		case LIBUSB_ERROR_OTHER:			return LIBUSB_STR_ERR_OTHER;
		default:							return LIBUSB_STR_ERR_UNKNOWN;
	}
}


/*
 *	device-specific data extraction functions
 */

void aq_get_device(aq_device *device)
{
	device->name = strdup(aq_trim_str(aq_data_buffer + AQ_DEV_NAME_OFFS,
			AQ_DEV_NAME_LEN));
	device->fw_name = strdup(aq_trim_str(aq_data_buffer + AQ_DEV_FW_OFFS,
			AQ_DEV_FW_LEN));
	device->prod_month = aq_data_buffer[AQ_DEV_PROD_M_OFFS];
	device->prod_year = aq_data_buffer[AQ_DEV_PROD_Y_OFFS];
	device->serial = aq_get_ushort(aq_data_buffer + AQ_DEV_SERIAL_OFFS);
	device->flash_count = aq_get_ushort(aq_data_buffer + AQ_DEV_FLASHC_OFFS);
	device->os_version = aq_get_ushort(aq_data_buffer + AQ_DEV_OS_OFFS);
}

void aq_get_fan(aq_fan *fan, short num)
{
	fan->name = strdup(aq_trim_str(aq_data_buffer + AQ_FAN_NAME_OFFS +
			(num * (AQ_FAN_NAME_LEN + 1)), AQ_FAN_NAME_LEN));
	fan->duty = (((uchar)*(aq_data_buffer + AQ_FAN_PWR_OFFS +
			(num * AQ_FAN_PWR_LEN))) * 100) >> 8;	/* TODO: simplify */
	fan->rpm = aq_get_ushort(aq_data_buffer + AQ_FAN_RPM_OFFS +
			(num * AQ_FAN_RPM_LEN));
}

void aq_get_temp(aq_temp *temp, short num)
{
	temp->name = strdup(aq_trim_str(aq_data_buffer + AQ_TEMP_NAME_OFFS +
			(num * (AQ_TEMP_NAME_LEN + 1)), AQ_TEMP_NAME_LEN));
	temp->value = (double)aq_get_ushort(aq_data_buffer + AQ_TEMP_VAL_OFFS +
			(num * AQ_TEMP_VAL_LEN)) / 10.0;
	temp->connected = (temp->value != AQ_TEMP_NCONN);
}

void aq_get_flow(aq_flow *flow)
{
	flow->name = strdup(aq_trim_str(aq_data_buffer + AQ_FLOW_NAME_OFFS,
			AQ_FLOW_NAME_LEN));
	flow->value = (double)aq_get_ushort(aq_data_buffer + AQ_FLOW_VAL_OFFS) /
			100.0;
	flow->connected = (flow->value != AQ_FLOW_NCONN);
}


/*
 *	device communication related functions
 */

libusb_device *aq_dev_find()
{
	libusb_device 	**dev_list;
	libusb_device 	*ret = NULL, *dev;
	struct 			libusb_device_descriptor desc;
	ssize_t 		n, i;

	if ((n = libusb_get_device_list(NULL, &dev_list)) < 0) {
		return NULL;
	}

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
	int		i, n = 0;
	struct 	libusb_device_handle *handle;

	if ((i = libusb_open(aq_usb_dev, &handle)) != 0) {
	    *err = aq_strcat("failed to open device: ", aq_libusb_strerr(i));
	    return -1;
	}

	/* detach kernel driver */
	if (libusb_kernel_driver_active(handle, 0)) {
		i = libusb_detach_kernel_driver(handle, 0);
		if (i != 0 && i != LIBUSB_ERROR_NOT_FOUND) {
			*err = aq_strcat("failed to detach kernel driver: ",
					aq_libusb_strerr(i));
			libusb_close(handle);
			return -1;
		}
	}

	/* soft-reset device, set configuration */
	while (n < AQ_USB_RETRIES) {
		i = libusb_set_configuration(handle, AQ_USB_CONF);
		if (i != LIBUSB_ERROR_BUSY)
			break;
		sleep(AQ_USB_RETRY_DELAY);
	}
	if (i != 0) {
		*err = aq_strcat("failed to set configuration: ", aq_libusb_strerr(i));
		libusb_close(handle);
		return -1;
	}

	libusb_close(handle);

	return 0;
}

int aq_dev_poll(char **err)
{
	int		i, n = 0, transferred;
	struct	libusb_device_handle *handle;

	if (aq_data_buffer == NULL || aq_usb_dev == NULL) {
		*err = "uninitialized";
		return -1;
	}

	if ((i = libusb_open(aq_usb_dev, &handle)) != 0) {
	    *err = aq_strcat("failed to open device: ", aq_libusb_strerr(i));
	    return -1;
	}

	while (n < AQ_USB_RETRIES) {
		if ((i = libusb_claim_interface(handle, 0)) != LIBUSB_ERROR_BUSY)
			break;
		sleep(AQ_USB_RETRY_DELAY);
	}
	if (i != 0) {
		*err = aq_strcat("failed to claim interface: ", aq_libusb_strerr(i));
		libusb_close(handle);
		return -1;
	}

	if ((i = libusb_interrupt_transfer(handle, AQ_USB_ENDP, aq_data_buffer,
			AQ_USB_READ_LEN, &transferred, AQ_USB_TIMEOUT)) != 0) {
		*err = aq_strcat("failed to read from device: ", aq_libusb_strerr(i));
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		return -1;
	}

	libusb_release_interface(handle, 0);
	libusb_close(handle);

	return 0;
}


/*
 *	one-for-all functions to be used from outside, proposed order
 */

int aquaero_init(char **err_msg)
{
	/* initialize libusb */
	if (libusb_init(NULL) != 0) {
		*err_msg = "failed to initialize libusb";
		return -1;
	}
	/* allocate raw data buffer */
	if ((aq_data_buffer = calloc(AQ_USB_READ_LEN, 1)) == NULL) {
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

int aquaero_poll_data(aquaero_data *aq_data, char **err_msg)
{
	int i;

	/* check for valid initialization */
	if (aq_data_buffer == NULL || aq_usb_dev == NULL) {
		*err_msg = "uninitialized, call aquaero_init() first";
		return -1;
	}

	/* poll raw data */
	if ((i = aq_dev_poll(err_msg)) < 0)
		return -1;

	/* process data, fill structure */
	aq_get_device(&aq_data->device);
	for (i = 0; i < AQ_FAN_NUM; i++)
		aq_get_fan(&aq_data->fans[i], i);
	for (i = 0; i < AQ_TEMP_NUM; i++)
		aq_get_temp(&aq_data->temps[i], i);
	aq_get_flow(&aq_data->flow);

	return 0;
}

uchar *aquaero_get_buffer()
{
	return aq_data_buffer;
}

void aquaero_exit()
{
	if (aq_usb_dev != NULL)
		libusb_unref_device(aq_usb_dev);
	libusb_exit(NULL);

	if (aq_data_buffer != NULL)
		free(aq_data_buffer);

	return;
}
