/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Auxiluary routines for TRC update tool.
 *
 *
 * Copyright (C) 2011 Test Environment authors (see file AUTHORS
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#include "te_alloc.h"
#include "te_queue.h"
#include "trc_update.h"

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
    memset(data, sizeof(*data), 0);
    SLIST_INIT(&data->new_results);
    SLIST_INIT(&data->df_results);
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

    if (data == NULL)
        return;

    SLIST_FOREACH_SAFE(q, &data->new_results, links, q_tvar)
    {
        SLIST_REMOVE_HEAD(&data->new_results, links);
        trc_exp_result_free(q);
    }

    SLIST_FOREACH_SAFE(q, &data->df_results, links, q_tvar)
    {
        SLIST_REMOVE_HEAD(&data->df_results, links);
        trc_exp_result_free(q);
    }

    SLIST_FOREACH_SAFE(args_group, &data->all_wilds,
                       links, args_group_tvar)
    {
        SLIST_REMOVE_HEAD(&data->all_wilds, links);
        trc_update_args_group_free(args_group);
    }

    for (data->args_n = 0; data->args_n < data->args_max; data->args_n++)
    {
        free(data->args[data->args_n].name);
        free(data->args[data->args_n].value);
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
    memset(ctx_p, 0, sizeof(ctx_p));
    TAILQ_INIT(&ctx_p->test_names);
    TAILQ_INIT(&ctx_p->tags_logs);
    TAILQ_INIT(&ctx_p->diff_logs);
    TAILQ_INIT(&ctx_p->tags_list);
}

/* See the description in trc_update.h */
void
trc_update_free_ctx(trc_update_ctx *ctx)
{
    if (ctx == NULL)
        return;

    trc_update_tags_logs_free(&ctx->tags_logs);
    trc_update_tags_logs_free(&ctx->diff_logs);
    tq_strings_free(&ctx->tags_list, free);
    tq_strings_free(&ctx->test_names, free);
    free(ctx->fake_log);
    free(ctx->fake_filt_log);
    free(ctx->rules_load_from);
    free(ctx->rules_save_to);
    free(ctx->cmd);
    free(ctx->tags_gather_to);
}

/* See the description in trc_update.h */
void
tag_logs_init(trc_update_tag_logs *tag_logs)
{
    memset(tag_logs, 0, sizeof(tag_logs));
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
int
trc_update_rentry_cmp(trc_exp_result_entry *p,
                      trc_exp_result_entry *q)
{
    int rc = 0;

    DUMMY_CMP(p, q);

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
    te_bool              p_is_void = (p == NULL || SLIST_EMPTY(p));
    te_bool              q_is_void = (q == NULL || SLIST_EMPTY(q));

    if (p_is_void && q_is_void)
        return 0;
    else if (!p_is_void && q_is_void)
        return 1;
    else if (p_is_void && !q_is_void)
        return -1;

    p_r = SLIST_FIRST(p);
    q_r = SLIST_FIRST(q);

    while (p_r != NULL && q_r != NULL)
    {
        rc = trc_update_result_cmp(p_r, q_r);
        if (rc != 0)
            return rc;

        p_r = SLIST_NEXT(p_r, links);
        q_r = SLIST_NEXT(q_r, links);
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
        case TRC_UPDATE_RRESULTS:
            rc = trc_update_result_cmp(p->def_res, q->def_res);
            if (rc != 0)
                return rc;
            rc = trc_update_results_cmp(p->confl_res, q->confl_res);
            if (rc != 0)
                return rc;

            /*@fallthrough@*/

        case TRC_UPDATE_RRESULT:
            rc = trc_update_results_cmp(p->new_res, q->new_res);
            if (rc != 0)
                return rc;
            rc = trc_update_results_cmp(p->old_res, q->old_res);
            return rc;

            break;

        case TRC_UPDATE_RRENTRY:
            rc = trc_update_rentry_cmp(p->new_re, q->new_re);
            if (rc != 0)
                return rc;
            rc = trc_update_rentry_cmp(p->old_re, q->old_re);
            return rc;
            break;

        case TRC_UPDATE_RVERDICT:
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
    {
        TAILQ_INSERT_BEFORE(next, rule, links);
#if 0
        printf("The rule is %s\n",
               rule->type == TRC_UPDATE_RRENTRY ?
               "entry" : "result");
        printf("The next is %s\n",
               next->type == TRC_UPDATE_RRENTRY ?
               "entry" : "result");
#endif
    }
    else
    {
        TAILQ_INSERT_TAIL(rules, rule, links);

#if 0
        printf("The tail is %s\n",
               rule->type == TRC_UPDATE_RRENTRY ?
               "entry" : "result");
#endif
    }

    return 0;
}
