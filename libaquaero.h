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

#ifndef DEVICE_H_
#define DEVICE_H_

/* includes */
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
/* DEBUG */
	#include <stdio.h>
/* DEBUG */

/* usb device communication related constants */
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
#define AQ_DEV_FW_OFFS			0x1f9
#define	AQ_DEV_NAME_LEN			9
#define AQ_DEV_NAME_OFFS		0x079
#define AQ_DEV_OS_OFFS			0x200
#define AQ_DEV_FLASHC_OFFS		0x204
#define AQ_DEV_SERIAL_OFFS		0x208
#define AQ_DEV_PROD_M_OFFS		0x20a
#define AQ_DEV_PROD_Y_OFFS		0x20b
#define AQ_FAN_NUM				4
#define AQ_FAN_NAME_LEN			10
#define AQ_FAN_NAME_OFFS		0x000
#define AQ_FAN_RPM_LEN			2
#define AQ_FAN_RPM_OFFS			0x1ba
#define AQ_FAN_PWR_LEN			1
#define AQ_FAN_PWR_OFFS			0x0c6
#define AQ_TEMP_NUM				6
#define AQ_TEMP_NAME_LEN		10
#define AQ_TEMP_NAME_OFFS  		0x037
#define AQ_TEMP_VAL_LEN			2
#define AQ_TEMP_VAL_OFFS		0x1cc
#define AQ_TEMP_NCONN			0x7d0
#define AQ_FLOW_NAME_LEN		10
#define AQ_FLOW_NAME_OFFS		0x02c
#define AQ_FLOW_VAL_OFFS		0x1c2
#define AQ_FLOW_NCONN			0x0c8

/* own types */
typedef unsigned char uchar;

/* aquaero(R) device data structures */

//enum fan_mode = { MANUAL_RPM, MANUAL_DUTY, ... };

typedef struct {
	char		*name;
	char		duty;
	ushort		rpm;
//	ushort		set_rpm;
//	ushort		set_duty;
//	fan_mode	mode;
//	ushort		max_rpm;
//	ushort		pulse;
//	uchar		min_power;
//	struct 		aq_temp *sensors[2];
} aq_fan;

typedef struct {
	char		*name;
	double		value;
//	uchar		factor;
//	uchar		offset;
//	ushort		alarm;
//	ushort		min;
//	ushort		max;
//	uchar		hyst;
//	ushort		opt;
	char		connected;
//	char		is_flow;
} aq_temp;

typedef struct {
	char		*name;
	double		value;
//	double		alarm[2];
//	uchar		pulse;
//	flow_mode	mode;
	char		connected;
} aq_flow;

typedef struct {
	char		*name;
	char		*fw_name;
	ushort 		os_version;
	uchar 		prod_year;
	uchar 		prod_month;
	ushort 		serial;
	ushort 		flash_count;
} aq_device;

typedef struct {
	aq_device	device;
	aq_fan		fans[AQ_FAN_NUM];
	aq_temp		temps[AQ_TEMP_NUM];
	aq_flow		flow;
} aquaero_data;

/* error messages */
#define LIBUSB_STR_SUCCESS				"success";
#define LIBUSB_STR_ERR_IO				"I/O error";
#define LIBUSB_STR_ERR_INVALID_PARAM	"invalid parameter";
#define LIBUSB_STR_ERR_ACCESS			"access denied";
#define LIBUSB_STR_ERR_NO_DEVICE		"no such device";
#define LIBUSB_STR_ERR_NOT_FOUND		"entity not found";
#define LIBUSB_STR_ERR_BUSY				"resource busy";
#define LIBUSB_STR_ERR_TIMEOUT			"operation timed out";
#define LIBUSB_STR_ERR_OVERFLOW			"overflow";
#define LIBUSB_STR_ERR_PIPE				"pipe error";
#define LIBUSB_STR_ERR_INTERRUPTED		"syscall interrupted";
#define LIBUSB_STR_ERR_NO_MEM			"insufficient memory";
#define LIBUSB_STR_ERR_NOT_SUPPORTED	"operation not supported";
#define LIBUSB_STR_ERR_OTHER			"other error";
#define LIBUSB_STR_ERR_UNKNOWN			"unknown error";

/* device communication */
libusb_device 	*aq_dev_find();
int				aq_dev_init(char **err);
int				aq_dev_poll(char **err);

/* helper functions */
ushort	aq_get_ushort(uchar *ptr);
char	*aq_trim_str(uchar *start, int max_length);
char 	*aq_libusb_strerr(int err);
char 	*aq_strcat(char *str1, char *str2);

/* data extraction, conversion */
void 	aq_get_device(aq_device *device);
void 	aq_get_fan(aq_fan *fan, short num);
void	aq_get_temp(aq_temp *temp, short num);
void	aq_get_flow(aq_flow *flow);

/* all-in-one query functions */
int		aquaero_init(char **err_msg);
int		aquaero_poll_data(aquaero_data *aq_data, char **err_msg);
uchar	*aquaero_get_buffer();
void	aquaero_exit();

#endif /* DEVICE_H_ */
