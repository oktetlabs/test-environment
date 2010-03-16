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
trc_re_substs key_substs = TAILQ_HEAD_INITIALIZER(key_substs);


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
trc_re_substs_read(const char *file, trc_re_substs *substs)
{
    te_errno        rc = 0;
    FILE           *f;
    char            buf[256];
    char           *s;
    trc_re_subst   *p;

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
        TAILQ_INSERT_TAIL(substs, p, links);

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

/* See the description in re_subst.h */
void
trc_re_substs_exec_start(const trc_re_substs *substs, const char *str,
                         FILE *f)
{
    trc_re_substs_exec(TAILQ_FIRST(substs), str, strlen(str), f);
}

/* See the description in re_subst.h */
void
trc_re_key_substs(const char *key, FILE *f)
{
    trc_re_substs_exec_start(&key_substs, key, f);
}
