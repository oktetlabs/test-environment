/** @file
 * @brief Test API - RPC
 *
 * Type definitions for remote calls of power switch.
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_POWER_SW_H__
#define __TE_POWER_SW_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_TYPE_STR_PARPORT    "parport"
#define DEV_TYPE_STR_TTY        "tty"
#define DEV_TYPE_STR_UNSPEC     "unspec"
#define DEV_TYPE_STR_INVAL      "inval"
#define CMD_STR_TURN_ON         "on"
#define CMD_STR_TURN_OFF        "off"
#define CMD_STR_RESTART         "rst"
#define CMD_STR_UNSPEC          "unspec"
#define CMD_STR_INVAL           "inval"

typedef enum {
    DEV_TYPE_UNSPEC,
    DEV_TYPE_PARPORT,
    DEV_TYPE_TTY,
    DEV_TYPE_INVAL
} power_sw_dev_type;

typedef enum {
    CMD_UNSPEC,
    CMD_TURN_ON,
    CMD_TURN_OFF,
    CMD_RESTART,
    CMD_INVAL
} power_sw_cmd;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_POWER_SW_H__ */
