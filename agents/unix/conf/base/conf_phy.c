/** @file
 * @brief Unix Test Agent
 *
 * Unix TA PHY interface support
 *
 * Copyright (C) 2004-2007 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Vadim V. Galitsyn <Vadim.Galitsyn@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "PHY Conf"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

#if defined __linux__
#include <linux/version.h>
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "rcf_pch.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "te_ethernet_phy.h"

static int phy_reset_value = 0; /**< Value of PHY reset */

/* Stdout output buffer size: need to pick up command execution output */
#define BUFFER_SIZE (1024)
/* Size of interface name */
#define IFNAME_MAX  (IFNAMSIZ)
/* Command line pattern to get duplex state */
#define KSTAT_GET_DUPLEX_CMD \
       "kstat -p %s:%d:mac:link_duplex | tr -d \"\t\" | sed -e 's/^.*[ ]//'"
/* Command line pattern to get duplex state */
#define KSTAT_GET_SPEED_CMD \
        "kstat -p ::%s:ifspeed | tr -d \"\t\" | sed -e 's/^.*[ ]//'"
/* Command line pattern to get link state */
#define KSTAT_GET_STATE_CMD \
        "kstat -p %s:%d::link_state | tr -d \"\t\" | sed -e 's/^.*[ ]//'"
/* Speed units */
#define KSTAT_SPEED_UNITS_IN_M 1000000


/*
 * Methods
 */


static te_errno phy_autoneg_get(unsigned int, const char *, char *,
                                const char *);

static te_errno phy_autoneg_set(unsigned int, const char *, const char *,
                                const char *);

static te_errno phy_duplex_get(unsigned int, const char *, char *,
                               const char *);

static te_errno phy_duplex_set(unsigned int, const char *, const char *,
                               const char *);

static te_errno phy_modes_speed_duplex_get(unsigned int, const char *,
                                           char *, const char *);

static te_errno phy_modes_speed_duplex_set(unsigned int, const char *,
                                           const char *, const char *);

static te_errno phy_modes_speed_duplex_list(unsigned int, const char *,
                                            char **, const char *);

static te_errno phy_modes_speed_list(unsigned int, const char *, char **,
                                     const char *);


static te_errno phy_speed_get(unsigned int, const char *, char *,
                              const char *);

static te_errno phy_speed_set(unsigned int, const char *, const char *,
                              const char *);

static te_errno phy_state_get(unsigned int, const char *, char *,
                              const char *);

static te_errno phy_reset_get(unsigned int, const char *, char *,
                              const char *);

static te_errno phy_reset_set(unsigned int, const char *, const char *,
                              const char *);


/*
 * Nodes
 */


RCF_PCH_CFG_NODE_RW(node_phy_reset, "reset", NULL, NULL,
                    phy_reset_get, phy_reset_set);

RCF_PCH_CFG_NODE_RO(node_phy_state, "state", NULL, &node_phy_reset,
                    phy_state_get);

static rcf_pch_cfg_object node_phy_modes_speed_duplex = {
    "duplex", 0, NULL, NULL,
    (rcf_ch_cfg_get)phy_modes_speed_duplex_get,
    (rcf_ch_cfg_set)phy_modes_speed_duplex_set,
    NULL, NULL,
    (rcf_ch_cfg_list)phy_modes_speed_duplex_list, NULL, NULL
};

RCF_PCH_CFG_NODE_COLLECTION(node_phy_modes_speed, "speed",
                            &node_phy_modes_speed_duplex, NULL,
                            NULL, NULL,
                            phy_modes_speed_list, NULL);

RCF_PCH_CFG_NODE_RO(node_phy_modes, "modes", &node_phy_modes_speed,
                    &node_phy_state, NULL);

RCF_PCH_CFG_NODE_RW(node_phy_speed, "speed", NULL, &node_phy_modes,
                    phy_speed_get, phy_speed_set);

RCF_PCH_CFG_NODE_RW(node_phy_duplex, "duplex", NULL, &node_phy_speed,
                    phy_duplex_get, phy_duplex_set);

RCF_PCH_CFG_NODE_RW(node_phy_autoneg, "autoneg", NULL, &node_phy_duplex,
                    phy_autoneg_get, phy_autoneg_set);

RCF_PCH_CFG_NODE_RO(node_phy, "phy",
                    &node_phy_autoneg, NULL,
                    NULL);

te_errno
ta_unix_conf_phy_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_phy);
}

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
/**
 * Get PHY property using ioctl() call.
 * Ethtool parameters will be stored in @p ecmd
 * (see linux/ethtool.h for more details)
 *
 * @param _ifname       name of the interface
 * @param _data         ethtool command
 *
 * @return              error code
 */
#define PHY_GET_PROPERTY(_ifname, _data) \
    phy_property(_ifname, _data, ETHTOOL_GSET)

/**
 * Set PHY property using ioctl() call.
 * Ethtool parameters will be stored in @p ecmd
 * (see linux/ethtool.h for more details)
 *
 * @param _ifname       name of the interface
 * @param _data         ethtool command
 *
 * @return              error code
 */
#define PHY_SET_PROPERTY(_ifname, _data) \
    phy_property(_ifname, _data, ETHTOOL_SSET)

/**
 * Get duplex state by name string
 *
 * @param name          Duplex name string
 *
 * @return DUPLEX_FULL, DUPLEX_HALF or -1 if
 *         duplex name string does not recognized
 */
static inline int
phy_get_duplex_by_name(const char *name)
{
    if (strcasecmp(name, TE_PHY_DUPLEX_STRING_FULL) == 0)
        return DUPLEX_FULL;
    
    if (strcasecmp(name, TE_PHY_DUPLEX_STRING_HALF) == 0)
        return DUPLEX_HALF;
    
    return -1;
}
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */

/**
 * Get duplex name string by id
 *
 * @param id            Duplex ID
 *
 * @return TE_PHY_DUPLEX_STRING_FULL, TE_PHY_DUPLEX_STRING_HALF or NULL if
 *         duplex ID does not recognized
 */

static inline char *
phy_get_duplex_by_id(int id)
{
    switch (id)
    {
#if defined __linux__
        case DUPLEX_FULL: return TE_PHY_DUPLEX_STRING_FULL;
        case DUPLEX_HALF: return TE_PHY_DUPLEX_STRING_HALF;
#elif defined __sun__
        case 0: return TE_PHY_DUPLEX_STRING_UNKNOWN;
        case 1: return TE_PHY_DUPLEX_STRING_HALF;
        case 2: return TE_PHY_DUPLEX_STRING_FULL;
#endif /* __linux__ */
    }
    
    return NULL;
}

/**
 * Get and set PHY property using ioctl() call.
 * Ethtool parameters will be stored in @p ecmd
 * (see linux/ethtool.h for more details)
 *
 * @param ifname        name of the interface
 * @param data          ethtool command
 * @param type          operation type:
 *                      ETHTOOL_GSET - get properties
 *                      ETHTOOL_SSET - set properties
 *
 * @return              error code
 */
#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
static int
phy_property(const char *ifname, struct ethtool_cmd *ecmd, int type)
{
    struct ifreq        ifr;
    int                 fd;
    
    /* Setup control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    /* Try to create control socket */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        ERROR("%s fails: failed to create control socket, "
              "errno=%d (%s)", __FUNCTION__,
              errno, strerror(errno));
        return errno;
    }
    
    /* Get|set properties */
    ecmd->cmd = type;
    ifr.ifr_data = (caddr_t)ecmd;
    ioctl(fd, SIOCETHTOOL, &ifr);
    close(fd);
    
    return errno;
}
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */

/**
 * Get PHY autonegotiation state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_autoneg_get(unsigned int gid, const char *oid, char *value,
                const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                state;
    int                rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get property */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        /*
         * Check for option support: if option is not
         * supported the leaf value should be set to -1
         */
        if (rc == EOPNOTSUPP)
        {
            snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_UNKNOWN);
            return 0;
        }
        ERROR("failed to get autonegatiation state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Set option state */
    state = (ecmd.autoneg == AUTONEG_ENABLE) ?
            TE_PHY_AUTONEG_ON : TE_PHY_AUTONEG_OFF;
    
    /* Store value */
    snprintf(value, RCF_MAX_VAL, "%d", state);
    
    return 0;
#endif /* __linux__ */
    UNUSED(ifname);
    snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_UNKNOWN);
    
    return 0;
}

/**
 * Change autonegatiation state.
 * Possible values: 0 - autonegatiation off
 *                  1 - autonegatiation on
 *
 * Autonegatiation state should turn off before
 * change the values of duplex and speed PHY parameters.
 *
 * WARNING. When autonegotiation turns to state ON
 *          all supported features become to advertised.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_autoneg_set(unsigned int gid, const char *oid, const char *value,
                const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                autoneg = -1;
    int                rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get all properties */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get PHY properties while "
              "trying to set autonegatiation state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Get value */
    sscanf(value, "%d", &autoneg);
    
    if (autoneg != AUTONEG_ENABLE &&
        autoneg != AUTONEG_DISABLE)
    {
        ERROR("fail while trying to set autonegotiation state "
              "to wrong value");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Set value */
    ecmd.autoneg = (uint8_t)autoneg;
    
    /* Enable features if needed */
    if (autoneg == AUTONEG_ENABLE)
        ecmd.advertising = ecmd.supported;
    
    /* Apply value */
    if ((rc = PHY_SET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to set PHY autonegatiation state, errno=%d (%s)",
              rc, strerror(rc));
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("change autonegatiation state is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

#if defined __sun__
/**
 * Execute shell command and return a value
 * of numeric constant that read from stdout after
 * commang execution.
 *
 * @param cmd           Command line
 *
 * @return              Numeric constant value or -1 if error occured
 */
static int
phy_execute_shell_cmd(char *cmd)
{
    pid_t pid;
    int   out_fd = -1;
    FILE *fp;
    char *out_buf;
    int   result = -1;
    
    /* Try to execute command line */
    pid = te_shell_cmd(cmd, -1, NULL, &out_fd, NULL);
    
    if (pid < 0)
    {
        ERROR("failed to execute command line while getting "
              "duplex state: %s", cmd);
        return -1;
    }
    
    /* Trying to open stdout */
    if ((fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("failed to get shell command execution result while "
              "getting duplex state: %s", cmd);
        return -1;
    }
    
    /* Allocate memory for output buffer */
    if ((out_buf = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("failed to allocate memory while getting duplex state");
        return -1;
    }
    
    /* Initialize buffer */
    memset(out_buf, 0, BUFFER_SIZE);
    
    /* Read data from stdout */
    if (fgets(out_buf, BUFFER_SIZE, fp) == NULL)
    {
        WARN("failed to read command execution result: %s", cmd);
        free(out_buf);
        return -1;
    }
    
    /* Remove \r\n */
    out_buf[strlen(out_buf) - 1] = '\0';
    
    result = atoi(out_buf);
    
    free(out_buf);
    
    return result;
}
#endif /* __sun__ */

/**
 * Get PHY duplex state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_duplex_get(unsigned int gid, const char *oid, char *value,
               const char *ifname)
{
    char               *duplex;
    
    UNUSED(gid);
    UNUSED(oid);
#if defined __linux__
    struct ethtool_cmd  ecmd;
    int                 rc;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get property */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        /* Check for option support */
        if (rc == EOPNOTSUPP)
        {
            /* If this option is not supported we should not
             * return negative result code because configurator
             * never starts */
            snprintf(value, RCF_MAX_VAL, "%s",
                     TE_PHY_DUPLEX_STRING_UNKNOWN);
            return 0;
        }
        
        ERROR("failed to get duplex state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    duplex = phy_get_duplex_by_id(ecmd.duplex);
    
    if (duplex == NULL)
    {
        ERROR("unknown duplex value: %d", ecmd.duplex);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Store value */
    snprintf(value, RCF_MAX_VAL, "%s", duplex);
    
#elif defined __sun__
    char   drv[IFNAME_MAX];
    int    instance = -1;
    char  *cmd;
    int    result = -1;
    
    /* Extract driver name and instance number from interface name */
    if (sscanf(ifname, "%[^0-9]%d", drv, &instance) != 2)
    {
        ERROR("failed to parse interface name");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Try to allocate memory for shell command buffer */
    if((cmd = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("Failed to allocate memory for shell command buffer "
              "while getting duplex state");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Construct a command line */
    sprintf(cmd, KSTAT_GET_DUPLEX_CMD, drv, instance);
    
    /* Execute command line */
    result = phy_execute_shell_cmd(cmd);
    
    /* Check output result value */
    if (result == -1)
    {
        WARN("failed to get duplex state at %s", ifname);
        free(cmd);
        
        /* If this option is not supported we should not
           * return negative result code because configurator
           * never starts */
        snprintf(value, RCF_MAX_VAL, "%s", TE_PHY_DUPLEX_STRING_UNKNOWN);
        
        return 0;
    }
    
    /* Get duplex string */
    duplex = phy_get_duplex_by_id(result);
    
    if (duplex == NULL)
    {
        ERROR("unknown duplex value %d at %s", result, ifname);
        free(cmd);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Set value */
    snprintf(value, RCF_MAX_VAL, "%s", duplex);
    
    free(cmd);
#endif /* __linux__ */
    return 0;
}

/**
 * Change duplex state.
 * Possible values: 0 - half duplex
 *                  1 - full duplex
 *
 * Autonegatiation state should be turned off before
 * change the values of duplex and speed PHY parameters.
 *
 * Supported for Linux only because there is no way to set
 * duplex state at other supported platforms now.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_duplex_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                rc = -1;
    int                duplex = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get all properties */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get PHY properties while setting duplex value");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Check for autonegatiation state off */
    if (ecmd.autoneg & AUTONEG_ENABLE)
    {
        ERROR("autonegatiation is ON; turn OFF autonegatiation "
              "to change the value of DUPLEX");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }
    
    duplex = phy_get_duplex_by_name(value);
    
    if (duplex == -1)
    {
        ERROR("cannot set unknown duplex state: %s", value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Set value */
    ecmd.duplex = duplex;
    
    /* Apply value */
    if ((rc = PHY_SET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to apply PHY properties while setting "
              "duplex value, errno=%d (%s)",
              rc, strerror(rc));
        
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("change duplex state is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

#if defined __linux__
/**
 * Calculate PHY interface mode.
 *
 * @param speed         Speed value
 * @param duplex        Duplex value
 *
 * @return              Mode value or 0 if no such mode
 */
static int
phy_get_mode(int speed, const char *duplex)
{
    switch (speed)
    {
        case SPEED_10:
        {
            if (strcmp(duplex, TE_PHY_DUPLEX_STRING_HALF) == 0)
                return ADVERTISED_10baseT_Half;
            
            if (strcmp(duplex, TE_PHY_DUPLEX_STRING_FULL) == 0)
                return ADVERTISED_10baseT_Full;
            
            break;
        }
        
        case SPEED_100:
        {
            if (strcmp(duplex, TE_PHY_DUPLEX_STRING_HALF) == 0)
                return ADVERTISED_100baseT_Half;
            
            if (strcmp(duplex, TE_PHY_DUPLEX_STRING_FULL) == 0)
                return ADVERTISED_100baseT_Full;
            
            break;
        }
        
        case SPEED_1000:
        {
            if (strcmp(duplex, TE_PHY_DUPLEX_STRING_HALF) == 0)
                return ADVERTISED_1000baseT_Half;
            
            if (strcmp(duplex, TE_PHY_DUPLEX_STRING_FULL) == 0)
                return ADVERTISED_1000baseT_Full;
            
            break;
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 21)
        case SPEED_10000:
        {
            if (strcmp(duplex, TE_PHY_DUPLEX_STRING_FULL) == 0)
                return ADVERTISED_10000baseT_Full;
            
            break;
        }
#endif /* LINUX_VERSION_CODE */
    }
    
    return 0;
}
#endif /* __linux__ */

/**
 * Get value for object "agent/interface/phy/speed/duplex".
 * Possible values are: 0 - configuration advertised
 *                      1 - configuration not advertised
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_modes_speed_duplex_get(unsigned int gid, const char *oid, char *value,
                           const char *ifname)
{
    UNUSED(gid);
#if defined __linux__
#define UNUSED_SIZE (10)
#define DUPLEX_SIZE (4)
    struct ethtool_cmd  ecmd;
    char                unused1[UNUSED_SIZE];
    char                unused2[UNUSED_SIZE];
    int                 speed = -1;
    char                duplex[DUPLEX_SIZE];
    int                 rc = -1;
    int                 mode = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get property */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        /*
         * Check for option support: if option is not
         * supported the leaf value should be set to -1
         */
        if (rc == EOPNOTSUPP)
        {
            snprintf(value, RCF_MAX_VAL, "%d", -1);
            return 0;
        }
        
        ERROR("failed to get interface properties");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Extract values */
    sscanf(oid,
           "/agent:%[^/]/interface:%[^/]/phy:/modes:/speed:%d/duplex:%s",
           unused1, unused2, &speed, duplex);
    
    /* Get current mode */
    mode = phy_get_mode(speed, duplex);
    
    /* Set mode state */
    if (ecmd.advertising & mode)
        snprintf(value, RCF_MAX_VAL, "%d", 1);
    else
        snprintf(value, RCF_MAX_VAL, "%d", 0);
    
    return 0;
#else
    UNUSED(oid);
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("advertising modes check is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

/**
 * Change value of object "agent/interface/phy/speed/duplex".
 * Means that such configuration will be advertised.
 * Possible values: 0 - configuration will not advertised
 *                  1 - configuration will be advertised
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_modes_speed_duplex_set(unsigned int gid, const char *oid,
                           const char *value, const char *ifname)
{
    UNUSED(gid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                set = -1;
    int                result = -1;
    char               unused1[UNUSED_SIZE];
    char               unused2[UNUSED_SIZE];
    int                speed = -1;
    char               duplex[DUPLEX_SIZE];
    int                rc = -1;
    int                advertised = -1;
    
#undef UNUSED_SIZE
#undef DUPLEX_SIZE
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get all properties */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get PHY properties while setting "
              "mode to advertising state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Check autonegotiation state */
    if (ecmd.autoneg == AUTONEG_DISABLE)
    {
        ERROR("autonegotiation should be ON while advertising change");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Extract values */
    sscanf(oid,
           "/agent:%[^/]/interface:%[^/]/phy:/modes:/speed:%d/duplex:%s",
           unused1, unused2, &speed, duplex);
    
    result = phy_get_mode(speed, duplex);
    
    /* Set or unset */
    set = atoi(value);
    
    if (set == 1)
        ecmd.advertising = ecmd.supported | result; /* Set value */
    else
        ecmd.advertising &= ~result; /* Unset value */
    
    /* Remember advertised modes value */
    advertised = ecmd.advertising;
    
    /* Apply value */
    if ((rc = PHY_SET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to apply PHY properties while setting "
              "mode to advertising state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /*
     * Check that mode advertised
     */
    
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to check PHY properties while setting "
              "mode to advertising state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    if (ecmd.advertising != (u32)advertised)
    {
        ERROR("cannot advertise this mode");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }
    
    return 0;
#else
    UNUSED(oid);
    UNUSED(value);
    UNUSED(ifname);
    
    ERROR("mode advertising is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif /* __linux__ */
}

/**
 * Insert value into list of object instances
 *
 * @param list          location for the list pointer
 * @param value         value to insert
 *
 * @return Status code
 */
static te_errno
phy_modes_list_ins_value(char **list, char *value)
{
    char *entry;
    char *realloced;
    
#define LIST_BULK (10)

    /* Try to allocate memory for temporary buffer */
    entry = (char *)malloc(LIST_BULK);
    if (entry == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    /* Try to reallocate list memory */
    realloced = realloc(*list, strlen(*list) + LIST_BULK);
    if (realloced == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

#undef LIST_BULK 

    /* Fill new list entry */
    sprintf(entry, "%s ", value);
    
    /* Store new list entry */
    strcat(realloced, entry);
    
    /* Restore list */
    *list = realloced;
    
    /* Free unused memory */
    free(entry);
    
    return 0;
}

/**
 * Get list of supported PHY speed/duplex
 * modes (object "agent/interface/phy/speed/duplex").
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return Status code
 */
static te_errno
phy_modes_speed_duplex_list(unsigned int gid, const char *oid, char **list,
                            const char *ifname)
{
    UNUSED(gid);
#if defined __linux__
    struct ethtool_cmd  ecmd;
    char               *speed_pattern = "speed:";
    int                 speed = -1;
    int                 rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Initialize list */
    if ((*list = strdup("")) == NULL)
    {
        ERROR("out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Get property */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get interface properties");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Extract speed value for current list item */
    speed = atoi(strstr(oid, speed_pattern) +
                 strlen(speed_pattern));
    
    /*
     * Insert an items
     */
    
    if (ecmd.supported & phy_get_mode(speed, TE_PHY_DUPLEX_STRING_HALF))
        phy_modes_list_ins_value(list, TE_PHY_DUPLEX_STRING_HALF);
    
    if (ecmd.supported & phy_get_mode(speed, TE_PHY_DUPLEX_STRING_FULL))
        phy_modes_list_ins_value(list, TE_PHY_DUPLEX_STRING_FULL);
#else
    UNUSED(oid);
    UNUSED(list);
    UNUSED(ifname);
#endif /* __linux__ */
    return 0;
}

/**
 * Get instance list for object "agent/interface/phy/speed".
 *
 * @param id            full identifier of the father instance
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return Status code
 */
static te_errno
phy_modes_speed_list(unsigned int gid, const char *oid, char **list,
                     const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd  ecmd;
    int                 rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Initialize list */
    if ((*list = strdup("")) == NULL)
    {
        ERROR("out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Get property */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        WARN("No modes supported/advertized by `%s'", ifname);
        return 0;
    }
    
    /* Add 10BaseT support */
    if (ecmd.supported & SUPPORTED_10baseT_Half ||
        ecmd.supported & SUPPORTED_10baseT_Full)
    {
        phy_modes_list_ins_value(list, TE_PHY_SPEED_STRING_10);
    }
    
    /* Add 100BaseT support */
    if (ecmd.supported & SUPPORTED_100baseT_Half ||
        ecmd.supported & SUPPORTED_100baseT_Full)
    {
        phy_modes_list_ins_value(list, TE_PHY_SPEED_STRING_100);
    }
  
    /* Add 1000BaseT support */
    if (ecmd.supported & SUPPORTED_1000baseT_Half ||
        ecmd.supported & SUPPORTED_1000baseT_Full)
    {
        phy_modes_list_ins_value(list, TE_PHY_SPEED_STRING_1000);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,22)
    /* Add 10000BaseT support */
    if (ecmd.supported & ADVERTISED_10000baseT_Full)
    {
        phy_modes_list_ins_value(list, TE_PHY_SPEED_STRING_10000);
    }
#endif /* LINUX_VERSION_CODE */
#else
    UNUSED(list);
    UNUSED(ifname);
#endif /* __linux__ */
    return 0;
}

/**
 * Get PHY current speed value.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_speed_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get property */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        /*
         * Check for option support: if option is not
         * supported the leaf value should be set to -1
         */
        
        if (rc == EOPNOTSUPP)
        {
            snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_SPEED_UNKNOWN);
            return 0;
        }
        
        ERROR("failed to get PHY properties while getting speed value");
        
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Store value */
    snprintf(value, RCF_MAX_VAL, "%d", ecmd.speed);
    
    return 0;
#elif defined __sun__
    char  *cmd;
    int    result = -1;
    int    speed = -1;
    
    /* Try to allocate memory for shell command buffer */
    if((cmd = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("Failed to allocate memory for shell command buffer "
              "while getting speed value");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Construct a command line */
    sprintf(cmd, KSTAT_GET_SPEED_CMD, ifname);
    
    /* Execute command line */
    if ((result = phy_execute_shell_cmd(cmd)) == -1)
    {
        WARN("failed to get speed value for interface %s", ifname);
        snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_SPEED_UNKNOWN);
        free(cmd);
        return 0;
    }
    
    speed = result / KSTAT_SPEED_UNITS_IN_M;
    
    /* Set value */
    snprintf(value, RCF_MAX_VAL, "%d", speed);
    
    free(cmd);
#endif /* __linux__ */
    return 0;
}

/**
 * Change speed value.
 *
 * Autonegatiation state should be turned off before
 * change the values of duplex and speed PHY parameters.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_speed_set(unsigned int gid, const char *oid, const char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                speed;
    int                rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get all properties */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get PHY properties while setting speed value");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Check for autonegatiation state off */
    if (ecmd.autoneg & AUTONEG_ENABLE)
    {
        ERROR("autonegatiation is ON; turn OFF autonegatiation "
              "to change the value of DUPLEX");
        return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
    }
    
    /* Get value provided by caller */
    sscanf(value, "%d", &speed);
    
    /* Set value */
    ecmd.speed = speed;
    
    /* Apply value */
    if ((rc = PHY_SET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to set PHY properties while applying speed value");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("change speed state is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

/**
 * Get PHY state value.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_state_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ifreq         ifr;
    int                  fd;
    struct ethtool_value edata;
    int                  state = -1;
    
    memset(&edata, 0, sizeof(edata));
    
    /* Setup control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    /* Try to create control socket */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (fd < 0)
    {
        ERROR("%s fails: failed to create control socket",
              __FUNCTION__);
        snprintf(value, RCF_MAX_VAL, "%d", 0);
        return 0;
    }
    
    /* Get properties */
    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t)&edata;
    
    /* Get link state */
    if (ioctl(fd, SIOCETHTOOL, &ifr) != 0)
    {
        /*
         * Check for option support: if option is not
         * supported the leaf value should be set to -1
         */
        if (errno == EOPNOTSUPP)
        {
            snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_STATE_UNKNOWN);
            return 0;
        }
        
        ERROR("failed to get interface state value");
        return TE_RC(TE_TA_UNIX, errno);;
    }
    
    state = edata.data ? TE_PHY_STATE_UP : TE_PHY_STATE_DOWN;
    
    /* Store value */
    snprintf(value, RCF_MAX_VAL, "%d", state);
    
    /* Close a socket */
    close(fd);
    
    return 0;
#elif defined __sun__
    char   drv[IFNAME_MAX];
    int    instance = -1;
    char  *cmd;
    int    state = -1;
    
    /* Extract driver name and instance number from interface name */
    if (sscanf(ifname, "%[^0-9]%d", drv, &instance) != 2)
    {
        ERROR("failed to parse interface name");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Try to allocate memory for shell command buffer */
    if((cmd = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("Failed to allocate memory for shell command buffer "
              "while getting link state");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Construct a command line */
    sprintf(cmd, KSTAT_GET_STATE_CMD, drv, instance);
    
    /* Execute command line */
    state = phy_execute_shell_cmd(cmd);
    
    /* Check that duplex state information is supported */
    if (state == -1)
    {
        WARN("cannot get state state on `%s'", ifname);
        snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_STATE_UNKNOWN);
        free(cmd);
        return 0;
    }
    
    /* Correct state */
    state = (state == 0 || state == 1) ? state : TE_PHY_STATE_UNKNOWN;
    
    /* Set value */
    snprintf(value, RCF_MAX_VAL, "%d", state);
    
    free(cmd);
#endif /* __linux__ */
    return 0;
}

/**
 * Get PHY reset state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_reset_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);
    
    snprintf(value, RCF_MAX_VAL, "%d", phy_reset_value);
    
    return 0;
}

/**
 * Restart autonegotiation.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_reset_set(unsigned int gid, const char *oid, const char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ifreq         ifr;
    int                  fd;
    int                  rc = -1;
    struct ethtool_value edata;
    
    memset(&edata, 0, sizeof(edata));
    
    /* Extract value */
    sscanf(value, "%d", &phy_reset_value);
    
    /* Setup control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    /* Try to create control socket */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (fd < 0)
    {
        ERROR("%s fails: failed to create control socket", __FUNCTION__);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    
    /* Get|set properties */
    edata.cmd = ETHTOOL_NWAY_RST;
    ifr.ifr_data = (caddr_t)&edata;
    rc = ioctl(fd, SIOCETHTOOL, &ifr);
    
    if (rc < 0)
    {
        ERROR("failed to restart autonegotiation, errno=%d (%s)",
              errno, strerror(errno));
        close(fd);
        return TE_RC(TE_TA_UNIX, errno);
    }
    
    close(fd);
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("autonegatiation restart is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}
