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

#ifndef DEVICE_H_
#define DEVICE_H_

/* includes */
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <stdlib.h>
/* debugging only!: */
#include <stdio.h>

/* usb device communication related stuff */
#define AQ_USB_VID 				0x0c70
#define AQ_USB_PID 				0xf0b0
#define AQ_USB_CONF				1
#define AQ_USB_ENDP				0x81
#define AQ_USB_TIMEOUT			1000
#define AQ_USB_RETRIES			3
#define AQ_USB_RETRY_DELAY		200
#define AQ_USB_READ_LEN			552

/* data offsets and formatting constants */
#define AQ_DEV_FW_LEN			5
#define AQ_DEV_FW_OFFS			0x21
#define	AQ_DEV_NAME_LEN			9
#define AQ_DEV_NAME_OFFS		0xc9
#define AQ_DEV_OS_OFFS			40
#define AQ_DEV_FLASHC_OFFS		44
#define AQ_DEV_SERIAL_OFFS		48
#define AQ_DEV_PROD_M_OFFS		50
#define AQ_DEV_PROD_Y_OFFS		51
#define AQ_FAN_NUM				4
#define AQ_FAN_NAME_LEN			10
#define AQ_FAN_NAME_OFFS		0x50
#define AQ_FAN_RPM_LEN			2
#define AQ_FAN_RPM_OFFS			522
#define AQ_FAN_PWR_LEN			1
#define AQ_FAN_PWR_OFFS			278
#define AQ_TEMP_NUM				6
#define AQ_TEMP_NAME_LEN		10
#define AQ_TEMP_NAME_OFFS  		135
#define AQ_TEMP_VAL_LEN			2
#define AQ_TEMP_VAL_OFFS		540
#define AQ_TEMP_NAN				0x4e20
#define AQ_TEMP_NCONN			-1.0

/* complete device data structure */
struct aquaero_data {
	char 		*device_name;
	char 		*firmware;
	char 		prod_year;
	char 		prod_month;
	ushort 		device_serial;
	ushort 		flash_count;
	ushort 		os_version;
	char 		*fan_names[AQ_FAN_NUM];
	char 		*temp_names[AQ_TEMP_NUM];
	ushort 		fan_rpm[AQ_FAN_NUM];
	char 		fan_duty[AQ_FAN_NUM];
	double 		temp_values[AQ_TEMP_NUM];
};

/* device communication */
libusb_device *aq_dev_find();
int		aq_dev_poll(char *buffer, char **err);

/* data processing */
ushort	aq_get_short(char *buffer, int offset);
char	*aq_get_string(char *buffer, int offset, int max_length);
char 	*aq_get_name(char *buffer);
char 	*aq_get_fw(char *buffer);
char 	*aq_get_fan_name(char n, char *buffer);
char    *aq_get_temp_name(char n, char *buffer);
char 	aq_get_fan_duty(char n, char *buffer);
char 	aq_get_prod_year(char *buffer);
char 	aq_get_prod_month(char *buffer);
ushort 	aq_get_fan_rpm(char n, char *buffer);
ushort 	aq_get_serial(char *buffer);
ushort	aq_get_flash_count(char *buffer);
ushort	aq_get_os(char *buffer);
double 	aq_get_temp_value(char n, char *buffer);

/* all-in-one API query function */
struct	aquaero_data *aquaero_poll_data(char *buffer, char **err_msg);

#endif /* DEVICE_H_ */
