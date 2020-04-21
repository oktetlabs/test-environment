/** @file
 * @brief Log processing
 *
 * Message filter implementation.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#define TE_LGR_USER "Log processing"

#include "te_config.h"

#include <stdio.h>

#include "logger_api.h"
#include "te_alloc.h"
#include "log_msg_filter.h"

/* Compile a PCRE */
static te_errno
prepare_pcre(const char *pattern, pcre **regex)
{
    const char *errstr;
    int         erroff;
    int         errcode;

    *regex = pcre_compile2(pattern, 0, &errcode, &errstr, &erroff, NULL);
    if (*regex == NULL)
    {
        ERROR("Failed to compile regex '%s' at offset %d: %s",
              pattern, erroff, errstr);
        if (errcode == 21)
            return TE_ENOMEM;
        return TE_EINVAL;
    }

    return 0;
}

/**
 * Check entity/user name against the name/PCRE stored in a filter.
 *
 * @param name          name to be checked
 * @param name_len      its length
 * @param fname         name stored in a filter
 * @param regex         pcre stored in a filter
 */
static te_bool
check_name(const char *name, size_t name_len, const char *fname, pcre *regex)
{
    if (regex == NULL)
        return strncmp(name, fname, name_len) == 0;
    else
        return pcre_exec(regex, NULL, name, name_len, 0, 0, NULL, 0) >= 0;
}

/**
 * Initialize a user filter.
 *
 * @param filter        user filter
 * @param name          user name or PCRE
 * @param regex         is name a PCRE?
 *
 * @returns Status code
 */
static te_errno
log_user_filter_init(log_user_filter *filter, const char *name, te_bool regex)
{
    te_errno rc;

    filter->level = LOG_LEVEL_ALL;
    filter->name = NULL;
    filter->regex = NULL;

    if (name != NULL)
    {
        filter->name = strdup(name);
        if (filter->name == NULL)
            return TE_ENOMEM;
    }

    if (regex)
    {
        if (name == NULL)
            return TE_EINVAL;

        rc = prepare_pcre(name, &filter->regex);
        if (rc != 0)
        {
            free(filter->name);
            return rc;
        }
    }

    return 0;
}

/**
 * Free the memory allocated by a user filter.
 *
 * @param filter        user filter
 */
static void
log_user_filter_free(log_user_filter *filter)
{
    free(filter->name);
    pcre_free(filter->regex);
}

/**
 * Initialize an entity filter.
 *
 * @param filter        entity filter
 * @param name          entity name or PCRE
 * @param regex         is name a PCRE?
 *
 * @returns Status code
 */
static te_errno
log_entity_filter_init(log_entity_filter *filter,
                       const char *name, te_bool regex)
{
    te_errno rc;

    filter->level = LOG_LEVEL_ALL;
    filter->name = NULL;
    filter->regex = NULL;

    if (name != NULL)
    {
        filter->name = strdup(name);
        if (filter->name == NULL)
            return TE_ENOMEM;
    }

    if (regex)
    {
        if (name == NULL)
            return TE_EINVAL;

        rc = prepare_pcre(name, &filter->regex);
        if (rc != 0)
        {
            free(filter->name);
            return rc;
        }
    }

    return 0;
}

/**
 * Free the memory allocated by the entity filter.
 *
 * @param filter        entity filter
 */
static void
log_entity_filter_free(log_entity_filter *filter)
{
    log_user_filter *user;
    log_user_filter *tmp;

    free(filter->name);
    pcre_free(filter->regex);

    SLIST_FOREACH_SAFE(user, &filter->users, links, tmp)
    {
        log_user_filter_free(user);
        free(user);
    }
}

/* See description in raw_log_filter.h */
te_errno
log_msg_filter_init(log_msg_filter *filter)
{
    SLIST_INIT(&filter->entities);
    return log_entity_filter_init(&filter->def_entity, NULL, FALSE);
}

/**
 * Find an entity filter for a given name.
 * Create one if it does not exist.
 *
 * @param filter        message filter
 * @param name          entity name or PCRE
 * @param rege          is name a PCRE?
 */
static log_entity_filter *
log_msg_filter_get_entity(log_msg_filter *filter,
                          const char *name, te_bool regex)
{
    log_entity_filter *entity = NULL;

    if (name == NULL || strcmp(name, "") == 0)
        return &filter->def_entity;

    SLIST_FOREACH(entity, &filter->entities, links)
    {
        if (entity == NULL)
            break;
        if (strcmp(entity->name, name) == 0)
            return entity;
    }

    /* Create if does not exist */
    entity = TE_ALLOC(sizeof(log_entity_filter));
    if (entity == NULL)
        return NULL;

    log_entity_filter_init(entity, name, regex);
    entity->level = filter->def_entity.level;

    /*
     * Users from the default entity are not copied to conform with the
     * original implementation.
     */

    SLIST_INSERT_HEAD(&filter->entities, entity, links);

    return entity;
}

/* See desctiption in raw_log_filter.h */
te_errno
log_msg_filter_set_default(log_msg_filter *filter, te_bool include,
                           te_log_level level_mask)
{
    /*
     * This change is not applied to existing entities in order to
     * conform to the current RGT behaviour.
     */
    if (include)
        filter->def_entity.level |= level_mask;
    else
        filter->def_entity.level &= ~level_mask;

    return 0;
}

/* See description in raw_log_filter.h */
te_errno
log_msg_filter_add_entity(log_msg_filter *filter, te_bool include,
                          const char *name, te_bool regex,
                          te_log_level level_mask)
{
    log_entity_filter *entity;

    entity = log_msg_filter_get_entity(filter, name, regex);
    if (entity == NULL)
        return TE_ENOMEM;

    if (include)
        entity->level |= level_mask;
    else
        entity->level &= ~level_mask;

    return 0;
}

/**
 * Add a user rule to the given entity filter.
 *
 * @param entity        entity filter
 * @param include       include or exclude
 * @param name          user name or PCRE
 * @param regex         is name a PCRE?
 * @param level_mask    which levels should be included/excluded
 *
 * @returns Status code
 */
static te_errno
log_entity_filter_add_user(log_entity_filter *entity, te_bool include,
                           const char *name, te_bool regex,
                           te_log_level level_mask)
{
    log_user_filter *user;

    SLIST_FOREACH(user, &entity->users, links)
    {
        if (strcmp(user->name, name) == 0)
            break;
    }

    if (user == NULL)
    {
        user = TE_ALLOC(sizeof(log_user_filter));
        if (user == NULL)
            return TE_ENOMEM;

        log_user_filter_init(user, name, regex);
        user->level = entity->level;

        SLIST_INSERT_HEAD(&entity->users, user, links);
    }

    if (level_mask != 0)
    {
        if (include)
            user->level |= level_mask;
        else
            user->level &= ~level_mask;
    }
    else
    {
        user->level = entity->level;
    }

    return 0;
}

/* See description in raw_log_filter.h */
te_errno
log_msg_filter_add_user(log_msg_filter *filter, te_bool include,
                        const char *entity, te_bool entity_regex,
                        const char *user, te_bool user_regex,
                        te_log_level level_mask)
{
    log_entity_filter *ent;
    int                rc;

    if (entity == NULL)
    {
        /* Add user to all entities */
        SLIST_FOREACH(ent, &filter->entities, links)
        {
            rc = log_entity_filter_add_user(ent, include,
                                            user, user_regex, level_mask);
            if (rc != 0)
                return rc;
        }

        rc = log_entity_filter_add_user(&filter->def_entity, include,
                                        user, user_regex, level_mask);
        if (rc != 0)
            return rc;
    }
    else
    {
        /* Add user to that specific entity */
        ent = log_msg_filter_get_entity(filter, entity, entity_regex);
        if (ent == NULL)
            return TE_ENOMEM;

        rc = log_entity_filter_add_user(ent, include, user, user_regex,
                                        level_mask);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/* See description in raw_log_filter.h */
log_filter_result
log_msg_filter_check(const log_msg_filter *filter, const log_msg_view *view)
{
    const log_entity_filter *entity;
    log_user_filter         *user;
    te_log_level             level_mask;

    /* Look for an entity */
    SLIST_FOREACH(entity, &filter->entities, links)
    {
        if (check_name(view->entity, view->entity_len,
                       entity->name, entity->regex))
            break;
    }

    if (entity == NULL)
        entity = &filter->def_entity;

    /* Get the user */
    SLIST_FOREACH(user, &entity->users, links)
    {
        if (check_name(view->user, view->user_len, user->name, user->regex))
            break;
    }

    if (user == NULL)
        level_mask = entity->level;
    else
        level_mask = user->level;

    return ((view->level & level_mask) != 0) ? LOG_FILTER_PASS : LOG_FILTER_FAIL;
}

/* See description in raw_log_filter.h */
void
log_msg_filter_free(log_msg_filter *filter)
{
    log_entity_filter *entity;
    log_entity_filter *tmp;

    log_entity_filter_free(&filter->def_entity);

    SLIST_FOREACH_SAFE(entity, &filter->entities, links, tmp)
    {
        log_entity_filter_free(entity);
        free(entity);
    }
}
