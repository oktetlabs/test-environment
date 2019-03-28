/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Auxiluary routines for TRC update tool.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TRC Update Tool"

#include "te_config.h"
#include "trc_config.h"

#if HAVE_STRING_H
#include <string.h>
#endif

#include <libxml/tree.h>

#include "te_errno.h"
#include "te_alloc.h"
#include "te_queue.h"
#include "logger_api.h"
#include "te_test_result.h"
#include "te_trc.h"
#include "te_string.h"
#include "trc_tags.h"
#include "log_parse.h"
#include "trc_diff.h"
#include "trc_db.h"
#include "gen_wilds.h"

#include "trc_update.h"
#include "trc_report.h"

#include <limits.h>

#if !defined(PATH_MAX)
#define PATH_MAX 1024
#endif

/** ID of tags in night logs report in binary form */
typedef enum logs_dump_tag_ids {
    LOGS_DUMP_TAG_UNDEF = 0,    /**< Undefined tag */
    LOGS_DUMP_TAG_TEST = 1,     /**< Test description */
    LOGS_DUMP_TAG_ITER,         /**< Iteration description */
    LOGS_DUMP_TAG_PARAMS,       /**< Iteration parameters */
    LOGS_DUMP_TAG_RESULTS,      /**< Iteration results */
    LOGS_DUMP_TAG_ERR,          /**< Error message */
    LOGS_DUMP_TAG_STATUS,       /**< Result status */
    LOGS_DUMP_TAG_VERDICTS,     /**< Result verdicts */
    LOGS_DUMP_TAG_LOGS,         /**< Logs in which a given result
                                     was encountered */
} logs_dump_tag_ids;

/**
 * This macro is used in comparing function only.
 * Check whether one or both of pointers are NULL;
 * if at least one of them is NULL - return result
 * of comparison.
 *
 * @param p_    The first pointer -  argument of
 *              a comparing function
 * @param q_    The second pointer - argument of
 *              a comparing function
 */
#define DUMMY_CMP(p_, q_) \
do {                                \
    if (p_ == NULL && q_ == NULL)   \
        return 0;                   \
    else if (p_ == NULL)            \
        return -1;                  \
    else if (q_ == NULL)            \
        return 1;                   \
} while (0)
 
/* See the description in trc_update.h */
void
trc_update_init_test_iter_data(trc_update_test_iter_data *data)
{
    memset(data, 0, sizeof(*data));
    STAILQ_INIT(&data->new_results);
    STAILQ_INIT(&data->df_results);
    SLIST_INIT(&data->all_wilds);
    data->rule = NULL;
    data->args = NULL;
    data->set_nums = NULL;
}

/* See the description in trc_update.h */
void
trc_update_free_test_iter_data(trc_update_test_iter_data *data)
{
    trc_exp_result             *q;
    trc_exp_result             *q_tvar;
    trc_update_args_group      *args_group;
    trc_update_args_group      *args_group_tvar;

    unsigned int i;

    if (data == NULL)
        return;

    STAILQ_FOREACH_SAFE(q, &data->new_results, links, q_tvar)
    {
        STAILQ_REMOVE_HEAD(&data->new_results, links);
        trc_exp_result_free(q);
    }

    STAILQ_FOREACH_SAFE(q, &data->df_results, links, q_tvar)
    {
        STAILQ_REMOVE_HEAD(&data->df_results, links);
        trc_exp_result_free(q);
    }

    SLIST_FOREACH_SAFE(args_group, &data->all_wilds,
                       links, args_group_tvar)
    {
        SLIST_REMOVE_HEAD(&data->all_wilds, links);
        trc_update_args_group_free(args_group);
    }

    for (i = 0; i < data->args_n; i++)
    {
        free(data->args[i].name);
        free(data->args[i].value);
    }
    free(data->args);
    free(data->set_nums);

    free(data);
}

/* See the description in trc_update.h */
void
trc_update_wilds_list_entry_free(trc_update_wilds_list_entry *entry)
{
    if (entry == NULL)
        return;

    trc_free_test_iter_args(entry->args);
}

/* See the description in trc_update.h */
void
trc_update_wilds_list_free(trc_update_wilds_list *list)
{
    trc_update_wilds_list_entry *entry;
    trc_update_wilds_list_entry *entry_aux;

    if (list == NULL)
        return;

    SLIST_FOREACH_SAFE(entry, list, links, entry_aux)
    {
        trc_update_wilds_list_entry_free(entry);
        free(entry);
    }
}

/* See the description in trc_update.h */
void
trc_update_rule_free(trc_update_rule *rule)
{
    if (rule == NULL)
        return;

    trc_exp_result_free(rule->def_res);
    trc_exp_results_free(rule->old_res);
    trc_exp_result_entry_free(rule->old_re);
    free(rule->old_v);
    trc_exp_results_free(rule->new_res);
    trc_exp_result_entry_free(rule->new_re);
    free(rule->new_v);
    trc_exp_results_free(rule->confl_res);
    trc_update_wilds_list_free(rule->wilds);
    if (rule->match_exprs != NULL)
        tq_strings_free(rule->match_exprs, free);
}

/* See the description in trc_update.h */
void
trc_update_rules_free(trc_update_rules *rules)
{
    trc_update_rule *p;

    if (rules == NULL)
        return;

    while ((p = TAILQ_FIRST(rules)) != NULL)
    {
        TAILQ_REMOVE(rules, p, links);
        trc_update_rule_free(p);
        free(p);
    }
}

/* See the description in trc_update.h */
void
trc_update_test_entry_free(trc_update_test_entry *test_entry)
{
    int i;

    if (test_entry == NULL)
        return;

    for (i = 0; i < test_entry->sets_cnt; i++)
        trc_update_args_groups_free(&test_entry->sets[i]);

    free(test_entry->sets);
}

/* See the description in trc_update.h */
void
trc_update_test_entries_free(trc_update_test_entries *tests)
{
    trc_update_test_entry *p;

    if (tests == NULL)
        return;

    while ((p = TAILQ_FIRST(tests)) != NULL)
    {
        TAILQ_REMOVE(tests, p, links);
        trc_update_test_entry_free(p);
        free(p);
    }
}

/* See the description in trc_update.h */
void
trc_update_tests_group_free(trc_update_tests_group *group)
{
    if (group == NULL)
        return;

    trc_update_rules_free(group->rules);
    free(group->path);
    trc_update_test_entries_free(&group->tests);
}

/* See the description in trc_update.h */
void
trc_update_tests_groups_free(trc_update_tests_groups *groups)
{
    trc_update_tests_group *p;

    if (groups == NULL)
        return;

    while ((p = TAILQ_FIRST(groups)) != NULL)
    {
        TAILQ_REMOVE(groups, p, links);
        trc_update_tests_group_free(p);
        free(p);
    }
}

/* See the description in trc_update.h */
void
trc_update_args_group_free(trc_update_args_group *args_group)
{
    trc_free_test_iter_args(args_group->args);
    trc_exp_results_free(args_group->exp_results);
    trc_exp_result_free(args_group->exp_default);
}

/* See the description in trc_update.h */
void
trc_update_args_groups_free(trc_update_args_groups *args_groups)
{
    trc_update_args_group   *args_group;

    while ((args_group = SLIST_FIRST(args_groups)) != NULL)
    {
        SLIST_REMOVE_HEAD(args_groups, links);
        trc_update_args_group_free(args_group);
        free(args_group);
    }
}

/* See the description in trc_update.h */
trc_test_iter_args *
trc_update_args_wild_dup(trc_test_iter_args *args)
{
    trc_test_iter_args  *dup;
    trc_test_iter_arg   *arg;
    trc_test_iter_arg   *arg_dup;

    dup = TE_ALLOC(sizeof(*dup));
    
    TAILQ_INIT(&dup->head);

    TAILQ_FOREACH(arg, &args->head, links)
    {
        arg_dup = TE_ALLOC(sizeof(*arg_dup));
        if (arg->name != NULL)
            arg_dup->name = strdup(arg->name);
        arg_dup->value = TE_ALLOC(1);
        arg_dup->value[0] = '\0';
        TAILQ_INSERT_TAIL(&dup->head, arg_dup, links);
    }

    return dup;
}

/* See the description in trc_update.h */
int
te_test_result_cmp(te_test_result *p, te_test_result *q)
{
    int              rc;
    te_test_verdict *p_v;
    te_test_verdict *q_v;

    DUMMY_CMP(p, q);

    rc = p->status - q->status;
    if (rc != 0)
        return rc;
    else
    {
        p_v = TAILQ_FIRST(&p->verdicts); 
        q_v = TAILQ_FIRST(&q->verdicts);
        
        while (p_v != NULL && q_v != NULL)
        {
            rc = strcmp(p_v->str, q_v->str);
            if (rc != 0)
                return rc;

            p_v = TAILQ_NEXT(p_v, links);
            q_v = TAILQ_NEXT(q_v, links);
        }

        if (p_v == NULL && q_v == NULL)
            return 0;
        else if (p_v != NULL)
            return 1;
        else
            return -1;
    }
}


/* See the description in trc_update.h */
te_bool
trc_update_is_to_save(void *data, te_bool is_iter)
{
    if (data == NULL)
        return FALSE;

    if (is_iter)
    {
        trc_update_test_iter_data *iter_d =
            (trc_update_test_iter_data *)data;

        return iter_d->to_save;
    }
    else
    {
        trc_update_test_data *test_d =
            (trc_update_test_data *)data;
    
        return test_d->to_save;
    }
}

/* See the description in trc_update.h */
char *
trc_update_set_user_attr(void *data, te_bool is_iter)
{
    char *attr_value;
    int   val_len = 20;

    if (data == NULL)
        return FALSE;

    if (is_iter)
    {
        trc_update_test_iter_data *iter_d =
            (trc_update_test_iter_data *)data;

        if (iter_d->rule_id == 0)
            return NULL;

        attr_value = TE_ALLOC(val_len);
        snprintf(attr_value, val_len, "rule_%d", iter_d->rule_id);

        return attr_value;
    }
    else
        return NULL;
}

/* See the description in trc_update.h */
void
trc_update_init_ctx(trc_update_ctx *ctx_p)
{
    memset(ctx_p, 0, sizeof(*ctx_p));
    TAILQ_INIT(&ctx_p->test_names);
    TAILQ_INIT(&ctx_p->tags_logs);
    TAILQ_INIT(&ctx_p->tags_gen_list);
    TAILQ_INIT(&ctx_p->tags);
    TAILQ_INIT(&ctx_p->collected_tags);
    TAILQ_INIT(&ctx_p->global_rules);
    TAILQ_INIT(&ctx_p->updated_tests);
    ctx_p->merge_expr = NULL;
    ctx_p->merge_str = NULL;
}

/* See the description in trc_update.h */
void
trc_update_free_ctx(trc_update_ctx *ctx)
{
    if (ctx == NULL)
        return;

    trc_update_tags_logs_free(&ctx->tags_logs);
    tq_strings_free(&ctx->tags_gen_list, free);
    tq_strings_free(&ctx->tags, free);
    tq_strings_free(&ctx->collected_tags, free);
    tq_strings_free(&ctx->test_names, free);
    free(ctx->fake_log);
    free(ctx->fake_filt_log);
    free(ctx->rules_load_from);
    free(ctx->rules_save_to);
    free(ctx->cmd);
    free(ctx->tags_gather_to);
    free(ctx->logs_dump);
    trc_update_tests_groups_free(&ctx->updated_tests);
    trc_update_rules_free(&ctx->global_rules);
}

/* See the description in trc_update.h */
void
tag_logs_init(trc_update_tag_logs *tag_logs)
{
    memset(tag_logs, 0, sizeof(*tag_logs));
    TAILQ_INIT(&tag_logs->logs);
}

/* See the description in trc_update.h */
void
trc_update_tag_logs_free(trc_update_tag_logs *tag_logs)
{
    if (tag_logs == NULL)
        return;

    free(tag_logs->tags_str);
    logic_expr_free(tag_logs->tags_expr);
    tq_strings_free(&tag_logs->logs, free);
}

/* See the description in trc_update.h */
void
trc_update_tags_logs_free(trc_update_tags_logs *tags_logs)
{
    trc_update_tag_logs     *tl = NULL;

    if (tags_logs == NULL)
        return;

    while ((tl = TAILQ_FIRST(tags_logs)) != NULL)
    {
        TAILQ_REMOVE(tags_logs, tl, links);
        trc_update_tag_logs_free(tl);
        free(tl);
    }
}

/* See the description in trc_update.h */
void
trc_update_tags_logs_remove_empty(trc_update_tags_logs *tags_logs)
{
    trc_update_tag_logs     *tl = NULL;
    trc_update_tag_logs     *tl_tmp = NULL;

    if (tags_logs == NULL)
        return;

    TAILQ_FOREACH_SAFE(tl, tags_logs, links, tl_tmp)
    {
        if (TAILQ_EMPTY(&tl->logs))
        {
            TAILQ_REMOVE(tags_logs, tl, links);
            trc_update_tag_logs_free(tl);
            free(tl);
        }
    }
}

/* See the description in trc_update.h */
int
trc_update_rentry_cmp(trc_exp_result_entry *p,
                      trc_exp_result_entry *q)
{
    int rc = 0;

    DUMMY_CMP(p, q);

    if (!p->is_expected && q->is_expected)
        return -1;
    else if (p->is_expected && !q->is_expected)
        return 1;

    rc = te_test_result_cmp(&p->result,
                            &q->result);
    if (rc != 0)
        return rc;
    else
    {
        rc = strcmp_null(p->key, q->key);
        if (rc != 0)
            return rc;
        else
        {
            rc = strcmp_null(p->notes, q->notes);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/* See the description in trc_update.h */
int
trc_update_result_cmp_gen(trc_exp_result *p, trc_exp_result *q,
                          te_bool tags_cmp)
{
    int                      rc;
    trc_exp_result_entry    *p_r;
    trc_exp_result_entry    *q_r;
    te_bool                  p_tags_str_void;
    te_bool                  q_tags_str_void;

    DUMMY_CMP(p, q);
   
    if (!tags_cmp)
        rc = 0;
    else
    {
        p_tags_str_void = (p->tags_str == NULL ||
                           strlen(p->tags_str) == 0);
        q_tags_str_void = (q->tags_str == NULL ||
                           strlen(q->tags_str) == 0);

        if (p_tags_str_void && q_tags_str_void)
            rc = 0;
        else if (p_tags_str_void)
            rc = -1;
        else if (q_tags_str_void)
            rc = 1;
        else
            rc = strcmp(p->tags_str, q->tags_str);
    }

    if (rc != 0)
        return rc;
    else
    {
        p_r = TAILQ_FIRST(&p->results);
        q_r = TAILQ_FIRST(&q->results);

        while (p_r != NULL && q_r != NULL)
        {
            rc = trc_update_rentry_cmp(p_r, q_r);
            if (rc != 0)
                return rc;

            p_r = TAILQ_NEXT(p_r, links);
            q_r = TAILQ_NEXT(q_r, links);
        }

        if (p_r == NULL && q_r == NULL)
        {
            rc = strcmp_null(p->key, q->key);
            if (rc != 0)
                return rc;
            else
                return strcmp_null(p->notes, q->notes);
        }   
        else if (p_r != NULL)
            return 1;
        else
            return -1;
    }
}

/* See the description in trc_update.h */
int
trc_update_result_cmp(trc_exp_result *p, trc_exp_result *q)
{
    return trc_update_result_cmp_gen(p, q, TRUE);
}

/* See the description in trc_update.h */
int
trc_update_result_cmp_no_tags(trc_exp_result *p, trc_exp_result *q)
{
    return trc_update_result_cmp_gen(p, q, FALSE);
}

/* See the description in trc_update.h */
int
trc_update_results_cmp(trc_exp_results *p, trc_exp_results *q)
{
    int                  rc;
    trc_exp_result      *p_r;
    trc_exp_result      *q_r;
    te_bool              p_is_void = (p == NULL || STAILQ_EMPTY(p));
    te_bool              q_is_void = (q == NULL || STAILQ_EMPTY(q));

    if (p_is_void && q_is_void)
        return 0;
    else if (!p_is_void && q_is_void)
        return 1;
    else if (p_is_void && !q_is_void)
        return -1;

    p_r = STAILQ_FIRST(p);
    q_r = STAILQ_FIRST(q);

    while (p_r != NULL && q_r != NULL)
    {
        rc = trc_update_result_cmp(p_r, q_r);
        if (rc != 0)
            return rc;

        p_r = STAILQ_NEXT(p_r, links);
        q_r = STAILQ_NEXT(q_r, links);
    }

    if (p_r == NULL && q_r == NULL)
        return 0;
    else if (p_r != NULL)
        return 1;
    else
        return -1;
}

/* See the description in trc_update.h */
int
trc_update_rules_cmp(trc_update_rule *p, trc_update_rule *q)
{
    int rc;

    DUMMY_CMP(p, q);

    if (p->type != q->type)
        return ((int)q->type - (int)p->type > 0 ? -1 : 1);

    switch (p->type)
    {
        case TRC_UPDATE_RULE_RESULTS:
            rc = trc_update_result_cmp(p->def_res, q->def_res);
            if (rc != 0)
                return rc;
            rc = trc_update_results_cmp(p->confl_res, q->confl_res);
            if (rc != 0)
                return rc;

            /*@fallthrough@*/

        case TRC_UPDATE_RULE_RESULT:
            rc = trc_update_results_cmp(p->new_res, q->new_res);
            if (rc != 0)
                return rc;
            rc = trc_update_results_cmp(p->old_res, q->old_res);
            return rc;

            break;

        case TRC_UPDATE_RULE_ENTRY:
            rc = trc_update_rentry_cmp(p->new_re, q->new_re);
            if (rc != 0)
                return rc;
            rc = trc_update_rentry_cmp(p->old_re, q->old_re);
            return rc;
            break;

        case TRC_UPDATE_RULE_VERDICT:
            rc = strcmp_null(p->new_v, q->new_v);
            if (rc != 0)
                return rc;
            rc = strcmp_null(p->old_v, q->old_v);
            return rc;
            break;

        default:
            break;
    }

    return 0;
}

/* See the description in trc_update.h */
te_errno
trc_update_ins_rule(trc_update_rule *rule,
                    trc_update_rules *rules,
                    int (*rules_cmp)(trc_update_rule *,
                                     trc_update_rule *))
{
    int              rc;
    trc_update_rule *next;

    TAILQ_FOREACH(next, rules, links)
    {
        rc = rules_cmp(rule, next);
        if (rc == 0)
            return TE_EEXIST;
        else if (rc < 0)
            break;
    }

    if (next != NULL)
        TAILQ_INSERT_BEFORE(next, rule, links);
    else
        TAILQ_INSERT_TAIL(rules, rule, links);

    return 0;
}

/**
 * Generate TRC Update user data.
 *
 * @param data      Pointer to data used for generationg
 *                  (boolean value determining whether to
 *                  save element to which generated user data
 *                  is attached in this case)
 * @param is_iter   Whether data is generated for iteration or test
 *
 * @return Generated data pointer
 */
void *
trc_update_gen_user_data(void *data, te_bool is_iter)
{
    te_bool     to_save = *((te_bool *)data);

    if (is_iter)
    {
        trc_update_test_iter_data *iter_d = TE_ALLOC(sizeof(*iter_d));

        trc_update_init_test_iter_data(iter_d);

        if (to_save)
            iter_d->to_save = TRUE;
        else
            iter_d->to_save = FALSE;

        return iter_d;
    }
    else
    {
        trc_update_test_data *test_d = TE_ALLOC(sizeof(*test_d));
    
        if (to_save)
            test_d->to_save = TRUE;
        else
            test_d->to_save = FALSE;

        return test_d;
    }
}



/**
 * Generate and attach TRC Update data to a given set of tests.
 *
 * @param tests         Set of tests in TRC DB
 * @param updated_tests Tests to be updated
 * @param user_id       TRC DB user ID
 *
 * @return Number of tests for which updating rules should be generated
 */
static te_errno trc_update_fill_tests_user_data(trc_tests *tests,
                                                trc_update_tests_groups
                                                        *updated_tests,
                                                unsigned int user_id);

/**
 * Generate and attach TRC Update data to a given set of iterations.
 *
 * @param iters         Set of iterations in TRC DB
 * @param updated_tests Tests to be updated
 * @param user_id       TRC DB user ID
 *
 * @return Number of tests for which updating rules should be generated
 */
static int
trc_update_fill_iters_user_data(trc_test_iters *iters,
                                trc_update_tests_groups *updated_tests,
                                unsigned int user_id)
{
    trc_update_test_iter_data   *user_data;
    trc_test_iter               *iter;
    te_bool                      to_save = TRUE;
    trc_report_argument         *args;
    unsigned int                 args_n;
    unsigned int                 args_max;
    trc_test_iter_arg           *arg;
    int                          tests_count = 0;

    TAILQ_FOREACH(iter, &iters->head, links)
    {
        user_data = trc_update_gen_user_data(&to_save, TRUE);
        trc_db_iter_set_user_data(iter, user_id, user_data);
        if (!TAILQ_EMPTY(&iter->args.head))
        {
            args_max = 20;
            args = TE_ALLOC(args_max * sizeof(trc_report_argument));
            args_n = 0;

            TAILQ_FOREACH(arg, &iter->args.head, links)
            {
                args[args_n].name = strdup(arg->name);
                args[args_n].value = strdup(arg->value);
                args[args_n].variable = FALSE;
                args_n++;

                if (args_n == args_max)
                {
                    args_max *= 2;
                    args = realloc(args, args_max *
                                          sizeof(trc_report_argument));
                }
            }
            
            user_data->args = args;
            user_data->args_n = args_n;
            user_data->args_max = args_max;
        }

        tests_count += trc_update_fill_tests_user_data(&iter->tests,
                                                       updated_tests,
                                                       user_id);
    }

    return tests_count;
}

/** See description above */
static int
trc_update_fill_tests_user_data(trc_tests *tests,
                                trc_update_tests_groups *updated_tests,
                                unsigned int user_id)
{
    trc_update_test_data    *user_data;
    trc_test                *test;
    te_bool                  to_save = TRUE;
    int                      tests_count = 0;
    trc_update_test_entry   *test_entry;
    trc_update_tests_group  *tests_group;

    TAILQ_FOREACH(test, &tests->head, links)
    {
        user_data = trc_update_gen_user_data(&to_save, FALSE);
        trc_db_test_set_user_data(test, user_id, user_data);
        if (trc_update_fill_iters_user_data(&test->iters,
                                            updated_tests,
                                            user_id) == 0)
        {
            TAILQ_FOREACH(tests_group, updated_tests, links)
                if (strcmp(test->path, tests_group->path) == 0)
                    break;

            if (tests_group == NULL)
            {
                tests_group = TE_ALLOC(sizeof(*tests_group));
                tests_group->rules = TE_ALLOC(sizeof(trc_update_rules));
                tests_group->path = strdup(test->path);
                TAILQ_INIT(&tests_group->tests);
                TAILQ_INIT(tests_group->rules);
                TAILQ_INSERT_TAIL(updated_tests, tests_group, links);
            }

            test_entry = TE_ALLOC(sizeof(*test_entry));
            test_entry->test = test;
            TAILQ_INSERT_TAIL(&tests_group->tests, test_entry, links);
        }
        tests_count++;
    }

    return tests_count;
}

/**
 * Generate and attach TRC Update data for all required
 * entries of TRC DB (tests and iterations to be updated).
 *
 * @param db            TRC DB
 * @param updated_tests Tests to be updated (pointers to all
 *                      tests not including other tests
 *                      will be inserted into this structure
 *                      as a result of this function call)
 * @param user_id       TRC DB user ID
 *
 * @return Number of tests for which updating rules should be generated
 */
static te_errno
trc_update_fill_db_user_data(te_trc_db *db,
                             trc_update_tests_groups *updated_tests,
                             unsigned int user_id)
{
    return trc_update_fill_tests_user_data(&db->tests, updated_tests,
                                           user_id);
}

/**
 * Set to_save property of user data for all the tests
 * from a given queue.
 *
 * @param tests         Tests queue
 * @param user_id       TRC DB user ID
 * @param to_save       Value of property to be set
 *
 * @return Status code
 */
static te_errno trc_update_set_tests_to_save(trc_tests *tests,
                                             unsigned int user_id,
                                             te_bool to_save);

/**
 * Set to_save property of user data for all the iterations
 * from a given queue.
 *
 * @param iters         Iterations queue
 * @param user_id       TRC DB user ID
 * @param to_save       Value of property to be set
 * @param restore       Restore previous value of to_save
 *                      for the iterations
 *
 * @return Status code
 */
static te_errno
trc_update_set_iters_to_save(trc_test_iters *iters,
                             unsigned int user_id,
                             te_bool to_save,
                             te_bool restore)
{
    trc_update_test_iter_data   *user_data;
    trc_test_iter               *iter;
    int                          rc;

    TAILQ_FOREACH(iter, &iters->head, links)
    {
        user_data = trc_db_iter_get_user_data(iter, user_id);
        if (user_data != NULL)
        {
            if (restore)
                user_data->to_save = user_data->to_save_old;
            else
            {
                user_data->to_save_old = user_data->to_save;
                user_data->to_save = to_save;

                rc = trc_update_set_tests_to_save(&iter->tests,
                                                  user_id, to_save);
                if (rc != 0)
                    return rc;
            }
        }
    }

    return 0;
}

/** See description above */
static te_errno
trc_update_set_tests_to_save(trc_tests *tests,
                             unsigned int user_id,
                             te_bool to_save)
{
    trc_update_test_data    *user_data;
    trc_test                *test;
    int                      rc;

    TAILQ_FOREACH(test, &tests->head, links)
    {
        user_data = trc_db_test_get_user_data(test, user_id);
        if (user_data != NULL)
        {
            user_data->to_save = to_save;

            rc = trc_update_set_iters_to_save(&test->iters,
                                              user_id, to_save,
                                              FALSE);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/**
 * Set to_save property of user data for all the tests and iterations
 * in TRC DB.
 *
 * @param db            TRC DB
 * @param user_id       TRC DB user ID
 * @param to_save       Value of property to be set
 *
 * @return Status code
 */
static te_errno
trc_update_set_to_save(te_trc_db *db,
                       unsigned int user_id,
                       te_bool to_save)
{
    return trc_update_set_tests_to_save(&db->tests, user_id, to_save);
}

/**
 * Set to_save property of user data for a given element (test or
 * iteration) and all its ancestors.
 *
 * @param element       Test or iterations
 * @param is_iter       Is it test or iteration?
 * @param user_id       TRC DB user ID
 * @param to_save       Value of property to be set
 *
 * @return Status code
 */
static te_errno
trc_update_set_propagate_to_save(void *element,
                                 te_bool is_iter,
                                 unsigned int user_id,
                                 te_bool to_save)
{
    trc_test_iter               *iter;
    trc_test                    *test;
    trc_update_test_data        *test_data;
    trc_update_test_iter_data   *iter_data;

    while (element != NULL)
    {
        if (is_iter)
        {
            iter = (trc_test_iter *)element;
            iter_data = trc_db_iter_get_user_data(iter, user_id);
            if (iter_data != NULL)
                iter_data->to_save = to_save;
            element = iter->parent;
        }
        else
        {
            test = (trc_test *)element;
            test_data = trc_db_test_get_user_data(test, user_id);
            if (test_data != NULL)
                test_data->to_save = to_save;
            element = test->parent;
        }

        is_iter = !is_iter;
    }

    return 0;
}

/**
 * Merge a new result into a list of new results for an iteration.
 *
 * @param ctx           TRC Update context
 * @param upd_iter_data TRC Update iteration data
 * @param te_result     Test result to be merged
 * @param is_expected   Is the result expected?
 *
 * @return Status code
 */
static te_errno trc_update_iter_data_merge_result(trc_update_ctx *ctx,
                                                  trc_update_test_iter_data
                                                            *upd_iter_data,
                                                  te_test_result
                                                                *te_result,
                                                  te_bool is_expected);

/**
 * Add a new unexpected result to all the iterations of a given tests.
 *
 * @param ctx            TRC Update context
 * @param tests          Queue of tests
 * @param te_resul       Result to be added
 *
 * @return Status code
 */
static te_errno trc_update_tests_add_result(trc_update_ctx *ctx,
                                            trc_tests *tests,
                                            te_test_result *te_result);

/**
 * Add a new unexpected result to all the iterations.
 *
 * @param ctx            TRC Update context
 * @param iters          Queue of iterations
 * @param te_resul       Result to be added
 *
 * @return Status code
 */
static te_errno
trc_update_iters_add_result(trc_update_ctx *ctx,
                            trc_test_iters *iters,
                            te_test_result *te_result)
{
    trc_update_test_iter_data   *user_data;
    trc_test_iter               *iter;
    int                          rc;

    TAILQ_FOREACH(iter, &iters->head, links)
    {
        user_data = trc_db_iter_get_user_data(iter, ctx->db_uid);
        if (user_data != NULL)
        {
            if (!TAILQ_EMPTY(&iter->tests.head))
                rc = trc_update_tests_add_result(ctx, &iter->tests,
                                                 te_result);
            else if (user_data->counter < ctx->cur_lnum)
                rc = trc_update_iter_data_merge_result(ctx,
                                                       user_data,
                                                       te_result,
                                                       TRUE);

            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/** See description above */
static te_errno
trc_update_tests_add_result(trc_update_ctx *ctx,
                            trc_tests *tests,
                            te_test_result *te_result)
{
    trc_update_test_data    *user_data;
    trc_test                *test;
    int                      rc;

    TAILQ_FOREACH(test, &tests->head, links)
    {
        user_data = trc_db_test_get_user_data(test, ctx->db_uid);
        if (user_data != NULL)
        {
            rc = trc_update_iters_add_result(ctx, &test->iters,
                                             te_result);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

/**
 * Add skipped results to skipped iterations.
 *
 * @param ctx            TRC Update context
 *
 * @return Status code
 */
static te_errno
trc_update_add_skipped(trc_update_ctx *ctx)
{
    te_test_result te_result;

    te_result.status = TE_TEST_SKIPPED;
    TAILQ_INIT(&te_result.verdicts);

    return trc_update_tests_add_result(ctx, &ctx->db->tests,
                                       &te_result);
}

/**
 * Typedef for a function checking some condition
 * on given test results.
 */
typedef te_bool (*check_res_cond)(trc_exp_results *);

/**
 * Check whether all the results are SKIPPED or not.
 *
 * @param results   Test results
 *
 * @return @c TRUE or @c FALSE
 */
static te_bool
is_skip_only(trc_exp_results *results)
{
    trc_exp_result              *result;
    trc_exp_result_entry        *rentry;

    STAILQ_FOREACH(result, results, links)
    {
        TAILQ_FOREACH(rentry, &result->results, links)
            if (rentry->result.status != TE_TEST_SKIPPED)
                break;

        if (rentry != NULL)
            break;
    }

    if (result == NULL)
        return TRUE;

    return FALSE;
}

/**
 * Check whether all the results are expected or not.
 *
 * @param results   Test results
 *
 * @return @c TRUE or @c FALSE
 */
static te_bool
is_exp_only(trc_exp_results *results)
{
    trc_exp_result              *result;
    trc_exp_result_entry        *rentry;

    STAILQ_FOREACH(result, results, links)
    {
        TAILQ_FOREACH(rentry, &result->results, links)
            if (!rentry->is_expected)
                break;

        if (rentry != NULL)
            break;
    }

    if (result == NULL)
        return TRUE;

    return FALSE;
}

/** Result editing operation */
typedef enum result_ops {
    RESULT_OP_CLEAR = 1,        /**< Clear results */
    RESULT_OP_DF_REPLACE,       /**< Replace new_results with
                                     df_results */
    RESULT_OP_EXCLUDE_EMPTY,    /**< Exclude tests having
                                     empty lists of unexpected
                                     results in all their
                                     iterations */
    RESULT_OP_EXCL_EXP_EMPTY,   /**< Exclude tests and iterations
                                     having empty lists of expected
                                     results */
    RESULT_OP_REMOVE_EXP,
} result_ops;

/**
 * For all the tests, perform a given operation on a list of
 * unexpected results where for all of them a given condition
 * is @c TRUE.
 *
 * @param ctx            TRC Update context
 * @param tests          Queue of tests
 * @param cond_func      Condition function
 * @param op             Result editing operation
 * @param all_empty      Will be set to @c TRUE if
 *                       all the lists of unexpected
 *                       results are empty
 *
 * @return Status code
 */
static te_errno trc_update_tests_cond_res_op(trc_update_ctx *ctx,
                                             trc_tests *tests,
                                             check_res_cond cond_func,
                                             result_ops op,
                                             te_bool *all_empty);

static void
trc_exp_results_remove_expected(trc_exp_results *results)
{
    trc_exp_result *result;
    trc_exp_result *result_aux;

    trc_exp_result_entry *entry;
    trc_exp_result_entry *entry_aux;

    STAILQ_FOREACH_SAFE(result, results, links, result_aux)
    {
        TAILQ_FOREACH_SAFE(entry, &result->results, links, entry_aux)
        {
            if (entry->is_expected)
                TAILQ_REMOVE(&result->results, entry, links);
        }
        if (TAILQ_EMPTY(&result->results)) /* Not efficient!!! */
            STAILQ_REMOVE(results, result, trc_exp_result, links);
    }
}

/**
 * For all the iterations, perform a given operation on a list of
 * unexpected results where for all of them a given condition
 * is @c TRUE.
 *
 * @param ctx            TRC Update context
 * @param iters          Queue of iterations
 * @param cond_func      Condition function
 * @param op             Result editing operation
 * @param all_empty      Will be set to @c TRUE if
 *                       all the lists of unexpected
 *                       results are empty
 *
 * @return Status code
 */
static te_errno
trc_update_iters_cond_res_op(trc_update_ctx *ctx,
                             trc_test_iters *iters,
                             check_res_cond cond_func,
                             result_ops op, te_bool *all_empty)
{
    trc_update_test_iter_data   *user_data;
    trc_test_iter               *iter;
    int                          rc;
    trc_exp_result              *q;
    trc_exp_result              *q_tvar;
    te_bool                      my_all_empty = TRUE;
    te_bool                      all_empty_aux = TRUE;

    TAILQ_FOREACH(iter, &iters->head, links)
    {
        user_data = trc_db_iter_get_user_data(iter, ctx->db_uid);
        if (user_data != NULL)
        {
            if (!TAILQ_EMPTY(&iter->tests.head))
            {
                rc = trc_update_tests_cond_res_op(ctx, &iter->tests,
                                                  cond_func, op,
                                                  &all_empty_aux);
                if (!all_empty_aux)
                    my_all_empty = FALSE;

                if (all_empty_aux && op == RESULT_OP_EXCL_EXP_EMPTY)
                    user_data->to_save = FALSE;
            }
            else
            {
                if (cond_func == NULL ||
                    cond_func(&user_data->new_results))
                {
                    switch (op)
                    {
                        case RESULT_OP_REMOVE_EXP:
                            trc_exp_results_remove_expected(
                                                  &user_data->new_results);
                            break;

                        case RESULT_OP_CLEAR:
                            trc_exp_results_free(&user_data->new_results);
                            break;

                        case RESULT_OP_DF_REPLACE:
                            trc_exp_results_free(&user_data->new_results);

                            STAILQ_FOREACH_SAFE(q, &user_data->df_results,
                                                links, q_tvar)
                            {
                                STAILQ_REMOVE_HEAD(&user_data->df_results,
                                                   links);
                                STAILQ_INSERT_TAIL(&user_data->new_results,
                                                   q, links);
                            }

                            break;

                        default:
                            break;
                    }
                }

                if (!STAILQ_EMPTY(&user_data->new_results) ||
                    (!STAILQ_EMPTY(&iter->exp_results) &&
                     op == RESULT_OP_EXCL_EXP_EMPTY))
                    my_all_empty = FALSE;

                if (STAILQ_EMPTY(&iter->exp_results) &&
                    op == RESULT_OP_EXCL_EXP_EMPTY)
                    user_data->to_save = FALSE;
            }

            if (rc != 0)
                return rc;
        }
    }

    if (my_all_empty && op == RESULT_OP_EXCLUDE_EMPTY)
    {
        TAILQ_FOREACH(iter, &iters->head, links)
        {
            user_data = trc_db_iter_get_user_data(iter, ctx->db_uid);
            if (user_data != NULL)
                user_data->to_save = FALSE;
        }
    }

    if (all_empty != NULL)
        *(te_bool *)all_empty = my_all_empty;

    return 0;
}

/** See description above */
static te_errno
trc_update_tests_cond_res_op(trc_update_ctx *ctx,
                             trc_tests *tests,
                             check_res_cond cond_func,
                             result_ops op,
                             te_bool *all_empty)
{
    trc_update_test_data    *user_data;
    trc_test                *test;
    int                      rc;
    te_bool                  my_all_empty = FALSE;

    if (all_empty != NULL)
        *(te_bool *)all_empty = TRUE;

    TAILQ_FOREACH(test, &tests->head, links)
    {
        user_data = trc_db_test_get_user_data(test, ctx->db_uid);
        if (user_data != NULL)
        {
            rc = trc_update_iters_cond_res_op(ctx, &test->iters,
                                              cond_func, op,
                                              &my_all_empty);
            if (rc != 0)
                return rc;

            if (my_all_empty == FALSE)
            {
                if (all_empty != NULL)
                    *(te_bool *)all_empty = FALSE;
            }
            else if (op == RESULT_OP_EXCLUDE_EMPTY ||
                     op == RESULT_OP_EXCL_EXP_EMPTY)
                user_data->to_save = FALSE;
        }
    }

    return 0;
}

/**
 * Perform a given operation on a list of unexpected results
 * where for all of them a given condition is @c TRUE.
 *
 * @param ctx            TRC Update context
 * @param cond_func      Condition function
 * @param op             Result editing operation
 *
 * @return Status code
 */
static te_errno
trc_update_cond_res_op(trc_update_ctx *ctx, check_res_cond cond_func,
                       result_ops op)
{
    return trc_update_tests_cond_res_op(ctx, &ctx->db->tests,
                                        cond_func, op, NULL);
}

/**
 * Get XML representation of TRC updating rule.
 *
 * @param rule      TRC updating rule
 * @param node      XML node where XML representation
 *                  should be placed
 *
 * @return Status code
 */
static te_errno
trc_update_rule_to_xml(trc_update_rule *rule, xmlNodePtr node)
{
#define ID_LEN 20
    char            id_val[ID_LEN];
    xmlNodePtr      rule_node;
    xmlNodePtr      args_node;
    xmlNodePtr      arg_node;
    xmlNodePtr      defaults;
    xmlNodePtr      old_res;
    xmlNodePtr      new_res;
    xmlNodePtr      confl_res;

    trc_update_wilds_list_entry *wild;
    trc_test_iter_arg           *arg;

    rule_node = xmlNewChild(node, NULL, BAD_CAST "rule", NULL);
    if (rule->rule_id > 0)
    {
        snprintf(id_val, ID_LEN, "%d", rule->rule_id);
        xmlNewProp(rule_node, BAD_CAST "id", BAD_CAST id_val);
    }

    if (rule->type == TRC_UPDATE_RULE_RESULT)
        xmlNewProp(rule_node, BAD_CAST "type", BAD_CAST "result");

    if (rule->wilds != NULL)
    {
        SLIST_FOREACH(wild, rule->wilds, links)
        {
            args_node = xmlNewChild(rule_node, NULL,
                                    BAD_CAST "args", NULL);
            if (args_node == NULL)
            {
                ERROR("%s(): failed to create <args> node", __FUNCTION__);
                return TE_ENOMEM;
            }

            if (wild->is_strict)
                xmlNewProp(args_node, BAD_CAST "type",
                           BAD_CAST "strict");

            assert(wild->args != NULL);
            TAILQ_FOREACH(arg, &wild->args->head, links)
            {
                arg_node = xmlNewChild(args_node, NULL,
                                       BAD_CAST "arg",
                                       BAD_CAST arg->value);
                if (arg_node == NULL)
                {
                    ERROR("%s(): failed to create <arg> node",
                          __FUNCTION__);
                    return TE_ENOMEM;
                }
                xmlNewProp(arg_node, BAD_CAST "name",
                           BAD_CAST arg->name);
            }
        }
    }

    if (rule->type == TRC_UPDATE_RULE_RESULTS)
    {
        defaults = xmlNewChild(rule_node, NULL,
                               BAD_CAST "defaults", NULL);
        if (defaults == NULL)
        {
            ERROR("%s(): failed to create <defaults> node", __FUNCTION__);
            return TE_ENOMEM;
        }

        trc_exp_result_to_xml(rule->def_res, defaults,
                              TRUE);
    }

    old_res = xmlNewChild(rule_node, NULL,
                       BAD_CAST "old", NULL);
    if (old_res == NULL)
    {
        ERROR("%s(): failed to create <old> node", __FUNCTION__);
        return TE_ENOMEM;
    }

    if (rule->type == TRC_UPDATE_RULE_RESULTS ||
        rule->type == TRC_UPDATE_RULE_RESULT)
        trc_exp_results_to_xml(rule->old_res, old_res, FALSE);
    else if (rule->type == TRC_UPDATE_RULE_ENTRY)
        trc_exp_result_entry_to_xml(rule->old_re, old_res);
    else if (rule->type == TRC_UPDATE_RULE_VERDICT)
        trc_verdict_to_xml(rule->old_v, old_res);

    if (rule->type == TRC_UPDATE_RULE_RESULTS)
    {
        confl_res = xmlNewChild(rule_node, NULL,
                                BAD_CAST "conflicts",
                                NULL);
        if (confl_res == NULL)
        {
            ERROR("%s(): failed to create <conflicts> node", __FUNCTION__);
            return TE_ENOMEM;
        }

        if (rule->confl_res != NULL)
            trc_exp_results_to_xml(rule->confl_res, confl_res, FALSE);
    }

    if ((rule->new_res != NULL && !STAILQ_EMPTY(rule->new_res) &&
         (rule->type == TRC_UPDATE_RULE_RESULTS ||
          rule->type == TRC_UPDATE_RULE_RESULT)) ||
        (rule->new_re != NULL && rule->type == TRC_UPDATE_RULE_ENTRY) ||
        (rule->new_v != NULL && rule->type == TRC_UPDATE_RULE_VERDICT))
    {
        new_res = xmlNewChild(rule_node, NULL,
                           BAD_CAST "new", NULL);
        if (new_res == NULL)
        {
            ERROR("%s(): failed to create <new> node", __FUNCTION__);
            return TE_ENOMEM;
        }

        if (rule->type == TRC_UPDATE_RULE_RESULTS ||
            rule->type == TRC_UPDATE_RULE_RESULT)
            trc_exp_results_to_xml(rule->new_res, new_res, FALSE);
        else if (rule->type == TRC_UPDATE_RULE_ENTRY)
            trc_exp_result_entry_to_xml(rule->new_re, new_res);
        else if (rule->type == TRC_UPDATE_RULE_VERDICT)
            trc_verdict_to_xml(rule->new_v, new_res);
    }

    return 0;
}

/**
 * Save updating rules to file.
 *
 * @param updated_tests    Tests to be updated
 * @param filename         Path to file where rules should be saved
 * @param cmd              Command used to run TRC Update tool
 */
static te_errno
save_test_rules_to_file(trc_update_tests_groups *updated_tests,
                        const char *filename,
                        char *cmd)
{
    FILE                    *f = NULL;
    trc_update_tests_group  *group;
    trc_update_rule         *rule;
    xmlDocPtr                xml_doc;
    xmlNodePtr               root;
    xmlNodePtr               test_node;
    xmlNodePtr               comment_node;
    xmlNodePtr               command_node;
    xmlChar                 *xml_cmd = NULL;

    char                    *comment_text =
        "\n"
        "This file contains rules for updating TRC of a given test(s).\n"
        "For each specified test a rule is generated for any unique\n"
        "combination of old results specified in TRC for some iteration\n"
        "before updating and non-expected results extracted from logs for\n"
        "the same iteration.\n"
        "\n"
        "Every updating rule (tag <rule>) can contain following sections:\n"
        "1. Default results for iteration (tag <defaults>);\n"
        "2. Results currently specified in TRC (tag <old>);\n"
        "3. Non-expected results extracted from logs (tag <conflicts>);\n"
        "4. Results to be placed in TRC during its update for all\n"
        "   iterations this update rule is related to (tag <new>).\n"
        "5. Wildcards describing to which iterations this rule can\n"
        "   be applied (currently they aren't generated, can be\n"
        "   used manually - tag <args>, more than one tag can\n"
        "   be used).\n"
        "\n"
        "Any section can be omitted. If after generating rules you\n"
        "see rule with all sections omitted it means just that there\n"
        "were some iterations not presented in current TRC (so no\n"
        "<defaults> and <old>) and also having no results from logs (for\n"
        "example due to the fact that these iterations are specified\n"
        "in package.xml but not appeared in logs specified as source\n"
        "for updating TRC) - so no <conflicts> too.\n"
        "\n"
        "<new> section is always omitted after automatic generation\n"
        "of rules. Without this section, rule has no effect. If this\n"
        "section is presented but void, applying of rule will delete\n"
        "results from TRC for all iterations the rule is related to.\n"
        "\n"
        "To use these rules, you should write your edition of results\n"
        "in <new> section of them, and then run trc_update.pl with\n" 
        "\"rules\" parameter set to path of this file. See help of\n"
        "trc_update.pl for more info.\n";

    xml_doc = xmlNewDoc(BAD_CAST "1.0");
    if (xml_doc == NULL)
    {
        ERROR("%s(): xmlNewDoc() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    root = xmlNewNode(NULL, BAD_CAST "update_rules");
    if (root == NULL)
    {
        ERROR("%s(): xmlNewNode() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    xmlDocSetRootElement(xml_doc, root);

    comment_node = xmlNewComment(BAD_CAST comment_text);
    if (comment_node == NULL)
    {
        ERROR("%s(): xmlNewComment() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    if (xmlAddChild(root, comment_node) == NULL)
    {
        ERROR("%s(): xmlAddChild() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    xml_cmd = xmlEncodeEntitiesReentrant(xml_doc,
                                         BAD_CAST cmd);
    if (xml_cmd == NULL)
    {
        ERROR("xmlEncodeEntitiesReentrant() failed");
        return TE_ENOMEM;
    }

    command_node = xmlNewChild(root, NULL, BAD_CAST "command",
                               xml_cmd);
    if (command_node == NULL)
    {
        ERROR("%s(): xmlNewChild() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    TAILQ_FOREACH(group, updated_tests, links)
    {
        if (TAILQ_EMPTY(group->rules))
            continue;

        test_node = xmlNewChild(root, NULL, BAD_CAST "test",
                                NULL);
        if (test_node == NULL)
        {
            ERROR("%s(): xmlNewChild() failed", __FUNCTION__);
            return TE_ENOMEM;
        }

        if (xmlNewProp(test_node, BAD_CAST "path",
                       BAD_CAST group->path) == NULL)
        {
            ERROR("%s(): xmlNewProp() failed", __FUNCTION__);
            return TE_ENOMEM;
        }

        if (group->rules != NULL)
            TAILQ_FOREACH(rule, group->rules, links)
                trc_update_rule_to_xml(rule, test_node);
    }

    f = fopen(filename, "w");
    if (xmlDocFormatDump(f, xml_doc, 1) == -1)
    {
        ERROR("%s(): xmlDocFormatDump() failed", __FUNCTION__);
        return TE_ENOMEM;
    }
    fclose(f);
    xmlFreeDoc(xml_doc);

    return 0;
}

/**
 * Try to find test iteration result in a given list.
 *
 * @param find_result       Result to be found
 * @param results           List of results where to search
 * @param no_tags           If @c TRUE, tag expressions won't
 *                          be taken into account
 * @param result_found      If result was found, it will be set
 *                          to @c TRUE
 * @param entry_found       If result entry was found, it will
 *                          be set to @c TRUE
 * @param result            If result was found, it will be set
 *                          to its pointer in list; otherwise
 *                          it will be set to the pointer of
 *                          the element of the list after which
 *                          new result can be placed
 *
 * @return Status code
 */
static te_errno
trc_update_find_result(trc_exp_result *find_result,
                       trc_exp_results *results,
                       te_bool no_tags, te_bool *result_found,
                       te_bool *entry_found,
                       trc_exp_result **result)
{
    trc_exp_result             *prev = NULL;
    trc_exp_result             *p;
    trc_exp_result_entry       *q;
    int                         rc = 0;
    te_bool                     tags_fit;

    *result_found = FALSE;
    *entry_found = FALSE;
    *result = NULL;

    if (!STAILQ_EMPTY(results))
    {
        STAILQ_FOREACH(p, results, links)
        {
            tags_fit = 
                (p->tags_str == NULL && find_result->tags_str == NULL) ||
                (p->tags_str != NULL && find_result->tags_str != NULL &&
                (rc = strcmp(p->tags_str, find_result->tags_str)) <= 0);

            if (no_tags || (rc == 0 && tags_fit))
            {
                TAILQ_FOREACH(q, &p->results, links)
                    if (te_test_result_cmp(
                            &q->result,
                            &TAILQ_FIRST(
                                &find_result->results)->result) == 0)
                        break;

                if (q != NULL)
                {
                    *result = p;
                    *result_found = TRUE;
                    *entry_found = TRUE;
                    return 0;
                }
                else if (!no_tags)
                {
                    *result = p;
                    *result_found = TRUE;
                    return 0;
                }
            }
            else if (!no_tags && rc != 0 && tags_fit)
            {
                *result = prev;
                return 0;
            }

            prev = p;
        }
    }

    return 0;
}

/** See decription above */
static te_errno
trc_update_iter_data_merge_result(trc_update_ctx *ctx,
                                  trc_update_test_iter_data *upd_iter_data,
                                  te_test_result *te_result,
                                  te_bool is_expected)
{
    trc_exp_result             *merge_result;
    trc_exp_result_entry       *result_entry;
    trc_exp_results            *results_list = NULL;
    trc_exp_result             *res = NULL;
    te_bool                     result_found = FALSE;
    te_bool                     entry_found = FALSE;
    te_test_verdict            *verdict;

    tqe_string  *tqs_p;
    tqe_string  *tqs_q;
    tqe_string  *tqs_r;
    char        *s;
    char        *dup_tag;
    int          max_expr_len = 1000;
    int          cur_pos = 0;
    tqh_strings  tag_names;
    tqh_strings *tags_list;
    te_errno     rc;

    if ((ctx->flags & TRC_UPDATE_NO_INCOMPL) &&
        te_result->status == TE_TEST_INCOMPLETE)
        return 0;

    if ((ctx->flags & TRC_UPDATE_NO_R_FAIL) &&
        te_result->status == TE_TEST_FAILED &&
        TAILQ_EMPTY(&te_result->verdicts))
        return 0;

    if (ctx->flags & TRC_UPDATE_NO_INT_ERR)
    {
        TAILQ_FOREACH(verdict, &te_result->verdicts, links)
            if (strcmp(verdict->str, "Internal error") == 0)
                return 0;
    }

    TAILQ_INIT(&tag_names);

    merge_result = TE_ALLOC(sizeof(*merge_result));
    TAILQ_INIT(&merge_result->results);

    result_entry = TE_ALLOC(sizeof(*result_entry));
    result_entry->key = NULL;
    result_entry->notes = NULL;
    result_entry->is_expected = is_expected;
    te_test_result_cpy(&result_entry->result, te_result);
    TAILQ_INSERT_HEAD(&merge_result->results,
                      result_entry,
                      links);

    if ((ctx->flags & TRC_UPDATE_GEN_TAGS) &&
        (ctx->merge_str == NULL ||
         strcmp(ctx->merge_str, "UNSPEC") == 0))
    {
        /*
         * Creating tag expression from the list of tags
         * got from the log.
         */

        free(ctx->merge_str);
        ctx->merge_str = TE_ALLOC(max_expr_len);

        if (!TAILQ_EMPTY(&ctx->tags_gen_list) &&
            strcmp(TAILQ_FIRST(&ctx->tags_gen_list)->v, "*") == 0)
        {
            TAILQ_FOREACH(tqs_q, &ctx->tags, links)
            {
                dup_tag = strdup(tqs_q->v);
                s = strchr(dup_tag, ':');
                if (s != NULL)
                    *s = '\0';
                if (tq_strings_add_uniq(&tag_names, dup_tag) != 0)
                    free(dup_tag);
            }

            tags_list = &tag_names;
        }
        else
            tags_list = &ctx->tags_gen_list;

        TAILQ_FOREACH(tqs_p, tags_list, links)
        {
            tqs_r = NULL;

            TAILQ_FOREACH(tqs_q, &ctx->tags, links)
            {
                s = strchr(tqs_q->v, ':');

                if ((s != NULL &&
                     (int)strlen(tqs_p->v) == s - tqs_q->v &&
                     memcmp(tqs_p->v, tqs_q->v, s - tqs_q->v) == 0) ||
                    strcmp(tqs_p->v, tqs_q->v) == 0 ||
                    (tqs_p->v[0] == '?' &&
                     strstr(tqs_q->v, tqs_p->v + 1) != NULL))
                    tqs_r = tqs_q;
            }

            if (tqs_r != NULL)
            {
                if (cur_pos != 0)
                {
                    snprintf(ctx->merge_str + cur_pos,
                             max_expr_len - cur_pos,
                             "&");
                    cur_pos++;
                }

                if (tqs_p->v[0] != '?')
                {
                    snprintf(ctx->merge_str + cur_pos,
                             max_expr_len - cur_pos,
                             "%s", tqs_r->v);
                    cur_pos += strlen(tqs_r->v);

                    s = strchr(ctx->merge_str, ':');
                    if (s != NULL)
                        *s = '=';
                }
                else
                {
                    snprintf(ctx->merge_str + cur_pos,
                             max_expr_len - cur_pos,
                             "%s", tqs_p->v + 1);
                    cur_pos += strlen(tqs_p->v) - 1;
                }
            }
        }

        tq_strings_free(&tag_names, free);
        logic_expr_free(ctx->merge_expr);
        ctx->merge_expr = NULL;
        if (ctx->merge_str[0] != '\0')
        {
            rc = logic_expr_parse(ctx->merge_str, &ctx->merge_expr);
            if (rc != 0)
            {
                trc_exp_result_free(merge_result);
                return rc;
            }
        }
        else
        {
            char *s;

            s = strdup("NO_TAGS_MATCHED");
            ctx->merge_expr = TE_ALLOC(sizeof(logic_expr));
            if (ctx->merge_expr == NULL || s == NULL)
            {
                trc_exp_result_free(merge_result);
                free(ctx->merge_expr);
                ctx->merge_expr = NULL;
                free(s);

                ERROR("%s(): out of memory", __FUNCTION__);
                return TE_ENOMEM;
            }

            ctx->merge_expr->type = LOGIC_EXPR_VALUE;
            ctx->merge_expr->u.value = s;
        }
    }

    merge_result->tags_expr = logic_expr_dup(ctx->merge_expr);
    if (ctx->flags & TRC_UPDATE_TAGS_STR)
    {
        merge_result->tags_str = strdup(ctx->merge_str);
        merge_result->tags = TE_ALLOC(sizeof(tqh_strings));
        TAILQ_INIT(merge_result->tags);
        tq_strings_add_uniq_dup(merge_result->tags,
                                ctx->merge_str);
    }
    else
        merge_result->tags_str = logic_expr_to_str(ctx->merge_expr);

    merge_result->key = NULL;
    merge_result->notes = NULL;

    results_list = &upd_iter_data->new_results;

    trc_update_find_result(merge_result, results_list, FALSE,
                           &result_found, &entry_found, &res);

    if (!result_found)
    {
        if (res != NULL)
            STAILQ_INSERT_AFTER(results_list, res, merge_result, links);
        else
            STAILQ_INSERT_TAIL(results_list, merge_result, links);
    }
    else if (result_found && !entry_found)
    {
        TAILQ_REMOVE(&merge_result->results, result_entry,
                     links);
        TAILQ_INSERT_TAIL(&res->results, result_entry, links);
        trc_exp_result_free(merge_result);
    }
    else /* if (result_found && entry_found) */
        trc_exp_result_free(merge_result);

    return 0;
}

/**
 * Match result from XML logs to results currently presented
 * in TRC, add it to list of new results for a given iteration if
 * it is not expected and not encountered yet.
 *
 * @param ctx           TRC Update context
 *
 * @return Status code
 */
static te_errno
trc_update_merge_result(trc_update_ctx *ctx,
                        trc_test_iter *iter,
                        te_test_result *test_result)
{
    trc_update_test_iter_data  *upd_iter_data; 
    trc_exp_result             *p;
    te_bool                     is_expected = FALSE;

    upd_iter_data = trc_db_iter_get_user_data(iter, ctx->db_uid);

    if (ctx->fake_filt_log != NULL && !upd_iter_data->filtered)
        return 0;

    p = (trc_exp_result *) trc_db_iter_get_exp_result(iter, &ctx->tags,
                                                      ctx->db->last_match);
    if (p != NULL)
        is_expected =
            (trc_is_result_expected(p, test_result) != NULL);

    if (!(ctx->flags & TRC_UPDATE_CONFLS_ALL) &&
        !((ctx->flags & TRC_UPDATE_LOG_WILDS) &&
          !(ctx->flags & TRC_UPDATE_LOG_WILDS_UNEXP)) &&
        is_expected)
        return 0;

    return trc_update_iter_data_merge_result(ctx, upd_iter_data,
                                             test_result, is_expected);
}

/* Predeclaration of function, see description below. */
static te_errno trc_update_gen_test_wilds_fss(unsigned int db_uid,
                                              trc_update_test_entry
                                                            *test_entry,
                                              te_bool grouped,
                                              int groups_cnt,
                                              trc_update_args_groups
                                                                *wildcards,
                                              uint64_t flags);

/* Predeclaration of function, see description below. */
static te_errno trc_update_generate_test_wilds(unsigned int db_uid,
                                               trc_test *test,
                                               te_bool grouped,
                                               int groups_cnt,
                                               trc_update_args_groups
                                                           *save_wildcards,
                                               uint64_t flags);
/**
 * Simplify tag expressions for a given list of iteration results.
 *
 * @param results   List of iteration results
 * @param flags     Flags
 *
 * @return Status code
 */
static te_errno
simplify_log_exprs(trc_exp_results *results,
                   uint64_t flags)
{
    trc_exp_result             *p;
    trc_exp_result             *q;
    trc_exp_result             *r;
    trc_exp_result_entry       *entry_p;
    trc_exp_result_entry       *entry_q;
    trc_test                   *test;
    trc_update_test_entry      *test_entry;
    trc_test_iter              *iter;
    logic_expr                 *expr_p;
    logic_expr                 *expr_q;
    logic_expr                **array;
    int                         size;
    trc_update_args_group      *args_group;
    trc_test_iter_arg          *arg;
    trc_test_iter_arg          *iter_arg; 
    unsigned int                n_args;
    trc_update_test_iter_data  *iter_data;
    te_string                   te_str;

    int   i = 0;
    int   j = 0;
    char *tag;
    char *s;

    /*
     * In this function a fake test is created. Every its iteration is
     * formed from one of results passed to this function. We get one of
     * conjuncts from logical tag expression of this result, and then
     * consider tags in this conjuncts as arguments, and result marked by
     * this expression - as a result of an iteration of fake test with such
     * arguments. This strange thing is done just to apply wildcard
     * generating algorithm which was not initially intended
     * to be used for such a purpose.
     */

    test_entry = TE_ALLOC(sizeof(*test_entry));

    test_entry->test = test = TE_ALLOC(sizeof(*test));
    test->type = TRC_TEST_SCRIPT;
    TAILQ_INIT(&test->iters.head);

    args_group = TE_ALLOC(sizeof(*args_group));
    args_group->args = TE_ALLOC(sizeof(trc_test_iter_args));
    TAILQ_INIT(&args_group->args->head);

    n_args = 0;

    STAILQ_FOREACH(p, results, links)
    {
        expr_p = logic_expr_dup(p->tags_expr);
        logic_expr_dnf(&expr_p, NULL);
        /*
         * Split disjunctive normal form into array
         * of its conjuncts.
         */
        logic_expr_dnf_split(expr_p, &array, &size);
        logic_expr_free(expr_p);
        
        /*
         * Extract names of all the tags.
         */
        for (i = 0; i < size; i++)
        {
            expr_p = array[i];

            while (expr_p != NULL)
            {
                if (expr_p->type == LOGIC_EXPR_AND)
                    expr_q = expr_p->u.binary.rhv;
                else
                    expr_q = expr_p;

                tag = logic_expr_to_str(expr_q);
                s = strchr(tag, '=');
                if (s != NULL)
                    *s = '\0';

                TAILQ_FOREACH(arg, &args_group->args->head, links)
                    if (strcmp(arg->name, tag) == 0)
                        break;

                if (arg == NULL)
                {
                    arg = TE_ALLOC(sizeof(*arg));
                    arg->name = tag;
                    TAILQ_INSERT_TAIL(&args_group->args->head, arg, links);
                    n_args++;
                }
                else
                    free(tag);

                if (expr_p->type == LOGIC_EXPR_AND)
                    expr_p = expr_p->u.binary.lhv;
                else
                    expr_p = NULL;
            }
            logic_expr_free(array[i]);
        }
        free(array);
    }

    /*
     * Generate fake test iterations.
     */
    STAILQ_FOREACH(p, results, links)
    {
        expr_p = logic_expr_dup(p->tags_expr);
        logic_expr_dnf(&expr_p, NULL);
        logic_expr_dnf_split(expr_p, &array, &size);
        logic_expr_free(expr_p);
        
        for (i = 0; i < size; i++)
        {
            expr_p = array[i];

            TAILQ_FOREACH(arg, &args_group->args->head, links)
                arg->value = strdup("NEXISTS");

            while (expr_p != NULL)
            {
                if (expr_p->type == LOGIC_EXPR_AND)
                    expr_q = expr_p->u.binary.rhv;
                else
                    expr_q = expr_p;

                tag = logic_expr_to_str(expr_q);
                s = strchr(tag, '=');
                if (s != NULL)
                    *s = '\0';

                TAILQ_FOREACH(arg, &args_group->args->head, links)
                    if (strcmp(arg->name, tag) == 0)
                        break;

                assert(arg != NULL);
                
                free(arg->value);
                if (s == NULL)
                    arg->value = strdup("EXISTS");
                else
                    arg->value = strdup(s + 1);

                free(tag);

                if (expr_p->type == LOGIC_EXPR_AND)
                    expr_p = expr_p->u.binary.lhv;
                else
                    expr_p = NULL;
            }

            TAILQ_FOREACH(iter, &test->iters.head, links)
            {
                iter_data = trc_db_iter_get_user_data(iter, 0);

                if (test_iter_args_match(args_group->args,
                                         iter_data->args_n,
                                         iter_data->args,
                                         TRUE) !=
                                            ITER_NO_MATCH)
                {
                    entry_p = TAILQ_FIRST(&p->results);
                    TAILQ_FOREACH(entry_q,
                                  &STAILQ_FIRST(
                                        &iter->exp_results)->results,
                                  links)
                        if (trc_update_rentry_cmp(entry_q, entry_p) == 0)
                            break;

                    if (entry_q == NULL)
                    {
                        entry_q = trc_exp_result_entry_dup(entry_p);
                        TAILQ_INSERT_TAIL(
                                &STAILQ_FIRST(
                                    &iter->exp_results)->results,
                                entry_q,
                                links);
                        break;
                    }
                }
            }
            
            if (iter == NULL)
            {
                iter = TE_ALLOC(sizeof(*iter));
                TAILQ_INIT(&iter->args.head);
                LIST_INIT(&iter->users);
                STAILQ_INIT(&iter->exp_results);
                iter_data = TE_ALLOC(sizeof(*iter_data));
                trc_update_init_test_iter_data(iter_data);
                iter_data->args =
                    TE_ALLOC(sizeof(trc_report_argument) * n_args);
                iter_data->args_n = n_args;
                iter_data->args_max = n_args;
                iter_data->to_save = TRUE;
                j = 0;

                TAILQ_FOREACH(arg, &args_group->args->head, links)
                {
                    iter_data->args[j].name = strdup(arg->name);
                    iter_data->args[j].value = strdup(arg->value);
                    j++;

                    iter_arg = TE_ALLOC(sizeof(*iter_arg));
                    iter_arg->name = strdup(arg->name);
                    iter_arg->value = arg->value;
                    TAILQ_INSERT_TAIL(&iter->args.head, iter_arg, links);
                    arg->value = NULL;
                }

                q = trc_exp_result_dup(p);
                q->tags_str = strdup("unspecified");
                logic_expr_parse(q->tags_str, &q->tags_expr);
                STAILQ_INSERT_TAIL(&iter->exp_results, q, links);

                trc_db_set_user_data(iter, TRUE, 0, iter_data);
                TAILQ_INSERT_TAIL(&test->iters.head, iter, links);
            }

            logic_expr_free(array[i]);
        }
        free(array);
    }

    /*
     * Generate wildcards for fake test.
     */
    if (trc_update_gen_test_wilds_fss(
                                0, test_entry, FALSE, 0, NULL,
                                flags | TRC_UPDATE_INTERSEC_WILDS) < 0)
        trc_update_generate_test_wilds(
                                    0, test, FALSE, 0, NULL,
                                    flags | TRC_UPDATE_INTERSEC_WILDS);

    trc_exp_results_free(results);

    p = NULL;
    q = NULL;
    r = NULL;

    /*
     * Transform wildcards into results marked by proper
     * tag expressions.
     */

    TAILQ_FOREACH(iter, &test->iters.head, links)
    {
        /*
         * In case of this fake test there is only one
         * expected result in fake iteration.
         */
        p = STAILQ_FIRST(&iter->exp_results);

        memset(&te_str, 0, sizeof(te_str));
        te_str.ptr = NULL;

        /*
         * After wildcards generation iterations are grouped
         * by results - so we can use it to group log expressions
         * by OR operation which are for the same resul.
         */
        if (trc_update_result_cmp(p, q) != 0)
        {
            if (r != NULL)
            {
                logic_expr_parse(r->tags_str, &r->tags_expr);
                STAILQ_INSERT_TAIL(results, r, links);
            }

            r = trc_exp_result_dup(p);
            logic_expr_free(r->tags_expr);
            r->tags_expr = NULL;
            free(r->tags_str);
            r->tags_str = NULL;
        }
        else
            te_string_append(&te_str, "%s|", r->tags_str);

        TAILQ_FOREACH(arg, &iter->args.head, links)
        {
            if (strlen(arg->value) > 0)
            {
                if (te_str.ptr != NULL &&
                    te_str.ptr[strlen(te_str.ptr) - 1] != '|')
                    te_string_append(&te_str, "&");

                if (strcmp(arg->value, "EXISTS") == 0)
                    te_string_append(&te_str, "%s", arg->name);
                else if (strcmp(arg->value, "NEXISTS") != 0)
                    te_string_append(&te_str, "%s=%s", arg->name,
                                     arg->value);
                else
                    te_string_append(&te_str, "!%s", arg->name);
            }
        }

        free(r->tags_str);
        if (te_str.ptr == NULL || strlen(te_str.ptr) == 0)
            r->tags_str = strdup("linux");
        else
            r->tags_str = te_str.ptr;

        if (TAILQ_NEXT(iter, links) == NULL)
        {
            logic_expr_parse(r->tags_str, &r->tags_expr);
            STAILQ_INSERT_TAIL(results, r, links);
        }

        q = p;
    }

    TAILQ_FOREACH(iter, &test->iters.head, links)
    {
        iter_data = trc_db_iter_get_user_data(iter, 0);
        trc_update_free_test_iter_data(iter_data);
    }

    trc_update_test_entry_free(test_entry);
    free(test_entry);
    trc_free_trc_test(test);
    free(test);

    return 0;
}

/**
 * Merge the same results having different tags into
 * single records.
 *
 * @param db_uid        TRC DB User ID
 * @param updated_tests Tests to be updated
 * @param flags         Flags
 *
 * @return Status code
 */
static te_errno
trc_update_simplify_results(unsigned int db_uid,
                            trc_update_tests_groups *updated_tests,
                            uint64_t flags)

{
    trc_update_tests_group      *group;
    trc_update_test_entry       *test_entry;
    trc_test_iter               *iter;
    trc_test_iter               *iter_aux;
    trc_update_test_iter_data   *iter_data;
    trc_update_test_iter_data   *iter_data_aux;
    trc_exp_results             *new_results = NULL;
    trc_exp_results             *new_res_aux = NULL;
    trc_exp_result              *p;
    trc_exp_result              *q;
    trc_exp_result              *tvar;
    logic_expr                  *tags_expr;

    UNUSED(flags);

    TAILQ_FOREACH(group, updated_tests, links)
    {
        TAILQ_FOREACH(test_entry, &group->tests, links)
        {
            if (flags & TRC_UPDATE_LOG_WILDS)
            {
                TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
                {
                    iter_data = trc_db_iter_get_user_data(iter, db_uid);
                    
                    if (iter_data == NULL || iter_data->to_save == FALSE)
                        continue;

                    new_results = &iter->exp_results;
                    trc_exp_results_free(new_results);
                    trc_exp_results_cpy(new_results,
                                        &iter_data->new_results);
                }
            }

            TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
            {
                iter_data = trc_db_iter_get_user_data(iter, db_uid);
                
                if (iter_data == NULL || iter_data->to_save == FALSE)
                    continue;

                if (iter_data->r_simple == RES_SIMPLE)
                    continue;

                if (flags & TRC_UPDATE_LOG_WILDS)
                    new_results = &iter->exp_results;
                else
                    new_results = &iter_data->new_results;

                for (iter_aux = TAILQ_NEXT(iter, links);
                     iter_aux != NULL;
                     iter_aux = TAILQ_NEXT(iter_aux, links))
                {
                    iter_data_aux =
                        trc_db_iter_get_user_data(iter_aux, db_uid);
                    
                    if (iter_data_aux == NULL ||
                        iter_data_aux->to_save == FALSE)
                        continue;

                    if (flags & TRC_UPDATE_LOG_WILDS)
                        new_res_aux = &iter_aux->exp_results;
                    else
                        new_res_aux = &iter_data_aux->new_results;

                    if (trc_update_results_cmp(new_results,
                                               new_res_aux) == 0)
                        iter_data_aux->r_simple = RES_TO_REPLACE;
                }

                if (flags & TRC_UPDATE_SIMPL_TAGS)
                    simplify_log_exprs(new_results, flags);

                STAILQ_FOREACH(p, new_results, links)
                {
                    for (q = STAILQ_NEXT(p, links);
                         q != NULL && (tvar = STAILQ_NEXT(q, links), 1);
                         q = tvar)
                        
                    {
                        if (trc_update_result_cmp_no_tags(p, q) == 0)
                        {
                            tags_expr = TE_ALLOC(sizeof(*tags_expr));
                            tags_expr->type = LOGIC_EXPR_OR;
                            tags_expr->u.binary.rhv = p->tags_expr;
                            tags_expr->u.binary.lhv = q->tags_expr;
                            
                            p->tags_expr = tags_expr;
                            q->tags_expr = NULL;
                            free(p->tags_str);

                            if (flags & TRC_UPDATE_TAGS_STR)
                            {
                                tqe_string *tqe_str;
                                te_string   te_str = TE_STRING_INIT;
                                
                                TAILQ_FOREACH(tqe_str, q->tags, links)
                                    tq_strings_add_uniq_dup(
                                                        p->tags,
                                                        tqe_str->v);

                                TAILQ_FOREACH(tqe_str, p->tags, links)
                                {
                                    if (te_str.ptr == NULL)
                                        te_string_append(&te_str, "%s",
                                                         tqe_str->v);
                                    else
                                        te_string_append(&te_str, "|%s",
                                                         tqe_str->v);
                                }
                                p->tags_str = strdup(te_str.ptr);
                                te_string_free(&te_str);
                            }
                            else
                                p->tags_str = logic_expr_to_str(
                                                        p->tags_expr);
                            STAILQ_REMOVE(new_results, q, trc_exp_result, links);
                            trc_exp_result_free(q);
                        }
                    }
                }

                iter_data->r_simple = RES_SIMPLE;

                /*
                 * Replace same results with simplified ones to
                 * avoid multiple processing of the same problem.
                 */
                for (iter_aux = TAILQ_NEXT(iter, links);
                     iter_aux != NULL;
                     iter_aux = TAILQ_NEXT(iter_aux, links))
                {
                    iter_data_aux =
                        trc_db_iter_get_user_data(iter_aux, db_uid);
                    
                    if (iter_data_aux == NULL ||
                        iter_data_aux->to_save == FALSE)
                        continue;

                    if (iter_data_aux->r_simple == RES_SIMPLE)
                        continue;

                    if (flags & TRC_UPDATE_LOG_WILDS)
                        new_res_aux = &iter_aux->exp_results;
                    else
                        new_res_aux = &iter_data_aux->new_results;

                    if (iter_data_aux->r_simple == RES_TO_REPLACE)
                    {
                        trc_exp_results_free(new_res_aux);
                        trc_exp_results_cpy(new_res_aux,
                                            new_results);
                        iter_data_aux->r_simple = RES_SIMPLE;
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * Apply updating rule of type @c TRC_UPDATE_RULE_VERDICT to a test
 * iteration.
 *
 * @param iter          Test iteration
 * @param db_uid        TRC DB user id
 * @param iter_data     Auxiliary data attached to the iteration
 * @param rule          TRC updating rule
 * @param flags         Flags
 */
static te_errno
trc_update_apply_rverdict(trc_test_iter *iter,
                          unsigned int db_uid,
                          trc_update_test_iter_data *iter_data,
                          trc_update_rule *rule, uint64_t flags)
{
    te_test_verdict         *dup_verdict;
    trc_exp_result          *iter_result;
    trc_exp_result_entry    *iter_rentry;
    te_test_verdict         *iter_verdict;

    UNUSED(iter_data);
    UNUSED(flags);

    if (strcmp_null(rule->old_v, rule->new_v) == 0 ||
        rule->old_v == NULL || !rule->apply)
        return 0;

    STAILQ_FOREACH(iter_result, &iter->exp_results, links)
    {
        TAILQ_FOREACH(iter_rentry, &iter_result->results, links)
        {
            TAILQ_FOREACH(iter_verdict, &iter_rentry->result.verdicts,
                          links)
            {
                if (strcmp_null(iter_verdict->str, rule->old_v) == 0)
                {
                    if (rule->new_v != NULL)
                    {
                        dup_verdict = TE_ALLOC(sizeof(*dup_verdict));
                        dup_verdict->str = strdup(rule->new_v);
                        TAILQ_INSERT_AFTER(&iter_rentry->result.verdicts,
                                           iter_verdict, dup_verdict,
                                           links);
                    }
                    else
                        dup_verdict = TAILQ_NEXT(iter_verdict, links);

                    TAILQ_REMOVE(&iter_rentry->result.verdicts,
                                 iter_verdict, links);
                    free(iter_verdict->str);
                    free(iter_verdict);

                    if ((flags & TRC_UPDATE_RULE_UPD_ONLY) &&
                        !iter_data->to_save)
                    {
                        trc_update_set_propagate_to_save(iter, TRUE,
                                                         db_uid, TRUE);
                        trc_update_set_iters_to_save(&iter->parent->iters,
                                                     db_uid, FALSE, TRUE);
                    }

                    iter_verdict = dup_verdict;
                    if (dup_verdict == NULL)
                        break;
                }
            }
        }
    }

    return 0;
}

/**
 * Apply updating rule of type @c TRC_UPDATE_RULE_RENTRY to a test
 * iteration.
 *
 * @param iter          Test iteration
 * @param db_uid        TRC DB user id
 * @param iter_data     Auxiliary data attached to the iteration
 * @param rule          TRC updating rule
 * @param flags         Flags
 */
static te_errno
trc_update_apply_rrentry(trc_test_iter *iter,
                         unsigned int db_uid,
                         trc_update_test_iter_data *iter_data,
                         trc_update_rule *rule, uint64_t flags)
{
    trc_exp_result_entry    *dup_rentry;
    trc_exp_result          *iter_result;
    trc_exp_result_entry    *iter_rentry;

    UNUSED(iter_data);
    UNUSED(flags);

    if (trc_update_rentry_cmp(rule->old_re, rule->new_re) == 0 ||
        rule->old_re == NULL || !rule->apply)
        return 0;

    STAILQ_FOREACH(iter_result, &iter->exp_results, links)
    {
        TAILQ_FOREACH(iter_rentry, &iter_result->results, links)
        {
            if (trc_update_rentry_cmp(iter_rentry,
                                      rule->old_re) == 0)
            {
                if (rule->new_re != NULL)
                {
                    dup_rentry = trc_exp_result_entry_dup(rule->new_re);
                    TAILQ_INSERT_AFTER(&iter_result->results, iter_rentry,
                                       dup_rentry, links);
                }
                else
                    dup_rentry = TAILQ_NEXT(iter_rentry, links);

                TAILQ_REMOVE(&iter_result->results, iter_rentry, links);
                trc_exp_result_entry_free(iter_rentry);
                free(iter_rentry);

                if ((flags & TRC_UPDATE_RULE_UPD_ONLY) &&
                    !iter_data->to_save)
                {
                    trc_update_set_propagate_to_save(iter, TRUE,
                                                     db_uid, TRUE);
                    trc_update_set_iters_to_save(&iter->parent->iters,
                                                 db_uid, FALSE, TRUE);
                }

                iter_rentry = dup_rentry;
                if (iter_rentry == NULL)
                    break;
            }
        }
    }
    return 0;
}

/**
 * Apply updating rule of type @c TRC_UPDATE_RULE_RESULT to a test
 * iteration.
 *
 * @param iter          Test iteration
 * @param db_uid        TRC DB user id
 * @param iter_data     Auxiliary data attached to the iteration
 * @param rule          TRC updating rule
 * @param flags         Flags
 */
static te_errno
trc_update_apply_rresult(trc_test_iter *iter,
                         unsigned int db_uid,
                         trc_update_test_iter_data *iter_data,
                         trc_update_rule *rule, uint64_t flags)
{
    trc_exp_result  *old_result = NULL;
    trc_exp_result  *new_result = NULL;
    trc_exp_result  *dup_result;
    trc_exp_result  *iter_result;

    UNUSED(iter_data);
    UNUSED(flags);

    if (rule->old_res != NULL)
        old_result = STAILQ_FIRST(rule->old_res);
    if (rule->new_res != NULL)
        new_result = STAILQ_FIRST(rule->new_res);

    if (trc_update_result_cmp(old_result, new_result) == 0 ||
        old_result == NULL || !rule->apply)
        return 0;

    STAILQ_FOREACH(iter_result, &iter->exp_results, links)
    {
        if (trc_update_result_cmp(iter_result, old_result) == 0)
        {
            if (new_result != NULL)
            {
                dup_result = trc_exp_result_dup(new_result);
                STAILQ_INSERT_AFTER(&iter->exp_results, iter_result,
                                    dup_result, links);
            }
            else
                dup_result = STAILQ_NEXT(iter_result, links);

            STAILQ_REMOVE(&iter->exp_results, iter_result,
                          trc_exp_result, links);
            trc_exp_result_free(iter_result);
            free(iter_result);

            if ((flags & TRC_UPDATE_RULE_UPD_ONLY) &&
                !iter_data->to_save)
            {
                trc_update_set_propagate_to_save(iter, TRUE, db_uid, TRUE);
                trc_update_set_iters_to_save(&iter->parent->iters,
                                             db_uid, FALSE, TRUE);
            }

            iter_result = dup_result;
            if (iter_result == NULL)
                break;
        }
    }
    return 0;
}

/**
 * Apply updating rule of type @c TRC_UPDATE_RULE_RESULTS to a
 * test iteration.
 *
 * @param iter          Test iteration
 * @param db_uid        TRC DB user id
 * @param iter_data     Auxiliary data attached to the iteration
 * @param rule          TRC updating rule
 * @param iter_rule_id  Iteration rule id
 * @param flags         Flags
 */
static te_errno
trc_update_apply_rresults(trc_test_iter *iter,
                          unsigned int db_uid,
                          trc_update_test_iter_data *iter_data,
                          trc_update_rule *rule,
                          int iter_rule_id, uint64_t flags)
{
    trc_exp_results             *results_dup;

    if (rule->apply &&
        ((rule->rule_id != 0 && iter_rule_id != 0 &&
          rule->rule_id == iter_rule_id) ||
         ((rule->def_res == NULL ||
           trc_update_result_cmp(
                              (struct trc_exp_result *)
                              iter->exp_default,
                              rule->def_res) == 0) &&
          (rule->old_res == NULL ||
           trc_update_results_cmp(
                               (trc_exp_results *)
                               &iter->exp_results,
                               rule->old_res) == 0) &&
          (rule->confl_res == NULL ||
           trc_update_results_cmp(
                               &iter_data->new_results,
                               rule->confl_res) == 0))) &&
        trc_update_results_cmp(&iter->exp_results,
                               rule->new_res) != 0)
    {
        results_dup = trc_exp_results_dup(rule->new_res);

        if ((flags & TRC_UPDATE_RULES_CONFL) &&
            !STAILQ_EMPTY(&iter->exp_results))
        {
            trc_exp_results_free(&iter_data->new_results);
            if (results_dup != NULL)
                memcpy(&iter_data->new_results, results_dup,
                       sizeof(*results_dup));
        }
        else
        {
            trc_exp_results_free(&iter->exp_results);
            if (results_dup != NULL)
                memcpy(&iter->exp_results, results_dup,
                       sizeof(*results_dup));
        }

        if ((flags & TRC_UPDATE_RULE_UPD_ONLY) &&
            !iter_data->to_save)
        {
            trc_update_set_propagate_to_save(iter, TRUE, db_uid, TRUE);
            trc_update_set_iters_to_save(&iter->parent->iters,
                                         db_uid, FALSE, TRUE);
        }

        free(results_dup);
    }

    return 0;
}

/**
 * Apply updating rules.
 *
 * @param db_uid        TRC DB User ID
 * @param updated_tests Tests to be updated
 * @param global_rules  Global updating rules
 * @param flags         Flags
 *
 * @return Status code
 */
static te_errno
trc_update_apply_rules(unsigned int db_uid,
                       trc_update_tests_groups *updated_tests,
                       trc_update_rules *global_rules,
                       uint64_t flags)
{
    trc_update_tests_group      *group;
    trc_update_test_entry       *test_entry;
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    char                        *value;
    int                          rule_id;
    trc_update_rule             *rule;
    trc_update_wilds_list_entry *wild;
    tqe_string                  *expr;
    te_bool                      rules_switch = FALSE;
    te_bool                      is_applicable = FALSE;

    UNUSED(flags);

    TAILQ_FOREACH(group, updated_tests, links)
    {
        TAILQ_FOREACH(test_entry, &group->tests, links)
        {
            TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
            {
                iter_data = trc_db_iter_get_user_data(iter, db_uid);
                
                if (iter_data == NULL ||
                    (iter_data->to_save == FALSE &&
                     !(flags & TRC_UPDATE_RULE_UPD_ONLY)))
                    continue;

                rule_id = 0;
                value = XML2CHAR(xmlGetProp(
                                    iter->node,
                                    CONST_CHAR2XML("user_attr")));
                if (value != NULL &&
                    strncmp(value, "rule_", 5) == 0)
                    rule_id = atoi(value + 5);

                rules_switch = FALSE;

                for (rule = TAILQ_FIRST(global_rules);
                     !(rule == NULL && rules_switch == TRUE);
                     rule = TAILQ_NEXT(rule, links))
                {
                    if (rule == NULL)
                    {
                        if (group->rules != NULL)
                            rule = TAILQ_FIRST(group->rules);
                        rules_switch = TRUE;

                        if (rule == NULL)
                            break;
                    }

                    if (rule->rule_id != rule_id &&
                        rule->rule_id != 0 &&
                        rule_id != 0)
                        continue;

                    is_applicable = FALSE;

                    if (rule->wilds != NULL &&
                        !SLIST_EMPTY(rule->wilds))
                    {
                        SLIST_FOREACH(wild, rule->wilds, links)
                        {
                            assert(wild->args != NULL);
                            if (test_iter_args_match(
                                                wild->args,
                                                iter_data->args_n,
                                                iter_data->args,
                                                wild->is_strict) !=
                                                    ITER_NO_MATCH)
                                is_applicable = TRUE;
                        }
                    }
                    else if (rule->match_exprs != NULL &&
                             !TAILQ_EMPTY(rule->match_exprs))
                    {
                        TAILQ_FOREACH(expr, rule->match_exprs,
                                      links)
                        {
                            /*
                             * Sorry, not implemented yet.
                             */
                        }
                    }
                    else
                        is_applicable = TRUE;

                    if (is_applicable)
                    {
                        switch(rule->type)
                        {
                            case TRC_UPDATE_RULE_RESULTS:
                                trc_update_apply_rresults(iter,
                                                          db_uid,
                                                          iter_data,
                                                          rule, rule_id,
                                                          flags);
                                break;

                            case TRC_UPDATE_RULE_RESULT:
                                trc_update_apply_rresult(iter,
                                                         db_uid,
                                                         iter_data,
                                                         rule, flags);
                                break;

                            case TRC_UPDATE_RULE_ENTRY:
                                trc_update_apply_rrentry(iter,
                                                         db_uid,
                                                         iter_data,
                                                         rule, flags);
                                break;

                            case TRC_UPDATE_RULE_VERDICT:
                                trc_update_apply_rverdict(iter,
                                                          db_uid,
                                                          iter_data,
                                                          rule, flags);
                                break;

                            default:
                                break;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * Determine whether a given rule matches to a given iteration.
 *
 * @param rule      TRC updating rule
 * @param iter      Test iteration
 * @param db_uid    TRC DB UID
 *
 * @return @c TRUE or @c FALSE
 */
static te_bool
trc_update_rule_match_iter(trc_update_rule *rule,
                           trc_test_iter *iter,
                           int db_uid)
{
    trc_exp_result              *iter_result;
    trc_exp_result_entry        *iter_rentry;
    te_test_verdict             *iter_verdict;
    trc_exp_result              *rule_result;
    trc_update_test_iter_data   *iter_data;

    switch (rule->type)
    {
        case TRC_UPDATE_RULE_RESULTS:
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            if ((rule->rule_id != 0 && iter_data->rule_id != 0 &&
                 rule->rule_id == iter_data->rule_id) ||
                ((rule->def_res == NULL ||
                  trc_update_result_cmp(
                                  (struct trc_exp_result *)
                                  iter->exp_default,
                                  rule->def_res) == 0) &&
                 (rule->old_res == NULL ||
                  trc_update_results_cmp(
                                   (trc_exp_results *)
                                   &iter->exp_results,
                                   rule->old_res) == 0) &&
                 (rule->confl_res == NULL ||
                  trc_update_results_cmp(
                                   &iter_data->new_results,
                                   rule->confl_res) == 0)))
                return TRUE;
            break;

        case TRC_UPDATE_RULE_RESULT:
            rule_result = STAILQ_FIRST(rule->old_res);
            if (rule_result == NULL)
                return TRUE;
            STAILQ_FOREACH(iter_result, &iter->exp_results, links)
                if (trc_update_result_cmp(iter_result, rule_result) == 0)
                    return TRUE;
          break;

        case TRC_UPDATE_RULE_ENTRY:
            if (rule->old_re == NULL)
                return TRUE;
            STAILQ_FOREACH(iter_result, &iter->exp_results, links)
                TAILQ_FOREACH(iter_rentry, &iter_result->results, links)
                    if (trc_update_rentry_cmp(iter_rentry,
                                              rule->old_re) == 0)
                    return TRUE;
          break;

        case TRC_UPDATE_RULE_VERDICT:
            STAILQ_FOREACH(iter_result, &iter->exp_results, links)
                TAILQ_FOREACH(iter_rentry, &iter_result->results, links)
                    TAILQ_FOREACH(iter_verdict,
                                  &iter_rentry->result.verdicts,
                                  links)
                        if (strcmp_null(iter_verdict->str,
                                        rule->old_v) == 0)
                            return TRUE;
        break;

        default:
        break;
    }

    return FALSE;
}

/**
 * Generate <args> tags for a rule (i.e. determine
 * wildcards describing all the iterations the rule
 * can be applied to).
 *
 * @param test_entry  TRC Update test entry
 * @param db_uid      TRC DB UID
 * @param rule        TRC updating rule
 * @param flags       Flags
 *
 * @return Status code
 */
static te_errno
trc_update_rule_gen_args(trc_update_test_entry *test_entry,
                         int db_uid,
                         trc_update_rule *rule,
                         uint64_t flags)
{
    trc_update_args_groups          wildcards;
    trc_update_args_group          *args_group;
    trc_update_wilds_list_entry    *wilds_list_entry;
    trc_test_iter                  *iter;
    trc_update_test_iter_data      *iter_data = NULL;
    int                             rc;

    rc = 0;

    do {
        TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
        {
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            if (iter_data == NULL || iter_data->to_save == FALSE)
                continue;

            iter_data->in_wildcard = FALSE;
            if (trc_update_rule_match_iter(rule, iter, db_uid))
                iter_data->results_id = 1;
            else
                iter_data->results_id = 2;
        }

        memset(&wildcards, 0, sizeof(wildcards));
        SLIST_INIT(&wildcards);

        if (rc == 0)
            rc = trc_update_gen_test_wilds_fss(db_uid, test_entry,
                                               TRUE, 2, &wildcards, flags);
        else
        {
            rc = trc_update_generate_test_wilds(db_uid, test_entry->test,
                                                TRUE, 2, &wildcards, flags);
            if (rc < 0)
                break;
        }
    } while (rc < 0);

    rule->wilds = TE_ALLOC(sizeof(*(rule->wilds)));
    SLIST_INIT(rule->wilds);

    SLIST_FOREACH(args_group, &wildcards, links)
    {
        if (args_group->group_id != 1)
            continue;

        wilds_list_entry = TE_ALLOC(sizeof(*wilds_list_entry));
        wilds_list_entry->is_strict = TRUE;
        assert(args_group->args != NULL);
        wilds_list_entry->args = args_group->args;
        args_group->args = NULL;
        SLIST_INSERT_HEAD(rule->wilds, wilds_list_entry, links);
    }

    trc_update_args_groups_free(&wildcards);

    TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
    {
        iter_data = trc_db_iter_get_user_data(iter, db_uid);
        if (iter_data == NULL || iter_data->to_save == FALSE)
            continue;
        iter_data->in_wildcard = FALSE;
    }

    return 0;
}

/**
 * Get updating rule from XML.
 *
 * @param rule_node     XML node with updating rule
 * @param rule          Where to save updating rule
 *
 * @return Status code
 */
static te_errno
trc_update_load_rule(xmlNodePtr rule_node, trc_update_rule *rule)
{
#define IF_RESULTS(result_) \
    if (xmlStrcmp(first_child_node->name,                       \
                  CONST_CHAR2XML("results")) == 0)              \
    {                                                           \
        if (rule->result_ ## _res != NULL)                      \
        {                                                       \
            ERROR("Duplicated <%s> node in TRC updating rules " \
                  "XML file", (char *)rule_section_node->name); \
            err_ = TE_EFMT;                                     \
        }                                                       \
        else                                                    \
        {                                                       \
            rule->result_ ## _res =                             \
                        TE_ALLOC(sizeof(trc_exp_results));      \
            STAILQ_INIT(rule->result_ ## _res);                 \
            get_expected_results(&first_child_node,             \
                                 rule->result_ ## _res);        \
            rtype_ = TRC_UPDATE_RULE_RESULTS;                   \
        }                                                       \
    }                                                           \

#define GET_RULE_CHECK \
    do {                                                            \
        if (err_ == 0 && !(rtype_ == TRC_UPDATE_RULE_RESULTS &&     \
              rule->type == TRC_UPDATE_RULE_RESULT))                \
        {                                                           \
            if (rtype_ != rule->type &&                             \
                rule->type != TRC_UPDATE_RULE_UNKNOWN)              \
            {                                                       \
                ERROR("<rule> node contains different type of "     \
                      "tags as direct children of <old>, "          \
                      "<conflicts> or <new>");                      \
                trc_update_rule_free(rule);                         \
                err_ = TE_EFMT;                                     \
            }                                                       \
            else                                                    \
                rule->type = rtype_;                                \
        }                                                           \
        if (err_ != 0)                                              \
        {                                                           \
            trc_update_rule_free(rule);                             \
            return err_;                                            \
        }                                                           \
    } while (0)

#define GET_RULE_CONFL \
    do {                                                            \
        int               err_ = 0;                                 \
        trc_update_rtype  rtype_ = TRC_UPDATE_RULE_UNKNOWN;         \
                                                                    \
        if (first_child_node == NULL)                               \
            break;                                                  \
        IF_RESULTS(confl)                                           \
        else                                                        \
        {                                                           \
            ERROR("Incorrect <%s> node in <conflicts> section of "  \
                  "TRC update rule",                                \
                  (char *)first_child_node->name);                  \
            err_ =  TE_EFMT;                                        \
        }                                                           \
        GET_RULE_CHECK;                                             \
    } while (0)

#define GET_RULE_SEC(result_) \
    do {                                                            \
        int               err_ = 0;                                 \
        trc_update_rtype  rtype_ = TRC_UPDATE_RULE_UNKNOWN;         \
                                                                    \
        if (first_child_node == NULL)                               \
            break;                                                  \
        IF_RESULTS(result_)                                         \
        else if (xmlStrcmp(first_child_node->name,                  \
                           CONST_CHAR2XML("result")) == 0)          \
        {                                                           \
            if (rule->result_ ## _re != NULL)                       \
            {                                                       \
                ERROR("Duplicated <%s> node in TRC updating rules " \
                      "XML file", (char *)rule_section_node->name); \
                err_ = TE_EFMT;                                     \
            }                                                       \
            else                                                    \
            {                                                       \
                rule->result_ ## _re =                              \
                            TE_ALLOC(sizeof(trc_exp_result_entry)); \
                get_expected_rentry(first_child_node,               \
                                    rule->result_ ## _re);          \
                rtype_ = TRC_UPDATE_RULE_ENTRY;                     \
            }                                                       \
        }                                                           \
        else if (xmlStrcmp(first_child_node->name,                  \
                           CONST_CHAR2XML("verdict")) == 0)         \
        {                                                           \
            if (rule->result_ ## _v != NULL)                        \
            {                                                       \
                ERROR("Duplicated <%s> node in TRC updating rules " \
                      "XML file", (char *)rule_section_node->name); \
                return TE_EFMT;                                     \
            }                                                       \
            else                                                    \
            {                                                       \
                get_expected_verdict(first_child_node,              \
                                     &rule->result_ ## _v);         \
                rtype_ = TRC_UPDATE_RULE_VERDICT;                   \
            }                                                       \
        }                                                           \
        else                                                        \
        {                                                           \
            ERROR("Incorrect <%s> node in <%s> section of "         \
                  "TRC update rule",                                \
                  (char *)first_child_node->name,                   \
                  (char *)rule_section_node->name);                 \
            err_ =  TE_EFMT;                                        \
        }                                                           \
        GET_RULE_CHECK;                                             \
    } while (0)

    xmlNodePtr              rule_section_node;
    xmlNodePtr              first_child_node;
    char                   *value;
    trc_exp_result_entry   *entry;
    tqe_string             *tqe_str;

    trc_update_wilds_list_entry *wilds_entry;

    rule_section_node = xmlFirstElementChild(rule_node);

    value = XML2CHAR(xmlGetProp(rule_node,
                                CONST_CHAR2XML("id")));
    if (value != NULL)
        rule->rule_id = atoi(value);

    rule->type = TRC_UPDATE_RULE_UNKNOWN;

    value = XML2CHAR(xmlGetProp(rule_node,
                                CONST_CHAR2XML("type")));
    if (value != NULL && strcmp(value, "result") == 0)
        rule->type = TRC_UPDATE_RULE_RESULT;

    while (rule_section_node != NULL)
    {
        first_child_node = xmlFirstElementChild(rule_section_node);

        if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("args")) == 0)
        {
            if (rule->wilds == NULL)
            {
                rule->wilds = TE_ALLOC(sizeof(*(rule->wilds)));
                SLIST_INIT(rule->wilds);
            }

            wilds_entry = TE_ALLOC(sizeof(*wilds_entry));
            wilds_entry->args = TE_ALLOC(sizeof(*(wilds_entry->args)));
            TAILQ_INIT(&wilds_entry->args->head);
            get_test_args(&first_child_node, wilds_entry->args);

            value = XML2CHAR(xmlGetProp(rule_section_node,
                                        CONST_CHAR2XML("type")));

            if (value != NULL && strcmp(value, "strict") == 0)
                wilds_entry->is_strict = TRUE;
            else
                wilds_entry->is_strict = FALSE;

            SLIST_INSERT_HEAD(rule->wilds, wilds_entry, links);
        }
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("match_expr")) == 0)
        {
            if (rule->match_exprs == NULL)
            {
                rule->match_exprs = TE_ALLOC(sizeof(*(rule->match_exprs)));
                TAILQ_INIT(rule->match_exprs);
            }

            tqe_str = TE_ALLOC(sizeof(*tqe_str));
            trc_db_get_text_content(rule_section_node,
                                    &tqe_str->v);
            TAILQ_INSERT_TAIL(rule->match_exprs, tqe_str, links);
        }
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("defaults")) == 0)
        {
            if (rule->def_res != NULL)
            {
                ERROR("Duplicated defaults node in TRC updating "
                      "rules XML file");
                return TE_EFMT;
            }
            else
            {
                value = XML2CHAR(xmlGetProp(rule_section_node,
                                            CONST_CHAR2XML("value")));
                rule->def_res = TE_ALLOC(sizeof(trc_exp_result));
                TAILQ_INIT(&rule->def_res->results);
                if (value == NULL)
                     get_expected_result(rule_section_node,
                                         rule->def_res, TRUE);
                else
                {
                    entry = TE_ALLOC(sizeof(*entry));
                    if (entry == NULL)
                        return TE_ENOMEM;
                    te_test_result_init(&entry->result);
                    TAILQ_INSERT_TAIL(&rule->def_res->results,
                                      entry, links);
                    te_test_str2status(value, &entry->result.status);
                    TAILQ_INIT(&entry->result.verdicts);
                }
            }
        }
        else if (xmlStrcmp(rule_section_node->name,
                           CONST_CHAR2XML("old")) == 0)
            GET_RULE_SEC(old);
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("conflicts")) == 0)
            GET_RULE_CONFL;
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("new")) == 0)
        {
            GET_RULE_SEC(new);
            rule->apply = TRUE;
        }
        else
        {
            ERROR("Unexpected %s node in <rule> node of "
                  "TRC updating rules XML file",
                  XML2CHAR(rule_section_node->name));
            return TE_EFMT;
        }

        rule_section_node = xmlNextElementSibling(rule_section_node);
    }

    return 0;
#undef GET_RULE_SEC
#undef GET_RULE_CONFL
#undef GET_RULE_CHECK
#undef IF_RESULTS
}

/**
 * Load updating rules from file.
 *
 * @param filename          Path to file
 * @param updated_tests     Tests to be updated
 * @param global_rules      Global updating rules
 * @param flags             Flags
 *
 * @return Status code
 */
static te_errno
trc_update_load_rules(char *filename,
                      trc_update_tests_groups *updated_tests,
                      trc_update_rules *global_rules,
                      uint64_t flags)
{
    xmlParserCtxtPtr        parser;
    xmlDocPtr               xml_doc;
    xmlNodePtr              root;
    xmlNodePtr              test_node;
    xmlNodePtr              rule_node;
    char                   *test_name = NULL;
    trc_update_tests_group *group = NULL;
    trc_update_rule        *rule = NULL;

#if HAVE_XMLERROR
    xmlError           *err;
#endif
    te_errno            rc = 0;

    UNUSED(flags);

    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return TE_ENOMEM;
    }
    
    xml_doc = xmlCtxtReadFile(parser, filename, NULL,
                              XML_PARSE_NONET | XML_PARSE_NOBLANKS);

    if (xml_doc == NULL)
    {
#if HAVE_XMLERROR
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing configuration file:\n"
              "    %s:%d\n    %s", filename,
              err->line, err->message);
#else
        ERROR("Error occured during parsing configuration file:\n"
              "%s", filename);
#endif
        xmlFreeParserCtxt(parser);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    root = xmlDocGetRootElement(xml_doc);
    if (root == NULL)
        return 0;

    if (xmlStrcmp(root->name, CONST_CHAR2XML("update_rules")) != 0)
    {
        ERROR("Unexpected root element of the TRC updating rules "
              "XML file");
        rc = TE_RC(TE_TRC, TE_EFMT);
        goto exit;
    }

    test_node = xmlFirstElementChild(root);
    if (xmlStrcmp(test_node->name, CONST_CHAR2XML("command")) == 0)
        test_node = xmlNextElementSibling(test_node);
    
    while (test_node != NULL)
    {
        if (xmlStrcmp(test_node->name, CONST_CHAR2XML("test")) != 0)
        {
            if (xmlStrcmp(test_node->name, CONST_CHAR2XML("rule")) == 0)
            {
                /*
                 * Rules not belonging to any test explicitly are
                 * considered as global ones
                 */
                rule = TE_ALLOC(sizeof(*rule));
                
                if ((rc = trc_update_load_rule(test_node, rule)) != 0)
                {
                    ERROR("Loading rule from file failed");
                    rc = TE_RC(TE_TRC, TE_EFMT);
                    goto exit;
                }

                TAILQ_INSERT_TAIL(global_rules, rule, links);
            }
            else
            {
                ERROR("Unexpected %s element of the TRC updating rules "
                     "XML file", test_node->name);
                rc = TE_RC(TE_TRC, TE_EFMT);
                goto exit;
            }
        }
        else
        {
            test_name = XML2CHAR(xmlGetProp(test_node, BAD_CAST "path"));
            
            TAILQ_FOREACH(group, updated_tests, links)
                if (strcmp(test_name, group->path) == 0)
                    break;
            
            if (group != NULL)
            {
                rule_node = xmlFirstElementChild(test_node);    

                if (group->rules == NULL)
                {
                    group->rules = TE_ALLOC(sizeof(*(group->rules)));
                    TAILQ_INIT(group->rules);
                }

                while (rule_node != NULL)
                {
                    if (xmlStrcmp(rule_node->name,
                                  CONST_CHAR2XML("rule")) != 0)
                    {
                         ERROR("Unexpected %s element of the TRC updating "
                               "rules XML file", rule_node->name);
                         rc = TE_RC(TE_TRC, TE_EFMT);
                         goto exit;
                    }

                    rule = TE_ALLOC(sizeof(*rule));
                    
                    if ((rc = trc_update_load_rule(rule_node, rule)) != 0)
                    {
                        ERROR("Loading rule from file for test %s failed",
                              group->path);
                        rc = TE_RC(TE_TRC, TE_EFMT);
                        goto exit;
                    }

                    TAILQ_INSERT_TAIL(group->rules, rule, links);

                    rule_node = xmlNextElementSibling(rule_node);
                }
            }
        }

        test_node = xmlNextElementSibling(test_node);
    }

exit:
    xmlFreeParserCtxt(parser);
    xmlFreeDoc(xml_doc);
    
    return rc;
}

/**
 * Delete all updating rules.
 *
 * @param db_uid            TRC DB User ID
 * @param updated_tests     Tests to be updated
 */
static void
trc_update_clear_rules(unsigned int db_uid,
                       trc_update_tests_groups *updated_tests)
{
    trc_update_tests_group      *group = NULL;
    trc_update_test_entry       *test = NULL;
    trc_test_iter               *iter = NULL;
    trc_update_test_iter_data   *iter_data = NULL;

    if (updated_tests == NULL)
        return;

    TAILQ_FOREACH(group, updated_tests, links)
    {
        trc_update_rules_free(group->rules);
        TAILQ_FOREACH(test, &group->tests, links)
        {
            TAILQ_FOREACH(iter, &test->test->iters.head, links)
            {
                iter_data = trc_db_iter_get_user_data(iter, db_uid);

                if (iter_data == NULL)
                    continue;

                iter_data->rule = NULL;
            }
        }
    }
}

/**
 * Generate updating rules of type @c TRC_UPDATE_RULE_VERDICT
 * for a given iteration.
 *
 * @param iter      Test iteration
 * @param rules     Where to store generated rules
 * @param flags     Flags
 */
static te_errno
trc_update_gen_rverdict(trc_test_iter *iter,
                        trc_update_rules *rules,
                        uint64_t flags)
{
    trc_exp_result              *result;
    trc_exp_result_entry        *rentry;
    te_test_verdict             *verdict;
    trc_update_rule             *rule;
    int                          rc;

    STAILQ_FOREACH(result, &iter->exp_results, links)
    {
        TAILQ_FOREACH(rentry, &result->results, links)
        {
            TAILQ_FOREACH(verdict, &rentry->result.verdicts, links)
            {
                rule = TE_ALLOC(sizeof(*rule));
                rule->type = TRC_UPDATE_RULE_VERDICT;
                rule->apply = FALSE;
                rule->old_v = strdup(verdict->str);
                if (flags & TRC_UPDATE_COPY_OLD)
                    rule->new_v = strdup(rule->old_v);

                rc = trc_update_ins_rule(rule, rules,
                                         &trc_update_rules_cmp);
                if (rc != 0)
                    trc_update_rule_free(rule);
            }
        }
    }

    return 0;
}

/**
 * Generate updating rules of type @c TRC_UPDATE_RULE_ENTRY
 * for a given iteration.
 *
 * @param iter      Test iteration
 * @param rules     Where to store generated rules
 * @param flags     Flags
 */
static te_errno
trc_update_gen_rrentry(trc_test_iter *iter,
                       trc_update_rules *rules,
                       uint64_t flags)
{
    trc_exp_result              *result;
    trc_exp_result_entry        *rentry;
    trc_update_rule             *rule;
    int                          rc;

    STAILQ_FOREACH(result, &iter->exp_results, links)
    {
        TAILQ_FOREACH(rentry, &result->results, links)
        {
            rule = TE_ALLOC(sizeof(*rule));
            rule->type = TRC_UPDATE_RULE_ENTRY;
            rule->apply = FALSE;
            rule->old_re = trc_exp_result_entry_dup(rentry);

            if (flags & TRC_UPDATE_COPY_OLD)
                rule->new_re =
                    trc_exp_result_entry_dup(rule->old_re);

            rc = trc_update_ins_rule(rule, rules, &trc_update_rules_cmp);
            if (rc != 0)
                trc_update_rule_free(rule);
        }
    }

    return 0;
}

/**
 * Generate updating rules of type @c TRC_UPDATE_RULE_RESULT
 * for a given iteration.
 *
 * @param iter      Test iteration
 * @param rules     Where to store generated rules
 * @param flags     Flags
 */
static te_errno
trc_update_gen_rresult(trc_test_iter *iter,
                       trc_update_rules *rules,
                       uint64_t flags)
{
    trc_exp_result              *result;
    trc_exp_result              *res_dup;
    trc_update_rule             *rule;
    int                          rc;

    STAILQ_FOREACH(result, &iter->exp_results, links)
    {
        rule = TE_ALLOC(sizeof(*rule));
        rule->type = TRC_UPDATE_RULE_RESULT;
        rule->apply = FALSE;
        rule->old_res = TE_ALLOC(sizeof(trc_exp_results));
        STAILQ_INIT(rule->old_res);
        res_dup = trc_exp_result_dup(result);
        STAILQ_INSERT_TAIL(rule->old_res, res_dup, links);

        if (flags & TRC_UPDATE_COPY_OLD)
            rule->new_res =
                trc_exp_results_dup(rule->old_res);

        rc = trc_update_ins_rule(rule, rules, &trc_update_rules_cmp);
        if (rc != 0)
            trc_update_rule_free(rule);
    }

    return 0;
}

/**
 * Generate updating rule of type @c TRC_UPDATE_RULE_RESULTS
 * for a given iteration.
 *
 * @param iter          Test iteration
 * @param iter_data     Test iteration data
 * @param rules         Where to store generated rules
 * @param flags         Flags
 */
static trc_update_rule *
trc_update_gen_rresults(trc_test_iter *iter,
                        trc_update_test_iter_data *iter_data,
                        trc_update_rules *rules,
                        uint64_t flags)
{
    trc_exp_result              *p;
    trc_exp_result              *q;
    trc_exp_result              *prev;
    te_bool                      was_changed;
    te_bool                      set_confls;
    trc_update_rule             *rule;
    int                          rc;

    rule = TE_ALLOC(sizeof(*rule));
    rule->def_res =
        trc_exp_result_dup((struct trc_exp_result *)
                                      iter->exp_default);
    rule->old_res =
        trc_exp_results_dup(&iter->exp_results);
    rule->confl_res = trc_exp_results_dup(
                                &iter_data->new_results);
    if (!STAILQ_EMPTY(rule->confl_res))
        rule->apply = TRUE;
    else
        rule->apply = FALSE;

    rule->new_res = TE_ALLOC(sizeof(*(rule->new_res)));
    STAILQ_INIT(rule->new_res);

    set_confls = FALSE;

    p = NULL;

    if ((((flags & TRC_UPDATE_COPY_OLD_FIRST) &&
         (flags & TRC_UPDATE_COPY_BOTH)) ||
         (!(flags & TRC_UPDATE_COPY_BOTH) &&
          !(flags & TRC_UPDATE_COPY_OLD_FIRST)) ||
         STAILQ_EMPTY(&iter->exp_results) ||
         !(flags & TRC_UPDATE_COPY_OLD)) &&
        (flags & TRC_UPDATE_COPY_CONFLS))
    {
        /* Do not forget about reverse order
         * of expected results in memory */
        set_confls = TRUE;
        p = STAILQ_FIRST(&iter_data->new_results);
    }
    else if (flags & TRC_UPDATE_COPY_OLD)
        p = STAILQ_FIRST(&iter->exp_results);

    q = NULL;
    prev = NULL;
    was_changed = FALSE;

    while (TRUE)
    {
        if (p == NULL && !was_changed)
        {
            if (STAILQ_EMPTY(rule->new_res) ||
                (flags & TRC_UPDATE_COPY_BOTH))
            {
                if (!set_confls &&
                    (flags & TRC_UPDATE_COPY_CONFLS)) 
                    p = STAILQ_FIRST(
                                &iter_data->new_results);
                else if (set_confls &&
                         (flags & TRC_UPDATE_COPY_OLD))
                    p = STAILQ_FIRST(&iter->exp_results);
            }

            set_confls = !set_confls;
            was_changed = TRUE;
        }

        if (p == NULL)
            break;

        q = trc_exp_result_dup(p);
        if (prev == NULL || (set_confls && !was_changed))
            STAILQ_INSERT_TAIL(rule->new_res, q, links);
        else
            STAILQ_INSERT_AFTER(rule->new_res, prev, q, links);

        /* Taking into account the fact that expected
         * results are loaded/saved in reverse order */
        if (!set_confls || (set_confls && prev == NULL &&
                            !was_changed))
            prev = q;
        p = STAILQ_NEXT(p, links);
    }

    rule->type = TRC_UPDATE_RULE_RESULTS;
    rc = trc_update_ins_rule(rule, rules, &trc_update_rules_cmp);
    
    if (rc != 0)
    {
        trc_update_rule_free(rule);
        return NULL;
    }

    return rule;
}

/**
 * Generate TRC updating rules.
 *
 * @param db_uid        TRC DB User ID
 * @param updated_tests Tests to be updated
 * @param flags         Flags
 *
 * @return Status code
 */
static te_errno
trc_update_gen_rules(unsigned int db_uid,
                     trc_update_tests_groups *updated_tests,
                     uint64_t flags)
{
    trc_update_tests_group      *group = NULL;
    trc_update_test_entry       *test1 = NULL;
    trc_update_test_entry       *test2 = NULL;
    trc_test_iter               *iter1 = NULL;
    trc_test_iter               *iter2 = NULL;
    trc_update_test_iter_data   *iter_data1 = NULL;
    trc_update_test_iter_data   *iter_data2 = NULL;
    trc_update_rule             *rule;
    int                          cur_rule_id = 0;

    TAILQ_FOREACH(group, updated_tests, links)
    {
        if (group->rules == NULL)
        {
            group->rules = TE_ALLOC(sizeof(trc_update_rules));
            TAILQ_INIT(group->rules);
        }

        TAILQ_FOREACH(test1, &group->tests, links)
        {
            TAILQ_FOREACH(iter1, &test1->test->iters.head, links)
            {
                iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);

                if (iter_data1 == NULL || iter_data1->to_save == FALSE)
                    continue;

                if ((!STAILQ_EMPTY(&iter_data1->new_results) ||
                     flags & TRC_UPDATE_RULES_ALL))
                {
                    if (flags & TRC_UPDATE_RVERDICT)
                        trc_update_gen_rverdict(iter1, group->rules,
                                                flags);

                    if (flags & TRC_UPDATE_RRENTRY)
                        trc_update_gen_rrentry(iter1, group->rules,
                                               flags);

                    if (flags & TRC_UPDATE_RRESULT)
                        trc_update_gen_rresult(iter1, group->rules,
                                               flags);

                    if ((flags & TRC_UPDATE_RRESULTS) &&
                        iter_data1->rule == NULL)
                        rule = trc_update_gen_rresults(iter1,
                                                       iter_data1,
                                                       group->rules,
                                                       flags);
                    else
                        continue;

                    if (rule == NULL)
                        continue;

                    if (flags & TRC_UPDATE_USE_RULE_IDS)
                        cur_rule_id++;
                    rule->rule_id = cur_rule_id;

                    iter_data1->rule = rule;
                    iter_data1->rule_id = rule->rule_id;

                    for (test2 = test1; test2 != NULL;
                         test2 = TAILQ_NEXT(test2, links))
                    {
                        if (test2 == test1)
                            iter2 = TAILQ_NEXT(iter1, links);
                        else
                            iter2 = TAILQ_FIRST(&test2->test->iters.head);

                        for ( ; iter2 != NULL;
                             iter2 = TAILQ_NEXT(iter2, links))
                        {
                            iter_data2 = trc_db_iter_get_user_data(
                                            iter2, db_uid);

                            if (iter_data2 == NULL ||
                                iter_data2->rule != NULL ||
                                iter_data2->to_save == FALSE)
                                continue;

                            if (trc_update_result_cmp(
                                    (struct trc_exp_result *)
                                        iter1->exp_default,
                                    (struct trc_exp_result *)
                                        iter2->exp_default) == 0 &&
                                trc_update_results_cmp(
                                    &iter_data1->new_results,
                                    &iter_data2->new_results) == 0 &&
                                trc_update_results_cmp(
                                    &iter1->exp_results,
                                    &iter2->exp_results) == 0)
                            {
                                iter_data2->rule = rule;
                                iter_data2->rule_id = rule->rule_id;
                            }
                        }
                    }
                }
            }

            if (flags & TRC_UPDATE_RULE_ARGS)
            {
                TAILQ_FOREACH(rule, group->rules, links)
                {
                    if (rule->wilds == NULL)
                        trc_update_rule_gen_args(test1, db_uid,
                                                 rule, flags);
                }
            }
        }
    }

    return 0;
}

/**
 * Group test iterations by results.
 *
 * @param db_uid    TRC DB User ID
 * @param test      TRC test
 *
 * @return Count of groups
 */
static int
trc_update_group_test_iters(unsigned int db_uid,
                            trc_test *test)
{
    trc_test_iter               *iter1 = NULL;
    trc_test_iter               *iter2 = NULL;
    trc_update_test_iter_data   *iter_data1 = NULL;
    trc_update_test_iter_data   *iter_data2 = NULL;
    int                          max_id = 0;

    TAILQ_FOREACH(iter1, &test->iters.head, links)
    {
        iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);

        if (iter_data1 == NULL || iter_data1->to_save == FALSE ||
            iter_data1->results_id != 0)
            continue;

        iter_data1->results_id = ++max_id;

        for (iter2 = TAILQ_NEXT(iter1, links); iter2 != NULL;
             iter2 = TAILQ_NEXT(iter2, links))
        {
            iter_data2 = trc_db_iter_get_user_data(
                            iter2, db_uid);

            if (iter_data2 == NULL || iter_data2->results_id != 0 ||
                iter_data2->to_save == FALSE)
                continue;

            if (trc_update_results_cmp(&iter1->exp_results,
                                       &iter2->exp_results) == 0 &&
                trc_update_result_cmp((struct trc_exp_result *)
                                            iter1->exp_default,
                                      (struct trc_exp_result *)
                                            iter2->exp_default) == 0)
                iter_data2->results_id = iter_data1->results_id;
        }
    }

    return max_id;
}

/**
 * Get all possible combinations of arguments (not values
 * of arguments) for iterations with a given kind of result.
 *
 * @param db_uid        TRC DB User ID
 * @param test          Test
 * @param results_id    Number of a specific kind of results
 * @param args_groups   Array of arguments groups to be filled
 *
 * @return Status code
 */
static te_errno
trc_update_get_iters_args_combs(unsigned int db_uid,
                                trc_test *test,
                                int results_id,
                                trc_update_args_groups *args_groups)
{
    trc_test_iter               *iter = NULL;
    trc_update_test_iter_data   *iter_data = NULL;
    trc_update_args_group       *args_group = NULL;
    
    TAILQ_FOREACH(iter, &test->iters.head, links)
    {
        iter_data = trc_db_iter_get_user_data(iter, db_uid);

        if (iter_data == NULL)
            continue;

        if (iter_data->results_id == results_id)
        {
            SLIST_FOREACH(args_group, args_groups, links)
            {
                if (test_iter_args_match(args_group->args,
                                         iter_data->args_n,
                                         iter_data->args,
                                         TRUE) != ITER_NO_MATCH)
                    break;
            }

            if (args_group != NULL)
                continue;

            args_group = TE_ALLOC(sizeof(*args_group));
            args_group->args = trc_update_args_wild_dup(&iter->args);
            args_group->group_id = results_id;

            SLIST_INSERT_HEAD(args_groups, args_group, links);
        }
    }

    return 0;
}

/**
 * Get next possible combination of arguments from a given
 * group of arguments. For example, if we have group of
 * arguments (a, b, c), this function helps us to enumerate
 * all possible combinations: (), (a), (b), (c), (a, b),
 * (a, c), (b, c), (a, b, c).
 *
 * @param args_comb         Current combination of arguments
 * @param args_count        Count of arguments in a group
 * @param cur_comb_count    Count of arguments in current
 *                          combination
 *
 * @return 0 if next combination was generated, -1 if current
 *         combination is the last one.
 */
static int
get_next_args_comb(te_bool *args_comb,
                   int args_count,
                   int *cur_comb_count)
{
    int j;
    int k;

    if (*cur_comb_count == args_count)
        return -1;
    else if (*cur_comb_count == 0)
    {
        args_comb[0] = TRUE;
        *cur_comb_count = 1;
    }
    else
    {
        for (j = args_count - 1; j >= args_count - *cur_comb_count; j--)
            if (args_comb[j] == FALSE)
                break;

        if (j < args_count - *cur_comb_count)
        {
            (*cur_comb_count)++;
            for (j = 0; j < args_count; j++)
                if (j < *cur_comb_count)
                    args_comb[j] = TRUE;
                else
                    args_comb[j] = FALSE;
        }
        else
        {
            k = args_count - j;

            while (args_comb[j] == FALSE)
                j--;

            args_comb[j] = FALSE;

            while (k > 0)
            {
                args_comb[++j] = TRUE;
                k--;
            }

            for (j++; j < args_count; j++)
                args_comb[j] = FALSE;
        }
    }

    return 0;
}

/**
 * Generate wildcard having boolean array determining
 * which arguments should be included in wildcard with
 * arguments, and iteration arguments.
 *
 * @param args_comb     Arguments combination
 * @param args_count    Count of arguments
 * @param iter_args     Iteration arguments
 * @param wildcard      Where to place generated wildcard
 *
 * @return Status code
 */
static te_errno
args_comb_to_wildcard(te_bool *args_comb,
                      int args_count,
                      trc_report_argument *iter_args,
                      trc_update_args_group *wildcard)
{
    int                  i = 0;
    trc_test_iter_arg   *arg;    

    for (i = 0; i < args_count; i++)
    {
        arg = TE_ALLOC(sizeof(*arg));
        arg->name = strdup(iter_args[i].name);

        if (args_comb == NULL || args_comb[i])
            arg->value = strdup(iter_args[i].value);
        else
        {
            arg->value = TE_ALLOC(1);
            arg->value[0] = '\0';
        }

        TAILQ_INSERT_TAIL(&wildcard->args->head, arg, links);
    }

    return 0;
}

/**
 * Specify in a given wildcard values for all the arguments
 * having only one possible value in all the iterations
 * descibed by this wildcard.
 *
 * @param db_uid    TRC DB user id
 * @param test      Test
 * @param wildcard  Wildcard
 *
 * @return Status code
 */
static te_errno
trc_update_extend_wild(unsigned int db_uid,
                       trc_test *test,
                       trc_update_args_group *wildcard)
{
    trc_update_args_group       *ext_wild = NULL;
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    trc_test_iter_arg           *iter_arg;
    trc_test_iter_arg           *wild_arg;

    TAILQ_FOREACH(iter, &test->iters.head, links)
    {
        iter_data = trc_db_iter_get_user_data(iter, db_uid);
        if (iter_data == NULL || iter_data->to_save == FALSE)
            continue;

        if (test_iter_args_match(wildcard->args, iter_data->args_n,
                                 iter_data->args, TRUE) !=
                                                    ITER_NO_MATCH)
        {
            if (ext_wild == NULL)
            {
                ext_wild = TE_ALLOC(sizeof(*ext_wild));
                ext_wild->args = TE_ALLOC(sizeof(trc_test_iter_args));
                TAILQ_INIT(&ext_wild->args->head);
                args_comb_to_wildcard(NULL, iter_data->args_n,
                                      iter_data->args,
                                      ext_wild);
            }
            else
            {
                iter_arg = TAILQ_FIRST(&iter->args.head);
                wild_arg = TAILQ_FIRST(&ext_wild->args->head);

                while (iter_arg != NULL)
                {
                    assert(wild_arg != NULL);
                    assert(strcmp(iter_arg->name, wild_arg->name) == 0);

                    if (strcmp(iter_arg->value, wild_arg->value) != 0)
                        wild_arg->value[0] = '\0';

                    iter_arg = TAILQ_NEXT(iter_arg, links);
                    wild_arg = TAILQ_NEXT(wild_arg, links);
                }
            }
        }
    }

    if (ext_wild != NULL)
    {
        trc_free_test_iter_args(wildcard->args);
        wildcard->args = ext_wild->args;
        free(ext_wild);
    }

    return 0;
}

/**
 * Generate all wildcards for a given group of arguments and
 * a given kind of results. We need considering several group
 * of arguments because some tests have different set of arguments
 * defined for different runs.
 *
 * @param db_uid        TRC DB User ID
 * @param test          Test
 * @param results_id    Number of specific kind of results to
 *                      which wildcards should be generated
 * @param args_group    Group of arguments
 * @param wildcards     Where to place generated wildcards
 * @param flags         Flags
 *
 * @return Status code
 */
static te_errno
trc_update_gen_args_group_wilds(unsigned int db_uid,
                                trc_test *test,
                                int results_id,
                                trc_update_args_group *args_group,
                                trc_update_args_groups *wildcards,
                                uint64_t flags)
{
    te_bool            *args_in_wild;
    int                 args_count = 0;
    int                 i;
    int                 args_wild_count = 0;

    trc_update_args_groups       cur_comb_wilds;
    trc_update_args_group       *p_group;
    trc_update_args_group       *new_group;
    trc_test_iter_arg           *arg;
    trc_test_iter               *iter1;
    trc_update_test_iter_data   *iter_data1;
    trc_test_iter               *iter2;
    trc_update_test_iter_data   *iter_data2;

    te_bool allow_intersect = !!(flags & TRC_UPDATE_INTERSEC_WILDS);
    te_bool new_iter_added;
    te_bool iter_remained;

    memset(&cur_comb_wilds, 0, sizeof(cur_comb_wilds));
    SLIST_INIT(&cur_comb_wilds);

    TAILQ_FOREACH(arg, &args_group->args->head, links)
        args_count++;

    args_in_wild = TE_ALLOC(sizeof(te_bool) * args_count);
    for (i = 0; i < args_count; i++)
        args_in_wild[i] = FALSE;

    do {
        TAILQ_FOREACH(iter1, &test->iters.head, links)
        {
            iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);
            if (iter_data1 == NULL ||
                iter_data1->results_id != results_id ||
                (!allow_intersect && iter_data1->in_wildcard))
                continue;

            if (test_iter_args_match(args_group->args, iter_data1->args_n,
                                     iter_data1->args, TRUE) !=
                                                        ITER_NO_MATCH)
            {
                new_group = TE_ALLOC(sizeof(*new_group));
                new_group->args = TE_ALLOC(sizeof(trc_test_iter_args));
                TAILQ_INIT(&new_group->args->head);

                args_comb_to_wildcard(args_in_wild, args_count,
                                      iter_data1->args,
                                      new_group);

                new_iter_added = FALSE;
                TAILQ_FOREACH(iter2, &test->iters.head, links)
                {
                    iter_data2 =
                        trc_db_iter_get_user_data(iter2, db_uid);
                    if (iter_data2 == NULL)
                        continue;

                    if (test_iter_args_match(new_group->args,
                                             iter_data2->args_n,
                                             iter_data2->args,
                                             TRUE) != ITER_NO_MATCH)
                    {
                        if ((!allow_intersect &&
                             iter_data2->in_wildcard) ||
                            iter_data2->results_id != results_id)
                            break;

                        if (!iter_data2->in_wildcard)
                            new_iter_added = TRUE;
                    }
                }

                if (iter2 != NULL || !new_iter_added)
                {
                    trc_free_test_iter_args(new_group->args);
                    free(new_group);
                    continue;
                }

                TAILQ_FOREACH(iter2, &test->iters.head, links)
                {
                    iter_data2 =
                        trc_db_iter_get_user_data(iter2, db_uid);
                    if (iter_data2 == NULL)
                        continue;

                    if (test_iter_args_match(new_group->args,
                                             iter_data2->args_n,
                                             iter_data2->args,
                                             TRUE) != ITER_NO_MATCH) 
                    {
                        iter_data2->in_wildcard = TRUE;
                    }
                }

                new_group->exp_results =
                    trc_exp_results_dup((struct trc_exp_results *)
                                        &iter1->exp_results);
                new_group->exp_default =
                    trc_exp_result_dup((struct trc_exp_result *)
                                       iter1->exp_default);
                new_group->group_id = args_group->group_id;

                SLIST_INSERT_HEAD(&cur_comb_wilds, new_group,
                                  links);
            }
        }

        if (!SLIST_EMPTY(&cur_comb_wilds))
        {
            while ((p_group =
                        SLIST_FIRST(&cur_comb_wilds)) != NULL)
            {
                SLIST_REMOVE_HEAD(&cur_comb_wilds, links);
                SLIST_INSERT_HEAD(wildcards, p_group, links);
                if (flags & TRC_UPDATE_EXT_WILDS)
                    trc_update_extend_wild(db_uid, test, p_group);
            }
        }

        iter_remained = FALSE;
        TAILQ_FOREACH(iter1, &test->iters.head, links)
        {
            iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);
            if (iter_data1 != NULL &&
                iter_data1->results_id == results_id &&
                !iter_data1->in_wildcard)
            {
                iter_remained = TRUE;
                break;
            }
        }
        if (!iter_remained)
            break;

    } while (get_next_args_comb(args_in_wild, args_count,
                                &args_wild_count) == 0);

    return 0;
}

/**
 * Generate wildcards for a given test.
 *
 * @param db_uid            TRC DB User ID
 * @param test              Test
 * @param grouped           Test iterations are grouped already
 * @param groups_cnt        Number of groups
 * @param save_wildcards    If not null, generated wildcards
 *                          should be stored here without changing
 *                          the set of test iterations
 * @param flags             Flags
 *
 * @return Status code
 */
static te_errno
trc_update_generate_test_wilds(unsigned int db_uid,
                               trc_test *test,
                               te_bool grouped,
                               int groups_cnt,
                               trc_update_args_groups *save_wildcards,
                               uint64_t flags)
{
    int ids_count;
    int res_id;

    trc_update_args_groups       args_groups;
    trc_update_args_group       *args_group;
    trc_update_args_groups       wildcards;
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    trc_exp_results             *dup_results;
    trc_test_iter_args          *dup_args;
    trc_test_iter_arg           *arg;

    if (test->type != TRC_TEST_SCRIPT)
        return 0;

    if (!grouped)
        ids_count = trc_update_group_test_iters(db_uid, test);
    else
        ids_count = groups_cnt;

    memset(&wildcards, 0, sizeof(wildcards));
    SLIST_INIT(&wildcards);

    for (res_id = 1; res_id <= ids_count; res_id++)
    {
        memset(&args_groups, 0, sizeof(args_groups));
        SLIST_INIT(&args_groups);

        trc_update_get_iters_args_combs(db_uid, test, res_id,
                                        &args_groups);
        SLIST_FOREACH(args_group, &args_groups, links)
            trc_update_gen_args_group_wilds(db_uid, test, res_id,
                                            args_group, &wildcards,
                                            flags);

        trc_update_args_groups_free(&args_groups);
    }

    /* Delete original iterations - they will be replaced by wildcards */
    iter = TAILQ_FIRST(&test->iters.head);

    if (iter != NULL && save_wildcards == NULL)
    {
        do {
            TAILQ_REMOVE(&test->iters.head, iter, links);
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            trc_update_free_test_iter_data(iter_data);
            if (iter->node != NULL)
            {
                xmlUnlinkNode(iter->node);
                xmlFreeNode(iter->node);
            }
            trc_free_test_iter(iter);
            free(iter);
        } while ((iter = TAILQ_FIRST(&test->iters.head)) != NULL);
    }

    /* Insert generated wildcards in TRC DB */
    if (save_wildcards == NULL)
    {
        SLIST_FOREACH(args_group, &wildcards, links)
        {
            iter = TE_ALLOC(sizeof(*iter));
            iter->exp_default = trc_exp_result_dup(
                                        args_group->exp_default);
            dup_results = trc_exp_results_dup(
                                        args_group->exp_results);
            memcpy(&iter->exp_results, dup_results, sizeof(*dup_results));
            free(dup_results);
            TAILQ_INIT(&iter->args.head);
            dup_args = trc_test_iter_args_dup(args_group->args);
            while ((arg = TAILQ_FIRST(&dup_args->head)) != NULL)
            {
                TAILQ_REMOVE(&dup_args->head, arg, links);
                TAILQ_INSERT_TAIL(&iter->args.head, arg, links);
            }

            free(dup_args);
            iter->parent = test;

            if (test->filename != NULL)
                iter->filename = strdup(test->filename);

            iter_data = TE_ALLOC(sizeof(*iter_data));
            iter_data->to_save = TRUE;
            trc_db_iter_set_user_data(iter, db_uid, iter_data);

            TAILQ_INSERT_TAIL(&test->iters.head, iter, links);
        }
        trc_update_args_groups_free(&wildcards);
    }
    else
        memcpy(save_wildcards, &wildcards, sizeof(wildcards));

    return 0;
}

/**
 * This variable is used to restrict time which can take
 * generation of full subset structure before it is terminated
 * and another approach not requiring it is used.
 */
static struct timeval   tv_before_gen_fss;

/**
 * Determine full subset structure for all the wildcards including
 * non-void values of arguments from a given group. Determining
 * "full subset structure" means determining subset of iterations
 * desribed by each of these wildcards.
 *
 * @param db_uid        TRC DB user ID
 * @param test_entry    Record describing test to be updated
 * @param results_id    ID of result wich should be in all the
 *                      iterations described by generated wildcards
 * @param args_group    Set of arguments which should have non-void
 *                      values in generated wildcards
 * @param flags         Flags
 *
 * @return Status code
 */
te_errno
trc_update_gen_args_group_fss(unsigned int db_uid,
                              trc_update_test_entry *test_entry,
                              int results_id,
                              trc_update_args_group *args_group,
                              uint64_t flags)
{
    te_bool            *args_in_wild;
    int                 args_count = 0;
    int                 i;
    int                 j;
    int                 args_wild_count = 0;
    int                *comm_sets = NULL;
    int                 sets_num = 0;
    int                 comm_set_num = 0;

    trc_update_args_groups       cur_comb_wilds;
    trc_update_args_groups       wrong_wilds;
    trc_update_args_group       *p_group;
    trc_update_args_group       *new_group;
    trc_test_iter_arg           *arg;
    trc_test_iter               *iter1;
    trc_update_test_iter_data   *iter_data1;
    trc_test_iter               *iter2;
    trc_update_test_iter_data   *iter_data2;
    trc_test                    *test = test_entry->test;

    struct timeval   tv_after;
    int              rc = 0;

#define TIME_DIFF \
    (gettimeofday(&tv_after, NULL),                                 \
         (tv_after.tv_sec - tv_before_gen_fss.tv_sec) * 1000000 +   \
         (tv_after.tv_usec = tv_before_gen_fss.tv_usec))

    memset(&cur_comb_wilds, 0, sizeof(cur_comb_wilds));
    SLIST_INIT(&cur_comb_wilds);
    memset(&wrong_wilds, 0, sizeof(wrong_wilds));
    SLIST_INIT(&wrong_wilds);

    TAILQ_FOREACH(arg, &args_group->args->head, links)
        args_count++;

    args_in_wild = TE_ALLOC(sizeof(te_bool) * args_count);
    for (i = 0; i < args_count; i++)
        args_in_wild[i] = FALSE;

    do {
        if (TIME_DIFF >= 1000000L &&
            !(flags & TRC_UPDATE_FSS_UNLIM))
        {
            rc = -2;
            break;
        }

        TAILQ_FOREACH(iter1, &test->iters.head, links)
        {
            if (TIME_DIFF >= 1000000L &&
                !(flags & TRC_UPDATE_FSS_UNLIM))
            {
                rc = -2;
                break;
            }

            iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);
            if (iter_data1 == NULL)
                continue;

            if (test_iter_args_match(args_group->args, iter_data1->args_n,
                                     iter_data1->args, TRUE) !=
                                                        ITER_NO_MATCH)
            {
                SLIST_FOREACH(p_group, &cur_comb_wilds, links)
                    if (test_iter_args_match(p_group->args,
                                             iter_data1->args_n,
                                             iter_data1->args,
                                             TRUE) != ITER_NO_MATCH)
                        break;

                if (p_group != NULL)
                    continue;
                else if (iter_data1->results_id == results_id)
                {
                    SLIST_FOREACH(p_group, &wrong_wilds, links)
                        if (test_iter_args_match(p_group->args,
                                                 iter_data1->args_n,
                                                 iter_data1->args,
                                                 TRUE) != ITER_NO_MATCH)
                            break;

                    if (p_group != NULL)
                        continue;

                    sets_num = iter_data1->nums_cnt;
                    comm_sets = TE_ALLOC(sizeof(int) * sets_num);
                    for (i = 0; i < sets_num; i++)
                        comm_sets[i] = iter_data1->set_nums[i];

                    new_group = TE_ALLOC(sizeof(*new_group));
                    new_group->args = TE_ALLOC(sizeof(trc_test_iter_args));
                    TAILQ_INIT(&new_group->args->head);

                    args_comb_to_wildcard(args_in_wild, args_count,
                                          iter_data1->args,
                                          new_group);
                    new_group->exp_results =
                        trc_exp_results_dup((struct trc_exp_results *)
                                            &iter1->exp_results);
                    new_group->exp_default =
                        trc_exp_result_dup((struct trc_exp_result *)
                                           iter1->exp_default);
                    new_group->group_id = args_group->group_id;

                    TAILQ_FOREACH(iter2, &test->iters.head, links)
                    {
                        iter_data2 =
                            trc_db_iter_get_user_data(iter2, db_uid);
                        if (iter_data2 == NULL)
                            continue;

                        if (test_iter_args_match(new_group->args,
                                                 iter_data2->args_n,
                                                 iter_data2->args,
                                                 TRUE) != ITER_NO_MATCH)
                        {
                            if (iter_data2->results_id != results_id)
                                break;

                            for (i = 0; i < sets_num; i++)
                            {
                                for (j = 0; j < iter_data2->nums_cnt; j++)
                                    if (iter_data2->set_nums[j] ==
                                                            comm_sets[i])
                                        break;

                                if (j == iter_data2->nums_cnt)
                                {
                                    j = i;
                                    for ( ; i < sets_num - 1; i++)
                                        comm_sets[i] = comm_sets[i + 1];
                                    i = j - 1;
                                    sets_num--;
                                }
                            }
                        }
                    }

                    if (sets_num > 0)
                    {
                        if (iter2 == NULL)
                        {
                            for (i = 0; i < sets_num; i++)
                            {
                                /* 
                                 * Having belonging to some sets in common
                                 * is not enough - set of iterations of
                                 * interest should be one of these sets
                                 * to be rejected.
                                 */
                                TAILQ_FOREACH(iter2, &test->iters.head,
                                              links)
                                {
                                    iter_data2 =
                                        trc_db_iter_get_user_data(iter2,
                                                                  db_uid);
                                    if (iter_data2 == NULL)
                                        continue;

                                    for (j = 0; j < iter_data2->nums_cnt;
                                         j++)
                                        if (iter_data2->set_nums[j] ==
                                                            comm_sets[i])
                                            break;

                                    if (j < iter_data2->nums_cnt &&
                                        test_iter_args_match(
                                                    new_group->args,
                                                    iter_data2->args_n,
                                                    iter_data2->args,
                                                    TRUE) == ITER_NO_MATCH)
                                        break;
                                }

                                if (iter2 != NULL)
                                {
                                    j = i;
                                    for ( ; i < sets_num - 1; i++)
                                        comm_sets[i] = comm_sets[i + 1];
                                    i = j - 1;
                                    sets_num--;
                                }
                            }

                            iter2 = NULL;
                        }

                        assert(sets_num <= 1 || iter2 != NULL);
                        if (sets_num == 1)
                            comm_set_num = comm_sets[0];
                    }

                    free(comm_sets);

                    if (iter2 != NULL)
                    {
                        /*
                         * Wildcards to which iterations with different
                         * results match
                         */
                        SLIST_INSERT_HEAD(&wrong_wilds, new_group,
                                          links);
                        continue;
                    }

                    if (sets_num > 0)
                    {
                        /*
                         * There is already such set - just one more
                         * wildcard defining it.
                         */
                        SLIST_INSERT_HEAD(&test_entry->sets[comm_set_num],
                                          new_group, links);
                    }
                    else
                    {
                        if (test_entry->sets_cnt >=
                                            test_entry->sets_max)
                        {
                            test_entry->sets =
                                realloc(test_entry->sets,
                                        ((test_entry->sets_max + 1) * 2) *
                                        sizeof(trc_update_args_group));
                            test_entry->sets_max =
                                        (test_entry->sets_max + 1) * 2;
                        }

                        SLIST_INIT(
                            &test_entry->sets[test_entry->sets_cnt]);

                        SLIST_INSERT_HEAD(
                            &test_entry->sets[test_entry->sets_cnt],
                            new_group, links);

                        TAILQ_FOREACH(iter2, &test->iters.head, links)
                        {
                            iter_data2 =
                                trc_db_iter_get_user_data(iter2, db_uid);
                            if (iter_data2 == NULL)
                                continue;
                            if (test_iter_args_match(new_group->args,
                                                     iter_data2->args_n,
                                                     iter_data2->args,
                                                     TRUE) != ITER_NO_MATCH)
                            {
                                if (iter_data2->nums_cnt >=
                                            iter_data2->nums_max)
                                {
                                    iter_data2->set_nums = 
                                             realloc(
                                                iter_data2->set_nums,
                                                ((iter_data2->nums_max +
                                                 1) * 2) * sizeof(int));

                                    iter_data2->nums_max += 1;
                                    iter_data2->nums_max *= 2;
                                }

                                iter_data2->set_nums[
                                            iter_data2->nums_cnt] =
                                                test_entry->sets_cnt;
                                iter_data2->nums_cnt++;
                            }
                        }

                        test_entry->sets_cnt++;
                    }

                    new_group = TE_ALLOC(sizeof(*new_group));
                    new_group->args = TE_ALLOC(sizeof(trc_test_iter_args));
                    TAILQ_INIT(&new_group->args->head);

                    args_comb_to_wildcard(args_in_wild, args_count,
                                          iter_data1->args,
                                          new_group);

                    SLIST_INSERT_HEAD(&cur_comb_wilds, new_group,
                                      links);
                }
            }
        }

        trc_update_args_groups_free(&cur_comb_wilds);
        trc_update_args_groups_free(&wrong_wilds);
    } while (get_next_args_comb(args_in_wild, args_count,
                                &args_wild_count) == 0);

    return rc;
#undef TIME_DIFF
}

/**
 * Generate wildcards for a given tests from full structure
 * of iteration sets (so it has postfix "fss") described by
 * (each set corresponds to some wildcard describing it).
 * It can require too much time to build such a structure,
 * use trc_update_generate_test_wilds() in such cases.
 *
 * @param db_uid        TRC DB User ID
 * @param test_entry    TRC Update test entry
 * @param grouped       Whether test iterations are grouped
 *                      already or not
 * @param groups_cnt    Number of iteration groups
 * @param wildcards     If not null, generated wildcards
 *                      should be stored here without changing
 *                      the set of test iterations
 * @param flags         Flags
 *
 * @return Status code
 */
te_errno
trc_update_gen_test_wilds_fss(unsigned int db_uid,
                              trc_update_test_entry *test_entry,
                              te_bool grouped,
                              int groups_cnt,
                              trc_update_args_groups *wildcards,
                              uint64_t flags)
{
    size_t       ids_count;
    int          res_id;
    int          i = 0;
    int          j = 0;
    int          k = 0;
    int          l = 0;
    int          rc = 0;

    trc_update_args_groups       args_groups;
    trc_update_args_group       *args_group;
    trc_update_args_group       *args_group_dup;
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    trc_exp_results             *dup_results;
    trc_test_iter_args          *dup_args;
    trc_test_iter_arg           *arg;

    int      set_num;
    
    problem     *wild_prbs;
    int          p_id = 0;

    if (flags & TRC_UPDATE_NO_GEN_FSS)
        return -1;

    if (test_entry->test->type != TRC_TEST_SCRIPT)
        return 0;

    if (!grouped)
        ids_count = trc_update_group_test_iters(db_uid, test_entry->test);
    else
        ids_count = groups_cnt;

    gettimeofday(&tv_before_gen_fss, NULL);

    for (res_id = 1; res_id <= (int)ids_count; res_id++)
    {
        memset(&args_groups, 0, sizeof(args_groups));
        SLIST_INIT(&args_groups);

        trc_update_get_iters_args_combs(db_uid, test_entry->test, res_id,
                                        &args_groups);

        SLIST_FOREACH(args_group, &args_groups, links)
        {
            rc = trc_update_gen_args_group_fss(db_uid, test_entry,
                                               res_id, args_group,
                                               flags);
            if (rc < 0)
            {
                trc_update_args_groups_free(&args_groups);
                TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
                {
                    iter_data = trc_db_iter_get_user_data(iter, db_uid);
                    if (iter_data == NULL || !iter_data->to_save)
                        continue;

                    iter_data->results_id = 0;
                }

                return rc;
            }
        }

        trc_update_args_groups_free(&args_groups);
    }

    wild_prbs = calloc(ids_count, sizeof(*wild_prbs));

    for (res_id = 1; res_id <= (int)ids_count; res_id++)
    {
        p_id = res_id - 1;
        wild_prbs[p_id].set_max = 10;
        wild_prbs[p_id].set_num = 0;
        wild_prbs[p_id].sets = calloc(wild_prbs[p_id].set_max,
                                   sizeof(set));
        k = 0;

        TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
        {
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            if (iter_data == NULL || !iter_data->to_save ||
                iter_data->results_id != res_id)
                continue;

            k++;

            for (i = 0; i < iter_data->nums_cnt; i++)
            {
                for (j = 0; j < wild_prbs[p_id].set_num; j++)
                    if (wild_prbs[p_id].sets[j].id ==
                                                iter_data->set_nums[i])
                        break;

                if (j == wild_prbs[p_id].set_num)
                {
                    if (wild_prbs[p_id].set_num ==
                                        wild_prbs[p_id].set_max)
                    {
                        wild_prbs[p_id].set_max *= 2;
                        wild_prbs[p_id].sets =
                            realloc(wild_prbs[p_id].sets,
                                    sizeof(set) *
                                            wild_prbs[p_id].set_max);
                    }

                    set_num = wild_prbs[p_id].set_num;
                    wild_prbs[p_id].sets[set_num].els =
                                                calloc(1, sizeof(int));
                    wild_prbs[p_id].sets[set_num].els[0] = k - 1;
                    wild_prbs[p_id].sets[set_num].max = 1;
                    wild_prbs[p_id].sets[set_num].num = 1;
                    wild_prbs[p_id].sets[set_num].id =
                                                iter_data->set_nums[i];
                    wild_prbs[p_id].set_num++;
                }
                else
                {
                    l = wild_prbs[p_id].sets[j].num;
                    if (l == wild_prbs[p_id].sets[j].max)
                    {
                        wild_prbs[p_id].sets[j].max *= 2;
                        wild_prbs[p_id].sets[j].els =
                           realloc(wild_prbs[p_id].sets[j].els,
                                   sizeof(int) *
                                    wild_prbs[p_id].sets[j].max);
                    }

                    wild_prbs[p_id].sets[j].els[l] = k - 1;
                    wild_prbs[p_id].sets[j].num++;
                }
            }
        }

        wild_prbs[p_id].elm_num = k;
        get_fss_solution(&wild_prbs[p_id],
                         !!(flags & TRC_UPDATE_INTERSEC_WILDS) ?
                             ALG_SET_COV_GREEDY : ALG_EXACT_COV_BOTH);
    }

    if (flags & TRC_UPDATE_EXT_WILDS)
    {
        for (i = 0; i < (int)ids_count; i++)
        {
            for (j = 0; j < wild_prbs[i].sol_num; j++)
            {
                l = wild_prbs[i].sol[j];

                args_group = SLIST_FIRST(
                                &test_entry->sets[wild_prbs[i].sets[l].id]);
                while (SLIST_NEXT(args_group, links) != NULL)
                    args_group = SLIST_NEXT(args_group, links);

                trc_update_extend_wild(db_uid, test_entry->test,
                                       args_group);
            }
        }
    }

    /* Delete original iterations - they will be replaced by wildcards */
    iter = TAILQ_FIRST(&test_entry->test->iters.head);
    if (iter != NULL && wildcards == NULL)
    {
        do {
            TAILQ_REMOVE(&test_entry->test->iters.head, iter, links);
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            if (iter_data != NULL)
            {
                free(iter_data->set_nums);
                iter_data->set_nums = NULL;
            }
            trc_update_free_test_iter_data(iter_data);
            if (iter->node != NULL)
            {
                xmlUnlinkNode(iter->node);
                xmlFreeNode(iter->node);
            }
            trc_free_test_iter(iter);
            free(iter);
        } while ((iter =
                    TAILQ_FIRST(&test_entry->test->iters.head)) != NULL);
    }

    /* Insert generated wildcards in TRC DB */
    for (i = 0; i < (int)ids_count; i++)
    {
        for (j = 0; j < wild_prbs[i].sol_num; j++)
        {
            l = wild_prbs[i].sol[j];

            args_group = SLIST_FIRST(
                            &test_entry->sets[wild_prbs[i].sets[l].id]);
            while (SLIST_NEXT(args_group, links) != NULL)
                args_group = SLIST_NEXT(args_group, links);

            if (wildcards != NULL)
            {
                args_group_dup = TE_ALLOC(sizeof(*args_group_dup));
                args_group_dup->exp_results = args_group->exp_results;
                args_group->exp_results = NULL;
                args_group_dup->exp_default = args_group->exp_default;
                args_group->exp_default = NULL;
                args_group_dup->group_id = args_group->group_id;
                args_group_dup->args = args_group->args;
                args_group->args = NULL;
                SLIST_INSERT_HEAD(wildcards, args_group_dup, links);
            }
            else
            {
                iter = TE_ALLOC(sizeof(*iter));
                iter->exp_default = trc_exp_result_dup(
                                            args_group->exp_default);
                dup_results = trc_exp_results_dup(
                                            args_group->exp_results);
                memcpy(&iter->exp_results, dup_results,
                       sizeof(*dup_results));
                free(dup_results);
                TAILQ_INIT(&iter->args.head);
                dup_args = trc_test_iter_args_dup(args_group->args);
                while ((arg = TAILQ_FIRST(&dup_args->head)) != NULL)
                {
                    TAILQ_REMOVE(&dup_args->head, arg, links);
                    TAILQ_INSERT_TAIL(&iter->args.head, arg, links);
                }

                free(dup_args);
                iter->parent = test_entry->test;

                if (test_entry->test->filename != NULL)
                    iter->filename = strdup(test_entry->test->filename);

                iter_data = TE_ALLOC(sizeof(*iter_data));
                iter_data->to_save = TRUE;
                trc_db_iter_set_user_data(iter, db_uid, iter_data);

                TAILQ_INSERT_TAIL(&test_entry->test->iters.head,
                                  iter, links);
            }
        }
        problem_free(&wild_prbs[i]);
    }

    for (i = 0; i < test_entry->sets_cnt; i++)
        trc_update_args_groups_free(&test_entry->sets[i]);

    free(test_entry->sets);
    test_entry->sets = NULL;
    test_entry->sets_cnt = test_entry->sets_max = 0;
    free(wild_prbs);

    return 0;
}

/**
 * Generate wildcards.
 *
 * @param db_uid        TRC DB User ID
 * @param updated_tests Tests to be updated
 * @param flags         Flags
 *
 * @return Status code
 */
te_errno
trc_update_generate_wilds_gen(unsigned int db_uid,
                              trc_update_tests_groups *updated_tests,
                              uint64_t flags)
{
    trc_update_tests_group  *group;
    trc_update_test_entry   *test_entry;
    int                      rc = 0;

    TAILQ_FOREACH(group, updated_tests, links)
    {
        TAILQ_FOREACH(test_entry, &group->tests, links)
        {
            rc = trc_update_gen_test_wilds_fss(db_uid, test_entry,
                                               FALSE, 0, NULL, flags);
            if (rc < 0)
                trc_update_generate_test_wilds(db_uid, test_entry->test,
                                               FALSE, 0, NULL, flags);
        }
    }

    return 0;
}

/**
 * Add tags from currently processed logs to the list of all tags
 * encountered in logs.
 */
static int
trc_update_collect_tags(trc_update_ctx *ctx)
{
    tqe_string   *tqe_p = NULL;
    tqe_string   *tqe_q = NULL;
    tqe_string   *tqe_r = NULL;
    int           cmp_result;

    TAILQ_FOREACH(tqe_p, &ctx->tags, links)
    {
        cmp_result = 1;
        TAILQ_FOREACH(tqe_q, &ctx->collected_tags, links)
        {
            cmp_result = strcmp(tqe_p->v, tqe_q->v);
            if (cmp_result <= 0)
                break;
        }

        if (cmp_result != 0)
        {
            tqe_r = TE_ALLOC(sizeof(*tqe_r));
            if (tqe_r == NULL)
                return -1;
            tqe_r->v = strdup(tqe_p->v);
            if (tqe_q != NULL)
                TAILQ_INSERT_BEFORE(tqe_q, tqe_r, links);
            else
                TAILQ_INSERT_TAIL(&ctx->collected_tags, tqe_r, links);
        }
    }

    return 0;
}

/**
 * Fix byte order in uint32 value read from night logs report in binary
 * form.
 */
static void
logs_dump_fix_byte_order(uint32_t *value)
{
    uint32_t  test_val = 1;
    char     *p = (char *)&test_val;

    if (*p == 0)
    {
        char tmp;

        p = (char *)value;
        tmp = *p;
        *p = *(p + 3);
        *(p + 3) = tmp;
        tmp = *(p + 1);
        *(p + 1) = *(p + 2);
        *(p + 2) = tmp;
    }
}

/** Read an uint32 value from night logs report in binary form */
static int
logs_dump_read_uint32(FILE *f, uint32_t *val)
{
    if (fread(val, 1, sizeof(*val), f) != sizeof(*val))
        return -1;
    logs_dump_fix_byte_order(val);
    return 0;
}

/** Read a string from night logs report in binary form */
static char *
logs_dump_str_read(FILE *f)
{
    char     *str;
    uint32_t  str_len;
    size_t    rc;

    if (logs_dump_read_uint32(f, &str_len) < 0)
    {
        ERROR("%s", "Failed to read string length");
        return NULL;
    }

    str = TE_ALLOC(str_len + 1);
    if (str == NULL)
    {
        ERROR("Out of memory trying to allocate %u bytes",
              str_len + 1);
        return NULL;
    }
    str[str_len] = '\0';

    if ((rc = fread(str, 1, str_len, f)) != str_len)
    {
        ERROR("Failed to read string: %ld returned instead of %u",
              (long int)rc, str_len);
        free(str);
        return NULL;
    }

    return str;
}

/** Compare test paths in TRC DB */
static int
test_paths_cmp(const char *path1, const char *path2)
{
    unsigned int i = 0;
    unsigned int j = 0;

    for (i = 0, j = 0; ; i++, j++)
    {
        while (path1[i] == '/')
        {
            i++;
        }
        while (path2[j] == '/')
        {
            j++;
        }

        if (path1[i] < path2[j])
            return -1;
        else if (path1[i] > path2[j])
            return 1;

        if (path1[i] == '\0' || path2[j] == '\0')
            break;
    }

    return 0;
}

/** Derive tag name from log path */
static const char *
tags_by_log(const char *log, const char *date)
{
    const char *p = NULL;
    unsigned int i = 0;
    unsigned int j = 0;

    static char tag_name[PATH_MAX] = "UNSPEC";

    for (p = log + strlen(log); p > log; p--)
    {
        if (*p == '/')
        {
            i++;
            if (i == 3)
                break;
        }
    }

    if (i < 3)
    {
        ERROR("Failed to derive tag name from %s", log);
        return tag_name;
    }
    p++;

    if (strlen(p) < 8 || p[2] != '.' || p[5] != '.')
    {
        const char *q;

        for (q = date; *q == '.' || (*q >= '0' && *q <= '9'); q++)
        {
            tag_name[j++] = *q;
        }
        tag_name[j++] = '-';
    }

    for ( ; *p != '/' && *p != '\0'; p++)
    {
        if (*p == '!')
        {
            const char *neg = "_not_";
            strcpy(tag_name + j, neg);
            j += strlen(neg);
        }
        else
            tag_name[j++] = *p;
    }

    tag_name[j++] = '\0';

    return tag_name;
}

/** Process night logs report in binary form */
static int
trc_update_process_logs_dump(trc_update_ctx *ctx)
{
#define LOGS_DUMP_ERR(s_...) \
    do {                                                              \
        if (f != NULL)                                                \
        {                                                             \
            ERROR("Problem encountered at offset %llu",               \
                  (long long unsigned int)ftello(f));                 \
        }                                                             \
        ERROR(s_);                                                    \
        rc = -1;                                                      \
        goto cleanup;                                                 \
    } while (0)

#define LOGS_DUMP_READ_TAG(tag_, exp_) \
    do {                                                                \
         if (logs_dump_read_uint32(f, &tag_) < 0)                       \
              LOGS_DUMP_ERR("%s",                                       \
                            "Failed to read tag ID from logs dump");    \
         if (exp_ != LOGS_DUMP_TAG_UNDEF && tag_ != exp_)               \
              LOGS_DUMP_ERR("Unexpected tag %u in logs dump", tag_);    \
    } while (0)

    FILE      *f = NULL;
    uint32_t   tag_id;
    int        rc = 0;
    off_t      file_len;

    char                *test_path = NULL;
    trc_report_argument *args = NULL;
    uint32_t             params_count;
    te_trc_db_walker    *db_walker = NULL;
    te_test_result      *result = NULL;
    te_test_verdict     *verdict = NULL;
    char                *status = NULL;
    char                *log = NULL;
    char                *date = NULL;
    char                *tags_str = NULL;
    trc_test_iter       *iter = NULL;

    trc_update_tests_group      *group = NULL;
    trc_update_tests_group      *cur_group = NULL;
    trc_update_test_entry       *test_entry = NULL;

    f = fopen(ctx->logs_dump, "r");
    if (f == NULL)
        LOGS_DUMP_ERR("Failed to open for reading '%s'", ctx->logs_dump);

    fseeko(f, 0LL, SEEK_END);
    file_len = ftello(f);
    fseeko(f, 0LL, SEEK_SET);

    db_walker = trc_db_new_walker(ctx->db);

    tq_strings_free(&ctx->tags, free);

    while (ftello(f) < file_len)
    {
        uint32_t  iters_count;
        uint32_t  results_count;
        uint32_t  verdicts_count;
        uint32_t  logs_count;
        uint32_t  i;
        te_bool   iter_found = TRUE;

        LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_TEST);
        test_path = logs_dump_str_read(f);
        if (test_path == NULL)
            LOGS_DUMP_ERR("%s", "Failed to read test name");

        cur_group = NULL;
        TAILQ_FOREACH(group, &ctx->updated_tests, links)
        {
            if (test_paths_cmp(group->path, test_path) == 0)
            {
                cur_group = group;
                break;
            }
        }

        if (cur_group == NULL) /* Skip? */
            LOGS_DUMP_ERR("Failed to find test %s in TRC DB", test_path);

        free(test_path);
        test_path = NULL;

        if (logs_dump_read_uint32(f, &iters_count) < 0)
            LOGS_DUMP_ERR("%s",
                          "Failed to read number of iterations "
                          "from logs dump");

        for (i = 0; i < iters_count; i++)
        {
            uint32_t j;

            LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_ITER);

            LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_PARAMS);
            if (logs_dump_read_uint32(f, &params_count) < 0)
                LOGS_DUMP_ERR("%s",
                              "Failed to read number of iterations "
                              "from logs dump");

            args = TE_ALLOC(params_count * sizeof(*args));
            if (args == NULL)
                LOGS_DUMP_ERR("%s", "Failed to allocate arguments array");

            for (j = 0; j < params_count; j++)
            {
                args[j].variable = FALSE;
                args[j].name = logs_dump_str_read(f);
                if (args[j].name == NULL)
                    LOGS_DUMP_ERR("%s", "Failed to read argument name");
                args[j].value = logs_dump_str_read(f);
                if (args[j].value == NULL)
                    LOGS_DUMP_ERR("%s", "Failed to read argument value");
            }

            TAILQ_FOREACH(test_entry, &cur_group->tests, links)
            {
                trc_db_walker_go_to_test(db_walker, test_entry->test);
                iter_found = trc_db_walker_step_iter(
                                                db_walker,
                                                params_count, args,
                                                (STEP_ITER_NO_MATCH_WILD |
                                                 STEP_ITER_NO_MATCH_OLD),
                                                ctx->db_uid,
                                                NULL);
                if (iter_found)
                    break;
            }
            if (!iter_found)
                LOGS_DUMP_ERR("%s", "Failed to find iteration in TRC DB");

            for (j = 0; j < params_count; j++)
            {
                free(args[j].name);
                free(args[j].value);
            }
            free(args);
            args = NULL;
            params_count = 0;

            iter = trc_db_walker_get_iter(db_walker);

            LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_RESULTS);
            if (logs_dump_read_uint32(f, &results_count) < 0)
                LOGS_DUMP_ERR("%s",
                              "Failed to read number of iteration results "
                              "from logs dump");

            for (j = 0; j < results_count; j++)
            {
                uint32_t k;

                result = TE_ALLOC(sizeof(*result));
                if (result == NULL)
                    LOGS_DUMP_ERR("%s", "Out of memory");
                TAILQ_INIT(&result->verdicts);

                LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_UNDEF);
                if (tag_id == LOGS_DUMP_TAG_ERR)
                {
                    char *err_msg = logs_dump_str_read(f);

                    if (err_msg == NULL)
                          LOGS_DUMP_ERR("%s",
                                        "Failed to read error message");
                    free(err_msg);
                    LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_STATUS);
                }
                    
                if (tag_id == LOGS_DUMP_TAG_STATUS)
                {
                    status = logs_dump_str_read(f);
                
                    if (status == NULL)
                        LOGS_DUMP_ERR("%s",
                                      "Failed to read result status");

                    if (te_test_str2status(status, &result->status) != 0)
                    {
                        rc = -1;
                        goto cleanup;
                    }

                    free(status);
                    status = NULL;
                }
                else
                    LOGS_DUMP_ERR("%s", "Result status is missed");

                LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_VERDICTS);
                if (logs_dump_read_uint32(f, &verdicts_count) < 0)
                    LOGS_DUMP_ERR("%s",
                                  "Failed to read number of "
                                  "verdicts from logs dump");

                for (k = 0; k < verdicts_count; k++)
                {
                    verdict = TE_ALLOC(sizeof(*verdict));
                    if (verdict == NULL)
                        LOGS_DUMP_ERR("%s", "Out of memory");

                    verdict->str = logs_dump_str_read(f);
                    if (verdict->str == NULL)
                        LOGS_DUMP_ERR("%s", "Failed to read verdict");
                    TAILQ_INSERT_TAIL(&result->verdicts, verdict, links);
                    verdict = NULL;
                }

                LOGS_DUMP_READ_TAG(tag_id, LOGS_DUMP_TAG_LOGS);
                if (logs_dump_read_uint32(f, &logs_count) < 0)
                    LOGS_DUMP_ERR("%s",
                                  "Failed to read number of logs "
                                  "from logs dump");

                for (k = 0; k < logs_count; k++)
                {
                    log = logs_dump_str_read(f);
                    if (log == NULL)
                        LOGS_DUMP_ERR("%s",
                                      "Failed to read log path "
                                      "from logs dump");

                    date = logs_dump_str_read(f);
                    if (date == NULL)
                        LOGS_DUMP_ERR("%s",
                                      "Failed to read log date "
                                      "from logs dump");

                    tags_str = logs_dump_str_read(f);
                    if (tags_str == NULL)
                        LOGS_DUMP_ERR("%s",
                                      "Failed to read log tags "
                                      "from logs dump");

                    trc_tags_str_to_list(&ctx->tags, tags_str);

                    if (ctx->flags & TRC_UPDATE_TAGS_GATHER)
                    {
                        if (trc_update_collect_tags(ctx) < 0)
                            LOGS_DUMP_ERR("%s", "Failed to process tags");
                    }

                    if (ctx->flags & TRC_UPDATE_TAGS_BY_LOGS)
                        ctx->merge_str = strdup(tags_by_log(log, date));
                    else
                        ctx->merge_str = strdup("UNSPEC");
                    logic_expr_parse(ctx->merge_str, &ctx->merge_expr);

                    if (trc_update_merge_result(ctx, iter, result) != 0)
                        LOGS_DUMP_ERR("%s",
                                      "Failed to process result "
                                      "from logs dump");

                    free(ctx->merge_str);
                    ctx->merge_str = NULL;
                    logic_expr_free(ctx->merge_expr);
                    ctx->merge_expr = NULL;

                    tq_strings_free(&ctx->tags, free);

                    free(log);
                    log = NULL;
                    free(date);
                    date = NULL;
                    free(tags_str);
                    tags_str = NULL;
                }

                te_test_result_free(result);
                result = NULL;
            }
        }

    }

cleanup:

    if (f != NULL)
        fclose(f);

    if (db_walker != NULL)
        trc_db_free_walker(db_walker);

    free(test_path);

    if (args != NULL)
    {
        uint32_t j;

        for (j = 0; j < params_count; j++)
        {
            free(args[j].name);
            free(args[j].value);
        }
        free(args);
    }

    free(status);
    if (verdict != NULL)
        te_test_result_free_verdict(verdict);
    if (result != NULL)
        te_test_result_free(result);

    free(ctx->merge_str);
    if (ctx->merge_expr != NULL)
        logic_expr_free(ctx->merge_expr);
    free(log);
    free(date);
    free(tags_str);

    tq_strings_free(&ctx->tags, free);

    return rc;
#undef LOGS_DUMP_ERR
}

/* See description in trc_update.h */
te_errno
trc_update_process_iter(trc_update_ctx *ctx,
                        te_trc_db_walker *db_walker,
                        trc_test_iter *iter,
                        trc_report_test_iter_entry *entry)
{
    trc_update_test_iter_data     *upd_iter_data;
    trc_update_test_iter_data     *upd_test_data;
    te_bool                        to_save;
    te_errno                       rc = 0;

    if (ctx->flags & TRC_UPDATE_PRINT_PATHS)
        return 0;

    upd_iter_data = trc_db_walker_get_user_data(db_walker,
                                                ctx->db_uid);

    upd_test_data = trc_db_walker_get_parent_user_data(
                          db_walker, ctx->db_uid);

    if (upd_iter_data == NULL)
    {
        assert(!(ctx->flags & TRC_UPDATE_FILT_LOG));
        to_save = TRUE;

        /* Attach iteration data to TRC database */
        if (upd_test_data != NULL)
            rc = trc_db_walker_set_user_data(
                            db_walker,
                            ctx->db_uid,
                            trc_update_gen_user_data(
                                                &to_save,
                                                TRUE));
        else
            rc = trc_db_walker_set_prop_ud(
                              db_walker,
                              ctx->db_uid, &to_save,
                              &trc_update_gen_user_data);

        if (rc != 0)
            return rc;

        upd_iter_data = trc_db_walker_get_user_data(
                            db_walker,
                            ctx->db_uid);

        assert(upd_iter_data != NULL);

        if (ctx->flags & (TRC_UPDATE_FAKE_LOG | TRC_UPDATE_LOG_WILDS))
            iter->log_found = TRUE;

        /*
         * Arguments here are sorted - qsort() was
         * called inside trc_db_walker_step_iter()
         */
        upd_iter_data->args = entry->args;
        upd_iter_data->args_n = entry->args_n;
        upd_iter_data->args_max = entry->args_max;

        entry->args = NULL;
        entry->args_n = 0;
        entry->args_max = 0;
    }
    else if (ctx->flags & TRC_UPDATE_FILT_LOG)
        upd_iter_data->filtered = TRUE;

    upd_iter_data->counter = ctx->cur_lnum;

    if ((ctx->flags & (TRC_UPDATE_MERGE_LOG | TRC_UPDATE_LOG_WILDS)))
        return trc_update_merge_result(ctx, iter, &entry->result);

    return rc;
}

/**
 * Initialize TRC Log Parse context for TRC Update
 * Tool.
 *
 * @param ctx       TRC Log Parse context to be initialized
 * @param gctx      Context of TRC Update Tool
 *
 * @return Status code
 */
static te_errno
trc_update_init_parse_ctx(trc_log_parse_ctx *ctx,
                          trc_update_ctx *gctx)
{
    free(ctx->stack_info);
    ctx->stack_info = NULL;
    ctx->stack_pos = 0;
    ctx->stack_size = 0;
 
    memset(ctx, 0, sizeof(*ctx));

    ctx->app_id = TRC_LOG_PARSE_APP_UPDATE;
    ctx->app_ctx = gctx;

    ctx->db = gctx->db;
    ctx->db_uid = gctx->db_uid;

    tq_strings_free(&gctx->tags, free);
    ctx->tags = &gctx->tags;

    return 0;
}

/** Set flags related to log processing in TRC Update context */
static void
trc_update_ctx_set_log_flags(trc_update_ctx *ctx,
                             uint64_t flags)
{
    ctx->flags = ctx->flags & ~(TRC_UPDATE_FAKE_LOG |
                                TRC_UPDATE_MERGE_LOG |
                                TRC_UPDATE_FILT_LOG);
    ctx->flags = ctx->flags | flags;
}

/** Get next log to be processed */
static void
get_next_log(tqe_string **tqe_str, trc_update_tag_logs **tl,
             trc_update_ctx *gctx, trc_log_parse_ctx *ctx)
{
    ctx->log = NULL;

    if (*tl == NULL)
        return;

    if (*tqe_str == NULL)
        *tqe_str = TAILQ_FIRST(&(*tl)->logs);
    else
        *tqe_str = TAILQ_NEXT(*tqe_str, links);
        
    if (*tqe_str == NULL)
    {
        *tl = TAILQ_NEXT(*tl, links);
        if (*tl != NULL)
            *tqe_str = TAILQ_FIRST(&(*tl)->logs);
    }

    if (*tqe_str == NULL)
        return;

    ctx->log = (*tqe_str)->v;
    gctx->merge_expr = logic_expr_dup((*tl)->tags_expr);
    if ((*tl)->tags_str != NULL)
        gctx->merge_str = strdup((*tl)->tags_str);
}

/* See the description in trc_update.h */
te_errno
trc_update_process_logs(trc_update_ctx *gctx)
{
#define CHECK_F_RC(func_) \
    do {                         \
        if ((rc = (func_)) != 0) \
            goto cleanup;        \
    } while (0)

    int                         log_cnt = 0;
    te_errno                    rc = 0;
    trc_log_parse_ctx           ctx;
    trc_update_tag_logs        *tl = NULL;
    tqe_string                 *tqe_str = NULL;
    logic_expr                 *expr = NULL;

    FILE                       *tags_file = NULL;
    tqe_string                 *tqe_p = NULL;

    memset(&ctx, 0, sizeof(ctx));
    trc_update_init_parse_ctx(&ctx, gctx);

    if (gctx->flags & TRC_UPDATE_TAGS_GATHER)
    {
        tags_file = fopen(gctx->tags_gather_to, "w");
        if (tags_file == NULL)
        {
            ERROR("Failed to open %s", gctx->tags_gather_to);
            rc = -1;
            goto cleanup;
        }
    }

    tl = TAILQ_FIRST(&gctx->tags_logs);

    if (gctx->fake_log != NULL)
    {
        ctx.log = gctx->fake_log;
        trc_update_ctx_set_log_flags(gctx, TRC_UPDATE_FAKE_LOG);

        gctx->merge_str = strdup("UNSPEC");
        logic_expr_parse(gctx->merge_str, &expr);
        gctx->merge_expr = expr;
    }
    else
    {
        get_next_log(&tqe_str, &tl, gctx, &ctx);
    }

    if (gctx->flags & TRC_UPDATE_PRINT_PATHS)
    {
        trc_update_tests_group     *tests_group;

        do {
            if (ctx.log == NULL)
                break;
            trc_log_parse_process_log(&ctx);
            trc_update_init_parse_ctx(&ctx, gctx);
            get_next_log(&tqe_str, &tl, gctx, &ctx);
        } while (1);

        TAILQ_FOREACH(tests_group, &gctx->updated_tests, links)
            printf("%s\n", tests_group->path[0] == '/' ?
                                    tests_group->path + 1 :
                                    tests_group->path);

        goto cleanup;
    }

    RING("Parsing logs...");

    do {
        log_cnt++;
        gctx->cur_lnum = log_cnt;

        if (ctx.log == NULL)
            break;

        trc_log_parse_process_log(&ctx);

        if (gctx->flags & TRC_UPDATE_TAGS_GATHER)
            CHECK_F_RC(trc_update_collect_tags(gctx));

        if ((gctx->flags & TRC_UPDATE_SKIPPED) &&
            !(gctx->flags & TRC_UPDATE_FILT_LOG))
            trc_update_add_skipped(gctx);

        if (ctx.log == gctx->fake_log)
        {
            if (gctx->fake_filt_log != NULL)
            {
                /*
                 * Parse fake filter log just after fake log
                 * to mark some iterations as filtered.
                 */
                ctx.log = gctx->fake_filt_log;
                trc_update_ctx_set_log_flags(gctx, TRC_UPDATE_FILT_LOG);
                free(ctx.stack_info);
                ctx.stack_info = NULL;
                ctx.stack_pos = 0;
                ctx.stack_size = 0;

                continue;
            }
        }
  
        trc_update_init_parse_ctx(&ctx, gctx);

        get_next_log(&tqe_str, &tl, gctx, &ctx);

        if (!(gctx->flags & TRC_UPDATE_LOG_WILDS))
            trc_update_ctx_set_log_flags(gctx, TRC_UPDATE_MERGE_LOG);

    } while (1);

    if (gctx->logs_dump != NULL)
    {
        trc_update_ctx_set_log_flags(gctx, TRC_UPDATE_MERGE_LOG);
        trc_update_process_logs_dump(gctx);
    }

    if (gctx->fake_log == NULL &&
        !(gctx->flags & TRC_UPDATE_LOG_WILDS))
    {
        RING("Filling user data in TRC DB...");
        trc_update_fill_db_user_data(gctx->db,
                                     &gctx->updated_tests,
                                     gctx->db_uid);
    }

    if (gctx->flags & TRC_UPDATE_NO_SKIP_ONLY)
        trc_update_cond_res_op(gctx, is_skip_only, RESULT_OP_CLEAR);

    if (gctx->flags & TRC_UPDATE_NO_EXP_ONLY)
        trc_update_cond_res_op(gctx, is_exp_only, RESULT_OP_CLEAR);

    RING("Simplifying expected results...");
    CHECK_F_RC(trc_update_simplify_results(gctx->db_uid,
                                           &gctx->updated_tests,
                                           gctx->flags));

    if (gctx->flags & TRC_UPDATE_RULE_UPD_ONLY)
        trc_update_set_to_save(gctx->db,
                               gctx->db_uid,
                               FALSE);

    if (gctx->rules_load_from != NULL)
    {
        RING("Initializing updating rules structures...");
        trc_update_clear_rules(ctx.db_uid, &gctx->updated_tests);

        RING("Loading updating rules...");
        CHECK_F_RC(trc_update_load_rules(gctx->rules_load_from,
                                         &gctx->updated_tests,
                                         &gctx->global_rules,
                                         gctx->flags));
        
        RING("Applying updating rules...");
        CHECK_F_RC(trc_update_apply_rules(gctx->db_uid,
                                          &gctx->updated_tests,
                                          &gctx->global_rules,
                                          gctx->flags));
    }

    if (gctx->rules_save_to != NULL ||
        gctx->flags & TRC_UPDATE_GEN_APPLY)
    {
        trc_update_clear_rules(ctx.db_uid, &gctx->updated_tests);
        trc_update_rules_free(&gctx->global_rules);

        RING("Generating updating rules...");
        CHECK_F_RC(trc_update_gen_rules(ctx.db_uid,
                                        &gctx->updated_tests,
                                        gctx->flags));

        if (gctx->flags & TRC_UPDATE_GEN_APPLY)
        {
            RING("Applying updating rules...");
            CHECK_F_RC(trc_update_apply_rules(
                                    gctx->db_uid,
                                    &gctx->updated_tests,
                                    &gctx->global_rules,
                                    gctx->flags &
                                        ~TRC_UPDATE_RULES_CONFL));
        }
    }

    if (gctx->rules_save_to != NULL)
    {
        RING("Saving updating rules...");
        CHECK_F_RC(save_test_rules_to_file(
                                    &gctx->updated_tests,
                                    gctx->rules_save_to,
                                    gctx->cmd));
    }

    if ((gctx->rules_save_to == NULL ||
        (gctx->flags & TRC_UPDATE_GEN_APPLY)) &&
        !(gctx->flags & TRC_UPDATE_NO_GEN_WILDS))
    {
        RING("Generating wildcards...");
        CHECK_F_RC(trc_update_generate_wilds_gen(gctx->db_uid,
                                                 &gctx->updated_tests,
                                                 gctx->flags));
    }

    if ((gctx->flags & TRC_UPDATE_TAGS_GATHER) && tags_file != NULL)
    {
        TAILQ_FOREACH(tqe_p, &gctx->collected_tags, links)
            fprintf(tags_file, "%s\n", tqe_p->v);
    }

    RING("Done.");

cleanup:
    if (tags_file != NULL)
        fclose(tags_file);
    trc_update_clear_rules(gctx->db_uid, &gctx->updated_tests);
    trc_update_tests_groups_free(&gctx->updated_tests);
    trc_update_rules_free(&gctx->global_rules);
    return rc;

#undef CHECK_F_RC
}
