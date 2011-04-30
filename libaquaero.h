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

#ifndef DEVICE_H_
#define DEVICE_H_

/* includes */
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

/* usb communication related constants */
#define AQ_USB_VID 				0x0c70
#define AQ_USB_PID 				0xf0b0
#define AQ_USB_CONF				1
#define AQ_USB_ENDP_IN			0x081
#define AQ_USB_ENDP_OUT			0x001
#define AQ_USB_TIMEOUT			1000
#define AQ_USB_RETRIES			3
#define AQ_USB_RETRY_DELAY		200
#define AQ_USB_READ_LEN			552
#define AQ_USB_WRITE_LEN		376
#define AQ_USB_WRITE_REQT		0x21
#define AQ_USB_WRITE_REQ		0x09
#define AQ_USB_WRITR_WVAL		0x200
#define AQ_USB_WRITR_INDEX		0


/* write request types */
typedef enum {
/*	AQ_REQ_DEFAULT	= 101,
	AQ_REQ_EEPROM	= 102,
	*/AQ_REQ_RAM		= 103,/*
	AQ_REQ_COPY		= 108,*/
	AQ_REQ_PROFILE	= 107
} aq_request;

/* data offsets */
#define AQ_OFFS_LANG			0x20c
#define AQ_OFFS_FW				0x1f9
#define AQ_OFFS_NAME			0x079
#define AQ_OFFS_OS				0x200
#define AQ_OFFS_FLASHC			0x204
#define AQ_OFFS_SERIAL			0x208
#define AQ_OFFS_PROD_M			0x20a
#define AQ_OFFS_PROD_Y			0x20b
#define AQ_OFFS_PROFILE			0x082
#define AQ_OFFS_FAN_NAME		0x000
#define AQ_OFFS_FAN_RPM			0x1ba
#define AQ_OFFS_FAN_PWR			0x0c6
#define AQ_OFFS_TEMP_NAME  		0x037
#define AQ_OFFS_TEMP_VAL		0x1cc
#define AQ_OFFS_FLOW_NAME		0x02c
#define AQ_OFFS_FLOW_VAL		0x1c2
#define AQ_OFFS_TIME_H			0x171
#define AQ_OFFS_TIME_M			0x172
#define AQ_OFFS_TIME_S			0x173
#define AQ_OFFS_TIME_D			0x174

/* device type length */
#define AQ_LEN_INT				2
#define AQ_LEN_BYTE				1

/* string lengths */
#define AQ_LEN_FW				5
#define	AQ_LEN_NAME				9
#define AQ_LEN_LANG				4
#define AQ_LEN_FAN_NAME			10
#define AQ_LEN_TEMP_NAME		10
#define AQ_LEN_FLOW_NAME		10

/* pseudo-values */
#define AQ_TEMP_NCONN			0x7d0
#define AQ_FLOW_NCONN			0x0c8

/* sensor / fan count */
#define AQ_NUM_PROFILE			2
#define AQ_NUM_FAN				4
#define AQ_NUM_TEMP				6

/* own types */
typedef uint8_t					aq_byte;
typedef uint16_t 				aq_int;

/* aquaero(R) device data structures */
typedef struct {
	char		*name;
	aq_byte		duty;
	aq_int		rpm;
} aq_fan;

typedef struct {
	char		*name;
	double		value;
	char		connected;
} aq_temp;

typedef struct {
	char		*name;
	double		value;
	char		connected;
} aq_flow;

typedef struct {
	char		*name;
	char		*fw_name;
	char		*language;
	aq_byte		profile;
	aq_byte		prod_year;
	aq_byte		prod_month;
	aq_byte		time_h;
	aq_byte		time_m;
	aq_byte		time_s;
	aq_byte		time_d;
	aq_int 		os_version;
	aq_int 		serial;
	aq_int 		flash_count;
} aq_device;

typedef struct {
	aq_device	device;
	aq_fan		fans[AQ_NUM_FAN];
	aq_temp		temps[AQ_NUM_TEMP];
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
int		aq_dev_init(char **err);
int		aq_dev_poll(char **err);
int 	aq_dev_push(aq_request req_type, char **err);

/* helper functions */
aq_int 	aq_get_int(int offset);
char 	*aq_libusb_strerr(int err);
char 	*aq_strcat(char *str1, char *str2);

/* data extraction, conversion */
void 	aq_get_device(aq_device *device);
void 	aq_get_fan(aq_fan *fan, short num);
void	aq_get_temp(aq_temp *temp, short num);
void	aq_get_flow(aq_flow *flow);

/* functions designated for use from application side */
int		aquaero_init(char **err_msg);
int		aquaero_poll_data(aquaero_data *aq_data, char **err_msg);
int		aquaero_load_profile(aq_byte profile, char **err_msg);
int		aquaero_set_time(aq_byte h, aq_byte m, aq_byte s, aq_byte d,
			char **err_msg);
void	aquaero_exit();
unsigned char *aquaero_get_buffer();

#endif /* DEVICE_H_ */
