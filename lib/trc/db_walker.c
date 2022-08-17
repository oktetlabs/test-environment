/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * TRC database walker API implementation.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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

#if 0
#undef TE_LOG_LEVEL
#define TE_LOG_LEVEL (TE_LL_WARN | TE_LL_ERROR | \
                      TE_LL_VERB | TE_LL_ENTRY_EXIT | TE_LL_RING)
#endif

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
te_trc_db_walker *
trc_db_walker_copy(const te_trc_db_walker *walker)
{
    te_trc_db_walker *new_walker;

    if (walker == NULL)
        return NULL;

    new_walker = TE_ALLOC(sizeof(*new_walker));
    if (new_walker == NULL)
        return NULL;

    memcpy(new_walker, walker, sizeof(*walker));

    return new_walker;
}

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

/* See the description in trc_db.h */
trc_users_data *
trc_db_walker_parent_users_data(const te_trc_db_walker *walker)
{
    return walker->is_iter ? &walker->iter->parent->users :
                                &walker->test->parent->users;
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
    for (k = 0; k < (int)n_args; k++)
        sorted[k] = k;

    MD5_Init(&md5);

    /* Sort arguments first */
    for (i = 0; i < (int)n_args - 1; i++)
    {
        for (j = i + 1; j < (int)n_args; j++)
        {
            if (strcmp(args[sorted[i]].name, args[sorted[j]].name) > 0)
            {
                k = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = k;
            }
        }
    }

    for (i = 0; i < (int)n_args; i++)
    {
        char *name = args[sorted[i]].name;
        char *value = trc_db_test_params_normalise(args[sorted[i]].value);

        if (value == NULL)
            return NULL;

        VERB("%s %s", name, value);
        len += snprintf(buf + len, sizeof(buf) - len,
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

/* See the description in trc_db.h */
void
trc_db_walker_go_to_test(te_trc_db_walker *walker, trc_test *test)
{
    walker->test = test;
    walker->iter = test->parent;
    walker->is_iter = FALSE;
    walker->unknown = 0;
    walker->motion = TRC_DB_WALKER_SON;
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

                if (walker->iter != NULL &&
                    walker->iter->filename != NULL)
                    walker->test->filename =
                        strdup(walker->iter->filename);
                else if (walker->db->filename != NULL)
                    walker->test->filename =
                        strdup(walker->db->filename);
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

#define VERB_CMP 0
#if VERB_CMP
#undef VERB
#define VERB RING
#endif

/* See the description in te_trc.h */
int
test_iter_args_match(const trc_test_iter_args  *db_args,
                     unsigned int               n_args,
                     trc_report_argument       *args,
                     te_bool                    is_strict)
{
    trc_test_iter_arg  *arg;
    unsigned int        i;
    te_bool             is_wildcard = FALSE;

#if VERB_CMP
    fprintf(stderr, "Args compare: \n");
#endif

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
            if (!is_strict)
            {
                i++;

                while (i < n_args)
                {
                    if (strcmp(args[i].name, arg->name) == 0)
                        break;
                    i++;
                }
            }

            if (is_strict || i == n_args)
            {
                VERB("Mismatch: %s vs %s", args[i].name, arg->name);
                return ITER_NO_MATCH;
            }
        }

        /* argument w/o a value */
        if (strlen(arg->value) == 0)
            is_wildcard = TRUE;
        else if (strncmp(args[i].value, TEST_ARG_VAR_PREFIX,
                         strlen(TEST_ARG_VAR_PREFIX)) != 0)
        {
            if (trc_db_compare_values(args[i].value, arg->value) != 0)
            {
                VERB("Value mismatch for %s: %s vs %s", arg->name,
                      args[i].value, arg->value);
                return ITER_NO_MATCH;
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
            {
                ERROR("In TRC DB there is no <global> corresponding to "
                      "'%s'. Iterations using this variable cannot be "
                      "matched to anything in database.",
                      args[i].value);
                return ITER_NO_MATCH;
            }

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
                return ITER_NO_MATCH;
            }
        }
        /* next arg */
        arg = TAILQ_NEXT(arg, links);
    }

    if (arg == NULL)
    {
        if (is_strict)
        {
            for (; i < n_args; i++)
            {
                if (!args[i].variable)
                {
                    VERB("Argument count mismatch: %d vs %d", i, n_args);
                    return ITER_NO_MATCH;
                }
            }
            assert(arg == NULL && i == n_args);
        }
    }
    else
        return ITER_NO_MATCH;

    return is_wildcard ? ITER_WILD_MATCH : ITER_EXACT_MATCH;
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
trc_db_walker_step_iter(te_trc_db_walker *walker, unsigned int n_args,
                        trc_report_argument *args,
                        uint32_t flags,
                        unsigned int db_uid,
                        func_args_match_ptr func_args_match)
{
    UNUSED(db_uid);

    assert(!walker->is_iter);

    if (walker->unknown > 0)
    {
        walker->unknown++;
        VERB("Step iteration - deep %u in unknown", walker->unknown);
    }
    else
    {
        int found = 0;
        int match_result = 0;

        trc_test_iter *iter = NULL;
        trc_test_iter *old_exact_iter = NULL;
        trc_test_iter *new_exact_iter = NULL;
        trc_test_iter *wild_iter = NULL;
        trc_test_iter *iter_to_copy = NULL;
        te_bool        dup_detected = FALSE;

        te_bool user_match = FALSE;
        const char *arg_names[n_args];
        unsigned int i;

        /*
         * Memorize initial order of arguments before sorting them
         * for TRC matching.
         */
        for (i = 0; i < n_args; i++)
            arg_names[i] = args[i].name;

        qsort(args, n_args, sizeof(*args), trc_report_argument_compare);
        for (walker->iter = TAILQ_FIRST(&walker->test->iters.head);
             walker->iter != NULL;
             walker->iter = TAILQ_NEXT(walker->iter, links))
        {
            if (func_args_match == NULL || walker->iter->log_found)
            {
                match_result = test_iter_args_match(&walker->iter->args,
                                                    n_args, args, TRUE);
            }
            else
            {
                match_result = func_args_match(walker->iter,
                                               n_args, args, FALSE);
                user_match = TRUE;
            }

            if (match_result != ITER_NO_MATCH)
            {
                iter = walker->iter;
                found++;

                if (match_result == ITER_WILD_MATCH)
                {
                    /*
                     * TRC Update tool does not create
                     * wildcards iterations to be matched
                     * during log processing - so no need
                     * in new_ or old_ prefix.
                     */
                    if (wild_iter != NULL ||
                        old_exact_iter != NULL)
                        dup_detected = TRUE;
                    wild_iter = walker->iter;
                }
                else if (iter->log_found)
                {
                    if (new_exact_iter != NULL)
                    {
                        dup_detected = TRUE;
                        ERROR("TRC Update generates duplicates!");
                    }
                    new_exact_iter = walker->iter;
                }
                else
                {
                    if (old_exact_iter != NULL ||
                        wild_iter != NULL)
                        dup_detected = TRUE;
                    old_exact_iter = walker->iter;
                }
            }
        }

        if ((flags & STEP_ITER_MATCH_FLAGS) == 0)
            walker->iter = iter;
        if (walker->iter == NULL &&
            !(flags & STEP_ITER_NO_MATCH_OLD))
            walker->iter = old_exact_iter;
        if (walker->iter == NULL &&
            !(flags & STEP_ITER_NO_MATCH_NEW))
            walker->iter = new_exact_iter;
        if (walker->iter == NULL &&
            !(flags &
              (STEP_ITER_NO_MATCH_WILD | STEP_ITER_NO_MATCH_OLD)))
            walker->iter = wild_iter;

        /* nothing found */
        if (walker->iter == NULL)
        {
            if (flags & STEP_ITER_CREATE_NFOUND)
            {
                /*
                 * iter_to_copy here is used by TRC Update.
                 * It needs full list of test iterations which
                 * TRC DB often does not contain because wildcard
                 * records are used there. So instead it obtains
                 * full list of iterations from fake run log and
                 * adds them to TRC DB copy stored in memory.
                 * It copies expected results from matching
                 * (possibly wildcard) records which were loaded
                 * from TRC XML files.
                 * An iteration is appended before iter_to_copy
                 * so that order of appended iterations matches
                 * order of existing records in TRC DB and TRC Update
                 * can try to minimize unnecessary rearrangements.
                 */

                if (wild_iter != NULL)
                    iter_to_copy = wild_iter;
                else if (old_exact_iter != NULL)
                    iter_to_copy = old_exact_iter;

                VERB("Step iteration - force to create");
                walker->iter = trc_db_new_test_iter(walker->test,
                                                    n_args, args,
                                                    iter_to_copy);
                if (walker->iter == NULL)
                {
                    ERROR("Cannot allocate a new test '%s' iteration",
                          walker->test->name);
                    return FALSE;
                }

                if (iter_to_copy != NULL && iter_to_copy->filename != NULL)
                    walker->iter->filename =
                                strdup(iter_to_copy->filename);
                else if (walker->test->filename != NULL)
                    walker->iter->filename =
                        strdup(walker->test->filename);
                else if (walker->db->filename != NULL)
                    walker->iter->filename =
                        strdup(walker->db->filename);

                if (iter_to_copy != NULL)
                {
                    trc_db_test_iter_res_cpy(walker->iter,
                                             iter_to_copy);
                }
                else
                {
                    if (flags & STEP_ITER_CREATE_UNSPEC)
                        walker->iter->exp_default =
                            exp_defaults_get(TE_TEST_UNSPEC);
                    else
                        walker->iter->exp_default =
                            exp_defaults_get(TE_TEST_PASSED);
                }

                if (iter_to_copy != NULL && !user_match)
                {
                    /*
                     * If possible, use arguments order from existing
                     * TRC record when saving updated TRC.
                     */
                    tq_strings_copy(&walker->iter->args.save_order,
                                    &iter_to_copy->args.save_order);
                }
                else
                {
                    for (i = 0; i < n_args; i++)
                    {
                        tq_strings_add_uniq_dup(
                                    &walker->iter->args.save_order,
                                    arg_names[i]);
                    }
                }
            }
            else
            {
                VERB("Step iteration - unknown");
                walker->unknown++;
            }
        }

        if (dup_detected)
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
            VERB("Step iteration - OK", walker->unknown);
    }

    walker->is_iter = TRUE;

    if ((flags & STEP_ITER_SPLIT_RESULTS) && (walker->unknown == 0))
        trc_db_test_iter_res_split(walker->iter);

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

    return trc_db_iter_get_exp_result(walker->iter, tags, walker->db->last_match);
}
