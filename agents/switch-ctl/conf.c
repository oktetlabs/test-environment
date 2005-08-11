/** @file
 * @brief Switch Control Proxy Test Agent
 *
 * Configuration tree support
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 e
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_NETINET_IN_H /* Required for PoE library */
#include <netinet/in.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(x)
#endif

#include <arpa/inet.h>

#include "te_defs.h"
#include "te_errno.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "poe_lib.h"

#define TE_LGR_USER      "Configurator"
#include "logger_ta.h"

#define VLAN_DEFAULT    1

/**
 * Check return code of the expression and call 'return' if expression
 * return code is not zero.
 *
 * @param _expr     - expression with integer return code
 */
#define CHECK_RC(_expr) \
    do {                                         \
        int _rc = (_expr);                       \
                                                 \
        if (_rc != 0)                            \
        {                                        \
            ERROR("ERROR[%s, %d]",   \
                       __FILE__, __LINE__);      \
            return TE_RC(TE_TA_SWITCH_CTL, _rc); \
        }                                        \
    } while (FALSE)


/** TA name pointer */
extern char *ta_name;

/** Buffer for error string to fill in by PoE library */
static char error_string[POE_LIB_MAX_STRING];
/** Temporary buffer */
static char static_global_buf[64];


/** PoE Switch global data */
static poe_global   poe_global_data;
/** Number of entries in PoE ports array */
static unsigned int n_poe_ports;
/** PoE Switch ports data */
static poe_port    *poe_ports;
/** PoE Switch STP configuration and state */
static poe_stp      poe_stp_data;


static int
snprintf_rc(char *s, size_t len, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if ((size_t)vsnprintf(s, len, fmt, ap) >= len)
    {
        return TE_RC(TE_TA_SWITCH_CTL, TE_ESMALLBUF);
    }
    else
    {
        return 0;
    }
}


/** @name Auxiluary converters */

/**
 * Convert boolean value to string.
 *
 * @param value     - boolean value
 *
 * @return "true", "false" or printed number, if value is unknown
 */
static const char *
boolean_to_string(te_bool value)
{
    int rc;

    switch (value)
    {
#ifdef ENUM_TO_STRINGS
        case FALSE:
            return "false";

        case TRUE:
            return "true";
#endif

        default:
            rc = snprintf_rc(static_global_buf,
                             sizeof(static_global_buf), "%u", value);
            if (rc != 0)
            {
                ERROR("snprintf_rc() failed %d", rc);
            }
            return static_global_buf;
    }
}

/**
 * Convert boolean value in string to number.
 *
 * @param str       - boolean value in string
 * @param p_val     - location for boolean value (OUT):
 *                      TRUE    - "true"
 *                      FALSE   - "false"
 *                      other   - unknown
 *
 * @retval 0        - success
 * @retval TE_EINVAL   - conversion failed
 */
static int
boolean_to_number(const char *str, te_bool *p_val)
{
    long int    val;
    char       *end;

#ifdef ENUM_TO_STRINGS
    if (strcmp(str, "true") == 0)
    {
        *p_val = TRUE;
        return 0;
    }
    if (strcmp(str, "false") == 0)
    {
        *p_val = FALSE;
        return 0;
    }
#endif

    val = strtol(str, &end, 10);
    if ((end == str) || (*end != '\0' && *end != ' '))
    {
        ERROR("Failed to convert string '%s' to number",
                          str);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    if ((val != TRUE) && (val != FALSE))
    {
        ERROR("Invalid boolean value %d", val);
    }

    *p_val = (te_bool)val;
    
    return 0;
}


/**
 * Convert 'unsigned long int' value in string to number.
 *
 * @param str       - boolean value in string
 * @param p_val     - location for value (OUT)
 *
 * @retval 0        - success
 * @retval TE_EINVAL   - conversion failed
 */
static int
ulong_to_number(const char *str, unsigned long int *p_val)
{
    long int    val;
    char       *end;

    val = strtol(str, &end, 10);
    if ((end == str) || (*end != '\0' && *end != ' '))
    {
        ERROR("Failed to convert string '%s' to number",
                          str);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    assert(val >= 0);

    *p_val = (unsigned long int)val;
    
    return 0;
}


/**
 * Convert link status to string.
 *
 * @param status    - link status
 *
 * @return "up", "down" or printed number, if status is unknown
 */
static const char *
link_status_to_string(poe_link_status status)
{
    int rc;

    switch (status)
    {
#ifdef ENUM_TO_STRINGS
        case POE_LINK_DOWN:
            return "down";

        case POE_LINK_UP:
            return "up";
#endif

        default:
            rc = snprintf_rc(static_global_buf,
                             sizeof(static_global_buf), "%d", status);
            if (rc != 0)
            {
                ERROR("snprintf_rc() failed %d", rc);
            }
            return static_global_buf;
    }
}

/**
 * Convert link status string to number.
 *
 * @param str       - link status string
 * @param p_val     - location for link status number (OUT):
 *                      POE_LINK_DOWN    - "down"
 *                      POE_LINK_UP      - "up"
 *                      other            - unknown
 *
 * @retval 0        - success
 * @retval TE_EINVAL   - conversion failed
 */
static int
link_status_to_number(const char *str, poe_link_status *p_val)
{
    long int    val;
    char       *end;

#ifdef ENUM_TO_STRINGS
    if (strcmp(str, "down") == 0)
    {
        *p_val = POE_LINK_DOWN;
        return 0;
    }
    if (strcmp(str, "up") == 0)
    {
        *p_val = POE_LINK_UP;
        return 0;
    }
#endif

    val = strtol(str, &end, 10);
    if ((end == str) || (*end != '\0' && *end != ' '))
    {
        ERROR("Failed to convert string '%s' to number",
                          str);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    if ((val != POE_LINK_DOWN) && (val != POE_LINK_UP))
    {
        ERROR("Invalid link status %d", val);
    }

    *p_val = (poe_link_status)val;
    
    return 0;
}


/**
 * Convert port speed to string.
 *
 * @param speed     - port speed
 *
 * @return "10Mbit", "100Mbit", "Gigabit" or printed number,
 *         if status is unknown
 */
static const char *
port_speed_to_string(poe_port_speed speed)
{
    int rc;

    switch (speed)
    {
#ifdef ENUM_TO_STRINGS
        case POE_SPEED_10:
            return "10Mbit";

        case POE_SPEED_100:
            return "100Mbit";

        case POE_SPEED_1000:
            return "Gigabit";
#endif

        default:
            rc = snprintf_rc(static_global_buf,
                             sizeof(static_global_buf), "%d", speed);
            if (rc != 0)
            {
                ERROR("snprintf_rc() failed %d", rc);
            }
            return static_global_buf;
    }
}

/**
 * Convert port speed string to number.
 *
 * @param str       - port speed string
 * @param p_val     - location for port speed number (OUT):
 *                      POE_SPEED_10    - "10Mbit"
 *                      POE_SPEED_100   - "100Mbit"
 *                      POE_SPEED_1000  - "Gigabit"
 *                      other            - unknown
 *
 * @retval 0        - success
 * @retval TE_EINVAL   - conversion failed
 */
static int
port_speed_to_number(const char *str, poe_port_speed *p_val)
{
    long int    val;
    char       *end;

#ifdef ENUM_TO_STRINGS
    if (strcmp(str, "10Mbit") == 0)
    {
        *p_val = POE_SPEED_10;
        return 0;
    }
    if (strcmp(str, "100Mbit") == 0)
    {
        *p_val = POE_SPEED_100;
        return 0;
    }
    if (strcmp(str, "Gigabit") == 0)
    {
        *p_val = POE_SPEED_1000;
        return 0;
    }
#endif

    val = strtol(str, &end, 10);
    if ((end == str) || (*end != '\0' && *end != ' '))
    {
        ERROR("Failed to convert string '%s' to number",
                          str);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    if ((val != POE_SPEED_10) && (val != POE_SPEED_100) &&
        (val != POE_SPEED_1000))
    {
        ERROR("Invalid port speed %d", val);
    }

    *p_val = (poe_port_speed)val;
    
    return 0;
}


/**
 * Convert duplexity type to string.
 *
 * @param duplexity - duplexity type
 *
 * @return "half", "full" or printed number, if status is unknown
 */
static const char *
duplexity_type_to_string(poe_duplexity_type duplexity)
{
    int rc;

    switch (duplexity)
    {
#ifdef ENUM_TO_STRINGS
        case POE_FULL_DUPLEX:
            return "full";

        case POE_HALF_DUPLEX:
            return "half";
#endif

        default:
            rc = snprintf_rc(static_global_buf,
                             sizeof(static_global_buf), "%d", duplexity);
            if (rc != 0)
            {
                ERROR("snprintf_rc() failed %d", rc);
            }
            return static_global_buf;
    }
}

/**
 * Convert duplexity type string to number.
 *
 * @param str       - duplexity type string
 * @param p_val     - location for duplexity type number (OUT):
 *                      POE_HALF_DUPLEX  - "half"-duplex
 *                      POE_FULL_DUPLEX  - "full"-duplex
 *                      other            - unknown
 *
 * @retval 0        - success
 * @retval TE_EINVAL   - conversion failed
 */
static int
duplexity_type_to_number(const char *str, poe_port_clocks *p_val)
{
    long int    val;
    char       *end;

#ifdef ENUM_TO_STRINGS
    if (strcmp(str, "full") == 0)
    {
        *p_val = POE_FULL_DUPLEX;
        return 0;
    }
    if (strcmp(str, "half") == 0)
    {
        *p_val = POE_HALF_DUPLEX;
        return 0;
    }
#endif

    val = strtol(str, &end, 10);
    if ((end == str) || (*end != '\0' && *end != ' '))
    {
        ERROR("Failed to convert string '%s' to number",
                          str);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    if ((val != POE_HALF_DUPLEX) && (val != POE_FULL_DUPLEX))
    {
        ERROR("Invalid duplexity type %d", val);
    }

    *p_val = (poe_port_clocks)val;
    
    return 0;
}


/**
 * Convert port port clocks to string.
 *
 * @param mode      - port port clocks
 *
 * @return "master", "slave", "auto" or printed number, if status is
 *         unknown
 */
static const char *
port_clocks_to_string(poe_port_clocks clocks)
{
    int rc;

    switch (clocks)
    {
#ifdef ENUM_TO_STRINGS
        case POE_SLAVE:
            return "slave";

        case POE_MASTER:
            return "master";

        case POE_AUTO:
            return "auto";
#endif

        default:
            rc = snprintf_rc(static_global_buf,
                             sizeof(static_global_buf), "%d", clocks);
            if (rc != 0)
            {
                ERROR("snprintf_rc() failed %d", rc);
            }
            return static_global_buf;
    }
}

/**
 * Convert port port clocks string to number.
 *
 * @param str       - port port clocks string
 * @param p_val     - location for port port clocks number (OUT):
 *                      POE_SLAVE   - "slave"
 *                      POE_MASTER  - "master"
 *                      POE_AUTO    - "auto"
 *                      other       - unknown
 *
 * @retval 0        - success
 * @retval TE_EINVAL   - conversion failed
 */
static int
port_clocks_to_number(const char *str, poe_port_clocks *p_val)
{
    long int    val;
    char       *end;

#ifdef ENUM_TO_STRINGS
    if (strcmp(str, "master") == 0)
    {
        *p_val = POE_MASTER;
        return 0;
    }
    if (strcmp(str, "slave") == 0)
    {
        *p_val = POE_SLAVE;
        return 0;
    }
    if (strcmp(str, "auto") == 0)
    {
        *p_val = POE_AUTO;
        return 0;
    }
#endif

    val = strtol(str, &end, 10);
    if ((end == str) || (*end != '\0' && *end != ' '))
    {
        ERROR("Failed to convert string '%s' to number",
                          str);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    if ((val != POE_MASTER) && (val != POE_SLAVE) && (val != POE_AUTO))
    {
        ERROR("Invalid port clocks %d", val);
    }

    *p_val = (poe_port_clocks)val;
    
    return 0;
}

/*@}*/


/** @name Auxiluary routines to work with PoE library */

/**
 * Update PoE global data.
 *
 * @param gid   - request group identifier
 *
 * @return Status code.
 */
static int
update_poe_global(unsigned int gid)
{
    static unsigned int sync_id = (unsigned int)-1;

    if (gid != sync_id)
    {
        int rc = poe_global_read(&poe_global_data, error_string);

        if (rc != 0)
        {
            ERROR("ERROR[%s, %d] %d: %s", __FILE__, __LINE__,
                       rc, error_string);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
        }
        sync_id = gid;
        VERB("Information about PoE switch globals "
                          "updated");
    }
    return 0;
}

/**
 * Update PoE switch ports data.
 *
 * @param gid   - request group identifier
 *
 * @return Status code.
 */
static int
update_poe_ports(unsigned int gid)
{
    static unsigned int sync_id = (unsigned int)-1;

    if (gid != sync_id)
    {
        int rc;

        if ((poe_ports == NULL) ||
            (n_poe_ports != poe_global_data.number_of_ports))
        {
            free(poe_ports);
            n_poe_ports = poe_global_data.number_of_ports;
            poe_ports = (poe_port *)malloc(sizeof(poe_port) * n_poe_ports);
            if (poe_ports == NULL)
                return TE_RC(TE_TA_SWITCH_CTL, TE_ENOMEM);
        }
        
        rc = poe_port_read_table(poe_ports, error_string);
        if (rc != 0)
        {
            ERROR("ERROR[%s, %d] %d: %s", __FILE__, __LINE__,
                       rc, error_string);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
        }
        sync_id = gid;
        VERB("Information about PoE switch ports updated");
    }
    return 0;
}


/**
 * Find up-to-date switch port.
 * 
 * @param gid       - request group identifer
 * @param pid_str   - port gidas string
 * @param p_port    - location for port pointer (OUT)
 *
 * @return Status code.
 * @retval 0        - success
 * @retval TE_ENOENT   - no match port found
 * @retval TE_EINVAL   - invalid argument
 * @retval other    - PoE library call return code
 */
static int
find_port(unsigned int gid, const char *pid_str, poe_port **p_port)
{
    char           *end;
    long int        pid;
    unsigned int    i;

    if (p_port == NULL)
    {
        ERROR("Invalid location for port pointer");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    if (pid_str == NULL)
    {
        ERROR("NULL pointer to port ID string");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    pid = strtol(pid_str, &end, 10);
    if (end == pid_str)
    {
        ERROR("Conversion of port ID from string '%s' "
                          "failed", pid_str);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    CHECK_RC(update_poe_global(gid));
    CHECK_RC(update_poe_ports(gid));

    for (i = 0;
         (i < poe_global_data.number_of_ports) &&
             (poe_ports[i].id != pid);
         ++i);

    if (i == poe_global_data.number_of_ports)
    {
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
    }
    else
    {
        *p_port = poe_ports + i;
        return 0;
    }
}


/**
 * Update PoE STP global data.
 *
 * @param gid   - request group identifier
 *
 * @return Status code.
 */
static int
update_poe_stp(unsigned int gid)
{
    static unsigned int sync_id = (unsigned int)-1;

    if (gid != sync_id)
    {
        int rc = poe_stp_read(&poe_stp_data, error_string);

        if (rc != 0)
        {
            ERROR("ERROR[%s, %d] %d: %s", __FILE__, __LINE__,
                       rc, error_string);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
        }
        sync_id = gid;
        VERB("Information about PoE switch STP "
                          "updated");
    }
    return 0;
}

/*@}*/


/** @name Configuration tree accessors */

/**
 * Get number of switch ports.
 *
 * @param gid       - request group identifier
 * @param oid       - instance identifer
 * @param value     - location for value as string
 *
 * @return Status code.
 */
static int
get_number_of_ports(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_global(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_global_data.number_of_ports);
}


/**
 * Get list of Switch ports.
 *
 * @param gid       - request group identifier
 * @param oid       - full identifier of the father instance
 * @param list      - location for the list pointer
 *
 * @return Status code
 * @retval 0        - success
 * @retval TE_ENOMEM   - cannot allocate memory
 */
static int
list_ports(unsigned int gid, const char *oid, char **list)
{
    size_t str_len;

    UNUSED(oid);

    CHECK_RC(update_poe_global(gid));
    CHECK_RC(update_poe_ports(gid));

    /*
     * It's assumed that 3 characters plus one for space of \0 are
     * sufficient for each port number.
     */
    str_len = (3 + 1) * poe_global_data.number_of_ports * sizeof(char);
    *list = (char *)malloc(str_len);
    if (*list == NULL)
    {
        ERROR("Failed to allocate %u octets", str_len);
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOMEM);
    }
    else
    {
        int             ret;
        char           *s;
        unsigned int    i;

        for (i = 0, s = *list;
             i < poe_global_data.number_of_ports;
             ++i, s += ret, str_len -= ret)
        {
            ret = snprintf(s, str_len, "%u ", poe_ports[i].id);
            if ((size_t)ret >= str_len)
            {
                free(*list);
                ERROR("Too small buffer allocated");
                return TE_RC(TE_TA_SWITCH_CTL, TE_ESMALLBUF);
            }
        }
        VERB("number_of_ports = %d\nPort list = %s",
                   poe_global_data.number_of_ports, *list);
        return 0;
    }
}


/**
 * Commit configuration to switch port.
 *
 * @param gid       - request group identifier
 * @param p_oid     - configurator object instance identifier
 *
 * @return Status code.
 */
static int 
port_commit(unsigned int gid, const cfg_oid *p_oid)
{
    int         rc;
    poe_port   *port;
    const char *pid_str;

    pid_str = ((cfg_inst_subid *)(p_oid->ids))[p_oid->len - 1].name;
    CHECK_RC(find_port(gid, pid_str, &port));

    VERB("Commit configuration of port #%u", port->id);

    rc = poe_port_update(port, error_string);
    if (rc != 0)
    {
        ERROR("ERROR[%s, %d] %d: %s", __FILE__, __LINE__,
                   rc, error_string);
        rc = TE_EIO;
    }

    return TE_RC(TE_TA_SWITCH_CTL, rc);
}


/**
 * Get type of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int
port_get_type(unsigned int gid, const char *oid, char *value,
              const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%d", port->type);
}


/**
 * Get administrative status of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_admin_status(unsigned int gid, const char *oid, char *value,
                      const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
                       link_status_to_string(port->admin.status));
}

/**
 * Set administrative status of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_set_admin_status(unsigned int gid, const char *oid, const char *value,
                      const char *pid_str)
{
    poe_link_status     val;
    poe_port           *port;

    UNUSED(oid);

    CHECK_RC(link_status_to_number(value, &val));

    CHECK_RC(find_port(gid, pid_str, &port));

    port->admin.status = val;

    return 0;
}


/**
 * Get administrative autonegotiation parameter of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_autonegotiation(unsigned int gid, const char *oid, char *value,
                         const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
                       boolean_to_string(port->admin._auto));
}


/**
 * Enable/disable autonegotiation on switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_set_autonegotiation(unsigned int gid, const char *oid,
                         const char *value, const char *pid_str)
{
    te_bool     val;
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(boolean_to_number(value, &val));

    CHECK_RC(find_port(gid, pid_str, &port));

    port->admin._auto = val;

    return 0;
}


/**
 * Get administrative speed of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_admin_speed(unsigned int gid, const char *oid, char *value,
                     const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
                       port_speed_to_string(port->admin.speed));
}

/**
 * Set administrative speed of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_set_admin_speed(unsigned int gid, const char *oid,
                     const char *value, const char *pid_str)
{
    poe_port_speed  val;
    poe_port       *port;

    UNUSED(oid);

    CHECK_RC(port_speed_to_number(value, &val));

    CHECK_RC(find_port(gid, pid_str, &port));

    port->admin.speed = val;

    return 0;
}


/**
 * Get administrative duplexity type of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_admin_duplexity(unsigned int gid, const char *oid, char *value,
                         const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
               duplexity_type_to_string(port->admin.duplexity));
}

/**
 * Set administrative duplexity of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_set_admin_duplexity(unsigned int gid, const char *oid,
                         const char *value, const char *pid_str)
{
    poe_duplexity_type  val;
    poe_port           *port;

    UNUSED(oid);

    CHECK_RC(duplexity_type_to_number(value, &val));

    CHECK_RC(find_port(gid, pid_str, &port));

    port->admin.duplexity = val;

    return 0;
}


/**
 * Get administrative port clocks (master/slave/auto) of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_admin_clocks(unsigned int gid, const char *oid, char *value,
                      const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
                       port_clocks_to_string(port->admin.master));
}

/**
 * Set administrative clock (master/slave/auto) of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_set_admin_clocks(unsigned int gid, const char *oid,
                      const char *value, const char *pid_str)
{
    poe_port_clocks     val;
    poe_port           *port;

    UNUSED(oid);

    CHECK_RC(port_clocks_to_number(value, &val));

    CHECK_RC(find_port(gid, pid_str, &port));

    port->admin.master = val;

    return 0;
}


/**
 * Get MTU of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_mtu(unsigned int gid, const char *oid, char *value,
             const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%u", port->admin.mtu);
}

/**
 * Set MTU of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_set_mtu(unsigned int gid, const char *oid, const char *value,
             const char *pid_str)
{
    long int    mtu;
    char       *end;
    poe_port   *port;

    UNUSED(oid);

    mtu = strtol(value, &end, 10);
    if ((end == value) || (*end != '\0' && *end != ' '))
    {
        ERROR("Failed to convert string '%s' to number",
                          value);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    if (mtu < 0)
    {
        ERROR("Invalid MTU value %d", mtu);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    CHECK_RC(find_port(gid, pid_str, &port));

    port->admin.mtu = mtu;

    return 0;
}

/**
 * Get default VLAN tag of the port.
 *
 * @param gid       group identifier
 * @param oid       full instance OID
 * @param value     location for value
 * @param pid_str   port identifer as string
 *
 * @return status code
 */
static int 
port_get_default_vlan(unsigned int gid, const char *oid, char *value,
                      const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));
    
    if (strcmp(port->vlan.default_vlan, "default") == 0)
    {
        
        sprintf(value, "%d", VLAN_DEFAULT);
        return 0;
    }
    else /* It is assumed that names are equal to tags in this model */
        return snprintf_rc(value, RCF_MAX_VAL, "%s",
                           port->vlan.default_vlan);
}

/**
 * Change default VLAN tag of the port.
 *
 * @param gid       group identifier
 * @param oid       full instance OID
 * @param value     new tag
 * @param pid_str   port identifer as string
 *
 * @return status code
 */
static int 
port_set_default_vlan(unsigned int gid, const char *oid, const char *value,
                      const char *pid_str)
{
    poe_port *port;

    UNUSED(oid);
    
    CHECK_RC(find_port(gid, pid_str, &port));

    if (*value - '0' == VLAN_DEFAULT)
        strcpy(port->vlan.default_vlan, "default");
    else
        strncpy(port->vlan.default_vlan, value,
                sizeof(port->vlan.default_vlan));

    return 0;
}

/**
 * Get CoS priority for untagged frames on the port.
 *
 * @param gid       group identifier
 * @param oid       full instance OID
 * @param value     location for value
 * @param pid_str   port identifer as string
 *
 * @return status code
 */
static int 
port_get_untagged_priority(unsigned int gid, const char *oid, char *value,
                           const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));
    
    return snprintf_rc(value, RCF_MAX_VAL, "%u", port->cos.untagged_prio);
}

/**
 * Change CoS priority for untagged frames on the port.
 *
 * @param gid       group identifier
 * @param oid       full instance OID
 * @param value     new priority
 * @param pid_str   port identifer as string
 *
 * @return status code
 */
static int 
port_set_untagged_priority(unsigned int gid, const char *oid,
                           const char *value, const char *pid_str)
{
    poe_port *port;

    UNUSED(oid);
    
    CHECK_RC(find_port(gid, pid_str, &port));

    if (*value > '7' || *value < '0' || 
        (*(value + 1) != ' ' && *(value + 1) != 0))
    {
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
        
    port->cos.untagged_prio = *value - '0';

    return 0;
}

/**
 * Get operational status of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_oper_status(unsigned int gid, const char *oid, char *value,
                     const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
                       link_status_to_string(port->state.status));
}

/**
 * Get operational speed of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_oper_speed(unsigned int gid, const char *oid, char *value,
                    const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
               port_speed_to_string(port->state.speed));
}


/**
 * Get operational duplexity of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_oper_duplexity(unsigned int gid, const char *oid, char *value,
                        const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
               duplexity_type_to_string(port->state.duplexity));
}


/**
 * Get operational clocks (master/slave/auto) of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_oper_clocks(unsigned int gid, const char *oid, char *value,
                     const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "-1");
}


/**
 * Get HOL blocking state of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_hol_blocking(unsigned int gid, const char *oid, char *value,
                      const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%s",
                       boolean_to_string(port->state.hol_blocking));
}


/**
 * Get backpresure state of switch port.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 * @param pid_str   - port identifer as string
 *
 * @return Status code.
 */
static int 
port_get_bps(unsigned int gid, const char *oid, char *value,
             const char *pid_str)
{
    poe_port   *port;

    UNUSED(oid);

    CHECK_RC(find_port(gid, pid_str, &port));

    return snprintf_rc(value, RCF_MAX_VAL, "%d", port->state.bps);
}

/**
 * Get Ageing time value configured on the switch.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - location for value
 *
 * @return Status code.
 */
static int 
get_a_time(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_global(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_global_data.a_time);
}

/**
 * Set a new Ageing time value on the switch.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 *
 * @return Status code.
 */
static int 
set_a_time(unsigned int gid, const char *oid, const char *value)
{
    long int  a_time;
    char     *end_ptr;
 
    UNUSED(oid);
 
    if (value == NULL || *value == '\0' ||
        (a_time = strtol(value, &end_ptr, 10), *end_ptr != '\0'))
    {
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    CHECK_RC(update_poe_global(gid));
    poe_global_data.a_time = a_time;

    if (poe_global_write(&poe_global_data, error_string) != 0)
    {
        VERB("Cannot update Ageing Time: %s", error_string);
        if (poe_global_read(&poe_global_data, error_string) != 0)
        {
            ERROR("Cannot retrieve switch global settings"
                       "after failure: %s", error_string);
        }
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    return 0;
}

/**
 * Get number of CoS queues.
 *
 * @param gid       request group identifier
 * @param oid       full instance identifier
 * @param value     location for value
 *
 * @return status code
 */
static int 
cos_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    
    if (update_poe_global(gid) != 0)
    {
        ERROR("ERROR[%s, %d]", __FILE__, __LINE__);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    sprintf(value, "%lu", poe_global_data.cos_number_of_queues);
    
    return 0;
}

/**
 * Set number of CoS queues.
 *
 * @param gid       request group identifier
 * @param oid       full instance identifier
 * @param value     new number of CoS queues
 *
 * @return status code
 */
static int 
cos_set(unsigned int gid, const char *oid, char *value)
{
    unsigned int i;
    unsigned int step; /* Number of priorities mapped to the one queue */
    
    UNUSED(gid);
    UNUSED(oid);
    
    if (*value != '1' && *value != '2' && *value != '4')
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);

    if (update_poe_global(gid) != 0)
    {
        ERROR("ERROR[%s, %d]", __FILE__, __LINE__);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    poe_global_data.cos_number_of_queues = *value - '0';
    
    step = 8 / poe_global_data.cos_number_of_queues;
    for (i = 0; i < poe_global_data.cos_number_of_queues; i++)
        memset(poe_global_data.cos_mapping + i * step, i, step);
        
    if (poe_global_write(&poe_global_data, error_string) != 0)
    {
        VERB("Cannot update CoS mapping: %s", error_string);
        if (poe_global_read(&poe_global_data, error_string) != 0)
        {
            ERROR("Cannot retrieve switch global settings"
                       "after failure: %s", error_string);
        }
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }
     
    return 0;   
}



/** Poiter to the cached ARL table */
static poe_arl  *arl_table = NULL;
/** Number of entries in ARL Table  */
static int       arl_table_num = 0;

/**
 * Update cache of the Switch ARL Table.
 * This function force actual update of the cache only if gid parameter is
 * different from the parameter of the previous call.
 *
 * @param gid   - request identifier
 *
 * @return Status code.
 */
static int
arl_cache_update(unsigned int gid)
{
    static unsigned int prev_gid = (unsigned int)-1;

    if (gid != prev_gid)
    {
        if (arl_table != NULL)
            free(arl_table);

        arl_table_num = 0;
        arl_table = NULL;

        if (poe_arl_read_table(&arl_table, &arl_table_num,
                               error_string) != 0)
        {
            ERROR("Cannot read ARL table ERROR %s", error_string);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
        }
        F_VERB("DUT ARL table contains %d entries", arl_table_num);

        prev_gid = gid;
    }

    return 0;
}

/**
 * Parses instance name of an ARL entry
 *
 * @param inst_name  - Instance name to be parsed
 * @param arl_entry  - Location for the parsed fields
 *
 * @return Status of the operation
 * @retval TE_EINVAL - Invalid format of the instance name
 * @retval 0      - It's all right.
 */
static int
arl_parse_inst_name(const char *inst_name, poe_arl *arl_entry)
{
    char         *vlan_name;
    char         *end;
    unsigned int  addr[6];
    unsigned int  i;
    int           entry_type;

    /* Extract all the values for a new ARL entry */
    entry_type = (int)strtol(inst_name, &end, 10);
    if (inst_name == end || *end != '.' ||
        (entry_type != 1 && entry_type != 0))
    {
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    arl_entry->is_static = (te_bool)entry_type;
    inst_name = end + 1;

    arl_entry->port = (poe_pid)strtol(inst_name, &end, 10);
    if (inst_name == end || *end != '.') 
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);

    inst_name = end + 1;

    if (sscanf(inst_name, "%02x:%02x:%02x:%02x:%02x:%02x",
               addr + 0, addr + 1, addr + 2,
               addr + 3, addr + 4, addr + 5) != 6)
    {
        ERROR("Instance name - bad format");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    for (i = 0; i < sizeof(addr) / sizeof(addr[0]); i++)
    {
        arl_entry->mac.v[i] = addr[i];
    }

    if ((vlan_name = strchr(inst_name, '.')) == NULL)
    {
        ERROR("Instance name - bad format");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    vlan_name++;

    if (strlen(vlan_name) >= sizeof(arl_entry->vlan))
    {
        ERROR("VLAN name is too long");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    strcpy(arl_entry->vlan, vlan_name);

    return 0;
}

/**
 * Finds ARL entry by instance name (ARL entry are searched in 
 * the list of local ARL entries and global one)
 *
 * @param gid         - request group identifier
 * @param inst_name   - ARL entry instance name
 * @param arl_entry   - Location for the ARL entry (OUT)
 *
 * @return Status of the operation
 * @retval TE_ENOENT  There is no ARL entry with requested instance name
 * @retval 0       An entry successfully found
 */
static int
arl_entry_find(unsigned int gid, const char *inst_name, poe_arl **arl_entry)
{
    poe_arl parsed_entry;
    int     i;

    /* Parse entry_name */
    CHECK_RC(arl_parse_inst_name(inst_name, &parsed_entry));
    arl_cache_update(gid);

    for (i = 0; i < arl_table_num; i++)
    {
        if (memcmp(&(arl_table[i].mac), &(parsed_entry.mac),
                   sizeof(arl_table[i].mac)) == 0 &&
            strcmp(arl_table[i].vlan, parsed_entry.vlan) == 0)
        {
            *arl_entry = arl_table + i;
            break;
        }
    }

    if (i == arl_table_num)
    {
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
    }

    return 0;
}

/**
 * Obtains Type of specified ARL entry (static/dynamic)
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value: (OUT)
 *                        dynamic - 0
 *                        static  - 1
 * @param table_name  - ARL table instance name - empty string
 * @param entry_name  - ARL entry name
 */
static int 
arl_get_type(unsigned int gid, const char *oid, char *value,
             const char *table_name, const char *entry_name)
{
    poe_arl *arl_entry;

    UNUSED(oid);
    UNUSED(table_name);

    ENTRY("gid = %d, oid = %s, entry_name = %s",
                 gid, oid, entry_name);

    CHECK_RC(arl_entry_find(gid, entry_name, &arl_entry));

    /* @todo Fix 1 and 0 hard code */
    return snprintf_rc(value, RCF_MAX_VAL, "%d", 
                       arl_entry->is_static == TRUE ? 1 : 0);
}

/**
 * Obtains VLAN name of specified ARL entry
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the VLAN name (OUT)
 * @param table_name  - ARL table instance name - empty string
 * @param entry_name  - ARL entry entity name
 */
static int 
arl_get_vlan(unsigned int gid, const char *oid, char *value,
             const char *table_name, const char *entry_name)
{
    poe_arl *arl_entry;

    UNUSED(oid);
    UNUSED(table_name);

    ENTRY("gid = %d, oid = %s, entry_name = %s",
                 gid, oid, entry_name);

    CHECK_RC(arl_entry_find(gid, entry_name, &arl_entry));

    return snprintf_rc(value, RCF_MAX_VAL, "%s", arl_entry->vlan);
}

/**
 * Obtains Port number of specified ARL entry
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value:
 *                      Port number in string format (OUT)
 * @param table_name  - ARL table instance name - empty string
 * @param entry_name  - ARL entry name
 */
static int 
arl_get_port(unsigned int gid, const char *oid, char *value,
             const char *table_name, const char *entry_name)
{
    poe_arl *arl_entry;

    UNUSED(oid);
    UNUSED(table_name);

    ENTRY("gid = %d, oid = %s, entry_name = %s",
                 gid, oid, entry_name);

    CHECK_RC(arl_entry_find(gid, entry_name, &arl_entry));

    return snprintf_rc(value, RCF_MAX_VAL, "%u", arl_entry->port);
}

/**
 * Obtains MAC address of specified entry
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value:
 *                      MAC address in string format (OUT)
 * @param table_name  - ARL table instance name - empty string
 * @param entry_name  - ARL entry name
 */
static int 
arl_get_mac(unsigned int gid, const char *oid, char *value,
            const char *table_name, const char *entry_name)
{
    poe_arl *arl_entry;

    UNUSED(oid);
    UNUSED(table_name);

    ENTRY("gid = %d, oid = %s, entry_name = %s",
                 gid, oid, entry_name);

    CHECK_RC(arl_entry_find(gid, entry_name, &arl_entry));

    return snprintf_rc(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
                       arl_entry->mac.v[0], arl_entry->mac.v[1],
                       arl_entry->mac.v[2], arl_entry->mac.v[3],
                       arl_entry->mac.v[4], arl_entry->mac.v[5]);
}

/**
 * Add a new ARL entry to the ARL table.
 *
 * @param gid        - request group identifier
 * @param oid        - full instance identifier
 * @param value      - value to set
 * @param table_name - Name of ARL table - MUST be an empty string
 * @param entry_name - Name of ARL entry to be added, it has 
 *                     the following format:
 *                         <entry_type>.<port_num>.<mac_addr>:<vlan_name>
 *                     Where <entry_type> can be one of the following:
 *                        dynamic - 0
 *                        static  - 1
 *
 * @return Status code.
 */
static int
arl_add_entry(unsigned int gid, const char *oid, const char *value,
              const char *table_name, const char *entry_name)
{
    poe_arl arl_entry;

    ENTRY("gid = %d, oid = %s, value = %s, tbl_name = %s, "
                 "entry_name = %s",
                 gid, oid, value, table_name, entry_name);

    CHECK_RC(arl_parse_inst_name(entry_name, &arl_entry));

    /* Add a new entry */
    ENTRY("Try to add:\n"
                 "MAC: %x:%x:%x:%x:%x:%x\n"
                 "Port: %d\n"
                 "VLAN name: %s\n"
                 "Type: %s",
                 arl_entry.mac.v[0], arl_entry.mac.v[1],
                 arl_entry.mac.v[2], arl_entry.mac.v[3],
                 arl_entry.mac.v[4], arl_entry.mac.v[5],
                 arl_entry.port,
                 arl_entry.vlan,
                 arl_entry.is_static ? "static" : "dynamic");

    if (poe_arl_create(&arl_entry, error_string) != 0)
    {
        ERROR("poe_arl_create FAIL: %s", error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    return 0;
}

/**
 * Deletes specified ARL entry from ARL table.
 *
 * @param gid        - request group identifier
 * @param oid        - full instance identifier
 * @param table_name - Name of ARL table - MUST be an empty string
 * @param entry_name - Name of ARL entry to be added, it has 
 *                     the following format:
 *                         <entry_type>.<port_num>.<mac_addr>:<vlan_name>
 *
 * @return Status code.
 */
static int
arl_del_entry(unsigned int gid, const char *oid,
              const char *table_name, const char *entry_name)
{
    poe_arl *arl_entry;
    poe_arl  parsed_entry;

    UNUSED(oid);
    UNUSED(table_name);

    ENTRY("oid = %s, entry_name = %s",
                 oid, entry_name);

    CHECK_RC(arl_parse_inst_name(entry_name, &parsed_entry));
    CHECK_RC(arl_entry_find(gid, entry_name, &arl_entry));

    /* Delete the entry from the Switch */
    if (poe_arl_delete(&parsed_entry, error_string) != 0)
    {
        ERROR("Cannot delete ARL entry from the NUT");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    return 0;
}

/**
 * Get the list of ARL entries.
 *
 * @param gid       - request group identifier
 * @param oid       - full identifier of the father instance
 * @param list      - location for the list pointer
 *
 * @return Status code
 * @retval 0        - success
 * @retval TE_ENOMEM   - cannot allocate memory
 */
static int
arl_list(unsigned int gid, const char *oid, char **list)
{
    poe_mac *mac;
    size_t   str_len;
    char    *ptr;
    int      i;

    UNUSED(oid);

    ENTRY("gid %d, oid = %s", gid, oid);

    arl_cache_update(gid);

    /* 
     * Each entry name has the following format:
     * <entry_type>.<port_num>.<mac_addr>:<vlan_name>
     */
    for (str_len = 1, i = 0; i < arl_table_num; i++)
    {
        str_len += (2 + 4 + 6 * 3 + strlen(arl_table[i].vlan) + 1);
    }
    if ((*list = (char *)malloc(str_len)) == NULL)
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOMEM);

    (*list)[0] = '\0';

    for (ptr = *list, i = 0; i < arl_table_num; i++)
    {
        mac = &(arl_table[i].mac);
        snprintf(ptr, str_len - ((size_t)ptr - (size_t)*list),
                 "%d.%d.%02x:%02x:%02x:%02x:%02x:%02x.%s ",
                 arl_table[i].is_static, arl_table[i].port,
                 mac->v[0], mac->v[1], mac->v[2],
                 mac->v[3], mac->v[4], mac->v[5],
                 arl_table[i].vlan);
        ptr = strchr(ptr, ' ');
        if (ptr == NULL)
        {
            free(*list);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EFAULT);
        }
        ptr++;
    }

    VERB("%s", *list);

    return 0;
}

/* STP module related functions */

/**
 * Variable to keep temporary STP port entry - not committed yet.
 * At the present time no more than one entry of this type can be
 * created. In the future, if there is need in having more than one
 * entry simultaniously, you may extend such variable to the list
 * of local entries.
 *
 * @sa 
 */
static poe_stp_port local_stp_port_entry;

/** Flags to control state of the local entry */
static uint32_t local_stp_port_entry_flags;

/**
 * Falags to control which fields in stp_port_entry_local structure are set
 */
#define STP_PORT_ENTRY_PNUM_SET      0x01
#define STP_PORT_ENTRY_PRIO_SET      0x02
#define STP_PORT_ENTRY_PATH_COST_SET 0x04

#define STP_PORT_ENTRY_CLEAR(flags) ((flags) = 0)
#define STP_PORT_ENTRY_READY(flags) \
            ((flags) == (STP_PORT_ENTRY_PNUM_SET |      \
                         STP_PORT_ENTRY_PRIO_SET |      \
                         STP_PORT_ENTRY_PATH_COST_SET))

/** An array of Switch ports available for STP */
static poe_stp_port *stp_port_table = NULL;
/** The number of entries in STP port table */
static int stp_port_table_num = 0;

/**
 * Determines if STP port entry is one of aready committed or not.
 *
 * @param stp_port_entry  - entry to check
 *
 * @return TRUE if the STP port entry has been alread committed,
 * and FALSE otherwise
 */
static te_bool
stp_port_entry_committed(const poe_stp_port *stp_port_entry)
{
    assert(stp_port_entry != NULL);

    return ((stp_port_entry == &local_stp_port_entry) ? FALSE : TRUE);
}

/**
 * Obtains flags of local STP port entry
 * (an entry that hasn't been committed yet)
 *
 * @param stp_port_entry  - local STP port entry
 *
 * @return Flags of the entry
 */
static uint8_t
local_stp_port_entry_get_flags(const poe_stp_port *stp_port_entry)
{
    UNUSED(stp_port_entry);

    return local_stp_port_entry_flags;
}

/**
 * Appends additional flags to local STP port entry
 *
 * @param stp_port_entry  - local STP port entry
 * @param flags      - Flags to add
 */
static void
local_stp_port_entry_add_flags(const poe_stp_port *stp_port_entry,
                               uint8_t flags)
{
    UNUSED(stp_port_entry);

    local_stp_port_entry_flags |= flags;
}

/**
 * Verifies if all the fields of the local STP port entry is set
 *
 * @param stp_port_entry  - local STP port entry
 *
 * @return TRUE if all the fields of the entry are filled in,
 * and FALSE otherwise
 */
static te_bool
local_stp_port_entry_ready(const poe_stp_port *stp_port_entry)
{
    UNUSED(stp_port_entry);

    return STP_PORT_ENTRY_READY(local_stp_port_entry_flags);
}

/**
 * Returns pointer to a new local STP port entry.
 *
 * @return Pointer to the new local entry
 */
static poe_stp_port *
local_stp_port_entry_new()
{
    if (local_stp_port_entry_flags != 0)
        return NULL;

    return &local_stp_port_entry;
}

/**
 * Marks local entry as unused, so that it returns to the pool of free
 * local STP port entries
 * 
 * @param stp_port_entry  - local STP port entry to free
 */
static void
local_stp_port_entry_delete(poe_stp_port *stp_port_entry)
{
    UNUSED(stp_port_entry);

    assert(stp_port_entry == &local_stp_port_entry);

    local_stp_port_entry_flags = 0;
}

/**
 * Update cache of the STP port related information.
 * This function force actual update of the cache only if gid parameter is
 * different from the parameter of the previous call.
 *
 * @param gid   - request identifier
 *
 * @return Status code.
 */
static int
stp_cache_update(unsigned int gid)
{
    static unsigned int prev_gid = (unsigned int)-1;

    int          rc;

    if (gid != prev_gid)
    {
        if (stp_port_table != NULL)
            free(stp_port_table);

        stp_port_table_num = 0;
        stp_port_table = NULL;

        if ((rc = poe_stp_read_table(&stp_port_table, 
                                     &stp_port_table_num,
                                     error_string)) != 0)
        {
            ERROR("Cannot read STP Port table ERROR %d, %s",
                       rc, error_string);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
        }
        prev_gid = gid;
        VERB("STP Port Table cache is updated: ptr = %x, "
                   " num = %d", stp_port_table, stp_port_table_num);
    }

    return 0;
}

/**
 * Finds specified STP Port entry by port number
 *
 * @param gid      - request group identifier
 * @param port_id  - Port identifier (port number) whose STP-related
 *                   information we will return
 * @param entry    - Location for entry found (OUT)
 *
 * @return Status of the operation
 * @retval TE_ENOENT   There is no STP port entry corresponding specified
 *                  port id
 * @retval TE_EINVAL   Format of port id is incorrect (not a number)
 * @retval 0        STP port entry successfully found
 */
static int
stp_port_entry_find(unsigned int gid, const char *port_id,
                    poe_stp_port **entry)
{
    poe_pid   pid;
    long int  tmp_pid;
    char     *end_ptr;
    int       i;

    if (port_id == NULL)
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);

    tmp_pid = strtol(port_id, &end_ptr, 10);
    if (*port_id == '\0' || *end_ptr != '\0' || tmp_pid < 0 || tmp_pid > 50)
    {
        ERROR("Invalid format of value of the Port ID: %s",
                   port_id);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    pid = (poe_pid)tmp_pid;

    stp_cache_update(gid);

    for (i = 0; i < stp_port_table_num; i++)
    {
        if (stp_port_table[i].port == pid)
        {
            *entry = stp_port_table + i;
            break;
        }
    }

    if (i == stp_port_table_num)
    {
        /*
         * Try to find entry in the list of the local entries.
         * At the present time it is allowed to have no more than one
         * local entry and it is located in local_stp_port_entry variable.
         * Flags related to the local entry are in
         * local_stp_port_entry_flags variable.
         */
        if (local_stp_port_entry_flags & STP_PORT_ENTRY_PNUM_SET)
        {
            if (local_stp_port_entry.port == pid)
            {
                *entry = &local_stp_port_entry;
                return 0;
            }
        }
        
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
    }

    return 0;
}

#define STP_COMMITTED_PORT_ENTRY_FIND(_gid, _port_num, _port_entry)  \
    do {                                                               \
        CHECK_RC(stp_port_entry_find(_gid, _port_num, (_port_entry))); \
        if (!stp_port_entry_committed(*(_port_entry)))                 \
            return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);                    \
    } while (0)

/**
 * Commit global STP configuration.
 *
 * @param gid       - request group identifier
 * @param p_oid     - configurator object instance identifier
 *
 * @return Status code.
 */
static int 
stp_commit(unsigned int gid, const cfg_oid *p_oid)
{
    int         rc;

    UNUSED(gid);
    UNUSED(p_oid);
    VERB("Commit STP configuration");

    rc = poe_stp_write(&poe_stp_data.admin, error_string);
    if (rc != 0)
    {
        ERROR("poe_stp_write() failed: %d: %s", rc,
                          error_string);
        rc = TE_EIO;
    }

    return TE_RC(TE_TA_SWITCH_CTL, rc);
}

/**
 * Obtains Status of STP module (disabled/enabled)
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the status of the STP module: (OUT)
 *                        disabled  - 0
 *                        enabled   - 1
 */
static int 
stp_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.admin.enabled);
}

/**
 * Change Status of STP module
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - New status of the STP module:
 *                        to disable  - 0
 *                        to enable   - 1
 */
static int 
stp_set(unsigned int gid, const char *oid, const char *value)
{
    unsigned long int val;

    UNUSED(gid);
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));
    CHECK_RC(ulong_to_number(value, &val));

    poe_stp_data.admin.enabled = (te_bool)val;

    return 0;
}

/**
 * Get STP priority parameter.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_prio(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.admin.prio);

    return 0;
}

/**
 * Set STP priority parameter.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 *
 * @return Status code.
 */
static int 
stp_set_prio(unsigned int gid, const char *oid, const char *value)
{
    unsigned long int   val;

    UNUSED(gid);
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));
    CHECK_RC(ulong_to_number(value, &val));

    poe_stp_data.admin.prio = val;

    return 0;
}


/**
 * Get STP 'bridge max age' parameter.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_bridge_max_age(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    VERB("stp_get_bridge_max_age [1]");

    CHECK_RC(update_poe_stp(gid));

    VERB("stp_get_bridge_max_age [2]");

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.admin.max_age);
}

/**
 * Set STP 'bridge max age' parameter.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 *
 * @return Status code.
 */
static int 
stp_set_bridge_max_age(unsigned int gid, const char *oid, const char *value)
{
    unsigned long int   val;

    UNUSED(gid);
    UNUSED(oid);

    VERB("stp_set_bridge_max_age [1]\n"
               "New bridge max age %s", value);

    CHECK_RC(ulong_to_number(value, &val));

    VERB("stp_set_bridge_max_age [2]");

    poe_stp_data.admin.max_age = val;

    return 0;
}


/**
 * Get STP 'bridge hello time' parameter.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_bridge_hello_time(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.admin.hello_time);
}

/**
 * Set STP 'bridge hello time' parameter.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 *
 * @return Status code.
 */
static int 
stp_set_bridge_hello_time(unsigned int gid, const char *oid,
                          const char *value)
{
    unsigned long int   val;

    UNUSED(gid);
    UNUSED(oid);

    CHECK_RC(ulong_to_number(value, &val));

    poe_stp_data.admin.hello_time = val;

    return 0;
}


/**
 * Get STP 'bridge forward delay' parameter.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_bridge_forward_delay(unsigned int gid, const char *oid,
                             char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.admin.forw_delay);
}

/**
 * Set STP 'bridge forward delay' parameter.
 *
 * @param gid       - request group identifier
 * @param oid       - full instance identifier
 * @param value     - value to set
 *
 * @return Status code.
 */
static int 
stp_set_bridge_forward_delay(unsigned int gid, const char *oid,
                             const char *value)
{
    unsigned long int   val;

    UNUSED(gid);
    UNUSED(oid);

    CHECK_RC(ulong_to_number(value, &val));

    poe_stp_data.admin.forw_delay = val;

    return 0;
}


/**
 * Get STP MAC address.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_mac(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL,
                       "%02x:%02x:%02x:%02x:%02x:%02x",
                       poe_stp_data.state.mac.v[0],
                       poe_stp_data.state.mac.v[1],
                       poe_stp_data.state.mac.v[2],
                       poe_stp_data.state.mac.v[3],
                       poe_stp_data.state.mac.v[4],
                       poe_stp_data.state.mac.v[5]);
}


/**
 * Get STP current Designated Root.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_designated_root(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL,
                       "%02x%02x%02x%02x%02x%02x%02x%02x",
                       poe_stp_data.state.designated_root.v[0],
                       poe_stp_data.state.designated_root.v[1],
                       poe_stp_data.state.designated_root.v[2],
                       poe_stp_data.state.designated_root.v[3],
                       poe_stp_data.state.designated_root.v[4],
                       poe_stp_data.state.designated_root.v[5],
                       poe_stp_data.state.designated_root.v[6],
                       poe_stp_data.state.designated_root.v[7]);
}

/**
 * Get STP current root path cost value.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_root_path_cost(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.root_cost);
}


/**
 * Get STP current Root Port.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_root_port(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.root_port);
}


/**
 * Get STP current Max Age value.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_max_age(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.max_age);
}


/**
 * Get STP current Hello Time value.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_hello_time(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.hello_time);
}


/**
 * Get STP current Forward Delay value.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_forward_delay(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.fw_delay);
}


/**
 * Get STP current Hold Time value.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_hold_time(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.hold_time);
}


/**
 * Get STP time (in hundredths of a second) since the last time
 * a topology change was detected by the bridge entity.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_time_since_tp_change(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.time_since_tp_change);
}


/**
 * Get the total number of topology changes detected by the bridge
 * since the management entity was last reset or initialized.
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - location for the value (OUT)
 *
 * @return Status code.
 */
static int
stp_get_tot_changes(unsigned int gid, const char *oid, char *value)
{
    UNUSED(oid);

    CHECK_RC(update_poe_stp(gid));

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       poe_stp_data.state.tot_changes);
}


/* STP Port related functions */

/**
 * Obtains the number of times this port has transitioned from the Learning
 * state to the Forwarding state
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value (OUT)
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "forward transitions" value
 *                      requested
 */
static int 
stp_get_port_forward_transitions(unsigned int gid, const char *oid,
                                 char *value,
                                 const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    STP_COMMITTED_PORT_ENTRY_FIND(gid, port_num, &stp_port_entry);

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       stp_port_entry->forw_transitions);
}

/**
 * Obtains Designated Port value set up for the specified port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value (OUT)
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Designated Port" value returned
 */
static int 
stp_get_port_designated_port(unsigned int gid, const char *oid, char *value,
                             const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    STP_COMMITTED_PORT_ENTRY_FIND(gid, port_num, &stp_port_entry);

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       stp_port_entry->designated_port);
}

/**
 * Obtains Designated Bridge value set up for the specified port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value (OUT)
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Designated Bridge" value 
 *                      will be returned
 */
static int 
stp_get_port_designated_bridge(unsigned int gid, const char *oid,
                               char *value, const char *stp_name,
                               const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    STP_COMMITTED_PORT_ENTRY_FIND(gid, port_num, &stp_port_entry);

    return snprintf_rc(value, RCF_MAX_VAL,
                       "%02x%02x%02x%02x%02x%02x%02x%02x",
                       stp_port_entry->designated_bridge.v[0],
                       stp_port_entry->designated_bridge.v[1],
                       stp_port_entry->designated_bridge.v[2],
                       stp_port_entry->designated_bridge.v[3],
                       stp_port_entry->designated_bridge.v[4],
                       stp_port_entry->designated_bridge.v[5],
                       stp_port_entry->designated_bridge.v[6],
                       stp_port_entry->designated_bridge.v[7]);
}

/**
 * Obtains Designated Cost value set up for the specified port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value (OUT)
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Designated Cost" value 
 *                      will be returned
 */
static int 
stp_get_port_designated_cost(unsigned int gid, const char *oid, char *value,
                             const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    STP_COMMITTED_PORT_ENTRY_FIND(gid, port_num, &stp_port_entry);

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       stp_port_entry->designated_cost);
}

/**
 * Obtains Designated Root value set up for the specified port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value: (OUT)
 *                        eight octets' value in hexdecimal format
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Designated Root" value 
 *                      will be returned
 */
static int 
stp_get_port_designated_root(unsigned int gid, const char *oid, char *value,
                             const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    STP_COMMITTED_PORT_ENTRY_FIND(gid, port_num, &stp_port_entry);

    return snprintf_rc(value, RCF_MAX_VAL,
                       "%02x%02x%02x%02x%02x%02x%02x%02x",
                       stp_port_entry->designated_root.v[0],
                       stp_port_entry->designated_root.v[1],
                       stp_port_entry->designated_root.v[2],
                       stp_port_entry->designated_root.v[3],
                       stp_port_entry->designated_root.v[4],
                       stp_port_entry->designated_root.v[5],
                       stp_port_entry->designated_root.v[6],
                       stp_port_entry->designated_root.v[7]);
}

/**
 * Obtains Current STP state of the port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value: (OUT)
 *                        Disabled   - 0
 *                        Blocking   - 1
 *                        Listening  - 2
 *                        Learning   - 3
 *                        Forwarding - 4
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Port State" value 
 *                      will be returned
 * @param field_name  - Instance name - an empty string
 */
static int 
stp_get_port_state(unsigned int gid, const char *oid, char *value,
                   const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    STP_COMMITTED_PORT_ENTRY_FIND(gid, port_num, &stp_port_entry);

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       stp_port_entry->state);
}

/**
 * Obtains Path Cost of the port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value (OUT)
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Path Cost" value 
 *                      will be returned
 */
static int 
stp_get_port_path_cost(unsigned int gid, const char *oid, char *value,
                       const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    CHECK_RC(stp_port_entry_find(gid, port_num, &stp_port_entry));

    if (!stp_port_entry_committed(stp_port_entry) && 
        !((local_stp_port_entry_get_flags(stp_port_entry) & 
          STP_PORT_ENTRY_PATH_COST_SET)))
    {
        ERROR("Path cost of the port is not defined yet");
        return snprintf_rc(value, RCF_MAX_VAL, "");
    }

    return snprintf_rc(value, RCF_MAX_VAL, "%u",
                       stp_port_entry->path_cost);
}

/**
 * Updates Path Cost of the port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - A new "Path Cost" value
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Path Cost" value 
 *                      will be updated
 */
static int 
stp_set_port_path_cost(unsigned int gid, const char *oid, const char *value,
                       const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;
    long int      new_path_cost;
    char         *end_ptr;

    UNUSED(oid);
    UNUSED(stp_name);

    CHECK_RC(stp_port_entry_find(gid, port_num, &stp_port_entry));

    new_path_cost = strtol(value, &end_ptr, 10);
    if (new_path_cost < 0 || *end_ptr != '\0')
    {
        ERROR("Invalid format for Path Cost value: %s", value);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    stp_port_entry->path_cost = (unsigned long)new_path_cost;

    if (!stp_port_entry_committed(stp_port_entry))
        local_stp_port_entry_add_flags(stp_port_entry,
                                       STP_PORT_ENTRY_PATH_COST_SET);
    return 0;
}

/**
 * Obtains Port Priority value
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - Location for the value (OUT)
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Port Priority" value 
 *                      will be returned
 */
static int 
stp_get_port_prio(unsigned int gid, const char *oid, char *value,
                  const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    CHECK_RC(stp_port_entry_find(gid, port_num, &stp_port_entry));

    if (!stp_port_entry_committed(stp_port_entry) && 
        !((local_stp_port_entry_get_flags(stp_port_entry) & 
          STP_PORT_ENTRY_PRIO_SET)))
    {
        ERROR("Port priority is not defined yet");
        return snprintf_rc(value, RCF_MAX_VAL, "");
    }

    return snprintf_rc(value, RCF_MAX_VAL, "%u", stp_port_entry->prio);
}

/**
 * Updates Priority of the port
 *
 * @param gid         - request group identifier
 * @param oid         - full instance identifier
 * @param value       - A new "Path Cost" value
 * @param stp_name    - Name of the "/agent/stp" object - an empty string
 * @param port_num    - Port number whose "Port Priority" value 
 *                      will be updated
 */
static int 
stp_set_port_prio(unsigned int gid, const char *oid, const char *value,
                  const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;
    long int      new_prio;
    char         *end_ptr;

    UNUSED(oid);
    UNUSED(stp_name);

    CHECK_RC(stp_port_entry_find(gid, port_num, &stp_port_entry));

    new_prio = strtol(value, &end_ptr, 10);
    if (new_prio < 0 || *end_ptr != '\0')
    {
        ERROR("Invalid format for Port Priority value: %s", value);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    
    stp_port_entry->prio = (unsigned long)new_prio;
    if (!stp_port_entry_committed(stp_port_entry))
        local_stp_port_entry_add_flags(stp_port_entry,
                                       STP_PORT_ENTRY_PRIO_SET);

    return 0;
}

/**
 * Add a new STP port entry into the STP port table.
 * This operation enables STP on a particular port.
 *
 * @param gid        - Request group identifier
 * @param oid        - Full instance identifier
 * @param value      - Value to set
 * @param stp_name   - Instance name of /agent/stp object - an empty string
 * @param port_num   - Port number
 *
 * @return Status code.
 */
static int
stp_add_port(unsigned int gid, const char *oid, const char *value,
             const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;
    char         *end_ptr;
    int           rc;

    UNUSED(oid);
    UNUSED(stp_name);

    VERB("stp_add_port: gid = %d, oid = %s, value = %s, "
               "tbl_name = %s, port_num = %s",
               gid, oid, value, stp_name, port_num);

    rc = stp_port_entry_find(gid, port_num, &stp_port_entry);
    if (TE_RC_GET_ERROR(rc) != TE_ENOENT)
        return ((rc == 0) ? TE_RC(TE_TA_SWITCH_CTL, TE_EEXIST) : rc);

    if ((stp_port_entry = local_stp_port_entry_new()) == NULL)
    {
        ERROR("stp_add_port: Cannot create local STP port entry");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EAGAIN);
    }

    stp_port_entry->port = (poe_pid)strtol(port_num, &end_ptr, 10);
    local_stp_port_entry_add_flags(stp_port_entry, STP_PORT_ENTRY_PNUM_SET);

    return 0;
}

/**
 * Deletes specified STP port entry from STP port table - 
 * disables STP on a particular port.
 *
 * @param gid        - Request group identifier
 * @param oid        - Full instance identifier
 * @param stp_name   - Instance name of /agent/stp object - an empty string
 * @param port_num   - Port number
 *
 * @return Status code.
 */
static int
stp_del_port(unsigned int gid, const char *oid,
             const char *stp_name, const char *port_num)
{
    poe_stp_port *stp_port_entry;

    UNUSED(oid);
    UNUSED(stp_name);

    VERB("stp_del_port: oid = %s, port_num = %s",
               oid, port_num);

    CHECK_RC(stp_port_entry_find(gid, port_num, &stp_port_entry));

    if (stp_port_entry_committed(stp_port_entry))
    {
        /* Disable STP on the port */
        if (poe_stp_delete(stp_port_entry->port, error_string) != 0)
        {
            ERROR("Cannot delete STP port entry from the NUT: %s",
                       error_string);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
        }
    }
    else
    {
        /* Local STP port entry */
        local_stp_port_entry_delete(stp_port_entry);
    }

    return 0;
}

/**
 * Get the list of STP ports.
 *
 * @param gid       - request group identifier
 * @param oid       - full identifier of the father instance
 * @param list      - location for the list pointer
 *
 * @return Status code
 * @retval 0        - success
 * @retval TE_ENOMEM   - cannot allocate memory
 */
static int
stp_port_list(unsigned int gid, const char *oid, char **list)
{
    int mem_len;
    int cur_len;
    int i;
    int num_local_entries = 1; /* Currently no more than one local entry */

    UNUSED(oid);

    VERB("stp_port_list: gid %d, oid = %s", gid, oid);

    stp_cache_update(gid);

    mem_len = (stp_port_table_num + num_local_entries) * strlen("50 ") + 1;
    
    if ((*list = (char *)malloc(mem_len)) == NULL)
    {
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOMEM);
    }

    for (**list = '\0', cur_len = 0, i = 0; i < stp_port_table_num; i++)
    {
        snprintf(*list + cur_len, mem_len - cur_len,
                 "%u ", stp_port_table[i].port);
        cur_len = strlen(*list);
    }

    /* Take into account all local entries */
    if (local_stp_port_entry_flags & STP_PORT_ENTRY_PNUM_SET)
    {
        snprintf(*list + cur_len, mem_len - cur_len,
                 "%u ", local_stp_port_entry.port);
        cur_len = strlen(*list);
    }

    VERB("stp_list: %s", *list);

    return 0;
}

/**
 * Do the commit operation for read-write objects of STP entry.
 *
 * @param gid       - request group identifier
 * @param p_oid     - configurator object instance identifier
 *
 * @return Status code.
 */
static int 
stp_port_commit(unsigned int gid, const cfg_oid *p_oid)
{
    cfg_inst_subid *inst;
    poe_stp_port   *stp_port_entry;

    inst = ((cfg_inst_subid *)(p_oid->ids)) + (p_oid->len - 1);
    VERB("stp_port_commit: gid = %d, Port ID: %s",
               gid, inst->name);

    CHECK_RC(stp_port_entry_find(gid, inst->name, &stp_port_entry));

    if (stp_port_entry_committed(stp_port_entry))
    {
        /* Update parameters of the STP entry */
        if (poe_stp_update(stp_port_entry, error_string) != 0)
        {
            ERROR("Updating STP Port information on %s port fails "
                       "ERROR: %s", inst->name, error_string);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
        }
    }
    else
    {
        /* Do commit operation for a local entry */
        if (local_stp_port_entry_ready(stp_port_entry))
        {
            VERB("stp_commit: STP port entry is ready");
            if (poe_stp_create(stp_port_entry, error_string) != 0)
            {
                ERROR("Fails to enable STP on %s port: %s",
                            inst->name, error_string);
                return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
            }
            /*
             * The entry is not local any more, so that we can remove
             * it from the list of local STP port entries
             */
            local_stp_port_entry_delete(stp_port_entry);
        }
        else
        {
            VERB("stp_port_commit: STP port entry is not ready");
        }
    }

    return 0;
}

/** Free memory allocated by poe_vlan_read_table() */
static void
free_vlan_table(poe_vlan *table, int num)
{
    int i;
    
    for (i = 0; i < num; i++)
        free(table[i].ports);
        
    free(table);
}

/** Retrieve the information about VLAN */
static int
find_vlan(char *vid, poe_vlan *vlan)
{
    char    *tmp;
    uint32_t tag;
    
    poe_vlan *table;
    int       num;
    int       i;
    
    if (strcmp(vid, "default") == 0)
    {
        tag = VLAN_DEFAULT;
    }
    else
    {
        tag = strtol(vid, &tmp, 10);
        if (tmp == vid || *tmp != 0 || tag > 0xFFFF)
            return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
    }
        
    if (poe_vlan_read_table(&table, &num, error_string) < 0)
    {
        ERROR("Cannot read VLAN table: %s", error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }
    
    for (i = 0; i < num; i++)
    {
        if (table[i].id == tag)
        {
            *vlan = table[i];
            table[i].ports = NULL;
            free_vlan_table(table, num);
            return 0;
        }
    }
    free_vlan_table(table, num);
    return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
}

/**
 * Get VLAN status.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value location for value
 * @param vid   VLAN tag as a string
 *
 * @return status code
 */
static int 
vlan_get(int gid, char *oid, char *value, char *vid)
{
    poe_vlan vlan;
    int      rc;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = find_vlan(vid, &vlan)) != 0)
        return TE_RC(TE_TA_SWITCH_CTL, rc);
    
    sprintf(value, "%s", vlan.up ? "1" : "0");
    
    free(vlan.ports);

    return 0;
}

/**
 * Get VLAN status.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value new status
 * @param vid   VLAN tag as a string
 *
 * @return status code
 */
static int 
vlan_set(int gid, char *oid, char *value, char *vid)
{
    poe_vlan vlan;
    int      rc;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = find_vlan(vid, &vlan)) != 0)
        return TE_RC(TE_TA_SWITCH_CTL, rc);

    switch (*value)
    {
        case '0':
            vlan.up = FALSE;
            break;
            
        case '1':
            vlan.up = TRUE;
            break;
         
        default:
            return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }

    rc = poe_vlan_update(&vlan, error_string);
    
    free(vlan.ports);

    if (rc < 0)
    {
        ERROR("Cannot change VLAN status: %s", error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    return 0;
}

/**
 * Add VLAN.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value VLAN status
 * @param vid   VLAN tag as a string
 *
 * @return status code
 */
static int 
vlan_add(int gid, char *oid, char *value, char *vid)
{
    char    *tmp;
    uint32_t tag;
    
    poe_vlan vlan;
    
    UNUSED(gid);
    UNUSED(oid);
    
    tag = strtol(vid, &tmp, 10);
    if (tmp == value || *tmp != 0 || tag > 0xFFFF)
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
        
    sprintf(vlan.name, "%d", tag);
    vlan.id = tag;
    vlan.num_ports = 0;
    vlan.ports = NULL;
    
    switch (*value)
    {
        case '0':
            vlan.up = FALSE;
            break;
            
        case '1':
            vlan.up = TRUE;
            break;
         
        default:
            return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
    }
    if (*value != '0' && *value != '1')
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
        
    if (poe_vlan_create(&vlan, error_string) < 0)
    {
        ERROR("Cannot create VLAN: %s", 
                   error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }
    return 0;
}

/**
 * Delete VLAN.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param vid   VLAN tag as a string
 *
 * @return status code
 */
static int 
vlan_del(int gid, char *oid, char *vid)
{
    char     name[POE_LIB_MAX_STRING];
    char    *tmp;
    uint32_t tag;
    
    UNUSED(gid);
    UNUSED(oid);
    
    tag = strtol(vid, &tmp, 10);
    if (tmp == vid || *tmp != 0 || tag > 0xFFFF)
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
        
    sprintf(name, "%d", tag);
        
    if (poe_vlan_delete(name, error_string) < 0)
    {
        ERROR("Cannot delete VLAN: %s", 
                   error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }
    return 0;
}

#define VID_STR_MAX     16

/**
 * Construct VLAN list.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param list  location for result
 *
 * @return status code
 */
static int 
vlan_list(int gid, char *oid, char **list)
{
    poe_vlan       *table;
    char           *tmp;
    int             num;
    int             i;
    unsigned int    len;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if (poe_vlan_read_table(&table, &num, error_string) < 0)
    {
        ERROR("Cannot read VLAN table: %s", error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }
    F_VERB("VLAN table contains %d entries", num);

    len = num * VID_STR_MAX + 1;
    if ((*list = (char *)calloc(1, len)) == NULL)
    {
        free_vlan_table(table, num);
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOMEM);
    }
    
    tmp = *list;
    for (i = 0; i < num && tmp <= (*list + len); i++)
        tmp += snprintf(tmp, (*list + len) - tmp, "%s ", table[i].name);

    free_vlan_table(table, num);

    if (i < num)
    {
        ERROR("List of all VLANs does not fit in allocated "
                          "string, increase VID_STR_MAX");
        free(*list);
        return TE_RC(TE_TA_SWITCH_CTL, TE_ESMALLBUF);
    }
    VERB("List of VLANs: %s", *list);

    return 0;
}


/**
 * Add port to the VLAN.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value unused
 * @param vid   VLAN tag as a string
 * @param p     port number as a string
 *
 * @return status code
 */
static int 
vlan_port_add(int gid, char *oid, char *value, char *vid, char *p)
{
    char    *tmp;
    uint32_t port;
    poe_vlan vlan;
    int      rc;
    uint8_t  i;
    poe_pid *ports;
    
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    
    port = strtol(p, &tmp, 10);
    if (tmp == p || *tmp != 0 || port > 0xFF)
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
        
    if ((rc = find_vlan(vid, &vlan)) != 0)
        return TE_RC(TE_TA_SWITCH_CTL, rc);

    if ((ports = (poe_pid *)malloc((vlan.num_ports + 1) * 
                                   sizeof(poe_pid))) == NULL)
    {
        free(vlan.ports);
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOMEM);
    }
    
    for (i = 0; i < vlan.num_ports; i++)
    {
        if ((ports[i] = vlan.ports[i]) == port)
        {
            free(vlan.ports);
            free(ports);
            return TE_RC(TE_TA_SWITCH_CTL, TE_EEXIST);
        }
    }
    ports[i] = port;
    free(vlan.ports);
    vlan.ports = ports;
    vlan.num_ports++;

    rc = poe_vlan_update(&vlan, error_string);
    
    free(vlan.ports);

    if (rc < 0)
    {
        ERROR("Cannot add VLAN port: %s", error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    return 0;
}

/**
 * Delete port from the VLAN.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param vid   VLAN tag as a string
 * @param p     port number as a string
 *
 * @return status code
 */
static int 
vlan_port_del(int gid, char *oid, char *vid, char *p)
{
    char    *tmp;
    uint32_t port;
    poe_vlan vlan;
    int      rc;
    uint8_t  i;
    uint8_t  k;
    
    UNUSED(gid);
    UNUSED(oid);
    
    port = strtol(p, &tmp, 10);
    if (tmp == p || *tmp != 0 || port > 0xFF)
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
        
    if ((rc = find_vlan(vid, &vlan)) != 0)
        return TE_RC(TE_TA_SWITCH_CTL, rc);

    for (i = 0, k = 0; i < vlan.num_ports; i++)
    {
        if (vlan.ports[i] != port)
            vlan.ports[k++] = vlan.ports[i];
    }
    
    if (i == k)
    {
        free(vlan.ports);
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOENT);
    }
    vlan.num_ports--;

    rc = poe_vlan_update(&vlan, error_string);
    
    free(vlan.ports);

    if (rc < 0)
    {
        ERROR("Cannot delete VLAN port: %s", error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }

    return 0;
}

#define VLAN_PORT_STR_MAX     6

/**
 * List VLAN ports.
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param list  location for result
 * @param vid   VLAN tag as a string
 *
 * @return status code
 */
static int 
vlan_port_list(int gid, char *oid, char **list, char *vid)
{
    poe_vlan vlan;
    int      rc;
    uint8_t  i;
    char    *tmp;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = find_vlan(vid, &vlan)) != 0)
        return TE_RC(TE_TA_SWITCH_CTL, rc);
        
    *list = (char *)calloc(1, vlan.num_ports * VLAN_PORT_STR_MAX + 1);
    if (*list == NULL)
    {
        free(vlan.ports);
        return TE_RC(TE_TA_SWITCH_CTL, TE_ENOMEM);
    }

    tmp = *list;
    for (i = 0; i < vlan.num_ports; i++)
        tmp += sprintf(tmp, "%d ", vlan.ports[i]);
    
    free(vlan.ports);

    return 0;
}

/**
 * Get IP address of IP over VLAN interface
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value location for value
 * @param vid   VLAN tag as a string
 *
 * @return status code
 */
static int 
vlan_ip_get(int gid, char *oid, char *value, char *vid)
{
    char name[POE_LIB_MAX_STRING];
    int  num;
    int  i;
    int  rc;
    
    poe_vlan     vlan;
    poe_vlan_ip *table;
    
    UNUSED(gid);
    UNUSED(oid);
    
    if ((rc = find_vlan(vid, &vlan)) != 0)
        return TE_RC(TE_TA_SWITCH_CTL, rc);
        
    free(vlan.ports);
        
    sprintf(name, "%d", vlan.id);

    if (poe_vlan_ip_read_table(&table, &num, error_string) < 0)
    {
        ERROR("Cannot read IP over VLAN interfaces: %s", 
                   error_string);
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }
    
    for (i = 0; i < num && strcmp(table[i].vlan, name) != 0; i++);
    
    sprintf(value, "%s", i == num ? "0.0.0.0" : inet_ntoa(table[i].addr));

    free(table);

    return 0;
}

/**
 * Set IP address of IP over VLAN interface
 *
 * @param gid   groupd identifier (unused)
 * @param oid   full instance identifier (unused)
 * @param value new 
 * @param vid   VLAN tag as a string
 *
 * @return status code
 */
static int 
vlan_ip_set(int gid, char *oid, char *value, char *vid)
{
    poe_vlan_ip ipif;

    UNUSED(gid);
    UNUSED(oid);
    
    memset(&ipif, 0, sizeof(ipif));
    strcpy(ipif.vlan, vid);
    
    if (inet_aton(value, &(ipif.addr)) == 0)
        return TE_RC(TE_TA_SWITCH_CTL, TE_EINVAL);
     
    if (ipif.addr.s_addr == 0)
    {
        poe_vlan_ip_delete(vid, NULL);
        return 0;
    }
    
    if (poe_vlan_ip_create(&ipif, NULL) < 0 &&
        poe_vlan_ip_update(&ipif, NULL) < 0)
    {
        ERROR("Failed to configure IP over VLAN interface");
        return TE_RC(TE_TA_SWITCH_CTL, TE_EIO);
    }
    
    return 0;
}


/*@}*/


/** @name Switch Configuration Tree in reverse order */

RCF_PCH_CFG_NODE_RO(node_port_bps, "bps",
                    NULL, NULL,
                    port_get_bps);

RCF_PCH_CFG_NODE_RO(node_port_hol_blocking, "hol_blocking",
                    NULL, &node_port_bps,
                    port_get_hol_blocking);

RCF_PCH_CFG_NODE_RO(node_port_oper_clocks, "role",
                    NULL, &node_port_hol_blocking,
                    port_get_oper_clocks);

RCF_PCH_CFG_NODE_RO(node_port_oper_duplexity, "duplexity",
                    NULL, &node_port_oper_clocks,
                    port_get_oper_duplexity);

RCF_PCH_CFG_NODE_RO(node_port_oper_speed, "speed",
                    NULL, &node_port_oper_duplexity,
                    port_get_oper_speed);

RCF_PCH_CFG_NODE_RO(node_port_oper_status, "status",
                    NULL, &node_port_oper_speed,
                    port_get_oper_status);

RCF_PCH_CFG_NODE_NA(node_port_state, "state",
                    &node_port_oper_status, NULL);


static rcf_pch_cfg_object  node_port;

RCF_PCH_CFG_NODE_RWC(node_port_untagged_priority, "untagged_priority",
                     NULL, NULL,
                     port_get_untagged_priority, port_set_untagged_priority,
                     &node_port);

RCF_PCH_CFG_NODE_RWC(node_port_default_vlan, "default_vlan",
                     NULL, &node_port_untagged_priority,
                     port_get_default_vlan, port_set_default_vlan,
                     &node_port);

RCF_PCH_CFG_NODE_RWC(node_port_mtu, "mtu",
                     NULL, &node_port_default_vlan,
                     port_get_mtu, port_set_mtu,
                     &node_port);

RCF_PCH_CFG_NODE_RWC(node_port_admin_clocks, "role",
                     NULL, &node_port_mtu,
                     port_get_admin_clocks, port_set_admin_clocks,
                     &node_port);

RCF_PCH_CFG_NODE_RWC(node_port_admin_duplexity, "duplexity",
                     NULL, &node_port_admin_clocks,
                     port_get_admin_duplexity, port_set_admin_duplexity,
                     &node_port);

RCF_PCH_CFG_NODE_RWC(node_port_admin_speed, "speed",
                     NULL, &node_port_admin_duplexity,
                     port_get_admin_speed, port_set_admin_speed,
                     &node_port);

RCF_PCH_CFG_NODE_RWC(node_port_autonegotiation, "auto",
                     NULL, &node_port_admin_speed,
                     port_get_autonegotiation, port_set_autonegotiation,
                     &node_port);

RCF_PCH_CFG_NODE_RWC(node_port_admin_status, "status",
                     NULL, &node_port_autonegotiation,
                     port_get_admin_status, port_set_admin_status,
                     &node_port);

RCF_PCH_CFG_NODE_NA(node_port_admin, "admin",
                    &node_port_admin_status, &node_port_state);

RCF_PCH_CFG_NODE_RO(node_port_type, "type", 
                    NULL, &node_port_admin,
                    port_get_type);

RCF_PCH_CFG_NODE_COLLECTION(node_port, "port",
                            &node_port_type, NULL,
                            NULL, NULL, list_ports, port_commit);


RCF_PCH_CFG_NODE_RO(node_n_ports, "n_ports",
                    NULL, &node_port,
                    get_number_of_ports);

RCF_PCH_CFG_NODE_RW(node_a_time, "ageing_time",
                    NULL, &node_n_ports,
                    get_a_time, set_a_time);

/* ARL related group of objects */

static rcf_pch_cfg_object node_arl_entry;

RCF_PCH_CFG_NODE_RO(node_arl_entry_type, "type",
                    NULL, NULL, arl_get_type);

RCF_PCH_CFG_NODE_RO(node_arl_entry_vlan, "vlan",
                    NULL, &node_arl_entry_type, arl_get_vlan);

RCF_PCH_CFG_NODE_RO(node_arl_entry_port, "port",
                    NULL, &node_arl_entry_vlan, arl_get_port);

RCF_PCH_CFG_NODE_RO(node_arl_entry_mac, "mac",
                    NULL, &node_arl_entry_port, arl_get_mac);

RCF_PCH_CFG_NODE_COLLECTION(node_arl_entry, "entry",
                            &node_arl_entry_mac, NULL,
                            arl_add_entry, arl_del_entry, arl_list, NULL);

RCF_PCH_CFG_NODE_NA(node_arl, "arl",
                    &node_arl_entry, &node_a_time);

/* STP related group of objects */

static rcf_pch_cfg_object node_stp_port;

RCF_PCH_CFG_NODE_RO(node_stp_port_forward_transitions,
                    "forward_transitions",
                    NULL, NULL,
                    stp_get_port_forward_transitions);

RCF_PCH_CFG_NODE_RO(node_stp_port_designated_port, "designated_port",
                    NULL, &node_stp_port_forward_transitions,
                    stp_get_port_designated_port);

RCF_PCH_CFG_NODE_RO(node_stp_port_designated_bridge, "designated_bridge",
                    NULL, &node_stp_port_designated_port,
                    stp_get_port_designated_bridge);

RCF_PCH_CFG_NODE_RO(node_stp_port_designated_cost, "designated_cost",
                    NULL, &node_stp_port_designated_bridge,
                    stp_get_port_designated_cost);

RCF_PCH_CFG_NODE_RO(node_stp_port_designated_root, "designated_root",
                    NULL, &node_stp_port_designated_cost,
                    stp_get_port_designated_root);

RCF_PCH_CFG_NODE_RO(node_stp_port_state, "state",
                    NULL, &node_stp_port_designated_root,
                    stp_get_port_state);

RCF_PCH_CFG_NODE_RWC(node_stp_port_path_cost, "path_cost",
                     NULL, &node_stp_port_state,
                     stp_get_port_path_cost, stp_set_port_path_cost,
                     &node_stp_port);

RCF_PCH_CFG_NODE_RWC(node_stp_port_prio, "prio",
                     NULL, &node_stp_port_path_cost,
                     stp_get_port_prio, stp_set_port_prio, &node_stp_port);

RCF_PCH_CFG_NODE_COLLECTION(node_stp_port, "port",
                            &node_stp_port_prio, NULL,
                            stp_add_port, stp_del_port,
                            stp_port_list, stp_port_commit);

RCF_PCH_CFG_NODE_RO(node_stp_tot_changes, "tot_changes",
                    NULL, &node_stp_port,
                    stp_get_tot_changes);

RCF_PCH_CFG_NODE_RO(node_stp_time_since_tp_change, "time_since_tp_change",
                    NULL, &node_stp_tot_changes,
                    stp_get_time_since_tp_change);

RCF_PCH_CFG_NODE_RO(node_stp_hold_time, "hold_time",
                    NULL, &node_stp_time_since_tp_change,
                    stp_get_hold_time);

RCF_PCH_CFG_NODE_RO(node_stp_forward_delay, "forward_delay",
                    NULL, &node_stp_hold_time,
                    stp_get_forward_delay);

RCF_PCH_CFG_NODE_RO(node_stp_hello_time, "hello_time",
                    NULL, &node_stp_forward_delay,
                    stp_get_hello_time);

RCF_PCH_CFG_NODE_RO(node_stp_max_age, "max_age",
                    NULL, &node_stp_hello_time,
                    stp_get_max_age);

RCF_PCH_CFG_NODE_RO(node_stp_root_port, "root_port",
                    NULL, &node_stp_max_age,
                    stp_get_root_port);

RCF_PCH_CFG_NODE_RO(node_stp_root_path_cost, "root_path_cost",
                    NULL, &node_stp_root_port,
                    stp_get_root_path_cost);

RCF_PCH_CFG_NODE_RO(node_stp_designated_root, "designated_root",
                    NULL, &node_stp_root_path_cost,
                    stp_get_designated_root);

RCF_PCH_CFG_NODE_RO(node_stp_mac, "mac",
                    NULL, &node_stp_designated_root,
                    stp_get_mac);

static rcf_pch_cfg_object node_stp;

RCF_PCH_CFG_NODE_RWC(node_stp_bridge_forward_delay, "bridge_forward_delay",
                     NULL, &node_stp_mac,
                     stp_get_bridge_forward_delay,
                     stp_set_bridge_forward_delay,
                     &node_stp);

RCF_PCH_CFG_NODE_RWC(node_stp_bridge_hello_time, "bridge_hello_time",
                     NULL, &node_stp_bridge_forward_delay,
                     stp_get_bridge_hello_time, stp_set_bridge_hello_time,
                     &node_stp);

RCF_PCH_CFG_NODE_RWC(node_stp_bridge_max_age, "bridge_max_age",
                     NULL, &node_stp_bridge_hello_time,
                     stp_get_bridge_max_age, stp_set_bridge_max_age,
                     &node_stp);

RCF_PCH_CFG_NODE_RWC(node_stp_prio, "prio",
                     NULL, &node_stp_bridge_max_age,
                     stp_get_prio, stp_set_prio,
                     &node_stp);

static rcf_pch_cfg_object node_stp =
        { "stp", 0, &node_stp_prio, &node_arl,
          (rcf_ch_cfg_get)stp_get, (rcf_ch_cfg_set)stp_set, 
          NULL, NULL, NULL, stp_commit, NULL };

RCF_PCH_CFG_NODE_RW(node_vlan_ip, "ip",
                    NULL, NULL,
                    vlan_ip_get, vlan_ip_set);

RCF_PCH_CFG_NODE_COLLECTION(node_vlan_port, "port", 
                            NULL, &node_vlan_ip, 
                            vlan_port_add, vlan_port_del, vlan_port_list, 
                            NULL);

static rcf_pch_cfg_object node_vlan = 
        { "vlan", 0, 
          &node_vlan_port, &node_stp,
          (rcf_ch_cfg_get)vlan_get, (rcf_ch_cfg_set)vlan_set, 
          (rcf_ch_cfg_add)vlan_add, (rcf_ch_cfg_del)vlan_del, 
          (rcf_ch_cfg_list)vlan_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_cos, "cos",
                    NULL, &node_vlan,
                    cos_get, cos_set);


RCF_PCH_CFG_NODE_AGENT(node_agent, &node_cos);

/*@}*/


/* See description in rcf_ch_api.h */
rcf_pch_cfg_object *
rcf_ch_conf_root()
{
    return &node_agent;
}

/* See description in rcf_ch_api.h */
const char *
rcf_ch_conf_agent()
{
    return ta_name;
}

/* See description in rcf_ch_api.h */
void
rcf_ch_conf_release()
{
    free(poe_ports);
    free(arl_table);    
    free(stp_port_table);
    poe_ports = NULL;
}
