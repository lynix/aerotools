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

#ifndef AEROCLI_H_
#define AEROCLI_H_

#include "helpers.h"
#include <usb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/* device communication related stuff */
#define USB_VID 			0x0c70
#define USB_PID 			0xf0b0
#define USB_CONF			1
#define USB_ENDP			0x81
#define USB_TIMEOUT			1000
#define BUFFS				560

/* data offsets and formatting */
#define DEV_FW_LEN			5
#define DEV_FW_OFFS			505
#define	DEV_NAME_LEN		8
#define DEV_NAME_OFFS		122
#define DEV_SERIAL_LEN		2
#define DEV_SERIAL_OFFS		520
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
#define TEMP_NAN			0x7d0

struct usb_device *dev_find(void);
struct usb_dev_handle *dev_init(struct usb_device *dev);

char *get_name(char *buffer);
char *get_fw(char *buffer);
char *get_product(char *buffer);
char *get_fan_name(char n, char *buffer);
ushort get_fan_rpm(char n, char *buffer);
char get_fan_duty(char n, char *buffer);
char *get_temp_name(char n, char *buffer);
double get_temp_value(char n, char *buffer);
ushort get_serial(char *buffer);

/* TODO: remove this */
/*void debug_buffer ( char *buffer, int len );*/


#endif /* AEROCLI_H_ */
