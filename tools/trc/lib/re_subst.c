/** @file
 * @brief Testing Results Comparator
 *
 * Helper functions to make regular expression substitutions
 *
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

    regfree(&subst->re);
    free(subst->str);

    while ((m = TAILQ_FIRST(&subst->with)) != NULL)
    {
        TAILQ_REMOVE(&subst->with, m, links);
        free(m);
    }

    free(subst->matches);
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
        WARN("Namespace '%s' already exist", name);
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

/* See the description in re_subst.h */
te_errno
trc_re_namespaces_read(const char *file, trc_re_namespaces *namespaces)
{
    char            buf[256];
    FILE             *f = NULL;
    char             *s = NULL;
    trc_re_subst     *p = NULL;
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

        p = malloc(sizeof(*p));
        if (p == NULL)
        {
            rc = TE_ENOMEM;
            break;
        }

        if (regcomp(&p->re, buf, REG_EXTENDED) != 0)
        {
            ERROR("Failed to compile regular expression '%s'\n", buf);
            rc = TE_EFAULT;
            free(p);
            break;
        }

        p->max_match = 0;
        p->str = strdup(s);
        TAILQ_INIT(&p->with);
        p->matches = NULL;
        TAILQ_INSERT_TAIL(&namespace->substs, p, links);

        if (p->str == NULL)
        {
            rc = TE_ENOMEM;
            break;
        }

        rc = trc_re_subst_parse(p);
        if (rc != 0)
            break;

        p->max_match++;
        p->matches = malloc(p->max_match * sizeof(*p->matches));
        if (p->matches == NULL)
        {
            rc = TE_ENOMEM;
            break;
        }
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
        ERROR("%s(): Namespace '%s' not found", __FUNCTION__, name);
        return NULL;
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
trc_key_substs_free()
{
    return trc_re_namespaces_free(&key_namespaces);
}

