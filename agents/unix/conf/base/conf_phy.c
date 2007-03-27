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

/* Maximum number of items in the list of speeds */
#define TE_PHY_SPEED_LIST_MAX_ITEMS (5)
/* Maximum number of items in the list of speeds */
#define TE_PHY_DUPLEX_LIST_MAX_ITEMS (2)
/* Maximum size of the item in the list */
#define TE_PHY_LIST_MAX_ITEM_SIZE (6)


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


static te_errno phy_speed_get(unsigned int, const char *, char *,
                              const char *);

static te_errno phy_speed_set(unsigned int, const char *, const char *,
                              const char *);

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

RCF_PCH_CFG_NODE_RWC(node_phy_speed, "speed", NULL, &node_phy_modes,
                     phy_speed_get, phy_speed_set, &node_phy);

RCF_PCH_CFG_NODE_RWC(node_phy_duplex, "duplex", NULL, &node_phy_speed,
                     phy_duplex_get, phy_duplex_set, &node_phy);

RCF_PCH_CFG_NODE_RWC(node_phy_autoneg, "autoneg", NULL, &node_phy_duplex,
                     phy_autoneg_get, phy_autoneg_set, &node_phy);

RCF_PCH_CFG_NODE_NA_COMMIT(node_phy, "phy", &node_phy_autoneg, NULL,
                           phy_commit);


#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
/* A list of the interfaces parameters */
struct phy_iflist_head {
    struct phy_iflist_head *next;   /**< Next list item */
    struct phy_iflist_head *prev;   /**< Previous list item */
    const char             *ifname; /**< Interface name  */
    struct ethtool_cmd      ecmd;   /**< Interface parameters */
} phy_iflist;

/**
 * Initialize PHY interfaces parameters list.
 *
 * @param list          A pointer to list head
 */
static void
phy_init_iflist(struct phy_iflist_head *list)
{
    list->next = NULL;
}

/**
 * Add a new list entry to the PHY interfaces parameters list.
 *
 * This function just allocates memory for a new list item,
 * sets interface name field and initialize ecmd field.
 * cmd field should be filled by caller after item memory
 * will be allocated and a pointer to item will be returned to caller.
 *
 * @param list          A pointer to list head
 * @param ifname        Interface name
 *
 * @return A pointer to newly allocated list item or NULL
 *         if no more memory available to allocate a new list item.
 */
static struct phy_iflist_head *
phy_iflist_add_item(struct phy_iflist_head *list, const char *ifname)
{
    struct phy_iflist_head *new_list_item;
    struct phy_iflist_head *next;
    
    /* Allocate memory for a new list item */
    new_list_item =
        (struct phy_iflist_head *)malloc(sizeof(struct phy_iflist_head));
    if (new_list_item == NULL)
        return NULL;
    
    /* Store interface name */
    new_list_item->ifname = ifname;
    
    /* Remember a pointer to current next list item */
    next = list->next;
    
    /* Insert new list item between list head and
       next element relative list head */
    
    /* Tell to list head that the next element after list head
       is newly allocated list item */
    list->next = new_list_item;
    /* Tell to newly allocated list item that previous list element is
       a list head */
    new_list_item->prev = list;
    
    /* If next (previuosly remembered) list element exists,
       tell this element that the previous list element is
       a newly allocated list item and tell the newly allocated
       list item that the next element is the first element at the list */
    if (next != NULL)
    {
        next->prev = new_list_item;
        new_list_item->next = next;
    }
    else /* Store the end of the list */
        new_list_item->next = NULL;
    
    /* Initialize ecmd */
    memset(&(new_list_item->ecmd), 0, sizeof(struct ethtool_cmd));
    new_list_item->ecmd.speed = TE_PHY_SPEED_UNKNOWN;
    new_list_item->ecmd.duplex = TE_PHY_DUPLEX_UNKNOWN;
    new_list_item->ecmd.autoneg = TE_PHY_AUTONEG_UNKNOWN;
    
    return new_list_item;
}

/**
 * Find the PHY interface properties list item or
 * add a new one to this list if no such item exists in list.
 *
 * @param list          A pointer to list head
 * @param ifname        Interface name
 *
 * @return A pointer to newly allocated list item or pointer to list item
 *         that already exists in the list or a NULL
 *         if no more memory available to allocate a new list item.
 */
static struct phy_iflist_head *
phy_iflist_find_or_add(struct phy_iflist_head *list, const char *ifname)
{
    struct phy_iflist_head *list_item = list;
    
    /* Walking through the list items */
    while (1)
    {
        /* Look for the next list item */
        if (list_item->next == NULL)
            break;
        
        /* Check for the list item existense */
        if (strcmp(list_item->next->ifname, ifname) == 0)
            return list_item;
        
        /* Switch to next list item */
        list_item = list_item->next;
        
        /* Looking for the list end */
        if (list_item->next == NULL)
            break;
    }
    
    /* If item does not exeists, add a new one */
    return phy_iflist_add_item(list, ifname);
}

/**
 * Find the PHY interface properties list item.
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
        /* Look for the next list item */
        if (list_item->next == NULL)
            break;
        
        /* Check for the list item existense */
        if (strcmp(list_item->next->ifname, ifname) == 0)
            return list_item;
        
        /* Switch to next list item */
        list_item = list_item->next;
        
        /* Looking for the list end */
        if (list_item->next == NULL)
            break;
    }
    
    /* If item does not exeists, return NULL */
    return NULL;
}
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */

te_errno
ta_unix_conf_phy_init(void)
{
#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
    /* Initialize interfaces configuration parameters list */
    phy_init_iflist(&phy_iflist);
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */
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
        case 255:         return TE_PHY_DUPLEX_STRING_UNKNOWN;
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
    
    /* Setup control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    /* Get|set properties */
    ecmd->cmd = type;
    ifr.ifr_data = (caddr_t)ecmd;
    ioctl(cfg_socket, SIOCETHTOOL, &ifr);
    
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
    int                     autoneg = -1;
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
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
    list_item->ecmd.autoneg = (uint8_t)autoneg;
    
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
#if defined __sun__
static inline int
phy_extract_ifname(const char *ifname, char *drv, int *instance)
{
    return sscanf(ifname, "%[^0-9]%d", drv, instance);
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
    if (phy_extract_ifname(ifname, drv, &instance) != 2)
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
    
    /* Set value */
    list_item->ecmd.duplex = duplex;
    
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
    
    /* Try to get current mode */
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
                           const char * speed, const char *duplex)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(phy_name);
    UNUSED(mode_name);
#if defined __linux__
    int                     set = -1;
    int                     mode = -1;
    struct ethtool_cmd      ecmd;
    struct phy_iflist_head *list_item;
    te_errno                rc;
    
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get all properties */
    if ((rc = PHY_GET_PROPERTY(ifname, &ecmd)) != 0)
    {
        ERROR("failed to get PHY properties while setting "
              "mode to advertising state");
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    
    /* Get mode numeric ID */
    mode = phy_get_mode(atoi(speed), duplex);
    if (mode == 0)
    {
        ERROR("trying to advertise bad mode");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    
    /* Set or unset */
    set = atoi(value);
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    if (set == 1)
        list_item->ecmd.advertising =
            ecmd.advertising | mode; /* Set value */
    else
        list_item->ecmd.advertising =
            ecmd.advertising & (~mode); /* Unset value */
    
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
    /* Insert the item to the end of list */
    strcat(*list, value);
    
    /* Insert delimeter to the end of list */
    strcat(*list, " ");
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
    
    /* Allocate memory for list buffer */
    *list = (char *)malloc(TE_PHY_DUPLEX_LIST_MAX_ITEMS *
                           TE_PHY_LIST_MAX_ITEM_SIZE);
    if (*list == NULL)
    {
        ERROR("out of memory");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    
    /* Initialize list buffer */
    memset(*list, 0, TE_PHY_DUPLEX_LIST_MAX_ITEMS *
           TE_PHY_LIST_MAX_ITEM_SIZE);
    
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
    
    /* Allocate memory for list buffer */
    *list = (char *)malloc(TE_PHY_SPEED_LIST_MAX_ITEMS *
                           TE_PHY_LIST_MAX_ITEM_SIZE);
    
    /* Initialize list buffer */
    memset(*list, 0, TE_PHY_SPEED_LIST_MAX_ITEMS *
           TE_PHY_LIST_MAX_ITEM_SIZE);
    
    if (*list == NULL)
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
    int                     speed;
    struct phy_iflist_head *list_item;
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find_or_add(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    
    /* Get value provided by caller */
    sscanf(value, "%d", &speed);
    
    /* Set value */
    list_item->ecmd.speed = speed;
    
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
    struct ethtool_value edata;
    int                  state = -1;
    
    memset(&edata, 0, sizeof(edata));
    
    /* Setup control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    /* Get properties */
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
    
    /* Store value */
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

#if defined __linux__ && HAVE_LINUX_ETHTOOL_H
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
    
    /* Setup control structure */
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    
    /* Get|set properties */
    edata.cmd = ETHTOOL_NWAY_RST;
    ifr.ifr_data = (caddr_t)&edata;
    rc = ioctl(cfg_socket, SIOCETHTOOL, &ifr);
    
    if (rc < 0)
    {
        ERROR("failed to restart autonegotiation at %s, errno=%d (%s)",
              ifname, errno, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    
    return 0;
}
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */

static te_errno
phy_commit(unsigned int gid, const cfg_oid *p_oid)
{
    UNUSED(gid);
#if defined (__linux__) && HAVE_LINUX_ETHTOOL_H
    char                   *ifname;
    struct phy_iflist_head *list_item;
    struct ethtool_cmd      ecmd;
    int                     rc = -1;
    
    /* Extract interface name */
    ifname = CFG_OID_GET_INST_NAME(p_oid, 2);
    
    /* Try to get a pointer to list item associated with
     * current interface name */
    list_item = phy_iflist_find(&phy_iflist, ifname);
    if (list_item == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    
    /* Initialize structure */
    memset(&ecmd, 0, sizeof(ecmd));
    
    /* Get property */
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
    
    /* Apply value */
    if ((rc = PHY_SET_PROPERTY(ifname, &(list_item->ecmd))) != 0)
    {
        ERROR("failed to apply PHY properties while setting "
              "interface properties");
        
        return TE_OS_RC(TE_TA_UNIX, rc);
    }
    
    /* Restart autonegatiation if needed */
    if (list_item->ecmd.autoneg == TE_PHY_AUTONEG_ON)
    {
        if ((rc = phy_reset(ifname)) != 0)
        {
            ERROR("failed to restart autonegatiation while setting "
                  "interface properties");
            
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
    }
    
    return 0;
#else
    UNUSED(p_oid);
    ERROR("change interface parameters is not supported at this platform");
    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif /* __linux__ && HAVE_LINUX_ETHTOOL_H */
}
