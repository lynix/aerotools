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
#include <usb.h>

/* device communication related stuff */
#define USB_VID 			0x0c70
#define USB_PID 			0xf0b0
#define USB_CONF			1
#define USB_ENDP			0x81
#define USB_TIMEOUT			1000
#define BUFFS				553
#define ENODATA				-61

/* data offsets and formatting */
#define DEV_FW_LEN			5
#define DEV_FW_OFFS			505
#define	DEV_NAME_LEN		8
#define DEV_NAME_OFFS		122
#define DEV_OS_OFFS			512
#define DEV_FLASHC_OFFS		516
#define DEV_SERIAL_OFFS		520
#define DEV_PROD_M_OFFS		522
#define DEV_PROD_Y_OFFS		523
#define FAN_NUM				4
#define FAN_NAME_LEN		10
#define FAN_NAME_OFFS		0
#define FAN_RPM_LEN			2
#define FAN_RPM_OFFS		442
#define FAN_PWR_LEN			1
#define FAN_PWR_OFFS		198
#define TEMP_NUM			6
#define TEMP_NAME_LEN		10
#define TEMP_NAME_OFFS  	55
#define TEMP_VAL_LEN		2
#define TEMP_VAL_OFFS		460
#define TEMP_NAN			0x4e20
#define TEMP_NCONN			-1.0

/* device communication */
struct 	usb_device *dev_find();
struct 	usb_dev_handle *dev_init(struct usb_device *dev, char **err);
int		dev_read(struct usb_dev_handle *devh, char *buffer);
int		dev_close(struct usb_dev_handle *devh);
/* data processing */
ushort	get_short(char *buffer, int offset);
char	*get_string(char *buffer, int offset, int max_length);
char 	*get_name(char *buffer);
char 	*get_fw(char *buffer);
char 	*get_fan_name(char n, char *buffer);
char    *get_temp_name(char n, char *buffer);
char 	get_fan_duty(char n, char *buffer);
char 	get_prod_year(char *buffer);
char 	get_prod_month(char *buffer);
ushort 	get_fan_rpm(char n, char *buffer);
ushort 	get_serial(char *buffer);
ushort	get_flash_count(char *buffer);
ushort	get_os(char *buffer);
double 	get_temp_value(char n, char *buffer);

#endif /* DEVICE_H_ */
