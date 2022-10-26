/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * Helper functions to make regular expression substitutions
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include "trc_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_REGEX_H
#include <regex.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_string.h"
#include "te_alloc.h"
#include "logger_api.h"

#include "re_subst.h"

/** Key substitutions */
trc_re_namespaces key_namespaces = TAILQ_HEAD_INITIALIZER(key_namespaces);

/**
 * Free resourses allocated for regular expression substitution.
 */
static void
trc_re_subst_free(trc_re_subst *subst)
{
    trc_re_match_subst *m;

    if (subst == NULL)
        return;

    regfree(&subst->re);
    free(subst->str);

    while ((m = TAILQ_FIRST(&subst->with)) != NULL)
    {
        TAILQ_REMOVE(&subst->with, m, links);
        free(m);
    }

    free(subst->matches);
    free(subst);
}

/* See the description in re_subst.h */
void
trc_re_substs_free(trc_re_substs *substs)
{
    trc_re_subst   *p;

    while ((p = TAILQ_FIRST(substs)) != NULL)
    {
        TAILQ_REMOVE(substs, p, links);
        trc_re_subst_free(p);
    }
}

/* See the description in re_subst.h */
void
trc_re_namespaces_free(trc_re_namespaces *namespaces)
{
    trc_re_namespace *p;

    while ((p = TAILQ_FIRST(namespaces)) != NULL)
    {
        TAILQ_REMOVE(namespaces, p, links);
        trc_re_substs_free(&p->substs);
        free(p->name);
        free(p);
    }
}

trc_re_namespace *
trc_re_key_namespace_find(trc_re_namespaces *namespaces, const char *name)
{
    trc_re_namespace *namespace = NULL;
    assert(namespaces != NULL);

    TAILQ_FOREACH(namespace, namespaces, links)
    {
        if ((name == NULL) && (namespace->name == NULL))
            return namespace;

        if ((name == NULL) || (namespace->name == NULL))
            continue;

        if (strcmp(name, namespace->name) == 0)
            return namespace;
    }

    return NULL;
}

trc_re_namespace *
trc_re_key_namespace_create(trc_re_namespaces *namespaces, const char *name)
{
    trc_re_namespace *namespace = NULL;
    assert(namespaces != NULL);

    VERB("%s('%s')", __FUNCTION__, name);

    if ((namespace = trc_re_key_namespace_find(namespaces, name)) != NULL)
    {
        /* If namespace exists already, it is not a problem here */
        return namespace;
    }

    if ((namespace = calloc(1, sizeof(trc_re_namespace))) == NULL)
        return NULL;

    if (name != NULL)
        namespace->name = strdup(name);
    TAILQ_INIT(&namespace->substs);

    TAILQ_INSERT_HEAD(namespaces, namespace, links);

    return namespace;
}

/**
 * Parse substitution string.
 *
 * @param p             Substitution info
 *
 * @return Status code.
 */
static te_errno
trc_re_subst_parse(trc_re_subst *p)
{
    char               *s = p->str;
    trc_re_match_subst *m;

    while (*s != '\0')
    {
        m = malloc(sizeof(*m));
        if (m == NULL)
            return TE_ENOMEM;

        m->match = FALSE;

        if (s[0] == '\\')
        {
            *s++ = '\0';
            if (isdigit(*s))
            {
                m->match = TRUE;
                m->u.match = *s - '0';
                if (p->max_match < m->u.match)
                    p->max_match = m->u.match;
                s++;
            }
        }
        if (!m->match)
        {
            m->u.str = s;
            s = strchrnul(s, '\\');
        }
        TAILQ_INSERT_TAIL(&p->with, m, links);
    }

    return 0;
}

/* Add substitution for a given regular expression */
static te_errno
add_subst(trc_re_namespace *namespace,
          const char *match_str,
          const char *replace_str)
{
    trc_re_subst *subst = NULL;
    te_errno rc = 0;

    subst = TE_ALLOC(sizeof(*subst));
    subst->max_match = 0;
    TAILQ_INIT(&subst->with);
    subst->matches = NULL;

    subst->str = strdup(replace_str);
    if (subst->str == NULL)
    {
        rc = TE_ENOMEM;
        goto cleanup;
    }

    if (regcomp(&subst->re, match_str, REG_EXTENDED) != 0)
    {
        ERROR("%s(): failed to compile regular expression '%s'",
              __FUNCTION__, match_str);
        rc = TE_EFAIL;
        goto cleanup;
    }

    rc = trc_re_subst_parse(subst);
    if (rc != 0)
        goto cleanup;

    subst->max_match++;
    subst->matches = TE_ALLOC(subst->max_match * sizeof(*subst->matches));
    if (subst->matches == NULL)
        rc = TE_ENOMEM;

cleanup:

    if (rc == 0)
        TAILQ_INSERT_TAIL(&namespace->substs, subst, links);
    else
        trc_re_subst_free(subst);

    return rc;
}

/* Read records matching key domains to URL prefixes */
static te_errno
key_domains_read(FILE *f)
{
    te_errno rc;
    te_string match_str = TE_STRING_INIT_STATIC(1024);
    te_string replace_str = TE_STRING_INIT_STATIC(1024);
    char buf[256];
    char *s = NULL;

    char *domain = NULL;
    char *url = NULL;
    trc_re_namespace *url_namespace = NULL;

    url_namespace = trc_re_key_namespace_create(&key_namespaces,
                                                TRC_RE_KEY_URL);
    if (url_namespace == NULL)
    {
        ERROR("%s(): failed to create namespace %s", __FUNCTION__,
              TRC_RE_KEY_URL);
        return TE_EFAIL;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        s = strchr(buf, '\n');
        if (s != NULL)
            *s = '\0';

        /* Empty string terminates KEY_DOMAINS section */
        if (buf[0] == '\0')
            break;

        s = buf;
        strsep(&s, " \t");
        if (s == NULL)
        {
            ERROR("%s(): space or TAB is missing: %s", __FUNCTION__, buf);
            continue;
        }

        for ( ; *s == ' ' || *s == '\t'; s++);

        url = s;
        domain = buf;

        for (s = domain; *s != '\0'; s++)
        {
            if (!isalnum(*s) && *s != '-')
            {
                ERROR("%s(): key domain '%s' contains not allowed "
                      "character '%c'", __FUNCTION__, domain, *s);
                break;
            }
        }
        if (*s != '\0')
            continue;

        rc = te_string_append(&match_str, "^ref://%s/(.*)", domain);

        if (rc == 0)
        {
            rc = te_string_append(&replace_str,
                                  "<a href=\"%s\\1\">%s:\\1</a>",
                                  url, domain);
        }

        if (rc == 0)
        {
            rc = add_subst(url_namespace, te_string_value(&match_str),
                           te_string_value(&replace_str));
        }

        if (rc != 0)
        {
            ERROR("%s(): failed to add URL substitution for domain %s",
                  __FUNCTION__, domain);
        }
    }

    return 0;
}

/* See the description in re_subst.h */
te_errno
trc_re_namespaces_read(const char *file, trc_re_namespaces *namespaces)
{
    char            buf[256];
    FILE             *f = NULL;
    char             *s = NULL;
    trc_re_namespace *namespace = NULL;
    te_errno          rc = 0;

    TAILQ_INIT(namespaces);

    namespace = trc_re_key_namespace_create(namespaces, NULL);

    f = fopen(file, "r");
    if (f == NULL)
    {
        ERROR("Cannot open '%s': %s", file, strerror(errno));
        return TE_EIO;
    }

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        /* Make sure that buffer is sufficient */
        s = strchr(buf, '\n');
        if (s == NULL)
        {
            ERROR("Line too long");
            continue;
        }
        *s = '\0';

        /* Skip empty line */
        if (buf[0] == '\0')
            continue;

        /* Find namespace name */
        if (buf[0] == '#')
        {
            /* Skip spaces */
            for (s = &buf[1]; *s == ' '; s++);
            if (*s == '\0')
                continue;

            if (strcmp(s, "KEY_DOMAINS") == 0)
            {
                rc = key_domains_read(f);
                if (rc != 0)
                {
                    ERROR("%s(): failed to read KEY_DOMAINS section",
                          __FUNCTION__);
                    break;
                }

                continue;
            }

            namespace = trc_re_key_namespace_create(namespaces, s);

            continue;
        }

        /* Find parrent/substitution separator */
        s = strchr(buf, '\t');
        if (s == NULL)
        {
            ERROR("Pattern/substitution separator not found");
            continue;
        }
        do {
            *s++ = '\0';
        } while (*s == '\t');

        rc = add_subst(namespace, buf, s);
        if (rc != 0)
            break;
    }

    (void)fclose(f);

    return rc;
}

/**
 * Execute substitutions.
 *
 * @param subst         Substitution to start with
 * @param str           String to substitute in
 * @param max           Maximum length of string to use
 * @param f             File to write to
 *
 * @return Status code.
 */
static void
trc_re_substs_exec(const trc_re_subst *subst, const char *str, regoff_t max,
                   FILE *f)
{
    const trc_re_subst *next;
    regoff_t            i;

    for (; subst != NULL; subst = next)
    {
        next = TAILQ_NEXT(subst, links);
        while (regexec(&subst->re, str,
                       subst->max_match, subst->matches, 0) == 0 &&
               subst->matches[0].rm_eo <= max)
        {
            const trc_re_match_subst   *ms;

            if (subst->matches[0].rm_so != 0)
                trc_re_substs_exec(next, str, subst->matches[0].rm_so, f);

            TAILQ_FOREACH(ms, &subst->with, links)
            {
                if (!ms->match)
                {
                    fputs(ms->u.str, f);
                }
                else if (subst->matches[ms->u.match].rm_so != -1)
                {
                    for (i = subst->matches[ms->u.match].rm_so;
                         i < subst->matches[ms->u.match].rm_eo;
                         ++i)
                    {
                        fputc(str[i], f);
                    }
                }
            }
            str += subst->matches[0].rm_eo;
            max -= subst->matches[0].rm_eo;
        }
    }
    for (i = 0; i < max; ++i)
        fputc(str[i], f);
}

/**
 * Execute substitutions.
 *
 * @param subst Substitution to start with
 * @param str   String to substitute in
 * @param max   Maximum length of string to use
 * @param f     File to write to
 *
 * @return      Status code.
 */
static ssize_t
trc_re_substs_exec_buf(const trc_re_subst *subst, const char *str,
                       regoff_t max, char *buf, ssize_t buf_size)
{
    const trc_re_subst *next;
    ssize_t             len = 0;
    regoff_t            i;

    for (; subst != NULL; subst = next)
    {
        next = TAILQ_NEXT(subst, links);
        while (regexec(&subst->re, str,
                       subst->max_match, subst->matches, 0) == 0 &&
               subst->matches[0].rm_eo <= max)
        {
            const trc_re_match_subst   *ms;

            if (subst->matches[0].rm_so != 0)
                len += trc_re_substs_exec_buf(next, str,
                                              subst->matches[0].rm_so,
                                              (buf != NULL) ?
                                              buf + len : NULL,
                                              (buf != NULL) ?
                                              buf_size - len : 0);

            TAILQ_FOREACH(ms, &subst->with, links)
            {
                if (!ms->match)
                {
                    if (buf != NULL)
                        len += snprintf(buf + len, buf_size - len,
                                        "%s", ms->u.str);
                    else
                        len += strlen(ms->u.str);
                }
                else if (subst->matches[ms->u.match].rm_so != -1)
                {
                    for (i = subst->matches[ms->u.match].rm_so;
                         i < subst->matches[ms->u.match].rm_eo;
                         ++i)
                    {
                        if (buf != NULL)
                        {
                            if (len < buf_size - 1)
                                buf[len++] = str[i];
                        }
                        else
                            len++;
                    }
                }
            }
            str += subst->matches[0].rm_eo;
            max -= subst->matches[0].rm_eo;
        }
    }
    for (i = 0; i < max; ++i)
    {
        if (buf != NULL)
        {
            if (len < buf_size - 1)
                buf[len++] = str[i];
        }
        else
            len++;
    }

    if (buf != NULL)
        buf[len] = '\0';

    return len;
}

/* See the description in re_subst.h */
void
trc_re_substs_exec_start(const trc_re_substs *substs, const char *str,
                         FILE *f)
{
    trc_re_substs_exec(TAILQ_FIRST(substs), str, strlen(str), f);
}

/* See the description in re_subst.h */
ssize_t
trc_re_substs_exec_buf_start(const trc_re_substs *substs, const char *str,
                         char *buf, ssize_t buf_size)
{
    return trc_re_substs_exec_buf(TAILQ_FIRST(substs), str, strlen(str),
                                  buf, buf_size);
}

/* See the description in re_subst.h */
void
trc_re_key_substs(const char *name, const char *key, FILE *f)
{
    trc_re_namespace *keyn =
        trc_re_key_namespace_find(&key_namespaces, name);

    if (keyn != NULL)
        trc_re_substs_exec_start(&keyn->substs, key, f);
    else if (key != NULL)
        fprintf(f, "%s", key);
}

/* See the description in re_subst.h */
char *
trc_re_key_substs_buf(const char *name, const char *key)
{
    char                *buf = NULL;
    trc_re_namespace    *keyn =
        trc_re_key_namespace_find(&key_namespaces, name);
    const trc_re_substs *substs;
    ssize_t              buf_size;

    if (keyn == NULL)
    {
        /*
         * If no namespace with substitutions can be found, return
         * exact copy of key.
         */

        if (key != NULL)
            buf = strdup(key);

        return buf;
    }

    substs = &keyn->substs;
    buf_size = trc_re_substs_exec_buf_start(substs, key, NULL, 0);

    if (buf_size++ <= 0)
        return NULL;

    if ((buf = calloc(1, buf_size)) == NULL)
        return NULL;

    trc_re_substs_exec_buf_start(substs, key, buf, buf_size);

    return buf;
}

te_errno
trc_key_substs_read(const char *file)
{
    return trc_re_namespaces_read(file, &key_namespaces);
}

void
trc_key_substs_free(void)
{
    return trc_re_namespaces_free(&key_namespaces);
}

