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
#include "logger_api.h"
#include "te_defs.h"
#include "rcf_pch.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_LINUX_ETHTOOL_H
#include "te_ethtool.h"
#endif

#include "te_ethernet_phy.h"

#if !defined __ANDROID__
/* Stdout output buffer size: need to pick up command execution output */
#define BUFFER_SIZE (1024)
/* Size of interface name */
#define IFNAME_MAX  (IFNAMSIZ)
/* Command line pattern to get autonegotiation state */
#define KSTAT_GET_AUTONEG_CMD \
       "kstat -p %s:%d:mac:link_autoneg | tr -d \"\t\" | "" \
       sed -e 's/^.*[link_autoneg]//'"
/* Command line pattern to get duplex state */
#define KSTAT_GET_DUPLEX_CMD \
       "kstat -p %s:%d:mac:link_duplex | tr -d \"\t\" | sed -e " \
       "'s/^.*[duplex]//'"
/* Command line pattern to get duplex state */
#define KSTAT_GET_SPEED_CMD \
        "kstat -p ::%s:ifspeed | tr -d \"\t\" | sed -e 's/^.*[ifspeed]//'"
/* Command line pattern to get link state */
#define KSTAT_GET_STATE_CMD \
        "kstat -p %s:%d::link_state | tr -d \"\t\" | sed -e " \
        "'s/^.*[state]//'"
/* Speed units */
#define KSTAT_SPEED_UNITS_IN_M 1000000

/* Maximum number of items in the list of speeds */
#define TE_PHY_SPEED_LIST_MAX_ITEMS (5)
/* Maximum number of items in the list of duplex values */
#define TE_PHY_DUPLEX_LIST_MAX_ITEMS (2)
/* Maximum size of the item in the list */
#define TE_PHY_LIST_MAX_ITEM_SIZE (6)

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
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */

/*
 * Methods
 */

static te_errno phy_autoneg_oper_get(unsigned int, const char *, char *,
                                     const char *);

static te_errno phy_autoneg_admin_get(unsigned int, const char *, char *,
                                      const char *);

static te_errno phy_autoneg_admin_set(unsigned int, const char *,
                                      const char *, const char *);

static te_errno phy_duplex_oper_get(unsigned int, const char *, char *,
                                    const char *);

static te_errno phy_duplex_admin_get(unsigned int, const char *, char *,
                                    const char *);

static te_errno phy_duplex_admin_set(unsigned int, const char *,
                                     const char *, const char *);

static te_errno phy_modes_speed_duplex_get(unsigned int, const char *,
                                           char *, const char *,
                                           const char *, const char *,
                                           const char *, const char *);

static te_errno phy_modes_speed_duplex_set(unsigned int, const char *,
                                           const char *, const char *,
                                           const char *, const char *,
                                           const char *, const char *);

static te_errno phy_modes_speed_duplex_list(unsigned int, const char *,
                                            char **, const char *);

static te_errno phy_modes_speed_list(unsigned int, const char *, char **,
                                     const char *);


static te_errno phy_speed_oper_get(unsigned int, const char *, char *,
                                   const char *);

static te_errno phy_speed_admin_get(unsigned int, const char *, char *,
                                    const char *);

static te_errno phy_speed_admin_set(unsigned int, const char *,
                                    const char *, const char *);

static te_errno phy_state_get(unsigned int, const char *, char *,
                              const char *);

static te_errno phy_commit(unsigned int, const cfg_oid *);

/*
 * Nodes
 */

static rcf_pch_cfg_object node_phy;

RCF_PCH_CFG_NODE_RO(node_phy_state, "state", NULL, NULL,
                    phy_state_get);

static rcf_pch_cfg_object node_phy_modes_speed_duplex = {
    "duplex", 0, NULL, NULL,
    (rcf_ch_cfg_get)phy_modes_speed_duplex_get,
    (rcf_ch_cfg_set)phy_modes_speed_duplex_set,
    NULL, NULL,
    (rcf_ch_cfg_list)phy_modes_speed_duplex_list, NULL, &node_phy
};

RCF_PCH_CFG_NODE_COLLECTION(node_phy_modes_speed, "speed",
                            &node_phy_modes_speed_duplex, NULL,
                            NULL, NULL,
                            phy_modes_speed_list, NULL);

RCF_PCH_CFG_NODE_RO(node_phy_modes, "modes", &node_phy_modes_speed,
                    &node_phy_state, NULL);

RCF_PCH_CFG_NODE_RWC(node_phy_speed_admin, "speed_admin", NULL,
                     &node_phy_modes, phy_speed_admin_get,
                     phy_speed_admin_set, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_speed_oper, "speed_oper", NULL,
                    &node_phy_speed_admin, phy_speed_oper_get);

RCF_PCH_CFG_NODE_RWC(node_phy_duplex_admin, "duplex_admin", NULL,
                     &node_phy_speed_oper, phy_duplex_admin_get,
                     phy_duplex_admin_set, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_duplex_oper, "duplex_oper", NULL,
                    &node_phy_duplex_admin,
                    phy_duplex_oper_get);

RCF_PCH_CFG_NODE_RWC(node_phy_autoneg_admin, "autoneg_admin", NULL,
                     &node_phy_duplex_oper, phy_autoneg_admin_get,
                     phy_autoneg_admin_set, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_autoneg_oper, "autoneg_oper", NULL,
                    &node_phy_autoneg_admin,
                    phy_autoneg_oper_get);

RCF_PCH_CFG_NODE_NA_COMMIT(node_phy, "phy", &node_phy_autoneg_oper, NULL,
                           phy_commit);


/* Admin values of interface parameters */
struct admin_params {
    uint16_t speed;     /**< Speed value */
    uint8_t  duplex;    /**< Duplex value */
    uint8_t  autoneg;   /**< Autonegotiation ON/OFF */
};

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
/* A list of the interfaces parameters */
static struct phy_iflist_head {
    struct phy_iflist_head *next;           /**< Next list item */
    struct phy_iflist_head *prev;           /**< Previous list item */
    const char             *ifname;         /**< Interface name  */
    struct ethtool_cmd      ecmd;           /**< Interface parameters */
    struct admin_params     admin;          /**< Admininstrative values 
                                                 of interface parameters */
    te_bool                 adver_cached;   /**< Adver cache state flag */
} phy_iflist;

/**
 * Add a new list entry to the PHY interfaces parameters list.
 *
 * This function just allocates memory for a new list item,
 * sets interface name field and initialize ecmd field.
 * cmd field should be filled by caller after item memory
 * will be allocated and a pointer to item will be returned to caller.
 * @p list should be a valid pointer to list head.
 *
 * @param list          Pointer to the list head
 * @param ifname        Interface name
 *
 * @return Pointer to the newly allocated list item or NULL
 *         if no more memory available to allocate a new list item.
 */
static struct phy_iflist_head *
phy_iflist_add_item(struct phy_iflist_head *list, const char *ifname)
{
    struct phy_iflist_head *new_list_item;
    struct phy_iflist_head *next;
    
    new_list_item =
        (struct phy_iflist_head *)malloc(sizeof(struct phy_iflist_head));
    if (new_list_item == NULL)
        return NULL;
    
    new_list_item->ifname = strdup(ifname);
    
    next = list->next;
    list->next = new_list_item;
    new_list_item->prev = list;
    
    if (next != NULL)
    {
        next->prev = new_list_item;
        new_list_item->next = next;
    }
    else
        new_list_item->next = NULL;
    
    memset(&(new_list_item->ecmd), 0, sizeof(struct ethtool_cmd));
    new_list_item->ecmd.speed = TE_PHY_SPEED_UNKNOWN;
    new_list_item->ecmd.duplex = TE_PHY_DUPLEX_UNKNOWN;
    new_list_item->ecmd.autoneg = TE_PHY_AUTONEG_UNKNOWN;
    new_list_item->adver_cached = FALSE;

    /* FIXME: */
    new_list_item->admin.speed = TE_PHY_SPEED_100;
    new_list_item->admin.duplex = TE_PHY_DUPLEX_FULL;
    new_list_item->admin.autoneg = TE_PHY_AUTONEG_ON;
    
    return new_list_item;
}

/**
 * Find the PHY interface properties list item.
 *
 * @p list should be a valid pointer to list head.
 *
 * @param list          A pointer to list head
 * @param ifname        Interface name
 *
 * @return A pointer to list item or a NULL
 *         if no more memory available to allocate
 *         a new list item.
 */
static struct phy_iflist_head *
phy_iflist_find(struct phy_iflist_head *list, const char *ifname)
{
    struct phy_iflist_head *list_item = list;
    
    /* Walking through the list items */
    while (1)
    {
        /* Check for valid pointer */
        if (list_item->next == NULL)
            break;
        
        /* Check for search condition */
        if (strcmp(list_item->next->ifname, ifname) == 0)
            return list_item->next;
        
        /* Switch to next list item */
        list_item = list_item->next;
    }
    
    /* If item does not exeists, return NULL */
    return NULL;
}

/**
 * Find the PHY interface properties list item or
 * add a new one to this list if no such item exists in list.
 *
 * @p list should be a valid pointer to list head.
 *
 * @param list          Pointer to the list head
 * @param ifname        Interface name
 *
 * @return Pointer to the newly allocated list item or pointer to list item
 *         that already exists in the list or a NULL
 *         if no more memory available to allocate a new list item.
 */
static struct phy_iflist_head *
phy_iflist_find_or_add(struct phy_iflist_head *list, const char *ifname)
{
    struct phy_iflist_head *list_item;
    
    list_item = phy_iflist_find(list, ifname);
    
    /* If item does not exists add a new one */
    if (list_item == NULL)
        return phy_iflist_add_item(list, ifname);
    
    return list_item;
}

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
    if (name == NULL)
        return -1;
    
    if (strcasecmp(name, TE_PHY_DUPLEX_STRING_FULL) == 0)
        return DUPLEX_FULL;
    
    if (strcasecmp(name, TE_PHY_DUPLEX_STRING_HALF) == 0)
        return DUPLEX_HALF;
    
    return -1;
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
static int
phy_property(const char *ifname, struct ethtool_cmd *ecmd, int type)
{
    struct ifreq        ifr;
    
    /* Reset errno value */
    errno = 0;
    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    ecmd->cmd = type;
    ifr.ifr_data = (caddr_t)ecmd;
    ioctl(cfg_socket, SIOCETHTOOL, &ifr);
    
    return errno;
}

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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 22)
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

/**
 * Restart autonegotiation.
 *
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_reset(const char *ifname)
{
    struct ifreq         ifr;
    int                  rc = -1;
    struct ethtool_value edata;
    
    memset(&edata, 0, sizeof(edata));
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    edata.cmd = ETHTOOL_NWAY_RST;
    ifr.ifr_data = (caddr_t)&edata;
    rc = ioctl(cfg_socket, SIOCETHTOOL, &ifr);
    
    if (rc < 0)
    {
        VERB("failed to restart autonegotiation at %s, errno=%d (%s)",
             ifname, errno, strerror(errno));
        
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    
    return 0;
}
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */

/**
 * Initialize PHY subtree
 */
extern te_errno
ta_unix_conf_phy_init(void)
{
#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
    phy_iflist.next = NULL;
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */
    return rcf_pch_add_node("/agent/interface", &node_phy);
}

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
        case 255:         return TE_PHY_DUPLEX_STRING_UNKNOWN;
#elif defined __sun__
        case 0: return TE_PHY_DUPLEX_STRING_UNKNOWN;
        case 1: return TE_PHY_DUPLEX_STRING_HALF;
        case 2: return TE_PHY_DUPLEX_STRING_FULL;
#endif /* __linux__ */
    }
    
    return NULL;
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
    
    pid = te_shell_cmd(cmd, -1, NULL, &out_fd, NULL);
    
    if (pid < 0)
    {
        ERROR("failed to execute command line while getting: %s: "
              "%s", cmd, strerror(errno));
        return -1;
    }
    
    if ((fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("failed to get shell command execution result while "
              "getting duplex state: %s", cmd);
        fclose(fp);
        
        return -1;
    }
    
    if ((out_buf = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("failed to allocate memory while getting duplex state");
        fclose(fp);
        close(out_fd);
        return -1;
    }
    
    memset(out_buf, 0, BUFFER_SIZE);
    
    if (fgets(out_buf, BUFFER_SIZE, fp) == NULL)
    {
        VERB("failed to read command execution result");
        free(out_buf);
        fclose(fp);
        close(out_fd);
        return -1;
    }
    
    /* Remove \r\n */
    out_buf[strlen(out_buf) - 1] = '\0';
    
    result = atoi(out_buf);
    
    free(out_buf);
    fclose(fp);
    close(out_fd);
    
    return result;
}

/**
 * Extract driver name and instance number from interface name.
 *
 * @param ifname        name of the interface
 * @param drv           driver name (output value)
 * @param instance      instance number (output value)
 *
 * @return              A number of arguments which has been parsed.
 *                      This number should be equal to 2 since we want
 *                      to extract two argumants: driver name and instance
 *                      number. If this number is not equal to 2 it means
 *                      that parsing error has been occured.
 */
static inline int
phy_extract_ifname(const char *ifname, char *drv, int *instance)
{
    return sscanf(ifname, "%[^0-9]%d", drv, instance);
}
#endif /* __sun__ */

/**
 * Get PHY autonegotiation oper state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_autoneg_oper_get(unsigned int gid, const char *oid, char *value,
                     const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                state;
    int                rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
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
    
    state = (ecmd.autoneg == AUTONEG_ENABLE) ?
            TE_PHY_AUTONEG_ON : TE_PHY_AUTONEG_OFF;
    
    snprintf(value, RCF_MAX_VAL, "%d", state);
    
#elif defined __sun__
    
    char   drv[IFNAME_MAX];
    int    instance = -1;
    char  *cmd;
    int    autoneg = -1;
    
    VERB("get autoneg");
    
    /* Extract driver name and instance number from interface name */
    if (phy_extract_ifname(ifname, drv, &instance) != 2)
    {
        ERROR("failed to parse interface name");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    if((cmd = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("Failed to allocate memory for shell command buffer "
              "while getting duplex state");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Construct a command line */
    sprintf(cmd, KSTAT_GET_AUTONEG_CMD, drv, instance);
    
    autoneg = phy_execute_shell_cmd(cmd);
    
    if (autoneg == -1)
    {
        VERB("failed to get autoneg state at %s", ifname);
        free(cmd);
        
        /* If this option is not supported we should not
           * return negative result code because configurator
           * never starts */
        snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_UNKNOWN);
        
        return 0;
    }
    
    /* Check for valid value */
    switch (autoneg) {
    
        case TE_PHY_AUTONEG_ON: snprintf(value, RCF_MAX_VAL, "%d",
                                         TE_PHY_AUTONEG_ON);
                                break;
        case TE_PHY_AUTONEG_OFF: snprintf(value, RCF_MAX_VAL, "%d",
                                          TE_PHY_AUTONEG_OFF);
                                 break;
        default: snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_UNKNOWN);
    };
    
    free(cmd);

#else

    UNUSED(ifname);
    snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_UNKNOWN);
    
#endif /* __linux__ */
    
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
phy_autoneg_admin_set(unsigned int gid, const char *oid, const char *value,
                      const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);
    
#if defined __linux__
    int                     autoneg = -1;
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    sscanf(value, "%d", &autoneg);
    
    if (autoneg != TE_PHY_AUTONEG_ON &&
        autoneg != TE_PHY_AUTONEG_OFF)
    {
        ERROR("fail while trying to set autonegotiation state "
              "to wrong value");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    list_item->ecmd.autoneg = (uint8_t)autoneg;
    list_item->admin.autoneg = (uint8_t)autoneg;
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("change autonegatiation state is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

/**
 * Get PHY autonegotiation admin state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_autoneg_admin_get(unsigned int gid, const char *oid, char *value,
                      const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);
    
#if defined __linux__
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

     /* Check for valid value */
    switch (list_item->admin.autoneg)
    {
        case TE_PHY_AUTONEG_ON:
            snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_ON);
            break;

        case TE_PHY_AUTONEG_OFF:
            snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_OFF);
            break;

        default:
            snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_UNKNOWN);
    };
#else
    UNUSED(ifname);
    snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_AUTONEG_UNKNOWN);
#endif /* __linux__ */

    return 0;
}

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
phy_duplex_oper_get(unsigned int gid, const char *oid, char *value,
                    const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
#if defined __linux__
    struct ethtool_cmd  ecmd;
    int                 rc;
    char               *duplex;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        /* Check that option does not supported */
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
    
    snprintf(value, RCF_MAX_VAL, "%s", duplex);
    
#elif defined __sun__
    char   drv[IFNAME_MAX];
    int    instance = -1;
    char  *cmd;
    int    result = -1;
    char  *duplex;
    
    /* Extract driver name and instance number from interface name */
    if (phy_extract_ifname(ifname, drv, &instance) != 2)
    {
        ERROR("failed to parse interface name");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    if((cmd = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("Failed to allocate memory for shell command buffer "
              "while getting duplex state");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Construct a command line */
    sprintf(cmd, KSTAT_GET_DUPLEX_CMD, drv, instance);
    
    result = phy_execute_shell_cmd(cmd);
    
    if (result == -1)
    {
        VERB("failed to get duplex state at %s", ifname);
        free(cmd);
        
        /* If this option is not supported we should not
           * return negative result code because configurator
           * never starts */
        snprintf(value, RCF_MAX_VAL, "%s", TE_PHY_DUPLEX_STRING_UNKNOWN);
        
        return 0;
    }
    
    duplex = phy_get_duplex_by_id(result);
    
    if (duplex == NULL)
    {
        ERROR("unknown duplex value %d at %s", result, ifname);
        free(cmd);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    snprintf(value, RCF_MAX_VAL, "%s", duplex);
    
    free(cmd);
#else
    UNUSED(value);
    UNUSED(ifname);
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
phy_duplex_admin_set(unsigned int gid, const char *oid, const char *value,
                     const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);
    
#if defined __linux__
    int                     duplex = -1;
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    duplex = phy_get_duplex_by_name(value);
    
    if (duplex == -1)
    {
        ERROR("cannot set unknown duplex state: %s", value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    list_item->ecmd.duplex = duplex;
    list_item->admin.duplex = duplex;
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("change duplex state is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

/**
 * Get PHY duplex admin state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_duplex_admin_get(unsigned int gid, const char *oid, char *value,
                     const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);
    
#if defined __linux__
    char                   *duplex = NULL;
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    duplex = phy_get_duplex_by_id(list_item->admin.duplex); 
    if (duplex == NULL)
    {
        ERROR("unknown duplex value %d at %s",
              list_item->admin.duplex, ifname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    snprintf(value, RCF_MAX_VAL, "%s", duplex);
    
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    return 0;
}

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
                           const char *ifname, const char *phy_name,
                           const char *mode_name, const char * speed,
                           const char *duplex)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(phy_name);
    UNUSED(mode_name);
#if defined __linux__
    struct ethtool_cmd  ecmd;
    int                 rc = -1;
    int                 mode = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
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
    
    mode = phy_get_mode(atoi(speed), duplex);
    
    /* Set mode state */
    if (ecmd.advertising & mode)
        snprintf(value, RCF_MAX_VAL, "%d", 1);
    else
        snprintf(value, RCF_MAX_VAL, "%d", 0);
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
    UNUSED(speed);
    UNUSED(duplex);
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
                           const char *value, const char *ifname,
                           const char *phy_name, const char *mode_name,
                           const char *speed, const char *duplex)
{
    UNUSED(oid);
    UNUSED(phy_name);
    UNUSED(mode_name);
    UNUSED(gid);
#if defined __linux__
    int                     set = -1;
    int                     mode = -1;
    struct ethtool_cmd      ecmd;
    struct phy_iflist_head *list_item;
    te_errno                rc;
    u32                     advertising = 0;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get PHY properties while setting "
              "mode to advertising state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    mode = phy_get_mode(atoi(speed), duplex);
    if (mode == 0)
    {
        ERROR("trying to advertise bad mode");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    set = atoi(value);
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    /* Extract from cache or not */
    if (list_item->adver_cached)
    {
        advertising = list_item->ecmd.advertising;
    }
    else
    {
        advertising = ecmd.advertising;
        list_item->adver_cached = TRUE;
    }
    
    if (set == 1)
        list_item->ecmd.advertising =
            advertising | mode;
    else
        list_item->ecmd.advertising =
            advertising & (~mode);
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
    UNUSED(speed);
    UNUSED(duplex);
    
    ERROR("mode advertising is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif /* __linux__ */
}

#if defined __linux__
/**
 * Insert value into list of object instances
 *
 * @param list          location for the list pointer
 * @param value         value to insert
 *
 * @return Status code
 */
static void
phy_modes_list_ins_value(char **list, char *value)
{
    strcat(*list, value);
    strcat(*list, " ");
}
#endif /* __linux__ */

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
    
    *list = (char *)malloc(TE_PHY_DUPLEX_LIST_MAX_ITEMS *
                           TE_PHY_LIST_MAX_ITEM_SIZE);
    if (*list == NULL)
    {
        ERROR("out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    memset(*list, 0, TE_PHY_DUPLEX_LIST_MAX_ITEMS *
           TE_PHY_LIST_MAX_ITEM_SIZE);
    
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
    
    *list = (char *)malloc(TE_PHY_SPEED_LIST_MAX_ITEMS *
                           TE_PHY_LIST_MAX_ITEM_SIZE);
    
    if (*list == NULL)
    {
        ERROR("out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    memset(*list, 0, TE_PHY_SPEED_LIST_MAX_ITEMS *
           TE_PHY_LIST_MAX_ITEM_SIZE);
    
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        VERB("No modes supported/advertized by `%s'", ifname);
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
phy_speed_oper_get(unsigned int gid, const char *oid, char *value,
                   const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    
#if defined __linux__
    struct ethtool_cmd ecmd;
    int                rc = -1;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
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
    
    if((cmd = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("Failed to allocate memory for shell command buffer "
              "while getting speed value");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Construct a command line */
    sprintf(cmd, KSTAT_GET_SPEED_CMD, ifname);
    
    if ((result = phy_execute_shell_cmd(cmd)) == -1)
    {
        VERB("failed to get speed value for interface %s", ifname);
        snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_SPEED_UNKNOWN);
        free(cmd);
        return 0;
    }
    
    speed = result / KSTAT_SPEED_UNITS_IN_M;
    
    snprintf(value, RCF_MAX_VAL, "%d", speed);
    
    free(cmd);
#else
    UNUSED(value);
    UNUSED(ifname);
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
phy_speed_admin_set(unsigned int gid, const char *oid, const char *value,
                    const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);
    
#if defined __linux__
    int                     speed;
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    /* Get value provided by caller */
    sscanf(value, "%d", &speed);
    
    list_item->ecmd.speed = speed;
    list_item->admin.speed = speed;
    
    return 0;
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    ERROR("change speed state is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
}

/**
 * Get PHY speed admin value.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_speed_admin_get(unsigned int gid, const char *oid, char *value,
                    const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

#if defined __linux__
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    snprintf(value, RCF_MAX_VAL, "%d", list_item->admin.speed);
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    return 0;
   
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
    struct ethtool_value edata;
    int                  state = -1;
    
    memset(&edata, 0, sizeof(edata));
    
    /* Initialize control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t)&edata;
    
    /* Get link state */
    if (ioctl(cfg_socket, SIOCETHTOOL, &ifr) != 0)
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
        return TE_RC(TE_TA_UNIX, errno);
    }
    
    state = edata.data ? TE_PHY_STATE_UP : TE_PHY_STATE_DOWN;
    
    snprintf(value, RCF_MAX_VAL, "%d", state);
    
    return 0;
#elif defined __sun__
    char   drv[IFNAME_MAX];
    int    instance = -1;
    char  *cmd;
    int    state = -1;
    
    /* Extract driver name and instance number from interface name */
    if (phy_extract_ifname(ifname, drv, &instance) != 2)
    {
        ERROR("failed to parse interface name");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    if((cmd = (char *)malloc(BUFFER_SIZE)) == NULL)
    {
        ERROR("Failed to allocate memory for shell command buffer "
              "while getting link state");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    sprintf(cmd, KSTAT_GET_STATE_CMD, drv, instance);
    
    state = phy_execute_shell_cmd(cmd);
    
    /* Check that duplex state information is supported */
    if (state == -1)
    {
        VERB("cannot get state state on `%s'", ifname);
        snprintf(value, RCF_MAX_VAL, "%d", TE_PHY_STATE_UNKNOWN);
        free(cmd);
        return 0;
    }
    
    /* Correct state */
    state = (state == 0 || state == 1) ? state : TE_PHY_STATE_UNKNOWN;
    
    snprintf(value, RCF_MAX_VAL, "%d", state);
    
    free(cmd);
#else
    UNUSED(value);
    UNUSED(ifname);
#endif /* __linux__ */
    return 0;
}

/**
 * Apply locally stored changes.
 *
 * @param gid           GID
 * @param p_oid         Pointer to the OID
 *
 * @return              Status code
 */
static te_errno
phy_commit(unsigned int gid, const cfg_oid *p_oid)
{
    UNUSED(gid);

#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
    char                   *ifname;
    struct phy_iflist_head *list_item;
    struct ethtool_cmd      ecmd;
    int                     rc = -1;
    te_bool                 advertise = FALSE;

    VERB("commit changes");

    /* Extract interface name */
    ifname = CFG_OID_GET_INST_NAME(p_oid, 2);

    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find(&phy_iflist, ifname);
    if (list_item == NULL)
    {
        ERROR("cannot find locally saved PHY setting for interface %s",
              ifname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    memset(&ecmd, 0, sizeof(ecmd));

    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get PHY properties while setting "
              "interface parameters");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    /* Merge current and new link parameters */
    list_item->ecmd.supported = ecmd.supported;
    list_item->ecmd.port = ecmd.port;
    list_item->ecmd.phy_address = ecmd.phy_address;
    list_item->ecmd.transceiver = ecmd.transceiver;
    list_item->ecmd.maxtxpkt = ecmd.maxtxpkt;
    list_item->ecmd.maxrxpkt = ecmd.maxrxpkt;

    /* Check the cache */
    if (!list_item->adver_cached)
        list_item->ecmd.advertising = ecmd.advertising;

    /* Speed and duplex must be unchanged if we are going to enable
     * autonegotiation. Also in case L5 NIC if speed and duplex values
     * are zeroed, then driver will try to change speed and duplex to 0,
     * so speed and duplex values should be equal to current speed 
     * and duplex values */
    if (list_item->ecmd.autoneg)
    {
        list_item->ecmd.speed = ecmd.speed;
        list_item->ecmd.duplex = ecmd.duplex;

        if ((list_item->ecmd.advertising &
            (ADVERTISED_10baseT_Half |
             ADVERTISED_10baseT_Full |
             ADVERTISED_100baseT_Half |
             ADVERTISED_100baseT_Full |
             ADVERTISED_1000baseT_Half |
             ADVERTISED_1000baseT_Full |
#ifdef ADVERTISED_2500baseX_Full
             ADVERTISED_2500baseX_Full |
#endif
             ADVERTISED_10000baseT_Full)) == 0)
        {
            list_item->ecmd.advertising = ecmd.supported &
                (ADVERTISED_10baseT_Half |
                 ADVERTISED_10baseT_Full |
                 ADVERTISED_100baseT_Half |
                 ADVERTISED_100baseT_Full |
                 ADVERTISED_1000baseT_Half |
                 ADVERTISED_1000baseT_Full |
#ifdef ADVERTISED_2500baseX_Full
                 ADVERTISED_2500baseX_Full |
#endif
                 ADVERTISED_10000baseT_Full);
        }
    }

    VERB("Properties to set:  %d %d %d %d %d %d %d %d %d %d %s",
         list_item->ecmd.supported,
         list_item->ecmd.advertising,
         list_item->ecmd.speed,
         list_item->ecmd.duplex,
         list_item->ecmd.port,
         list_item->ecmd.phy_address,
         list_item->ecmd.transceiver,
         list_item->ecmd.autoneg,
         list_item->ecmd.maxtxpkt,
         list_item->ecmd.maxrxpkt,
         ifname);

    VERB("Current properties: %d %d %d %d %d %d %d %d %d %d %s",
         ecmd.supported,
         ecmd.advertising,
         ecmd.speed,
         ecmd.duplex,
         ecmd.port,
         ecmd.phy_address,
         ecmd.transceiver,
         ecmd.autoneg,
         ecmd.maxtxpkt,
         ecmd.maxrxpkt,
         ifname);

    /* Remember this to check advertising completion later */
    advertise = !(list_item->ecmd.advertising == ecmd.advertising);

    /* Flush cache */
    list_item->adver_cached = FALSE;

    if ((rc = PHY_SET_PROPERTY(ifname, &(list_item->ecmd))) != 0)
    {
        ERROR("failed to apply PHY properties while setting "
              "interface %s properties", ifname);

        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    /* Check that advertised modes were advertised */
    if (advertise &&
        list_item->ecmd.autoneg == TE_PHY_AUTONEG_ON)
    {
        memset(&ecmd, 0, sizeof(ecmd));
        if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
        {
            ERROR("failed to get PHY properties while checking "
                  "advertised modes at interface %s", ifname);

            return TE_OS_RC(TE_TA_UNIX, rc);
        }

        if ((ecmd.advertising &
             (ADVERTISED_10baseT_Half |
              ADVERTISED_10baseT_Full |
              ADVERTISED_100baseT_Half |
              ADVERTISED_100baseT_Full |
              ADVERTISED_1000baseT_Half |
              ADVERTISED_1000baseT_Full |
#ifdef ADVERTISED_2500baseX_Full
              ADVERTISED_2500baseX_Full |
#endif
              ADVERTISED_10000baseT_Full)) != list_item->ecmd.advertising)
        {
            ERROR("failed to advertise needfull modes at %s", ifname);
            return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
        }
    }

    /* Restart autonegatiation if needed */
    if (list_item->ecmd.autoneg == TE_PHY_AUTONEG_ON)
    {
        if ((rc = phy_reset(ifname)) != 0)
            VERB("failed to restart autonegatiation while setting "
                 "interface %s properties", ifname);
    }

    return 0;
#else
    UNUSED(p_oid);
    ERROR("change interface parameters is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */
}
#else
extern te_errno
ta_unix_conf_phy_init(void)
{
    INFO("Phy interface configurations are not supported");
    return 0;
}
#endif /* __ANDROID__ */
