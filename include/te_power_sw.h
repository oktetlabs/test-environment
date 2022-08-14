/** @file
 * @brief Test API - RPC
 *
 * Type definitions for remote calls of power switch.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_POWER_SW_H__
#define __TE_POWER_SW_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_TYPE_STR_PARPORT    "parport"
#define DEV_TYPE_STR_TTY        "tty"
#define DEV_TYPE_STR_DIGISPARK  "digispark"
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
    DEV_TYPE_DIGISPARK,
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
