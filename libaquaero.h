/* Copyright 2010-2012 lynix <lynix47@gmail.com>
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

#ifndef LIBAQUAERO_H_
#define LIBAQUAERO_H_

#include <stdint.h>

#define AQ_USB_READ_LEN			552

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

int		aquaero_init(char **err_msg);
int		aquaero_poll_data(aquaero_data *aq_data, char **err_msg);
int		aquaero_load_profile(aq_byte profile, char **err_msg);
int		aquaero_set_time(aq_byte h, aq_byte m, aq_byte s, aq_byte d,
			char **err_msg);
int 	aquaero_set_fan_duty(char fan_no, aq_byte duty, char **err_msg);
void	aquaero_exit();
unsigned char *aquaero_get_buffer();


#endif /* LIBAQUAERO_H_ */
