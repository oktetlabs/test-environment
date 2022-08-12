/** @file
 * @brief Power Distribution Unit Proxy Test Agent
 *
 * Power-ctl TA definitions
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TA_POWER_CTL_INTERNAL_H__
#define __TE_TA_POWER_CTL_INTERNAL_H__

#include "te_config.h"
#include "te_errno.h"

/**
 * Prepare the cold reboot via SNMP.
 *
 * @param param Parameter for initialization the cold reboot
 *
 * @return Status code.
 */
extern int ta_snmp_init_cold_reboot(char *param);

/**
 * Cold reboot for the specified host via SNMP
 *
 * @param id Name of the host
 *
 * @return Status code
 */
extern te_errno ta_snmp_cold_reboot(const char *id);

/**
 * Prepare the cold reboot via shell command.
 *
 * @param param Parameter for initialization the cold reboot
 *
 * @return Status code.
 */
extern int ta_shell_init_cold_reboot(char *param);

/**
 * Cold reboot for the specified host via shell command
 *
 * @param id Name of the host
 *
 * @return Status code
 */
extern te_errno ta_shell_cold_reboot(const char *id);

#endif /* __TE_TA_POWER_CTL_INTERNAL_H__ */