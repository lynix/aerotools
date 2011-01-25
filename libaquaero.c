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

#include "libaquaero.h"


/*
 *	globals
 */

libusb_device 	*aq_usb_dev = NULL;
unsigned char	*aq_data_buffer = NULL;


/*
 *	helper functions
 */

aq_int aq_get_int(int offset)
{
	return (*(aq_data_buffer + offset) << 8) + *(aq_data_buffer + offset + 1);
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
	device->name = strdup((char *)aq_data_buffer + AQ_OFFS_NAME);
	device->fw_name = strdup((char *)aq_data_buffer + AQ_OFFS_FW);
	device->prod_month = aq_data_buffer[AQ_OFFS_PROD_M];
	device->prod_year = aq_data_buffer[AQ_OFFS_PROD_Y];
	device->serial = aq_get_int(AQ_OFFS_SERIAL);
	device->flash_count = aq_get_int(AQ_OFFS_FLASHC);
	device->os_version = aq_get_int(AQ_OFFS_OS);
	device->language = strdup((char *)aq_data_buffer + AQ_OFFS_LANG);
	device->profile = aq_data_buffer[AQ_OFFS_PROFILE] + 1;
	device->time_h = aq_data_buffer[AQ_OFFS_TIME_H];
	device->time_m = aq_data_buffer[AQ_OFFS_TIME_M];
	device->time_s = aq_data_buffer[AQ_OFFS_TIME_S];
	device->time_d = aq_data_buffer[AQ_OFFS_TIME_D];
}

void aq_get_fan(aq_fan *fan, short num)
{
	fan->name = strdup((char *)aq_data_buffer + AQ_OFFS_FAN_NAME +
			(num * (AQ_LEN_FAN_NAME + 1)));
	fan->duty = (((unsigned char)*(aq_data_buffer + AQ_OFFS_FAN_PWR +
			(num * AQ_LEN_BYTE))) * 100) >> 8;	/* TODO: simplify */
	fan->rpm = aq_get_int(AQ_OFFS_FAN_RPM +	(num * AQ_LEN_INT));
}

void aq_get_temp(aq_temp *temp, short num)
{
	temp->name = strdup((char *)aq_data_buffer + AQ_OFFS_TEMP_NAME +
			(num * (AQ_LEN_TEMP_NAME + 1)));
	temp->value = (double)aq_get_int(AQ_OFFS_TEMP_VAL +
			(num * AQ_LEN_INT)) / 10.0;
	temp->connected = (temp->value != AQ_TEMP_NCONN);
}

void aq_get_flow(aq_flow *flow)
{
	flow->name = strdup((char *)aq_data_buffer + AQ_OFFS_FLOW_NAME);
	flow->value = (double)aq_get_int(AQ_OFFS_FLOW_VAL) / 100.0;
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
	    if (desc.idVendor == AQ_USB_VID && desc.idProduct == AQ_USB_PID) {
	        ret = libusb_ref_device(dev);
	        break;
	    }
	}
	libusb_free_device_list(dev_list, 1);

	return ret;
}

int aq_dev_init(char **err)
{
	int		i, n;
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
	for (n=0; n<AQ_USB_RETRIES; n++) {
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
	int		i, n, transferred;
	struct	libusb_device_handle *handle;

	if ((i = libusb_open(aq_usb_dev, &handle)) != 0) {
	    *err = aq_strcat("failed to open device: ", aq_libusb_strerr(i));
	    return -1;
	}

	for (n=0; n<AQ_USB_RETRIES; n++) {
		if ((i = libusb_claim_interface(handle, 0)) != LIBUSB_ERROR_BUSY)
			break;
		sleep(AQ_USB_RETRY_DELAY);
	}
	if (i != 0) {
		*err = aq_strcat("failed to claim interface: ", aq_libusb_strerr(i));
		libusb_close(handle);
		return -1;
	}

	if ((i = libusb_interrupt_transfer(handle, AQ_USB_ENDP_IN,
			(unsigned char *)aq_data_buffer, AQ_USB_READ_LEN, &transferred,
			AQ_USB_TIMEOUT)) != 0) {
		*err = aq_strcat("failed to read from device: ", aq_libusb_strerr(i));
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		return -1;
	}

	libusb_release_interface(handle, 0);
	libusb_close(handle);

	return 0;
}

int aq_dev_push(aq_byte req_type, char **err)
{
	int				i, n;
	unsigned char	*buffer;
	struct			libusb_device_handle *handle;

	if ((buffer = malloc(AQ_USB_WRITE_LEN)) == NULL) {
		*err = "out of memory";
		return -1;
	}
	buffer[0] = req_type;
	memcpy(buffer + 1, aq_data_buffer, AQ_USB_WRITE_LEN - 1);

	if ((i = libusb_open(aq_usb_dev, &handle)) != 0) {
	    *err = aq_strcat("failed to open device: ", aq_libusb_strerr(i));
	    return -1;
	}

	for (n=0; n<AQ_USB_RETRIES; n++) {
		if ((i = libusb_claim_interface(handle, 0)) != LIBUSB_ERROR_BUSY)
			break;
		sleep(AQ_USB_RETRY_DELAY);
	}
	if (i != 0) {
		*err = aq_strcat("failed to claim interface: ", aq_libusb_strerr(i));
		libusb_close(handle);
		return -1;
	}

	if ((i = libusb_control_transfer(handle, AQ_USB_WRITE_REQT,
			AQ_USB_WRITE_REQ, AQ_USB_WRITR_WVAL, AQ_USB_WRITR_INDEX, buffer,
			AQ_USB_WRITE_LEN, AQ_USB_TIMEOUT)) < 0) {
		*err = aq_strcat("failed to write to device: ", aq_libusb_strerr(i));
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		return -1;
	}

	libusb_release_interface(handle, 0);
	libusb_close(handle);
	free(buffer);

	return 0;
}


/*
 *	functions to be used from outside
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
	if (aq_dev_poll(err_msg) < 0)
		return -1;

	/* process data, fill structure */
	aq_get_device(&aq_data->device);
	for (i = 0; i < AQ_NUM_FAN; i++)
		aq_get_fan(&aq_data->fans[i], i);
	for (i = 0; i < AQ_NUM_TEMP; i++)
		aq_get_temp(&aq_data->temps[i], i);
	aq_get_flow(&aq_data->flow);

	return 0;
}

int aquaero_load_profile(aq_byte profile, char **err_msg)
{
	/* check for valid initialization */
	if (aq_data_buffer == NULL || aq_usb_dev == NULL) {
		*err_msg = "uninitialized, call aquaero_init() first";
		return -1;
	}

	/* check input */
	if (profile > AQ_NUM_PROFILE || profile == 0) {
		*err_msg = "invalid profile number";
		return -1;
	}

	/* poll raw data */
	if (aq_dev_poll(err_msg) < 0)
		return -1;

	/* change profile */
	aq_data_buffer[AQ_OFFS_PROFILE] = profile - 1;

	/* write out to device */
	if (aq_dev_push(AQ_REQ_PROFILE, err_msg) < 0)
		return -1;

	return 0;
}

void aquaero_exit()
{
	if (aq_usb_dev != NULL) {
		libusb_unref_device(aq_usb_dev);
		aq_usb_dev = NULL;
	}
	libusb_exit(NULL);

	if (aq_data_buffer != NULL) {
		free(aq_data_buffer);
		aq_data_buffer = NULL;
	}

	return;
}

unsigned char *aquaero_get_buffer()
{
	return aq_data_buffer;
}
