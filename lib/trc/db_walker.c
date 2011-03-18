/** @file
 * @brief Testing Results Comparator
 *
 * TRC database walker API implementation.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TRC DB walker"

#include "te_config.h"

#include <ctype.h>
#include <search.h>
#include <openssl/md5.h>

#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"

#include "te_trc.h"
#include "trc_db.h"

/** Internal data of the TRC database walker */
struct te_trc_db_walker {
    te_trc_db           *db;        /**< TRC database pointer */
    te_bool              is_iter;   /**< Is current position an
                                         iteration? */
    trc_test            *test;      /**< Test entry */
    trc_test_iter       *iter;      /**< Test iteration */
    unsigned int         unknown;   /**< Unknown depth counter */
    trc_db_walker_motion motion;    /**< The last motion */
};

/* See the description in te_trc.h */
te_bool
trc_db_walker_is_iter(const te_trc_db_walker *walker)
{
    return walker->is_iter;
}

/* See the description in trc_db.h */
trc_test *
trc_db_walker_get_test(const te_trc_db_walker *walker)
{
    return walker->is_iter ? walker->iter->parent : walker->test;
}

/* See the description in trc_db.h */
trc_test_iter *
trc_db_walker_get_iter(const te_trc_db_walker *walker)
{
    return walker->is_iter ? walker->iter : walker->test->parent;
}

/* See the description in trc_db.h */
trc_users_data *
trc_db_walker_users_data(const te_trc_db_walker *walker)
{
    return walker->is_iter ? &walker->iter->users : &walker->test->users;
}

/* See the description in te_trc.h */
void
trc_db_free_walker(te_trc_db_walker *walker)
{
    free(walker);
}

/* See the description in te_trc.h */
te_trc_db_walker *
trc_db_new_walker(te_trc_db *trc_db)
{
    te_trc_db_walker   *walker;

    walker = TE_ALLOC(sizeof(*walker));
    if (walker == NULL)
        return NULL;

    walker->db = trc_db;
    walker->is_iter = TRUE;
    walker->test = NULL;
    walker->iter = NULL;
    walker->motion = TRC_DB_WALKER_ROOT;

    INFO("A new TRC DB walker allocated - 0x%p", walker);

    return walker;
}

/**
 * Normalise parameter value. Remove trailing spaces and newlines.
 *
 * @param param   Parameter value to normalise
 *
 * @return Allocated string or NULL.
 */
static char *
trc_db_test_params_normalise(char *param)
{
    char *p;
    char *q;
    char *str = NULL;
    te_bool skip_spaces = TRUE;

    if (param == NULL)
        return NULL;

    str = (char *)calloc(1, strlen(param) + 1);
    if (str == NULL)
        return NULL;

    for (p = param, q = str; *p != '\0'; p++)
    {
        if (isspace(*p))
        {
            if (!skip_spaces)
            {
                *q++ = ' ';
                skip_spaces = TRUE;
            }
        }
        else
        {
            *q++ = *p;
            skip_spaces = FALSE;
        }
    }

    /* remove trainling space at the end, if any */
    if ((q > str) && (skip_spaces))
        q--;

    *q = '\0';

    return str;
}

/**
 * Calculate hash (MD5 checksum) for set of test arguments.
 *
 * @param args   Array of test parameters
 * @param n_args Number of of test parameters
 *
 * @return Allocated string or NULL.
 */
static char *
trc_db_test_params_hash(unsigned int n_args, trc_report_argument *args)
{
    MD5_CTX md5;

    int   i;
    int   j;
    int   k;
    unsigned char digest[MD5_DIGEST_LENGTH];
    char *hash_str = calloc(1, MD5_DIGEST_LENGTH * 2 + 1);
    int  *sorted = calloc(n_args, sizeof(int));
    char  buf[8192] = {0, };
    int   len = 0;

    if (sorted == NULL)
        return NULL;
    for (k = 0; k < n_args; k++)
        sorted[k] = k;

    MD5_Init(&md5);

    /* Sort arguments first */
    for (i = 0; i < n_args - 1; i++)
    {
        for (j = i + 1; j < n_args; j++)
        {
            if (strcmp(args[sorted[i]].name, args[sorted[j]].name) > 0)
            {
                k = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = k;
            }
        }
    }

    for (i = 0; i < n_args; i++)
    {
        char *name = args[sorted[i]].name;
        char *value = trc_db_test_params_normalise(args[sorted[i]].value);

        if (value == NULL)
            return NULL;

        VERB("%s %s", name, value);
        len += snprintf(buf + len, sizeof(buf) - len - 1,
                        "%s%s %s", (i != 0) ? " " : "", name, value);

        if (i != 0)
            MD5_Update(&md5, " ", (unsigned long) 1);
        MD5_Update(&md5, name, (unsigned long) strlen(name));
        MD5_Update(&md5, " ", (unsigned long) 1);
        MD5_Update(&md5, value, (unsigned long) strlen(value));

        free(value);
    }

    MD5_Final(digest, &md5);

    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(hash_str + strlen(hash_str), "%02hhx", digest[i]);
    }

    VERB("\nHash: %s\n", hash_str);
    VERB("%s->%s", buf, hash_str);

    return hash_str;
}

/* See the description in te_trc.h */
te_bool
trc_db_walker_step_test(te_trc_db_walker *walker, const char *test_name,
                        te_bool force)
{
    assert(walker->is_iter);

    ENTRY("test_name = '%s'", test_name);

    if (walker->unknown > 0)
    {
        walker->unknown++;
        VERB("Step test '%s' - deep %u in unknown",
             test_name, walker->unknown);
    }
    else
    {
        trc_tests *tests = (walker->iter == NULL) ? &walker->db->tests :
                                                    &walker->iter->tests;

        for (walker->test = TAILQ_FIRST(&tests->head);
             walker->test != NULL &&
             strcmp(walker->test->name, test_name) != 0;
             walker->test = TAILQ_NEXT(walker->test, links));

        if (walker->test == NULL)
        {
            if (force)
            {
                INFO("Step test '%s' - force to create", test_name);
                walker->test = trc_db_new_test(tests, walker->iter,
                                               test_name);
                if (walker->test == NULL)
                {
                    ERROR("Cannot allocate a new test '%s'", test_name);
                    return FALSE;
                }
            }
            else
            {
                ERROR("Step test '%s' - unknown", test_name);
                walker->unknown++;
            }
        }
        else
        {
            VERB("Step test '%s' - OK", test_name);
        }
    }

    walker->is_iter = FALSE;

    return (walker->unknown == 0);
}

int (*trc_db_compare_values)(const char *s1, const char *s2) =
    trc_db_strcmp_normspace;

static unsigned
next_token(const char *pos, const char **start, te_bool *is_numeric)
{
    unsigned len = 0;
    
    *is_numeric = FALSE;
    while(isspace(*pos))
        pos++;
    *start = pos;
    if (*pos != '\0')
    {
        while(isalnum(*pos) || *pos == '-' || *pos == '_' || *pos == '.')
        {
            pos++;
            len++;
        }
        if (len == 0)
            len = 1;
        else
        {
            char *tmp;
            strtol(*start, &tmp, 0); // scanning only
            if (tmp == pos)
                *is_numeric = TRUE;
        }
    }
    return len;
}

int
trc_db_strcmp_tokens(const char *s1, const char *s2)
{
    unsigned wlen1 = 0;
    te_bool numeric1 = FALSE;
    unsigned wlen2 = 0;
    te_bool numeric2 = FALSE;
    int rc;
    
    do
    {
        wlen1 = next_token(s1, &s1, &numeric1);
        wlen2 = next_token(s2, &s2, &numeric2);
        if (numeric1 && numeric2)
        {
            rc = strtol(s1, NULL, 0) - strtol(s2, NULL, 0);
            if (rc != 0)
                return rc;
        }
        if (wlen1 < wlen2)
        {
            return memcmp(s1, s2, wlen1) > 0 ? 1 : -1;
        }
        else if (wlen1 > wlen2)
        {
            return memcmp(s1, s2, wlen2) >= 0 ? 1 : -1;
        }
        else
        {
            rc = memcmp(s1, s2, wlen1);
            if (rc != 0)
                return rc;
        }
        s1 += wlen1;
        s2 += wlen2;
    } while (wlen1 != 0 && wlen2 != 0);
    return 0;
}

int
trc_db_strcmp_normspace(const char *s1, const char *s2)
{
    while (isspace(*s1))
        s1++;
    while (isspace(*s2))
        s2++;
    
    while (*s1 != '\0' && *s2 != '\0')
    {
        if (isspace(*s1))
        {
            if (!isspace(*s2))
                return (int)' ' - (int)*s2;
            while (isspace(*s1))
                s1++;
            while (isspace(*s2))
                s2++;
        }
        else
        {
            if (*s1 != *s2)
                return (int)*s1 - (int)*s2;
            s1++;
            s2++;
        }
    }
    while (isspace(*s1))
        s1++;
    while (isspace(*s2))
        s2++;

    return (int)*s1 - (int)*s2;
}

/**
 * Match TRC database arguments vs arguments specified by caller.
 *
 * @param db_args       List with TRC database arguments
 * @parma n_args        Number of elements in @a names and @a values
 *                      arrays
 * @param names         Array with names of arguments
 * @param values        Array with values of arguments
 *
 * @return Is arguments match?
 */
#define VERB_CMP 0
#if VERB_CMP
#undef VERB
#define VERB RING
#endif

static te_bool
test_iter_args_match(const trc_test_iter_args  *db_args,
                     unsigned int               n_args,
                     trc_report_argument       *args)
{
    trc_test_iter_arg  *arg;
    unsigned int        i;

#if VERB_CMP
    fprintf(stderr, "Args compare: \n");
#endif
\
    for (arg = TAILQ_FIRST(&db_args->head), i = 0;
         arg != NULL && i < n_args;
         i++)
    {
        /* Skip variables */
        if (args[i].variable)
            continue;

#if VERB_CMP
        fprintf(stderr, "   %s=%s VS %s=%s\n",
                arg->name, arg->value,
                args[i].name, args[i].value);
#endif

        if (strcmp(args[i].name, arg->name) != 0)
        {
            VERB("Mismatch: %s vs %s", args[i].name, arg->name);
            return FALSE;
        }
        /* argument w/o a value */
        else if (strlen(arg->value) == 0);
        else if (strncmp(args[i].value, TEST_ARG_VAR_PREFIX,
                         strlen(TEST_ARG_VAR_PREFIX)) != 0)
        {
            if (trc_db_compare_values(args[i].value, arg->value) != 0)
            {
                VERB("Value mismatch for %s: %s vs %s", arg->name, 
                      args[i].value, arg->value);
                return FALSE;
            }
        }
        else
        {
            trc_global *g;

            TAILQ_FOREACH(g, &current_db->globals.head, links)
            {
                if (strcmp(g->name, args[i].value +
                           strlen(TEST_ARG_VAR_PREFIX)) == 0)
                {
                    VERB("Value is a var, var=(%s, %s)",
                         g->name, g->value);
                    break;
                }
            }
            if (g == NULL)
                return FALSE;

            /*
             * we found a variable, but in TRC it can be either var name or
             * it's value
             */
            if (strcmp(args[i].value, arg->value) != 0 &&
                trc_db_compare_values(g->value, arg->value) != 0)
            {
                VERB("Value mismatch for %s: %s vs %s AND %s vs %s",
                     arg->name, g->value, arg->value, arg->value,
                     args[i].value);
                return FALSE;
            }
        }
        /* next arg */
        arg = TAILQ_NEXT(arg, links);
    }
    if (arg == NULL)
    {
        for (; i < n_args; i++)
        {
            if (!args[i].variable)
            {
                VERB("Argument count mismatch: %d vs %d", i, n_args);
                return FALSE;
            }
        }
        assert(arg == NULL && i == n_args);
    }
    else
        return FALSE;

    return TRUE;
}
#undef VERB_CMP

static int
trc_report_argument_compare (const void *arg1, const void *arg2)
{
    return strcmp(((trc_report_argument *)arg1)->name,
                  ((trc_report_argument *)arg2)->name);
}


/* See the description in te_trc.h */
te_bool
trc_db_walker_step_iter(te_trc_db_walker    *walker,
                        unsigned int         n_args,
                        trc_report_argument *args,
                        te_bool              force)
{
    assert(!walker->is_iter);

    if (walker->unknown > 0)
    {
        walker->unknown++;
        VERB("Step iteration - deep %u in unknown", walker->unknown);
    }
    else
    {
        int found = 0;
        trc_test_iter *iter = NULL;

        qsort(args, n_args, sizeof(*args), trc_report_argument_compare);
        for (walker->iter = TAILQ_FIRST(&walker->test->iters.head);
             walker->iter != NULL;
             walker->iter = TAILQ_NEXT(walker->iter, links))
        {
            if (test_iter_args_match(&walker->iter->args,
                                     n_args, args))
            {
                iter = walker->iter;
                found++;
            }
        }
        walker->iter = iter;

        /* nothing found */
        if (walker->iter == NULL)
        {
            if (force)
            {
                unsigned i;
                VERB("Step iteration - force to create");
                walker->iter = trc_db_new_test_iter(walker->test,
                                                    n_args, args);
                if (walker->iter == NULL)
                {
                    ERROR("Cannot allocate a new test '%s' iteration",
                          walker->test->name);
                    return FALSE;
                }
            }
            else
            {
                VERB("Step iteration - unknown");
                walker->unknown++;
            }
        }
        else if (found > 1)
        {
            char *hash = trc_db_test_params_hash(n_args, args);
            ERROR("TEST='%s || %s'\n"
                  "Hash: %s\n"
                  "Duplicated iteration in the database! "
                  "May be caused by wrong wildcards. "
                  "Will match the last entry for compatibility, but FIX "
                  "THE DATABASE!!!",
                  walker->test->name, walker->test->path, hash);
            free(hash);
        }
        else
        {
            VERB("Step iteration - OK");
        }
    }

    walker->is_iter = TRUE;

    return (walker->unknown == 0);
}

/* See the description in te_trc.h */
void
trc_db_walker_step_back(te_trc_db_walker *walker)
{
    if (walker->unknown > 0)
    {
        walker->unknown--;
        walker->is_iter = !walker->is_iter;
        VERB("Step back from unknown -> %u", walker->unknown);
    }
    else if (walker->is_iter)
    {
        assert(walker->iter != NULL);
        walker->test = walker->iter->parent;
        walker->is_iter = FALSE;
        VERB("Step back from iteration");
    }
    else
    {
        assert(walker->test != NULL);
        walker->iter = walker->test->parent;
        walker->is_iter = TRUE;
        VERB("Step back from test");
    }
}


/* See the description in te_trc.h */
trc_db_walker_motion
trc_db_walker_move(te_trc_db_walker *walker)
{
    switch (walker->motion)
    {
        case TRC_DB_WALKER_ROOT:
            walker->test = TAILQ_FIRST(&walker->db->tests.head);
            if (walker->test == NULL)
            {
                return TRC_DB_WALKER_ROOT;
            }
            else
            {
                walker->is_iter = FALSE;
                return (walker->motion = TRC_DB_WALKER_SON);
            }
            break;

        case TRC_DB_WALKER_SON:
        case TRC_DB_WALKER_BROTHER:
            if (walker->is_iter)
            {
                walker->test = TAILQ_FIRST(&walker->iter->tests.head);
                if (walker->test != NULL)
                {
                    walker->is_iter = FALSE;
                    return (walker->motion = TRC_DB_WALKER_SON);
                }
            }
            else
            {
                walker->iter = TAILQ_FIRST(&walker->test->iters.head);
                if (walker->iter != NULL)
                {
                    walker->is_iter = TRUE;
                    return (walker->motion = TRC_DB_WALKER_SON);
                }
            }
            /*@fallthrough@*/

        case TRC_DB_WALKER_FATHER:
            if (walker->is_iter)
            {
                if (TAILQ_NEXT(walker->iter, links) != NULL)
                {
                    walker->iter = TAILQ_NEXT(walker->iter, links);
                    return (walker->motion = TRC_DB_WALKER_BROTHER);
                }
                else
                {
                    walker->test = walker->iter->parent;
                    assert(walker->test != NULL);
                    walker->is_iter = FALSE;
                    return (walker->motion = TRC_DB_WALKER_FATHER);
                }
            }
            else
            {
                if (TAILQ_NEXT(walker->test, links) != NULL)
                {
                    walker->test = TAILQ_NEXT(walker->test, links);
                    return (walker->motion = TRC_DB_WALKER_BROTHER);
                }
                else
                {
                    walker->is_iter = TRUE;
                    return (walker->motion =
                        ((walker->iter = walker->test->parent) == NULL) ?
                            TRC_DB_WALKER_ROOT : TRC_DB_WALKER_FATHER);
                }
            }
            break;

        default:
            assert(FALSE);
            return (walker->motion = TRC_DB_WALKER_ROOT);
    }
    /* Unreachable */
    assert(FALSE);
}


/* See the description in te_trc.h */
const trc_exp_result *
trc_db_walker_get_exp_result(const te_trc_db_walker *walker,
                             const tqh_strings      *tags)
{
    const trc_exp_result       *result;
    int                         prio;
    const trc_exp_result       *p;
    int                         res;
    const trc_exp_result_entry *q;

    assert(walker->is_iter);

    if (walker->unknown > 0)
    {
        /*
         * Here an ERROR was logged, but as TRC is often out of date -
         * tonns of ERRORs in the log are observed
         */
        VERB("Iteration is not known");
        /* Test iteration is unknown. No expected result. */
        return NULL;
    }

    /* Do we have a tag with expected SKIPPED result? */
    result = NULL; prio = 0;
    SLIST_FOREACH(p, &walker->iter->exp_results, links)
    {
        res = logic_expr_match(p->tags_expr, tags);
        if (res != 0)
        {
            INFO("Matching tag found");
            TAILQ_FOREACH(q, &p->results, links)
            {
                if (q->result.status == TE_TEST_SKIPPED)
                {
                    /* Skipped results have top priority in any case */
                    result = p;
                    prio = res;
                    break;
                }
            }
            if (q != NULL)
                break;

            if (result == NULL || res < prio)
            {
                result = p;
                prio = res;
            }
        }
    }

    /* We have not found matching tagged result */
    if (result == NULL)
    {
        /* May be default expected result exists? */
        result = walker->iter->exp_default;
    }

    if (result == NULL)
    {
        INFO("Expected result is not known");
    }

    return result;
}

