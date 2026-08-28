/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Link trunking and bridging support
 *
 * Unix TA link bonding, team and link bridges support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Link Aggr"

#include "te_config.h"
#include "config.h"


#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch_ta_cfg.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "logger_api.h"
#include "ta_common.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "te_str.h"
#include "te_alloc.h"

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif

/**
 * Type of link aggregations. This enum should be synchronized with
 * aggr_types_data array.
 */
typedef enum aggr_type {
    AGGREGATION_BONDING = 0,        /**< bonding */
    AGGREGATION_TEAM,               /**< team */
    AGGREGATION_BRIDGE,             /**< ethernet bridge */
    AGGREGATION_INVALID             /**< Invalid (error) type */
} aggr_type;

typedef enum aggr_mode {
    AGGREGATION_MODE_AB = 1,   /**< Active backup mode */
    AGGREGATION_MODE_LACP = 4, /**< LACP mode */
    AGGREGATION_MODE_INVALID   /**< Invalid (error) type */
} aggr_mode;

/** Internal structure for each created aggregation */
typedef struct aggregation {
    struct aggregation *next;       /**< Link pointer */

    const char *name;               /**< User-provided name */
    aggr_type   type;               /**< Type of this aggregation */
    aggr_mode   mode;               /**< Mode of this aggregation */
    char        ifname[IFNAMSIZ];   /**< Interface name */
} aggregation;

typedef te_errno (*aggr_create)(aggregation *aggr);
typedef te_errno (*aggr_destroy)(aggregation *aggr);
typedef te_errno (*aggr_add)(aggregation *aggr, const char *ifname);
typedef te_errno (*aggr_del)(aggregation *aggr, const char *ifname);
typedef te_errno (*aggr_list)(aggregation *aggr, char **member_list);

/** By-type information of aggregation handling */
typedef struct aggr_type_info {
    const char     *value;    /**< human-readable string used by users */
    aggr_create     create;   /**< create aggregation of this type */
    aggr_destroy    destroy;  /**< destroy aggregation of this type */
    aggr_add        add;      /**< add a new member into this aggregation */
    aggr_del        del;      /**< delete a member from this aggregation */
    aggr_list       list;     /**< list all members of this aggregation */
} aggr_type_info;


static char buf[4096];
static char rsrc[RCF_MAX_VAL];

/*
 * Common helpers
 */

/**
 * Get a non-existent interface with the name matching format,
 * using integer from 0 to INT_MAX to sprintf with this format.
 * The interface found is locked with ta_rsrc_create_lock().
 *
 * @param format    format of the interface name, such as "bond%d"
 * @param ifname    array to store the interface name found
 *
 * @return              Status code
 *
 * @note this function sets static rsrc variable to the value of this
 * interface resource.
 */
static te_errno
aggr_interface_get_free(const char *format, char ifname[IFNAMSIZ])
{
    bool shared = false;
    int  i;

    for (i = 0; i < INT_MAX ; i++)
    {
        snprintf(ifname, IFNAMSIZ, format, i);
        snprintf(rsrc, RCF_MAX_VAL, "/agent:%s/interface:%s",
                 ta_name, ifname);
        if (if_nametoindex(ifname) == 0 &&
            ta_rsrc_create_lock(rsrc, &shared, false, 0) == 0)
        {
            break;
        }
    }
    if (i == INT_MAX)
    {
        ERROR("Can't find non-existing bond device");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    return 0;
}

/**
 * Check that the user interface name is free, otherwise
 * get a non-existent interface.
 *
 * @param aggr      Aggregation information.
 * @param format    Format of the interface name, such as "bond%d".
 * @param ifname    Array to store the interface name found.
 *
 * @return Status code.
 */
static te_errno
aggr_interface_get_user_or_free(const aggregation *aggr,
                                const char *format,
                                char ifname[IFNAMSIZ])
{
    bool shared = false;

    if (aggr->name != NULL &&
        aggr->name[0] != '\0' &&
        strlen(aggr->name) < IFNAMSIZ)
    {
        snprintf(ifname, IFNAMSIZ, "%s", aggr->name);
        snprintf(rsrc, RCF_MAX_VAL, "/agent:%s/interface:%s",
                 ta_name, ifname);

        if (if_nametoindex(ifname) == 0 &&
            ta_rsrc_create_lock(rsrc, &shared, false, 0) == 0)
        {
            return 0;
        }

    }

    return aggr_interface_get_free(format, ifname);
}

/*
 * Bonding support
 */

static te_errno
trunk_create(aggregation *aggr)
{
    te_errno rc;
    FILE *f;

    /* Get a name for the new bond device */
    rc = aggr_interface_get_user_or_free(aggr, "bond%d", aggr->ifname);
    if (rc != 0)
        return rc;

    if (ta_system("/sbin/modprobe bonding max_bonds=0") != 0)
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);

    f = fopen("/sys/class/net/bonding_masters", "a");
    if (f == NULL)
    {
        ERROR("Failed to open /sys/class/net/bonding_masters: "
              "is bonding module loaded?");
        ta_rsrc_delete_lock(rsrc);
        return TE_RC(TE_TA_UNIX, errno);
    }
    fwrite("+", 1, 1, f);
    fwrite(aggr->ifname, strlen(aggr->ifname), 1, f);
    fclose(f);

    if (if_nametoindex(aggr->ifname) <= 0)
    {
        ERROR("Bonding driver failed to create interface \"%s\"", aggr->ifname);
        ta_rsrc_delete_lock(rsrc);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    /* Configure bonding mode */
    memset(buf, 0, sizeof(buf));
    if (snprintf(buf, sizeof(buf), "/sys/class/net/%s/bonding/mode",
                 aggr->ifname) <= 0)
    {
        ERROR("Failed to snprintf filename in /sys");
        return TE_RC(TE_TA_UNIX, errno);
    }
    f = fopen(buf, "w");
    if (f == NULL)
    {
        ERROR("Failed to open /sys/class/net/bonding_masters: "
              "is bonding module loaded?");
        ta_rsrc_delete_lock(rsrc);
        return TE_RC(TE_TA_UNIX, errno);
    }
    memset(buf, 0, sizeof(buf));
    if (snprintf(buf, sizeof(buf), "%d", aggr->mode) <= 0)
    {
        ERROR("Failed to snprintf filename in /sys");
        return TE_RC(TE_TA_UNIX, errno);
    }
    fwrite(buf, strlen(buf), 1, f);
    fclose(f);

    /* Configure bonding policy */
    if (aggr->mode == AGGREGATION_MODE_LACP)
    {
        memset(buf, 0, sizeof(buf));
        if (snprintf(buf, sizeof(buf),
                     "/sys/class/net/%s/bonding/xmit_hash_policy",
                     aggr->ifname) <= 0)
        {
            ERROR("Failed to snprintf filename in /sys");
            return TE_RC(TE_TA_UNIX, errno);
        }
        f = fopen(buf, "w");
        if (f == NULL)
        {
            ERROR("Failed to open /sys/class/net/bonding_masters: "
                  "is bonding module loaded?");
            ta_rsrc_delete_lock(rsrc);
            return TE_RC(TE_TA_UNIX, errno);
        }
        fwrite("1", 1, 1, f);
        fclose(f);
    }

    /* Linux does not allow to add slave interfaces when master is not up */
    ta_interface_status_set(aggr->ifname, true);

    return 0;
}

static te_errno
trunk_destroy(aggregation *aggr)
{
    FILE *f;

    ta_interface_status_set(aggr->ifname, false);
    /* Down the interface and release the lock */
    /* Assert there are no members ? */
    f = fopen("/sys/class/net/bonding_masters", "a");
    if (f == NULL)
    {
        ERROR("Failed to open /sys/class/net/bonding_masters");
        return TE_RC(TE_TA_UNIX, errno);
    }
    fwrite("-", 1, 1, f);
    fwrite(aggr->ifname, strlen(aggr->ifname), 1, f);
    fclose(f);

    if (if_nametoindex(aggr->ifname) > 0)
    {
        ERROR("Bonding driver failed to delete interface \"%s\"", aggr->ifname);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }
    return 0;
}

static te_errno
trunk_add(aggregation *aggr, const char *ifname)
{
    struct ifreq ifr;

    TE_STRLCPY(ifr.ifr_name, aggr->ifname, IFNAMSIZ);
    TE_STRLCPY(ifr.ifr_slave, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCBONDENSLAVE, &ifr) < 0)
        return TE_RC(TE_TA_UNIX, errno);
    return 0;
}

static te_errno
trunk_del(aggregation *aggr, const char *ifname)
{
    struct ifreq ifr;

    TE_STRLCPY(ifr.ifr_name, aggr->ifname, IFNAMSIZ);
    TE_STRLCPY(ifr.ifr_slave, ifname, IFNAMSIZ);
    if (ioctl(cfg_socket, SIOCBONDRELEASE, &ifr) < 0)
        return TE_RC(TE_TA_UNIX, errno);
    return 0;
}

static te_errno
trunk_list(aggregation *aggr, char **member_list)
{
    FILE *f;
    char *c;
    int   sysrc;

    if (snprintf(buf, sizeof(buf), "/sys/class/net/%s/bonding/slaves",
                 aggr->ifname) <= 0)
    {
        ERROR("Failed to snprintf filename in /sys");
        return TE_RC(TE_TA_UNIX, errno);
    }
    f = fopen(buf, "r");
    if (f == NULL)
    {
        ERROR("Failed to open file \"%s\"", buf);
    }

    memset(buf, 0, sizeof(buf));
    sysrc = fread(buf, 1, sizeof(buf), f);
    if (sysrc < 0)
    {
        ERROR("Failed to read data from file \"%s\"", buf);
        fclose(f);
        return TE_RC(TE_TA_UNIX, errno);
    }
    fclose(f);

    if ((c = strchr(buf, '\n')) != NULL)
        *c = 0;

    if ((*member_list = te_str_strip_spaces(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/*
 * Team support
 */

static te_errno
team_create(aggregation *aggr)
{
    te_errno rc;

    /* Get a name for the new bond device */
    rc = aggr_interface_get_user_or_free(aggr, "bond%d", aggr->ifname);
    if (rc != 0)
        return rc;

    if (aggr->mode == AGGREGATION_MODE_AB)
    {
        snprintf(buf, sizeof(buf),
                 "/usr/bin/teamd -t %s -d -g -c '{\"device\":\"%s\", "
                 "\"runner\":{\"name\":\"activebackup\"}}'",
                aggr->ifname, aggr->ifname);
    }
    else if (aggr->mode == AGGREGATION_MODE_LACP)
    {
        snprintf(buf, sizeof(buf),
                 "/usr/bin/teamd -t %s -d -g -c '{\"device\":\"%s\", "
                 "\"runner\":{\"name\":\"lacp\", "
                 "\"tx_hash\": [\"l3\", \"l4\"]}}'",
                 aggr->ifname, aggr->ifname);
    }
    else
    {
        ERROR("Incorrect mode value: %d", aggr->mode);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if (ta_system(buf) != 0)
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    if (if_nametoindex(aggr->ifname) <= 0)
    {
        ERROR("teamd failed to create interface \"%s\"", aggr->ifname);
        ta_rsrc_delete_lock(rsrc);
        return TE_RC(TE_TA_UNIX, TE_ENODEV);
    }

    ta_interface_status_set(aggr->ifname, true);

    return 0;
}

static te_errno
team_destroy(aggregation *aggr)
{
    ta_interface_status_set(aggr->ifname, false);

    snprintf(buf, sizeof(buf),
             "/usr/bin/teamd -t %s -k", aggr->ifname);
    if (ta_system(buf) != 0)
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);

    if (if_nametoindex(aggr->ifname) > 0)
    {
        ERROR("teamd failed to delete interface \"%s\"", aggr->ifname);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }
    return 0;
}

static te_errno
team_add(aggregation *aggr, const char *ifname)
{
    ta_interface_status_set(ifname, false);
    snprintf(buf, sizeof(buf),
             "/usr/bin/teamdctl %s port add %s", aggr->ifname, ifname);
    if (ta_system(buf) != 0)
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    return 0;
}

static te_errno
team_del(aggregation *aggr, const char *ifname)
{
    snprintf(buf, sizeof(buf),
             "/usr/bin/teamdctl %s port remove %s", aggr->ifname, ifname);
    if (ta_system(buf) != 0)
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    return 0;
}

static te_errno
team_list(aggregation *aggr, char **member_list)
{
    int         out_fd = -1;
    pid_t       cmd_pid;
    FILE       *f = NULL;
    te_errno    rc = 0;
    int         status;
    int         sysrc;

    TE_SPRINTF(buf,
               "/usr/bin/teamnl %s ports | sed s/[0-9]*:\\ *// "
               "| sed s/:.*// | awk \'{print}\' ORS=\'\'",
               aggr->ifname);

    cmd_pid = te_shell_cmd(buf, -1, NULL, &out_fd, NULL);
    if (cmd_pid < 0)
    {
        ERROR("%s(): getting list of teaming interfaces failed",
              __FUNCTION__);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
    if ((f = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("Failed to obtain file pointer for shell command output");
        rc = TE_OS_RC(TE_TA_UNIX, te_rc_os2te(errno));
        goto cleanup;
    }

    memset(buf, 0, sizeof(buf));
    sysrc = fread(buf, 1, sizeof(buf), f);
    if (sysrc < 0)
    {
        ERROR("Failed to read data from the file with command output");
        rc = TE_OS_RC(TE_TA_UNIX, te_rc_os2te(errno));
        goto cleanup;
    }

    if ((*member_list = te_str_strip_spaces(buf)) == NULL)
        rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);

cleanup:
    if (f != NULL)
        fclose(f);
    close(out_fd);

    ta_waitpid(cmd_pid, &status, 0);
    if (status != 0)
    {
        ERROR("%s(): Non-zero status of teamnl: %d",
              __FUNCTION__, status);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return rc;
}


/*
 * General aggregation code
 */

/**
 * Array to be used in conversions to/from aggr_type and human-readable
 * values provided by users. This array should be synchronized with
 * aggr_type enum.
 */
static aggr_type_info aggr_types_data[] = {
    {"bond", trunk_create, trunk_destroy, trunk_add, trunk_del, trunk_list},
    {"team", team_create, team_destroy, team_add, team_del, team_list},
    //{"bridge", bridge_create, bridge_destroy, bridge_add, bridge_del}
};
#define AGGR_TYPES_NUMBER \
    ((ssize_t)(sizeof(aggr_types_data) / sizeof(aggr_type_info)))

/** Head of list with all created aggregations */
static  aggregation *aggregation_list_head = NULL;


/**
 * Convert aggregation type from human-readable value provided by user
 * to aggr_type
 */
static inline aggr_type
aggr_value_to_type(const char *type_string, aggr_mode *mode)
{
    aggr_type_info *t;

    for (t = aggr_types_data; t - aggr_types_data < AGGR_TYPES_NUMBER; t++)
    {
        if (strncmp(t->value, type_string, strlen(t->value)) == 0)
        {
            *mode = atoi(type_string + strlen(t->value));
            return (aggr_type)(t - aggr_types_data);
        }
    }
    ERROR("Failed to convert string \"%s\" to aggregation type",
          type_string);
    return AGGREGATION_INVALID;
}

/**
 * Convert aggregation type from aggr_type to human-readable value.
 * Should be called for valid types only.
 */
static inline const char *
aggr_type_to_value(aggr_type type)
{
    assert(type < AGGREGATION_INVALID);
    return aggr_types_data[type].value;
}

/**
 * Find an aggregation with a given name.
 *
 * @param name  Name of aggregation to find
 *
 * @return      Aggregation with the given name or NULL
 */
static inline aggregation *
aggregation_find(const char *name)
{
    aggregation *a;

    for (a = aggregation_list_head; a != NULL; a = a->next)
    {
        if (strcmp(a->name, name) == 0)
        {
#if 0
            if (!ta_interface_is_mine(a->ifname))
            {
                ERROR("Internal test error: aggregation interface "
                      "\"%s\" is not marked as \"mine\". "
                      "Probably, TAPI should be used instead of "
                      "direct configurator API", a->ifname);
                return NULL;
            }
#endif
            return a;
        }
    }
    return NULL;
}

/**
 * Get aggregation value (i.e. type of aggregation)
 *
 * @param ctx           request context
 * @param val           location for the aggregation type
 *
 * @return              Status code
 */
static te_errno
aggregation_get(const ta_conf_ctx *ctx, te_string *val)
{
    aggregation *a = aggregation_find(ta_conf_ctx_inst(ctx, "aggregation"));

    if (a == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", aggr_type_to_value(a->type));
    return 0;
}

/**
 * Create a new aggregation
 *
 * @param ctx           request context
 * @param val           aggregation type to create
 *
 * @return              Status code
 */
static te_errno
aggregation_add(const ta_conf_ctx *ctx, const char *val)
{
    const char  *aggr_name = ta_conf_ctx_inst(ctx, "aggregation");
    aggregation *a = aggregation_find(aggr_name);
    aggr_mode    mode;
    aggr_type    type = aggr_value_to_type(val, &mode);
    te_errno     rc;

    if (type == AGGREGATION_INVALID)
    {
        ERROR("Can't aggregation aggregation with type \"%s\"", val);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    if (a != NULL)
    {
        ERROR("Can't create aggregation with name \"%s\" "
              "because it already exists", aggr_name);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    /* Do the real job */
    a = TE_ALLOC(sizeof(aggregation));
    a->type = type;
    a->mode = mode;
    a->name = TE_STRDUP(aggr_name);

    rc = aggr_types_data[type].create(a);
    if (rc != 0)
    {
        free(a);
        return rc;
    }

    /* Insert aggregation into the linked list */
    a->next = aggregation_list_head;
    aggregation_list_head = a;

    return 0;
}

/**
 * Delete an aggregation
 *
 * @param ctx           request context
 *
 * @return              Status code
 */
static te_errno
aggregation_del(const ta_conf_ctx *ctx)
{
    aggregation *a = aggregation_find(ta_conf_ctx_inst(ctx, "aggregation"));
    te_errno     rc;

    if (a == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    /* Do the real actions */
    rc = aggr_types_data[a->type].destroy(a);
    if (rc != 0)
        return rc;

    /* Remove aggregation from the list */
    if (a == aggregation_list_head)
        aggregation_list_head = aggregation_list_head->next;
    else
    {
        aggregation *prev;

        for (prev = aggregation_list_head; prev != NULL; prev = prev->next)
        {
            if (prev->next == a)
            {
                prev->next = a->next;
                break;
            }
        }
    }
    free(a);
    return 0;
}

/**
 * List all aggregations
 *
 * @param ctx           request context
 * @param names         vector of heap-allocated names to append to
 *
 * @return              Status code
 */
static te_errno
aggregation_list(const ta_conf_ctx *ctx, te_vec *names)
{
    aggregation *a;

    UNUSED(ctx);

    for (a = aggregation_list_head; a != NULL; a = a->next)
    {
        char *name = TE_STRDUP(a->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * Get interface name of the aggregation
 *
 * @param ctx           request context
 * @param val           location for the interface name
 *
 * @return              status code
 */
static te_errno
aggr_interface_get(const ta_conf_ctx *ctx, te_string *val)
{
    aggregation *a = aggregation_find(ta_conf_ctx_inst(ctx, "aggregation"));

    if (a == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", a->ifname);
    return 0;
}

/**
 * Add a member to the aggregation
 *
 * @param ctx           request context
 *
 * @return              status code
 */
static te_errno
aggr_member_add(const ta_conf_ctx *ctx)
{
    aggregation *a = aggregation_find(ta_conf_ctx_inst(ctx, "aggregation"));
    const char  *member_name = ta_conf_ctx_inst(ctx, "member");

    if (a == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return aggr_types_data[a->type].add(a, member_name);
}

/**
 * Delete a member from the aggregation
 *
 * @param ctx           request context
 *
 * @return              status code
 */
static te_errno
aggr_member_del(const ta_conf_ctx *ctx)
{
    aggregation *a = aggregation_find(ta_conf_ctx_inst(ctx, "aggregation"));
    const char  *member_name = ta_conf_ctx_inst(ctx, "member");

    if (a == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return aggr_types_data[a->type].del(a, member_name);
}

/**
 * List all members of the aggregation
 *
 * @param ctx           request context
 * @param names         vector of heap-allocated names to append to
 *
 * @return              status code
 */
static te_errno
aggr_member_list(const ta_conf_ctx *ctx, te_vec *names)
{
    const char  *aggr_name = ta_conf_ctx_inst(ctx, "aggregation");
    aggregation *a = aggregation_find(aggr_name);
    char        *member_list;
    char        *copy;
    char        *saveptr;
    char        *tok;
    te_errno     rc;

    if (a == NULL)
    {
        ERROR("Failed to find aggregation %s", aggr_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    rc = aggr_types_data[a->type].list(a, &member_list);
    if (rc != 0)
        return rc;

    copy = TE_STRDUP(member_list);
    for (tok = strtok_r(copy, " ", &saveptr); tok != NULL;
         tok = strtok_r(NULL, " ", &saveptr))
    {
        char *name = TE_STRDUP(tok);

        TE_VEC_APPEND(names, name);
    }
    free(copy);
    free(member_list);

    return 0;
}

static const ta_conf_node *const node_aggr =
    TA_CONF_COLL_STR("aggregation",
                     aggregation_get, aggregation_add,
                     aggregation_del, aggregation_list,

        TA_CONF_COLL("member", aggr_member_add, aggr_member_del,
                     aggr_member_list),
        TA_CONF_RO_STR("interface", aggr_interface_get));

te_errno
ta_unix_conf_aggr_init(void)
{
    return ta_conf_register("/agent", node_aggr);
}

