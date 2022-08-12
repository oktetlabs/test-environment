/** @file
 * @brief Power Distribution Unit Proxy Test Agent
 *
 * Cold reboot via Net-SNMP library
 *
 * Copyright (C) 2003-2021 OKTET Labs. All rights reserved.
 *
 *
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 *
 */

#include "ta_snmp.h"
#include "te_sockaddr.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_ta.h"

#include "agentlib.h"

/* IP address of the unit */
static struct sockaddr unit_netaddr;

/* Number of outlets in the unit */
static long int unit_size = 0;

/* APC: Default name of SNMP community with read-write access */
const char *apc_rw_community = "private";

/* APC: Commands for controlling outlets */
typedef enum {
    OUTLET_IMMEDIATE_ON     = 1,
    OUTLET_IMMEDIATE_OFF    = 2,
    OUTLET_IMMEDIATE_REBOOT = 3,
} outlet_cmd_t;

/**
 * Open SNMP session to the power unit with current default settings.
 *
 * @return Pointer to new SNMP session (should be closed
 *         with ta_snmp_close_session()), or NULL in the case of error.
 */
ta_snmp_session *
power_snmp_open()
{
    ta_snmp_session *ss;

    ss = ta_snmp_open_session(&unit_netaddr, SNMP_VERSION_1, apc_rw_community);
    if (ss == NULL)
    {
        ERROR("Failed to open SNMP session for %s",
              te_sockaddr_get_ipstr(&unit_netaddr));
    }
    return ss;
}

/**
 * Get the size of outlet array of the unit
 *
 * @param size  Number of outlets (OUT)
 *
 * @return Status code.
 */
te_errno
power_get_size(long int *size)
{
    /* APC PowerMIB sPDUOutletControlTableSize */
    static ta_snmp_oid oid_sPDUOutletControlTableSize[] =
                    { 1, 3, 6, 1, 4, 1, 318, 1, 1, 4, 4, 1, 0 };

    ta_snmp_session *ss;
    te_errno         rc;

    ss = power_snmp_open();
    if (ss == NULL)
        return TE_EFAIL;

    rc = ta_snmp_get_int(ss, oid_sPDUOutletControlTableSize,
                         TE_ARRAY_LEN(oid_sPDUOutletControlTableSize),
                         size);
    ta_snmp_close_session(ss);
    return rc;
}

/**
 * Find number of outlet by its human-readable name
 *
 * @param name  Name of outlet
 * @param num   Number of outlet (OUT)
 *
 * @return Status code.
 */
te_errno
power_find_outlet(const char *name, long int *num)
{
    /* APC PowerMIB sPDUOutletName */
    static ta_snmp_oid oid_sPDUOutletName[] =
                    { 1, 3, 6, 1, 4, 1, 318, 1, 1, 4, 5, 2, 1, 3, 0 };

    long int         i;
    te_errno         rc, retval = TE_ENOENT;
    ta_snmp_session *ss;

    ss = power_snmp_open();
    if (ss == NULL)
        return TE_EFAIL;

    for (i = 1; i <= unit_size; i++)
    {
        char    buf[128];
        size_t  buf_len;

        oid_sPDUOutletName[TE_ARRAY_LEN(oid_sPDUOutletName) - 1] = i;
        buf_len = sizeof(buf);
        if ((rc = ta_snmp_get_string(ss, oid_sPDUOutletName,
                                     TE_ARRAY_LEN(oid_sPDUOutletName),
                                     buf, &buf_len)) != 0)
            continue;

        if (strcmp(buf, name) == 0)
        {
            *num = i;
            retval = 0;
            break;
        }
    }
    ta_snmp_close_session(ss);
    return retval;
}

/**
 * Perform command on specific outlet
 *
 * @param outlet_num  Number of outlet to control
 * @param command     Command to perform
 *
 * @return Status code.
 */
te_errno
power_set_outlet(long int outlet_num, outlet_cmd_t command)
{
    /* APC PowerMIB rPDUOutletControlOutletCommand */
    static oid oid_rPDUOutletControlOutletCommand[] =
                    { 1, 3, 6, 1, 4, 1, 318, 1, 1, 12, 3, 3, 1, 1, 4, 0 };
    ta_snmp_session *ss;
    int              rc;

    if (outlet_num > unit_size || outlet_num == 0)
        return TE_ENOENT;

    ss = power_snmp_open();
    if (ss == NULL)
        return TE_EFAIL;

    oid_rPDUOutletControlOutletCommand[
        TE_ARRAY_LEN(oid_rPDUOutletControlOutletCommand) - 1] = outlet_num;

    if ((rc = ta_snmp_set(ss, oid_rPDUOutletControlOutletCommand,
                          TE_ARRAY_LEN(oid_rPDUOutletControlOutletCommand),
                          TA_SNMP_INTEGER,
                          (uint8_t *)&command, sizeof(command))) != 0)
    {
        ERROR("%s(): failed to perform power outlet command", __FUNCTION__);
    }

    ta_snmp_close_session(ss);
    return rc;
}

/**
 * Perform rebooting an outlet
 *
 * @param id  Decimal number or human-readable name of the outlet
 *
 * @return Status code.
 */
te_errno
power_reboot_outlet(const char *id)
{
    int       rc;
    long int  outlet_num;
    char     *endptr;

    WARN("Rebooting host at outlet '%s'", id);
    outlet_num = strtol(id, &endptr, 10);
    if (*endptr != '\0')
    {
        WARN("Outlet is referenced by name '%s', looking up", id);
        rc = power_find_outlet(id, &outlet_num);
        if (rc != 0)
        {
            ERROR("Failed to find outlet named '%s'", id);
            return rc;
        }
        WARN("Found outlet number %u named '%s'", outlet_num, id);
    }

    return power_set_outlet(outlet_num, OUTLET_IMMEDIATE_REBOOT);
}

/* See description in power_ctl_internal.h */
te_errno
ta_snmp_cold_reboot(const char *id)
{
    return power_reboot_outlet(id);
}

/* See description in power_ctl_internal.h */
int
ta_snmp_init_cold_reboot(char *param)
{
    int rc;

    if ((te_sockaddr_netaddr_from_string(param, &unit_netaddr) != 0))
    {
        ERROR("Failed to start for '%s': invalid unit address",
              param);
        return -1;
    }

    ta_snmp_init();

    if ((rc = power_get_size(&unit_size)) != 0)
    {
        ERROR("Failed to detect the number of outlets of unit, rc=%r", rc);
        return -1;
    }

    RING("Found APC Power Unit at %s with %d outlets",
         param, unit_size);

    return 0;
}
