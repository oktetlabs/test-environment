/** @file 
 * @brief Test Environment: Programm that creates TCL filter file
 * from XML filter file.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <setjmp.h>
#include <popt.h>

#include <libxml/parser.h>
#include <glib.h>

#include "te_defs.h"
#include "logger_defs.h"
#include "te_raw_log.h"
#include "te_stdint.h"
#include "rgt_tmpls_lib.h"
#include "rgt-filter-xml2tcl.h"

#define UTILITY_NAME "rgt-filter-xml2tcl"

/** The list of possible states in XML processing state machine */
typedef enum rgt_state {
    RGT_ST_INITIAL, /**< Initial state */

    RGT_ST_FILTERS,

    RGT_ST_ENTITY_FILTER,    
    RGT_ST_ENTITY_INCLUDE,
    RGT_ST_ENTITY_EXCLUDE,
    RGT_ST_ENTITY_USER,

    RGT_ST_BRANCH_FILTER,
    RGT_ST_BRANCH_INCLUDE,
    RGT_ST_BRANCH_EXCLUDE,

    RGT_ST_DURATION_FILTER,
    RGT_ST_DURATION_INCLUDE,
    RGT_ST_DURATION_EXCLUDE,
} rgt_state_t;

/** Structure to keep information about the particular log entity */
typedef struct log_entity {
    char         *name; /**< Entity name */
    GHashTable   *users; /**< Users of this entity */
    te_bool       include; /**< Default behaviour of this entity */
    te_log_level  level; /**< Log levels to include */
} log_entity_t;

/** Structure to keep information about the particular user */
typedef struct rgt_log_user {
    char         *name; /**< User name */
    te_log_level  level; /**< Log levels to include */
} log_user_t;

/** Structure to keep information about suite branch */
typedef struct branch_entry {
    char    *path; /**< Path of this branch */
    te_bool  include; /**< Whether to include or exclude this entry */
} branch_entry_t;

typedef enum suite_node {
    SUITE_NODE_PACKAGE = 0,
    SUITE_NODE_SESSION = 1,
    SUITE_NODE_TEST = 2,
    SUITE_NODE_NUM = 3,
} suite_node_t;

#define SUITE_NODE_PACKAGE_STR "pkg"
#define SUITE_NODE_SESSION_STR "session"
#define SUITE_NODE_TEST_STR "test"

/** Structure to keep informaiton about duration */
typedef struct duration_entry {
    te_bool  include; /**< Whether this is inclusive duration */
    uint32_t min; /**< lower bound of the entry */
    uint32_t max; /**< Upper bound of the entry */
} duration_entry_t;

/** 
 * Structure that keeps basic data used in processing XML file.
 */
typedef struct rgt_gen_ctx {
    const char   *xml_fname; /**< XML file name */
    FILE         *fd; /**< Output file handle */
    rgt_state_t   state; /**< Current processing state */
    rgt_state_t   bkp_state; /**< Field to keep previous processing 
                                  state, if necessary. */

    /** @name Entity filter related fields */
    GHashTable   *entities; /**< A set of entities with defined names */
    log_entity_t *def_entity; /**< Default entity */
    log_entity_t *cur_entity; /**< Entity currently used */

    /**
     * Storage for information about entity.
     * While parsing "include/exclude" tags we do not know wheter 
     * there will be users under it or not.
     */
    log_entity_t  entity_cache;

    /** Wheter the cache was used by user or not? */
    te_bool       entity_cache_used;
    /*@}*/

    /** @name Branch filter related fields */
    GHashTable   *pathes; /**< Test tree pathes */
    /*@}*/

    /** @name Duration filter related fields */
    GQueue       *nodes[SUITE_NODE_NUM];
    /*@}*/

    jmp_buf       jmp_env; /**< Context to generate an exception */
    char          err_msg[255]; /**< Error message to print */
} rgt_gen_ctx_t;


static te_log_level get_level_mask(const char *level_str);
static void entity_filter_start_rule(rgt_gen_ctx_t *ctx,
                                     const xmlChar **xml_attrs);
static void entity_filter_end_rule(rgt_gen_ctx_t *ctx);
static void entity_filter_process_user(rgt_gen_ctx_t *ctx,
                                       const xmlChar **xml_attrs);

/**
 * Terminates execution flow generating an exception.
 *
 * @param ctx      Global programm context
 * @param fmt_str  Error message format string followed by arguments
 */
static inline void
throw_exception(rgt_gen_ctx_t *ctx, const char *fmt_str, ...)
{
    va_list ap;
    
    va_start(ap, fmt_str);
    vsnprintf(ctx->err_msg, sizeof(ctx->err_msg), fmt_str, ap);
    va_end(ap);

    longjmp(ctx->jmp_env, 1);
}
    
/**
 * Checks that expression @p expr_ is not NULL,
 * otherwise it generates an exception.
 */
#define CHECK_NOT_NULL(expr_) \
    do {                                               \
        if (!(expr_))                                  \
        {                                              \
            throw_exception(ctx, "Not enough memory"); \
        }                                              \
    } while (0);

static inline const char *
suite_node_h2str(suite_node_t node_type)
{
    switch (node_type)
    {
#define NODE_TYPE_CASE(val_) \
        case SUITE_NODE_ ## val_: return SUITE_NODE_ ## val_ ## _STR
        
        NODE_TYPE_CASE(PACKAGE);
        NODE_TYPE_CASE(SESSION);
        NODE_TYPE_CASE(TEST);

#undef NODE_TYPE_CASE
        
        default:
            return "UNKNOWN";
    }
}

/**
 * Returns log level bit mask based on string value of it.
 *
 * @param level_str  Log level in string representation
 *
 * @return Log level as a bit mask.
 */
static te_log_level
get_level_mask(const char *level_str)
{
    const char   *end_ptr;
    const char   *ptr = level_str;
    char         *tmp;
    uint32_t      len;
    te_log_level  val = 0;
    te_log_level  prev_val;

    if (level_str == NULL || strlen(level_str) == 0)
        return 0xffff;

    end_ptr = level_str + strlen(level_str);

    while (ptr != NULL)
    {
        tmp = strchr(ptr, ',');
        if (tmp == NULL)
            len = end_ptr - ptr;
        else
            len = tmp - ptr;

#define CHECK_SET_BIT(level_) \
        do {                                       \
            if (prev_val == val &&                 \
                strncmp(ptr, #level_, len) == 0 && \
                strlen(#level_) == len)            \
            {                                      \
                if ((ptr = tmp) != NULL)           \
                    ptr++;                         \
                                                   \
                val |= TE_LL_ ## level_;           \
            }                                      \
        } while (0)

        prev_val = val;

        CHECK_SET_BIT(ERROR);
        CHECK_SET_BIT(WARN);
        CHECK_SET_BIT(RING);
        CHECK_SET_BIT(INFO);
        CHECK_SET_BIT(VERB);
        CHECK_SET_BIT(ENTRY_EXIT);

#undef CHECK_SET_BIT

        if (prev_val == val)
        {
            char buf[len + 1];
            
            memcpy(buf, ptr, len);
            buf[len] = '\0';

            fprintf(stderr,
                    "WARN: Unrecognized log level '%s' found\n", buf);
        }
    }

    return val;
}

/**
 * Returns the range of nodes which need to be updated accoring to
 * the name of the node.
 *
 * @param node     Name of the node
 * @param start_i  First index in the range
 * @param start_i  Last index in the range
 *
 * @return Log level as a bit mask.
 */
static void
get_node_range(const char *node, suite_node_t *start_i, suite_node_t *end_i)
{
    if (node == NULL || strlen(node) == 0)
    {
        *start_i = SUITE_NODE_PACKAGE;
        *end_i = SUITE_NODE_TEST;
        return;
    }

#define CHECK_NODE(val_) \
    do {                                     \
        if (strcmp(node, val_ ## _STR) == 0) \
        {                                    \
            *start_i = *end_i = val_;        \
            return;                          \
        }                                    \
    } while (0)

    CHECK_NODE(SUITE_NODE_PACKAGE);
    CHECK_NODE(SUITE_NODE_SESSION);
    CHECK_NODE(SUITE_NODE_TEST);
    
    assert(0);
}

/* -- ENTRY FILTER OUTPUT -- */

static void
cb_user_func(gpointer key, gpointer value, gpointer user_data)
{
    rgt_gen_ctx_t *ctx = (rgt_gen_ctx_t *)user_data;
    log_user_t    *user = (log_user_t *)value;
    rgt_attrs_t   *attrs;

    assert(strcmp(key, user->name) == 0);

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_fstr(attrs, "name", "%s", user->name);
    rgt_tmpls_attrs_add_uint32(attrs, "levels", user->level);
    rgt_tmpls_output(ctx->fd, 
                     &xml2fmt_tmpls[ENTITY_FILTER_USER_ENTRY], attrs);
    rgt_tmpls_attrs_free(attrs);

    free(user->name);
    free(user);
}

static void
out_entity(rgt_gen_ctx_t *ctx, log_entity_t *entity)
{
    rgt_attrs_t *attrs;

    if (strlen(entity->name) != 0)
    {
        attrs = rgt_tmpls_attrs_new(NULL);
        rgt_tmpls_attrs_add_fstr(attrs, "name", "%s", entity->name);
        rgt_tmpls_output(ctx->fd,
                         &xml2fmt_tmpls[ENTITY_FILTER_ENTRY_START], attrs);
        rgt_tmpls_attrs_free(attrs);
    }
    else
    {
        rgt_tmpls_output(ctx->fd,
                         &xml2fmt_tmpls[ENTITY_FILTER_ENTRY_START_DEF],
                         NULL);
    }

    if (entity->users != NULL)
        g_hash_table_foreach(entity->users, cb_user_func, ctx);

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_uint32(attrs, "levels", entity->level);
    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[ENTITY_FILTER_USER_ENTRY_DEF], attrs);
    rgt_tmpls_attrs_free(attrs);

    if (entity->users != NULL)
        g_hash_table_destroy(entity->users);

    free(entity->name);
    free(entity);
}

static void
cb_func(gpointer key, gpointer value, gpointer user_data)
{
    rgt_gen_ctx_t *ctx = (rgt_gen_ctx_t *)user_data;
    log_entity_t  *entity = (log_entity_t *)value;

    assert(strcmp(key, entity->name) == 0);

    if (strcmp(key, "") == 0)
        return;

    out_entity(ctx, entity);
}

static void
create_entry_filter(rgt_gen_ctx_t *ctx)
{
    rgt_tmpls_output(ctx->fd, &xml2fmt_tmpls[ENTITY_FILTER_START], NULL);

    g_hash_table_foreach(ctx->entities, cb_func, ctx);

    out_entity(ctx, ctx->def_entity);

    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[ENTITY_FILTER_END], NULL);

    if (ctx->entities != NULL)
        g_hash_table_destroy(ctx->entities);
}

/* -- END ENTRY FILTER OUTPUT -- */

/* -- TIME FILTER OUTPUT -- */

static void
create_time_filter(rgt_gen_ctx_t *ctx)
{
    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[TIME_FILTER], NULL);
}

/* -- END TIME FILTER OUTPUT -- */

/* -- DURATION FILTER OUTPUT -- */

/**
 * Callback function to be run for every duration interval
 *
 * @param data       Pointer to duration interval data structure
 * @param user_data  Global context
 */
static void
duration_cb_func(gpointer data, gpointer user_data)
{
    duration_entry_t *duration = (duration_entry_t *)data;
    rgt_gen_ctx_t    *ctx = (rgt_gen_ctx_t *)user_data;
    rgt_attrs_t      *attrs;

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_uint32(attrs, "min", duration->min);
    rgt_tmpls_attrs_add_uint32(attrs, "max", duration->max);
    rgt_tmpls_attrs_add_fstr(attrs, "result",
                             duration->include ? "pass" : "fail");
    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[DURATION_FILTER_ENTRY], attrs);

    rgt_tmpls_attrs_free(attrs);
}

/**
 * Function to flush information about duration filter.
 *
 * @param ctx  Global context
 */
static void
create_duration_filter(rgt_gen_ctx_t *ctx)
{
    rgt_attrs_t  *attrs;
    unsigned int  i;

    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[DURATION_FILTER_START], NULL);

    for (i = SUITE_NODE_PACKAGE; i <= SUITE_NODE_TEST; i++)
    {
        attrs = rgt_tmpls_attrs_new(NULL);
        rgt_tmpls_attrs_add_fstr(attrs, "name", suite_node_h2str(i));
        rgt_tmpls_output(ctx->fd,
                         &xml2fmt_tmpls[DURATION_FILTER_NODE_START],
                         attrs);
        rgt_tmpls_attrs_free(attrs);
        
        g_queue_foreach(ctx->nodes[i], duration_cb_func, ctx);

        rgt_tmpls_output(ctx->fd,
                         &xml2fmt_tmpls[DURATION_FILTER_NODE_END], NULL);
    }

    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[DURATION_FILTER_END], NULL);
}

/* -- END DURATION FILTER OUTPUT -- */

/* -- BRANCH FILTER OUTPUT -- */

/**
 * Callback function to be run for every branch filter element.
 *
 * @param key        Branch path value
 * @param value      Pointer to branch data structure
 * @param user_data  Global context
 */
static void
branch_cb_func(gpointer key, gpointer value, gpointer user_data)
{
    rgt_gen_ctx_t  *ctx = (rgt_gen_ctx_t *)user_data;
    branch_entry_t *entry = (branch_entry_t *)value;
    rgt_attrs_t    *attrs;

    assert(strcmp(key, entry->path) == 0);

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_fstr(attrs, "path", "%s", entry->path);
    rgt_tmpls_attrs_add_fstr(attrs, "result", "%s",
                             entry->include ? "pass" : "fail");
    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[BRANCH_FILTER_ENTRY], attrs);
    rgt_tmpls_attrs_free(attrs);

    free(entry->path);
    free(entry);
}

/**
 * Function to flush information about branch filter.
 *
 * @param ctx  Global context
 */
static void
create_branch_filter(rgt_gen_ctx_t *ctx)
{
    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[BRANCH_FILTER_START], NULL);

    g_hash_table_foreach(ctx->pathes, branch_cb_func, ctx);

    rgt_tmpls_output(ctx->fd,
                     &xml2fmt_tmpls[BRANCH_FILTER_END], NULL);

    g_hash_table_destroy(ctx->pathes);
}

/* -- END BRANCH FILTER OUTPUT -- */


/**
 * Update duration fileter information for the specified node 
 * (inserts a new duration interval).
 *
 * @param ctx      Global context
 * @param queue    Queue of duration intervals
 * @param include  Whether this is inclusive interval
 * @param min      Lower bound of the interval
 * @param max      Upper bound of the interval
 */
static void
duration_filter_update(rgt_gen_ctx_t *ctx, GQueue *queue,
                       te_bool include, uint32_t min, uint32_t max)
{
    duration_entry_t *duration;
    duration_entry_t *entry;
    GList            *lst_entry;
    GList            *lst_next_entry;

    assert(!g_queue_is_empty(queue));

    for (lst_entry = g_queue_peek_head_link(queue);
         lst_entry != NULL;
         lst_entry = g_list_next(lst_entry))
    {
        entry = (duration_entry_t *)lst_entry->data;

        if (entry->min > min)
            break;

        if (entry->min <= min && entry->max > min)
        {
            if (entry->min == min && entry->max <= max)
            {
                entry->include = include;
                min = entry->max;
                continue;
            }

            /* We've found a starting place for our new record */
            if (entry->max >= max)
            {
                /*
                 * we're inside a single interval
                 * |-------------|
                 *  |          |
                 * min ------ max
                 */
                duration = (duration_entry_t *)
                    calloc(sizeof(*duration), 1);
                CHECK_NOT_NULL(duration);
                duration->min = min;
                duration->max = max;
                duration->include = include;

                if (entry->min == min)
                {
                    entry->min = max + 1;
                    g_queue_insert_before(queue, lst_entry, duration);
                }
                else
                {
                    g_queue_insert_after(queue, lst_entry, duration);
                    if (entry->max > max)
                    {
                        duration = (duration_entry_t *)
                            calloc(sizeof(*duration), 1);
                        CHECK_NOT_NULL(duration);
                        duration->min = max + 1;
                        duration->max = entry->max;
                        duration->include = entry->include;
                        lst_next_entry = g_list_next(lst_entry);
                        g_queue_insert_after(queue, lst_next_entry,
                                             duration);
                    }
                    entry->max = min - 1;
                }
                break;
            }
            else
            {
                /*
                 * Upper bound is out of single interval
                 * |-----|--------|
                 *   |
                 *  min ------->
                 */
                if (entry->min < min)
                {
                    /*
                     * We should split this entry into two 
                     * |-----|--------|
                     *    |
                     *   min in the middle.
                     */
                    duration = (duration_entry_t *)
                        calloc(sizeof(*duration), 1);
                    CHECK_NOT_NULL(duration);
                    duration->min = min;
                    duration->max = entry->max;
                    duration->include = include;

                    g_queue_insert_after(queue, lst_entry, duration);

                    entry->max = min - 1;
                    min = duration->max;
                }
                else
                {
                    assert(0);
                }
            }

            break;
        }
    }
}

/**
 * This function is called when we meet <include> or <exclude> TAG
 * in the context of "duration-filter".
 *
 * @param ctx        Global context
 * @param xml_attrs  Attributes of <include>/<exclude> TAG
 */
static void
duration_filter_rule(rgt_gen_ctx_t *ctx, const xmlChar **xml_attrs)
{
    const char     *node = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "node");
    const char     *min_str = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "min");
    const char     *max_str = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "max");
    suite_node_t    i;
    suite_node_t    n;
    uint32_t        min = 0;
    uint32_t        max = UINT32_MAX;
    char           *end_ptr;
    te_bool         include = (ctx->state == RGT_ST_DURATION_INCLUDE);

    get_node_range(node, &i, &n);
    
    if (min_str != NULL)
    {
        min = strtol(min_str, &end_ptr, 10);
        if (min_str == end_ptr || *end_ptr != '\0')
        {
            throw_exception(ctx, "Incorrect value of 'min' attribute "
                            "'%s' in duration-filter\n", min_str);
        }
    }

    if (max_str != NULL)
    {
        max = strtol(max_str, &end_ptr, 10);
        if (max_str == end_ptr || *end_ptr != '\0')
        {
            throw_exception(ctx, "Incorrect value of 'max' attribute "
                            "'%s' in duration-filter\n", max_str);
        }
    }
    
    if (min >= max)
    {
        throw_exception(ctx,
                        "'min' value should be less than 'max' value\n");
    }

    while (i <= n)
    {
        /* Update duration filter for the node */
        duration_filter_update(ctx, ctx->nodes[i], include, min, max);
        i++;
    }
}

/**
 * This function is called when we meet <include> or <exclude> TAG
 * in the context of "branch-filter".
 *
 * @param ctx        Global context
 * @param xml_attrs  Attributes of <include>/<exclude> TAG
 */
static void
branch_filter_rule(rgt_gen_ctx_t *ctx, const xmlChar **xml_attrs)
{
    const char     *path = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "path");
    te_bool         include = (ctx->state == RGT_ST_BRANCH_INCLUDE);
    branch_entry_t *branch;

    if (path == NULL)
    {
        throw_exception(ctx,
                        "There is no 'path' attribute in '%s' TAG "
                        "of branch filter\n", 
                        include ? "include" : "exclude");
    }

    branch = (branch_entry_t *)g_hash_table_lookup(ctx->pathes, path);
    if (branch == NULL)
    {
        branch = (branch_entry_t *)calloc(sizeof(*branch), 1);
        CHECK_NOT_NULL(branch);

        branch->path = strdup(path);
        CHECK_NOT_NULL(branch->path);

        g_hash_table_insert(ctx->pathes, branch->path, branch);
    }
    branch->include = include;
}

/**
 * This function is called when we meet <include> or <exclude> TAG
 * in the context of "entity-filter".
 *
 * @param ctx        Global context
 * @param xml_attrs  Attributes of <include>/<exclude> TAG
 */
static void
entity_filter_start_rule(rgt_gen_ctx_t *ctx, const xmlChar **xml_attrs)
{
    const char   *entity = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "entity");
    const char   *level = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "level");
    te_log_level  level_mask;

    if (entity == NULL || strlen(entity) == 0)
    {
        ctx->cur_entity = ctx->def_entity;
    }
    else
    {
        if ((ctx->cur_entity = (log_entity_t *)
                g_hash_table_lookup(ctx->entities, entity)) == NULL)
        {
            ctx->cur_entity = 
                (log_entity_t *)calloc(sizeof(log_entity_t), 1);
            CHECK_NOT_NULL(ctx->cur_entity);

            ctx->cur_entity->name = strdup(entity);
            CHECK_NOT_NULL(ctx->cur_entity->name);

            g_hash_table_insert(ctx->entities, ctx->cur_entity->name,
                                ctx->cur_entity);

            ctx->cur_entity->include = ctx->def_entity->include;
            ctx->cur_entity->level = ctx->def_entity->level;
        }
    }

    ctx->entity_cache.include = (ctx->state == RGT_ST_ENTITY_INCLUDE);

    level_mask = get_level_mask(level);

    ctx->entity_cache.level = ctx->cur_entity->level;
    if (ctx->entity_cache.include)
        ctx->entity_cache.level |= level_mask;
    else
        ctx->entity_cache.level &= (~level_mask);

    ctx->entity_cache_used = FALSE;
}

/**
 * This function is called when we meet closing <include> or 
 * <exclude> TAG in the context of "entity-filter".
 *
 * @param ctx  Global context
 */
static void
entity_filter_end_rule(rgt_gen_ctx_t *ctx)
{
    if (ctx->entity_cache_used)
    {
        /* 
         * Information was related to the particular user(s) 
         * and it is already taken into account.
         */
        return;
    }

    assert(ctx->entity_cache.include == 
                    (ctx->state == RGT_ST_ENTITY_INCLUDE));

    /* Change default configuration of current entity */
    ctx->cur_entity->include = ctx->entity_cache.include;
    ctx->cur_entity->level = ctx->entity_cache.level;
}

/**
 * Data structure to be used as user-provided parameter for
 * add_user() callback function.
 */
typedef struct user_attrs {
    rgt_gen_ctx_t *ctx; /**< Global context */
    const char    *name; /**< User's name */
    const char    *level; /**< Log level specified for the user */
} user_attrs_t;

/**
 * Callback function to insert user into the particular entity.
 *
 * @param key        Entity name
 * @param value      Pointer to entity data structure
 * @param user_data  Pointer to the structure of type "user_attrs_t"
 */
static void
add_user(gpointer key, gpointer value, gpointer user_data)
{
    log_entity_t  *entity = (log_entity_t *)value;
    user_attrs_t  *attrs = (user_attrs_t *)user_data;
    rgt_gen_ctx_t *ctx = attrs->ctx;
    const char    *name = attrs->name;
    const char    *level = attrs->level;
    log_user_t    *user;

    UNUSED(key);

    assert(strcmp(key, entity->name) == 0);
    assert(entity->users != NULL);

    user = (log_user_t *)g_hash_table_lookup(entity->users, name);
    if (user == NULL)
    {
        user = (log_user_t *)calloc(sizeof(*user), 1);
        CHECK_NOT_NULL(user);

        user->name = strdup(name);
        CHECK_NOT_NULL(user->name);

        g_hash_table_insert(entity->users, user->name, user);
        user->level = entity->level;
    }

    if (level == NULL)
    {
        user->level = ctx->entity_cache.level;
    }
    else
    {
        te_log_level level_mask = get_level_mask(level);

        if (ctx->entity_cache.include)
            user->level |= level_mask;
        else
            user->level &= (~level_mask);
    }
}

/**
 * This function is called when we meet <user> TAG in 
 * the context of "entity-filter".
 *
 * @param ctx        Global context
 * @param xml_attrs  Attributes of <user> TAG
 */
static void
entity_filter_process_user(rgt_gen_ctx_t *ctx, const xmlChar **xml_attrs)
{
    const char   *name = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "name");
    const char   *level = 
        rgt_tmpls_xml_attrs_get((const char **)xml_attrs, "level");
    user_attrs_t  user_attr = { ctx, name, level };

    if (ctx->cur_entity->users == NULL)
    {
        ctx->cur_entity->users = g_hash_table_new(g_str_hash, g_str_equal);
    }

    if (strlen(ctx->cur_entity->name) == 0)
    {
        /* Default entity, so add user to every entity */
        g_hash_table_foreach(ctx->entities, add_user, &user_attr);
    }
    else
    {
        add_user(ctx->cur_entity->name, ctx->cur_entity, &user_attr);
    }

    ctx->entity_cache_used = TRUE;
}

/* ---------------------------------- */
/* -- XML PARSER CALLBACK HANDLERS -- */
/* ---------------------------------- */

/**
 * Callback function that is called before parsing the document.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 *
 * @return Nothing
 */
static void
rgt_start_document(void *user_data)
{
    rgt_gen_ctx_t    *ctx = (rgt_gen_ctx_t *)user_data;
    rgt_attrs_t      *attrs;
    log_entity_t     *def_entity;
    duration_entry_t *duration;
    unsigned int      i;

    /* Entity filter preparation */
    ctx->entities = g_hash_table_new(g_str_hash, g_str_equal);

    def_entity = (log_entity_t *)calloc(sizeof(*ctx->def_entity), 1);
    CHECK_NOT_NULL(def_entity);

    def_entity->name = strdup("");
    CHECK_NOT_NULL(def_entity->name);

    def_entity->include = TRUE;
    def_entity->level = 0xffff;

    g_hash_table_insert(ctx->entities, def_entity->name, def_entity);
    ctx->def_entity = def_entity;

    /* Branch filter preparation */
    ctx->pathes = g_hash_table_new(g_str_hash, g_str_equal);

    /* Duration filter preparation */
    for (i = 0; i < sizeof(ctx->nodes) / sizeof(ctx->nodes[0]); i++)
    {
        ctx->nodes[i] = g_queue_new();
        duration = (duration_entry_t *)calloc(sizeof(*duration), 1);
        CHECK_NOT_NULL(duration);
        duration->min = 0;
        duration->max = UINT32_MAX;
        duration->include = TRUE;
        g_queue_push_head(ctx->nodes[i], duration);
    }

    attrs = rgt_tmpls_attrs_new(NULL);
    rgt_tmpls_attrs_add_fstr(attrs, "utility", UTILITY_NAME);
    rgt_tmpls_output(ctx->fd, &xml2fmt_tmpls[DOC_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

/**
 * Callback function that is called when XML parser reaches the end 
 * of the document.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 *
 * @return Nothing
 */
static void
rgt_end_document(void *user_data)
{
    rgt_gen_ctx_t *ctx = (rgt_gen_ctx_t *)user_data;

    /* Flush the information */
    create_entry_filter(ctx);
    create_time_filter(ctx);
    create_duration_filter(ctx);
    create_branch_filter(ctx);

    rgt_tmpls_output(ctx->fd, &xml2fmt_tmpls[DOC_END], NULL);
}

/**
 * Callback function that is called when XML parser meets an opening tag.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  name       The element name
 * @param  attrs       An array of attribute name, attribute value pairs
 *
 * @return Nothing
 */
static void
rgt_start_element(void *user_data,
                  const xmlChar *tag, const xmlChar **attrs)
{
    rgt_gen_ctx_t *ctx = (rgt_gen_ctx_t *)user_data;

    switch (ctx->state)
    {
        case RGT_ST_INITIAL:
            if (strcmp(tag, "filters") == 0)
            {
                ctx->state = RGT_ST_FILTERS;
                break;
            }
            throw_exception(ctx,
                            "Unexpected TAG '%s' in INIT state\n", tag);
            break;

        case RGT_ST_FILTERS:
            if (strcmp(tag, "entity-filter") == 0)
            {
                ctx->state = RGT_ST_ENTITY_FILTER;
                break;
            }
            else if (strcmp(tag, "branch-filter") == 0)
            {
                ctx->state = RGT_ST_BRANCH_FILTER;
                break;
            }
            else if (strcmp(tag, "duration-filter") == 0)
            {
                ctx->state = RGT_ST_DURATION_FILTER;
                break;
            }
            throw_exception(ctx,
                            "Unexpected TAG '%s' in FILTERS state\n", tag);
            break;

        case RGT_ST_DURATION_FILTER:
            if (strcmp(tag, "include") == 0)
            {
                ctx->state = RGT_ST_DURATION_INCLUDE;
            }
            else if (strcmp(tag, "exclude") == 0)
            {
                ctx->state = RGT_ST_DURATION_EXCLUDE;
            }
            else
            {
                throw_exception(ctx,
                    "Unexpected TAG '%s' in DURATION_FILTER state\n", tag);
            }
            duration_filter_rule(ctx, attrs);
            break;
        
        case RGT_ST_DURATION_INCLUDE:
        case RGT_ST_DURATION_EXCLUDE:
            assert(0);
            break;

        case RGT_ST_BRANCH_FILTER:
            if (strcmp(tag, "include") == 0)
            {
                ctx->state = RGT_ST_BRANCH_INCLUDE;
            }
            else if (strcmp(tag, "exclude") == 0)
            {
                ctx->state = RGT_ST_BRANCH_EXCLUDE;
            }
            else
            {
                throw_exception(ctx,
                    "Unexpected TAG '%s' in BRANCH_FILTER state\n", tag);
            }
            branch_filter_rule(ctx, attrs);
            break;
        
        case RGT_ST_BRANCH_INCLUDE:
        case RGT_ST_BRANCH_EXCLUDE:
            assert(0);
            break;
        
        case RGT_ST_ENTITY_FILTER:
            if (strcmp(tag, "include") == 0)
            {
                ctx->state = RGT_ST_ENTITY_INCLUDE;
            }
            else if (strcmp(tag, "exclude") == 0)
            {
                ctx->state = RGT_ST_ENTITY_EXCLUDE;
            }
            else
            {
                throw_exception(ctx,
                    "Unexpected TAG '%s' in ENTITY_FILTER state\n", tag);
            }
            entity_filter_start_rule(ctx, attrs);
            break;
            
        case RGT_ST_ENTITY_INCLUDE:
        case RGT_ST_ENTITY_EXCLUDE:
            if (strcmp(tag, "user") == 0)
            {
                ctx->bkp_state = ctx->state;
                ctx->state = RGT_ST_ENTITY_USER;
            }
            else
            {
                throw_exception(ctx,
                                "Unexpected TAG '%s' in %s state\n",
                                tag, 
                                (ctx->state == RGT_ST_ENTITY_INCLUDE) ?
                                    "INCLUDE" : "EXCLUDE");
            }
            entity_filter_process_user(ctx, attrs);
            break;
        
        default:
            assert(0);
            break;
    }
}

/**
 * Callback function that is called when XML parser meets the end of 
 * an element.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  name       The element name
 *
 * @return Nothing
 */
static void
rgt_end_element(void *user_data, const xmlChar *tag)
{
    rgt_gen_ctx_t *ctx = (rgt_gen_ctx_t *)user_data;

    switch (ctx->state)
    {
        case RGT_ST_INITIAL:
            break;

        case RGT_ST_FILTERS:
            assert(strcmp(tag, "filters") == 0);
            ctx->state = RGT_ST_INITIAL;
            break;

        case RGT_ST_DURATION_FILTER:
            assert(strcmp(tag, "duration-filter") == 0);
            ctx->state = RGT_ST_FILTERS;
            break;

        case RGT_ST_DURATION_INCLUDE:
            assert(strcmp(tag, "include") == 0);
            ctx->state = RGT_ST_DURATION_FILTER;
            break;
            
        case RGT_ST_DURATION_EXCLUDE:
            assert(strcmp(tag, "exclude") == 0);
            ctx->state = RGT_ST_DURATION_FILTER;
            break;

        case RGT_ST_BRANCH_FILTER:
            assert(strcmp(tag, "branch-filter") == 0);
            ctx->state = RGT_ST_FILTERS;
            break;

        case RGT_ST_BRANCH_INCLUDE:
            assert(strcmp(tag, "include") == 0);
            ctx->state = RGT_ST_BRANCH_FILTER;
            break;
            
        case RGT_ST_BRANCH_EXCLUDE:
            assert(strcmp(tag, "exclude") == 0);
            ctx->state = RGT_ST_BRANCH_FILTER;
            break;
        
        case RGT_ST_ENTITY_FILTER:
            assert(strcmp(tag, "entity-filter") == 0);
            ctx->state = RGT_ST_FILTERS;
            break;
            
        case RGT_ST_ENTITY_INCLUDE:
            assert(strcmp(tag, "include") == 0);
            entity_filter_end_rule(ctx);
            ctx->state = RGT_ST_ENTITY_FILTER;
            break;

        case RGT_ST_ENTITY_EXCLUDE:
            assert(strcmp(tag, "exclude") == 0);
            entity_filter_end_rule(ctx);
            ctx->state = RGT_ST_ENTITY_FILTER;
            break;

        case RGT_ST_ENTITY_USER:
            assert(strcmp(tag, "user") == 0);

            /* Get state from the current context */
            ctx->state = ctx->bkp_state;
            break;

        default:
            assert(0);
    }
}

static void
rgt_report_problem(void *user_data, const char *msg, ...)
{
    va_list ap;

    UNUSED(user_data);
    
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

/**
 * The structure specifies all types callback routines 
 * that should be called.
 */
static xmlSAXHandler sax_handler = {
    .internalSubset         = NULL,
    .isStandalone           = NULL,
    .hasInternalSubset      = NULL,
    .hasExternalSubset      = NULL,
    .resolveEntity          = NULL,
    .getEntity              = NULL,
    .entityDecl             = NULL,
    .notationDecl           = NULL,
    .attributeDecl          = NULL, /* attributeDeclDebug, */
    .elementDecl            = NULL, /* elementDeclDebug, */
    .unparsedEntityDecl     = NULL, /* unparsedEntityDeclDebug, */
    .setDocumentLocator     = NULL,
    .startDocument          = rgt_start_document,
    .endDocument            = rgt_end_document,
    .startElement           = rgt_start_element,
    .endElement             = rgt_end_element,
    .reference              = NULL, /* referenceDebug, */
    .characters             = NULL,
    .ignorableWhitespace    = NULL, /* ignorableWhitespaceDebug, */
    .processingInstruction  = NULL, /* processingInstructionDebug, */
    .comment                = NULL, /* commentDebug, */
    .warning                = rgt_report_problem, /* warningDebug, */
    .error                  = rgt_report_problem, /* errorDebug, */
    .fatalError             = rgt_report_problem, /* fatalErrorDebug, */
    .getParameterEntity     = NULL, /* getParameterEntityDebug, */
    .cdataBlock             = NULL, /* cdataBlockDebug, */
    .externalSubset         = NULL, /* externalSubsetDebug, */
    .initialized            = 1,
    /* The following fields are extensions available only on version 2 */
#if HAVE___STRUCT__XMLSAXHANDLER__PRIVATE
    ._private               = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_STARTELEMENTNS
    .startElementNs         = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_ENDELEMENTNS
    .endElementNs           = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_SERROR___
    .serror                 = NULL
#endif
};

/**
 * Print "usage" how to.
 *
 * @param  optCon    Context for parsing command line arguments.
 * @param  exitcode  Code that is passed to the "exit" call.
 * @param  error     Error message string.
 * @param  addl      Additional notes that is output.
 *
 * @return  Nothing.
 *
 * @se  Frees popt Context (specified in optCon) and 
 * exits with specified code.
 */
static void
usage(poptContext optCon, int exitcode, char *error, char *addl)
{
    poptSetOtherOptionHelp(optCon, 
        "<XML filter file> [<Result TCL file>]");
    poptPrintUsage(optCon, stderr, 0);
    if (error)
    {
        fprintf(stderr, "%s", error);
        if (addl != NULL)
            fprintf(stderr, ": %s", addl);
        fprintf(stderr, "\n");
    }

    poptFreeContext(optCon);

    exit(exitcode);
}

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated 
 * if some new options are going to be added.
 *
 * @param  argc   Number of elements in array "argv".
 * @param  argv   Array of strings that represents all
 *                command line arguments.
 *
 * @return  Nothing.
 *
 * @se
 *    The function updates the following global variables according to 
 *    the command line arguments specified in argv:
 *      html_tmpls    - Array of template file names
 *
 *    In the case of an error it calls exit() function with code 1.
 */
static void
process_cmd_line_opts(int argc, char **argv, rgt_gen_ctx_t *ctx)
{
    const char  *opt_out_fname = NULL;
    const char  *out_fname = NULL;
    poptContext  optCon;
    int          rc;
    
    /* Option Table */
    struct poptOption optionsTable[] = {
        { "version", 'v', POPT_ARG_NONE, NULL, 'v', 
          "Display version information.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            optionsTable, 0);

    poptSetOtherOptionHelp(optCon,
            "[OPTIONS...] <XML filter file> [<Result TCL file>]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 'f')
        {
            if ((ctx->xml_fname = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify XML filter file", NULL);
            }
        }
        else if (rc == 'o')
        {
            if ((opt_out_fname = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify Output file name", NULL);
            }
        }
        else if (rc == 'v')
        {
            printf("Package %s: %s version %s\n%s\n", 
                   PACKAGE, UTILITY_NAME, VERSION, TE_COPYRIGHT);
            poptFreeContext(optCon);
            exit(0);
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        exit(1);
    }

    if (ctx->xml_fname == NULL && 
        (ctx->xml_fname = poptGetArg(optCon)) == NULL)
    {
        usage(optCon, 1, "Specify XML filter file", NULL);
    }

    /* Get output file name */
    out_fname = poptGetArg(optCon);

    if (out_fname != NULL && opt_out_fname != NULL)
    {
        usage(optCon, 1, "Output file name specified twice: "
              "with -f option and as a command line argument", NULL);
    }

    if (opt_out_fname != NULL)
        out_fname = opt_out_fname;

    if (poptPeekArg(optCon) != NULL)
    {
        usage(optCon, 1, "Too many parameters specified", NULL);
    }

    if (out_fname == NULL)
    {
        ctx->fd = stdout;
    }
    else if ((ctx->fd = fopen(out_fname, "w")) == NULL)
    {
        perror(out_fname);
        poptFreeContext(optCon);
        exit(1);
    }

    poptFreeContext(optCon);
}

int
main(int argc, char **argv)
{
    rgt_gen_ctx_t gen_ctx;
    int           rc = 0;

    memset(&gen_ctx, 0, sizeof(gen_ctx));

    process_cmd_line_opts(argc, argv, &gen_ctx);

    if (rgt_tmpls_parse(xml2fmt_files, xml2fmt_tmpls,
                        xml2fmt_tmpls_num) != 0)
    {
        assert(0);
    }

    gen_ctx.state = RGT_ST_INITIAL;

    if ((rc = setjmp(gen_ctx.jmp_env)) == 0)
    {

        if (xmlSAXUserParseFile(&sax_handler, &gen_ctx, 
                                gen_ctx.xml_fname) != 0)
        {
            rc = 1;
            snprintf(gen_ctx.err_msg, sizeof(gen_ctx.err_msg),
                     "Cannot parse XML document\n");
        }
    }

    if (rc != 0)
        fprintf(stderr, gen_ctx.err_msg);

    rgt_tmpls_free(xml2fmt_tmpls, xml2fmt_tmpls_num);

    xmlCleanupParser();

    return rc;
}
