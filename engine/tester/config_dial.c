/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Tester Subsystem
 *
 * Implementation of --dial option.
 */

#define TE_LGR_USER "Run Dial"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <search.h>

#include "te_alloc.h"
#include "te_queue.h"
#include "te_str.h"
#include "te_string.h"
#include "tq_string.h"
#include "logger_api.h"
#include "tester_conf.h"
#include "tester_run.h"
#include "test_path.h"

/** Chosen iterations in a given scenario act */
typedef struct act_chosen {
    /** Acts queue links */
    TAILQ_ENTRY(act_chosen) links;

    /** First iteration */
    unsigned int first;
    /** Last iteration */
    unsigned int last;
    /** Scenario act flags */
    tester_flags flags;

    /**
     * Array corresponding to all iterations of a given act
     * where elements corresponding to chosen iterations are
     * set to 1.
     */
    uint8_t *chosen;
} act_chosen;

/** Head of the queue of act_chosen structures */
typedef TAILQ_HEAD(acts_chosen, act_chosen) acts_chosen;

/** Compute iterations count for a scenario act */
#define ITERS_NUM(p_) (((uint64_t)(p_)->last) - (p_)->first + 1)

struct dial_node;
/** Head of queue of dial_node structures */
typedef TAILQ_HEAD(dial_nodes, dial_node) dial_nodes;

/** A node in a tree of all test iterations */
typedef struct dial_node {
    /** Queue links (for children list) */
    TAILQ_ENTRY(dial_node) links;
    /** Parent node */
    struct dial_node *parent;
    /** Child nodes */
    dial_nodes children;

    /** Index of the first iteration */
    unsigned int first;
    /** Index of the last iteration */
    unsigned int last;
    /** Selection weight */
    unsigned int sel_weight;
    /** Total selection weight of all children together */
    unsigned int children_sel_weight;
    /**
     * Initial selection weight (before any iterations are
     * chosen and removed).
     */
    unsigned int init_sel_weight;
    /** Initial iterations number */
    unsigned int init_iters;
    /** Current iterations number */
    unsigned int cur_iters;
    /** Test path */
    char *path;

    /**
     * TRUE if this node was created by splitting parent
     * node after removing chosen iteration from it.
     */
    te_bool split;
    /**
     * TRUE if this is a leaf node from which iterations
     * can be chosen.
     */
    te_bool leaf;

    /** Associated run item */
    const run_item *ri;

    /** Associated scenario act */
    act_chosen *act_ptr;
} dial_node;

/** Context for walking configuration tree */
typedef struct dial_ctx {
    /** Current node in selection tree */
    dial_node *cur_node;
    /**
     * Skip counter. Node is not created if
     * it is positive.
     */
    unsigned int skip;
} dial_ctx;

/** Iterations count for test path */
typedef struct path_iters {
    /** Test path */
    char *path;
    /** Iterations count */
    uint64_t iters;
} path_iters;

/* Default initial weight */
#define DEF_INIT_WEIGHT 100

/* Remove and free all elements from acts_chosen queue */
static void
acts_chosen_free(acts_chosen *head)
{
    act_chosen *act;

    while ((act = TAILQ_FIRST(head)) != NULL)
    {
        TAILQ_REMOVE(head, act, links);
        free(act->chosen);
        free(act);
    }
}

/* Initialize a node */
static void
node_init(dial_node *node)
{
    memset(node, 0, sizeof(*node));
    TAILQ_INIT(&node->children);
}

/*
 * Append (part of) a selection tree to TE string with a given
 * indentation.
 */
static void
dial_tree2str(te_string *str, dial_node *node, unsigned int indent)
{
    dial_node *child;

    te_string_append(str, "%*s[%u, %u]: %u/%u",
                     indent, "", node->first, node->last,
                     node->sel_weight, node->children_sel_weight);
    if (node->ri != NULL)
    {
        const char *name = run_item_name(node->ri);

        te_string_append(str, " %s", ri_type2str(node->ri->type));
        if (name != NULL)
            te_string_append(str, " %s", name);
    }

    if (node->act_ptr != NULL)
        te_string_append(str, " -> %p", node->act_ptr);

    te_string_append(str, "\n");

    TAILQ_FOREACH(child, &node->children, links)
    {
        dial_tree2str(str, child, indent + 2);
    }
}

/* Auxiliary function for logging selection tree */
static void
dial_tree_print_aux(dial_node *node, te_log_level level,
                    const char *stage)
{
    te_string str = TE_STRING_INIT;

    dial_tree2str(&str, node, 0);
    LOG_MSG(level, "Selection tree for --dial option%s%s%s:\n%s",
            stage ? " (" : "", stage, stage ? ")" : "", str.ptr);
    te_string_free(&str);
}

/* Log selection tree with a given log level */
static void
dial_tree_print(dial_node *node, te_log_level level, const char *stage)
{
    TE_DO_IF_LOG_LEVEL(level, dial_tree_print_aux(node, level, stage));
}

/* Destroy selection tree releasing memory */
static void
dial_tree_free(dial_node *node)
{
    dial_node *p;
    dial_node *p_aux;

    if (node == NULL)
        return;

    TAILQ_FOREACH_SAFE(p, &node->children, links, p_aux)
    {
        TAILQ_REMOVE(&node->children, p, links);
        dial_tree_free(p);
    }

    free(node->path);
    free(node);
}

/* Add a child node */
static void
node_add_child(dial_node *node, dial_node *child)
{
    TAILQ_INSERT_TAIL(&node->children, child, links);
    child->parent = node;
    assert(UINT_MAX - child->sel_weight >= node->children_sel_weight);
    node->children_sel_weight += child->sel_weight;
}

/*
 * Clone selection tree node with its children adding a given
 * offset to first/last numbers. This is used when we have
 * a run item which contains other run items and is iterated as
 * a whole multiple times.
 */
static dial_node *
node_clone(dial_node *node, unsigned int id_off)
{
    dial_node *cloned_node;
    dial_node *child;
    dial_node *cloned_child;

    cloned_node = TE_ALLOC(sizeof(*cloned_node));
    node_init(cloned_node);

    cloned_node->first = node->first + id_off;
    cloned_node->last = node->last + id_off;
    cloned_node->init_iters = node->init_iters;
    cloned_node->cur_iters = node->cur_iters;
    /* Will be filled automatically when adding children */
    cloned_node->children_sel_weight = 0;
    cloned_node->sel_weight = node->sel_weight;
    cloned_node->init_sel_weight = node->init_sel_weight;
    cloned_node->split = node->split;
    cloned_node->leaf = node->leaf;
    cloned_node->ri = node->ri;

    TAILQ_FOREACH(child, &node->children, links)
    {
        cloned_child = node_clone(child, id_off);
        node_add_child(cloned_node, cloned_child);
    }

    return cloned_node;
}

/* Remove a node from selection tree */
static void
node_del(dial_node *node)
{
    dial_node *parent = node->parent;

    /* Never remove tree root */
    if (parent != NULL)
    {
        TAILQ_REMOVE(&parent->children, node, links);

        assert(parent->children_sel_weight >= node->sel_weight);
        parent->children_sel_weight -= node->sel_weight;

        if (TAILQ_EMPTY(&parent->children))
        {
            assert(parent->children_sel_weight == 0);
            node_del(parent);
        }

        dial_tree_free(node);
    }
    else
    {
        /*
         * If root node is the only leaf and the last
         * iteration was taken from it, mark it as
         * non-leaf to tell select_test_iter() that it is
         * empty now. We cannot remove root here,
         * tester_get_dial_scenario() will remove it
         * itself.
         */
        node->leaf = FALSE;
    }
}

/* Start selection tree node */
static void
node_start(unsigned int cfg_id_off, unsigned int self_iters,
           unsigned int inner_iters, dial_ctx *ctx,
           run_item *ri)
{
    dial_node *node;

    if (inner_iters == 0 || (ri != NULL && ri->role != RI_ROLE_NORMAL) ||
        ctx->skip)
    {
        ctx->skip++;
        return;
    }

    if (self_iters > 1)
    {
        /*
         * If content of a node is iterated more than once, create
         * intermediary parent whose children will be iteration
         * instances.
         */
        node_start(cfg_id_off, 1, inner_iters * self_iters, ctx, ri);

        /*
         * Do not set run item in auxiliary nodes: currently it
         * is used only for debugging logs, seems better to log
         * run item name and type only for the main node related
         * to it.
         */
        ri = NULL;
    }

    node = TE_ALLOC(sizeof(*node));
    node_init(node);
    node->ri = ri;
    node->first = cfg_id_off;
    node->last = cfg_id_off + inner_iters - 1;
    node->cur_iters = node->init_iters = inner_iters;

    if (ri != NULL && ri->type == RUN_ITEM_SCRIPT)
        node->leaf = TRUE;

    node_add_child(ctx->cur_node, node);
    ctx->cur_node = node;
}

/* Terminate selection tree node */
static void
node_end(unsigned int self_iters, dial_ctx *ctx)
{
    dial_node *parent;
    dial_node *cloned_node;
    unsigned int i;
    unsigned int node_off;

    if (ctx->skip)
    {
        ctx->skip--;
        return;
    }

    parent = ctx->cur_node->parent;

    node_off = ctx->cur_node->last - ctx->cur_node->first + 1;
    for (i = 1; i < self_iters; i++)
    {
        cloned_node = node_clone(ctx->cur_node, node_off * i);
        node_add_child(parent, cloned_node);
    }

    ctx->cur_node = parent;

    /* Close intermediary parent if it was created before */
    if (self_iters > 1)
        node_end(1, ctx);
}

/* Callback for iteration start in configuration tree */
static tester_cfg_walk_ctl
iter_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
           unsigned int iter, void *opaque)
{
    UNUSED(ri);
    UNUSED(cfg_id_off);
    UNUSED(flags);
    UNUSED(opaque);

    /*
     * Other iterations are taken into account when cloning
     * a node.
     */
    if (iter > 0)
        return TESTER_CFG_WALK_SKIP;

    return TESTER_CFG_WALK_CONT;
}

/* Callback for run item start in configuration tree */
static tester_cfg_walk_ctl
ri_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
         void *opaque)
{
    node_start(cfg_id_off,
               ri->type == RUN_ITEM_SCRIPT ? 1 : ri->n_iters,
               ri->type == RUN_ITEM_SCRIPT ? ri->n_iters : ri->weight,
               opaque, ri);

    return TESTER_CFG_WALK_CONT;
}

/* Callback for run item end in configuration tree */
static tester_cfg_walk_ctl
ri_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
       void *opaque)
{
    if (ri->type == RUN_ITEM_SCRIPT)
        node_end(1, opaque);
    else
        node_end(ri->n_iters, opaque);

    return TESTER_CFG_WALK_CONT;
}

/* Callback for configuration start in configuration tree */
static tester_cfg_walk_ctl
cfg_start(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    node_start(cfg_id_off, 1, cfg->total_iters, opaque, NULL);
    return TESTER_CFG_WALK_CONT;
}

/* Callback for configuration end in configuration tree */
static tester_cfg_walk_ctl
cfg_end(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    node_end(1, opaque);
    return TESTER_CFG_WALK_CONT;
}

/* Compare two path_iters structures by stored paths */
static int
compare_path_iters(const void *a, const void *b)
{
    return strcmp(((path_iters *)a)->path, ((path_iters *)b)->path);
}

/* Release memory allocated for path_iters structure */
static void
free_path_iters(void *arg)
{
    path_iters *iters = arg;

    free(iters->path);
    free(iters);
}

/*
 * Find out all the unique test paths and total iterations count
 * for every one of them.
 */
static void
count_path_iters(dial_node *node, const char *path, void **path_tree)
{
    path_iters *iters;
    path_iters *iters_found;
    void *result;
    te_string str = TE_STRING_INIT;
    const char *name = NULL;
    dial_node *child;

    if (node->ri != NULL)
    {
        name = run_item_name(node->ri);

        if (!te_str_is_null_or_empty(name))
        {
            if (!te_str_is_null_or_empty(path))
                te_string_append(&str, "%s/", path);

            te_string_append(&str, "%s", name);

            path = str.ptr;

            if (node->ri->type == RUN_ITEM_SCRIPT)
            {
                iters = TE_ALLOC(sizeof(*iters));
                iters->path = strdup(path);
                iters->iters = node->init_iters;
                assert(iters->iters != 0);

                node->path = strdup(path);

                result = tsearch(iters, path_tree, compare_path_iters);
                if (result == NULL)
                    TE_FATAL_ERROR("Failed to add new path to the tree");

                iters_found = *(path_iters **)result;
                if (iters_found != iters)
                {
                    iters_found->iters += iters->iters;
                    free_path_iters(iters);
                }
            }
        }
    }

    TAILQ_FOREACH(child, &node->children, links)
    {
        count_path_iters(child, path, path_tree);
    }

    te_string_free(&str);
}

/*
 * Assign selection weights to nodes taking into account
 * how many unique test paths are represented by a given
 * node.
 */
static void
set_weights_by_paths(dial_node *node, void **path_tree)
{
    dial_node *child;

    node->children_sel_weight = 0;
    TAILQ_FOREACH(child, &node->children, links)
    {
        set_weights_by_paths(child, path_tree);
        assert(UINT_MAX - child->sel_weight >= node->children_sel_weight);
        node->children_sel_weight += child->sel_weight;
    }

    if (node->path != NULL)
    {
        void *result;
        path_iters *iters;
        path_iters key;

        key.path = node->path;

        result = tfind(&key, path_tree, compare_path_iters);
        if (result == NULL)
            TE_FATAL_ERROR("Path '%s' was not found", node->path);

        iters = *(path_iters **)result;

        /*
         * If it is the only node matching a given unique test path,
         * its weight is exactly default initial one. If there are a
         * few nodes corresponding to the same unique path (for instance,
         * the single test is described by multiple run items),
         * then every one of them gets part of the default initial
         * weight proportional to the share of iterations for
         * the path contained in this node.
         */
        assert(node->init_iters <= iters->iters);
        node->sel_weight = DEF_INIT_WEIGHT * node->init_iters / iters->iters;
        if (node->sel_weight == 0)
            node->sel_weight = 1;
    }
    else if (node->act_ptr != NULL)
    {
        /*
         * Auxiliary node representing (part of) a scenario act.
         * Let its selection weight to be equal to the number
         * of iterations in it.
         */
        node->sel_weight = node->init_iters;
    }
    else
    {
        /*
         * For higher level nodes set their weights to
         * sum of the weights of their children.
         */
        node->sel_weight = node->children_sel_weight;
    }

    assert(node->sel_weight != 0);
    node->init_sel_weight = node->sel_weight;
}

/* Set initial weights for tree nodes */
static void
set_init_weights(dial_node *node)
{
    void *path_tree = NULL;

    count_path_iters(node, NULL, &path_tree);
    set_weights_by_paths(node, &path_tree);

    tdestroy(path_tree, free_path_iters);
}

/* Construct selection tree */
static te_errno
dial_tree_construct(const struct tester_cfgs *cfgs,
                    dial_node **root_out)
{
    dial_node *root = NULL;
    dial_ctx ctx;

    const tester_cfg_walk cbs = {
        .cfg_start = cfg_start,
        .cfg_end = cfg_end,
        .run_start = ri_start,
        .run_end = ri_end,
        .iter_start = iter_start
    };
    tester_cfg_walk_ctl ctl;

    if (cfgs->total_iters == 0)
    {
        ERROR("%s(): no iterations available", __FUNCTION__);
        return TE_ENOENT;
    }

    root = TE_ALLOC(sizeof(*root));
    node_init(root);
    root->sel_weight = 1;
    root->last = cfgs->total_iters - 1;
    root->cur_iters = root->init_iters = cfgs->total_iters;

    memset(&ctx, 0, sizeof(ctx));
    ctx.cur_node = root;

    ctl = tester_configs_walk(cfgs, &cbs, 0, &ctx);
    if (ctl != TESTER_CFG_WALK_CONT)
    {
        ERROR("%s(): failed to walk configuration tree", __FUNCTION__);
        dial_tree_free(root);
        return TE_EFAIL;
    }

    dial_tree_print(root, TE_LL_INFO, "initial");

    *root_out = root;
    return 0;
}

/*
 * Adjust selection weights for a node and its ancestors after
 * removing chosen iteration from the node.
 */
static void
adjust_weights(dial_node *node)
{
    unsigned int cur_sel_weight;
    unsigned int weight_diff;

    cur_sel_weight = node->sel_weight;
    node->cur_iters--;

    if (node->init_sel_weight == node->init_iters)
    {
        /* Use integer arithmetic if possible */
        node->sel_weight--;
    }
    else
    {
        node->sel_weight = (double)(node->cur_iters) / node->init_iters *
                                node->init_sel_weight;
        if (node->sel_weight < node->init_sel_weight)
            node->sel_weight++;
        assert(node->sel_weight <= cur_sel_weight);
    }

    if (node->parent != NULL)
    {
        weight_diff = cur_sel_weight - node->sel_weight;
        node->parent->children_sel_weight -= weight_diff;

        adjust_weights(node->parent);
    }
}

/*
 * Randomly select an iteration from selection tree, removing that
 * iteration from the tree.
 *
 * Probability of choosing a given node is proportional to its selection
 * weight. Selection starts from the root and, until leaf node is achieved,
 * for every current node one of its children is chosen randomly and
 * then the process is repeated for the chosen child.
 * From leaf node one of the iterations is randomly chosen.
 * By default all children have the same selection weight (1).
 */
static te_errno
select_test_iter(dial_node *node, unsigned int *iter_out,
                 act_chosen **act_ptr)
{
    if (TAILQ_EMPTY(&node->children))
    {
        unsigned int iter = rand_range(node->first, node->last);

        if (!node->leaf)
        {
            if (node->parent != NULL)
            {
                ERROR("Stopped at non-root node which is not a leaf");
            }
            return TE_ENOENT;
        }

        *iter_out = iter;
        if (act_ptr != NULL)
            *act_ptr = node->act_ptr;

        adjust_weights(node);

        if (node->first == node->last)
        {
            /* The only remaining iteration is gone */
            node_del(node);
        }
        else if (iter == node->first)
        {
            /* First iteration is gone */
            node->first++;
        }
        else if (iter == node->last)
        {
            /* Last iteration is gone */
            node->last--;
        }
        else if (node->split)
        {
            dial_node *brother;

            /*
             * We take iteration in the middle, so that we
             * now have two ranges instead of a single one
             * and each range should be represented by
             * a separate node.
             *
             * Here the current node is already a node
             * obtained by splitting its parent, so we
             * leave one range in it and create brother
             * for the remaining range.
             */

            /* Weight should already be ajusted */
            assert(node->sel_weight == node->last - node->first);

            brother = TE_ALLOC(sizeof(*brother));
            node_init(brother);
            brother->first = iter + 1;
            brother->last = node->last;
            brother->sel_weight = node->last - iter;
            brother->init_sel_weight = brother->sel_weight;
            brother->cur_iters = brother->init_iters = brother->sel_weight;
            brother->split = TRUE;
            brother->leaf = TRUE;
            brother->act_ptr = node->act_ptr;
            brother->parent = node->parent;

            node->last = iter - 1;
            node->sel_weight = iter - node->first;

            TAILQ_INSERT_AFTER(&node->parent->children,
                               node, brother, links);
        }
        else
        {
            dial_node *left;
            dial_node *right;

            /*
             * Here we too take an iteration in the middle,
             * but the current node is original one and must
             * be left intact with its existing selection
             * weight. So we create two children for
             * representing two resulting ranges.
             */

            left = TE_ALLOC(sizeof(*left));
            right = TE_ALLOC(sizeof(*right));

            node_init(left);
            left->split = TRUE;
            left->first = node->first;
            left->last = iter - 1;
            left->sel_weight = iter - node->first;
            left->init_sel_weight = left->sel_weight;
            left->cur_iters = left->init_iters = left->sel_weight;
            left->parent = node;
            left->leaf = TRUE;
            left->act_ptr = node->act_ptr;

            node_init(right);
            right->split = TRUE;
            right->first = iter + 1;
            right->last = node->last;
            right->sel_weight = node->last - iter;
            right->init_sel_weight = right->sel_weight;
            right->cur_iters = right->init_iters = right->sel_weight;
            right->parent = node;
            right->leaf = TRUE;
            right->act_ptr = node->act_ptr;

            node->leaf = FALSE;
            node->children_sel_weight = node->last - node->first;

            TAILQ_INSERT_TAIL(&node->children, left, links);
            TAILQ_INSERT_TAIL(&node->children, right, links);
        }
    }
    else
    {
        /*
         * This is not a leaf node, choose one of its children
         * randomly taking into account selection weights.
         */

        unsigned int chosen_val;
        dial_node *child;
        unsigned int total_weight = 0;

        assert(node->children_sel_weight < INT_MAX &&
               node->children_sel_weight < RAND_MAX + 1LLU);
        chosen_val = rand_range(1, node->children_sel_weight);

        TAILQ_FOREACH(child, &node->children, links)
        {
            if (chosen_val > total_weight &&
                chosen_val <= total_weight + child->sel_weight)
            {
                return select_test_iter(child, iter_out, act_ptr);
            }

            total_weight += child->sel_weight;
        }

        ERROR("%s(): failed to choose one of the children, weights "
              "may be wrong", __FUNCTION__);
        return TE_EINVAL;
    }

    return 0;
}

/*
 * Add iteration from a scenario to a selection tree. Iteration
 * is added to some child of an original leaf. Reference
 * to the scenario act containing the added iteration is stored
 * together with added iteration to keep information about
 * scenario sequence.
 */
static te_errno
add_from_scen(dial_node **cur_node, unsigned int iter,
              act_chosen *act_ptr)
{
    dial_node *node = *cur_node;
    dial_node *child;

    if (node == NULL)
        return TE_ENOENT;

    if (node->act_ptr == act_ptr)
    {
        /* Try to extend current node if possible. */
        if (node->parent != NULL && iter >= node->parent->first &&
            iter <= node->parent->last)
        {
            if (node->first > iter && iter == node->first - 1)
            {
                node->first--;
                assert(node->init_iters < UINT_MAX);
                node->init_iters++;
                node->cur_iters++;
                return 0;
            }
            else if (node->last < iter && iter == node->last + 1)
            {
                node->last++;
                assert(node->init_iters < UINT_MAX);
                node->init_iters++;
                node->cur_iters++;
                return 0;
            }
        }
    }

    if (node->act_ptr != NULL)
    {
        /* Current node is new leaf and it cannot be extended */
        node = *cur_node = node->parent;
        if (node == NULL)
        {
            ERROR("%s(): cannot find a place for iteration %u",
                  __FUNCTION__, iter);
            return TE_ENOENT;
        }
    }

    assert(node->act_ptr == NULL);

    if (iter >= node->first && iter <= node->last)
    {
        if (node->leaf)
        {
            child = TE_ALLOC(sizeof(*child));
            node_init(child);
            child->init_iters = 1;
            child->cur_iters = 1;
            child->first = child->last = iter;
            child->act_ptr = act_ptr;
            node_add_child(node, child);
            *cur_node = child;
            return 0;
        }
        else
        {
            TAILQ_FOREACH(child, &node->children, links)
            {
                if (iter >= child->first && iter <= child->last)
                {
                    assert(child->act_ptr == NULL);
                    *cur_node = child;
                    return add_from_scen(cur_node, iter, act_ptr);
                }
            }

            ERROR("%s(): iteration %u fits into a given non-leaf "
                  "node but not to any of its children", __FUNCTION__,
                  iter);
            return TE_ENOENT;
        }
    }

    *cur_node = node->parent;
    return add_from_scen(cur_node, iter, act_ptr);
}

/*
 * After adding iterations from scenario as new leafs to
 * selection tree, mark the newly added nodes as leafs
 * (while unmarking their parents) and remove all nodes
 * not having new leafs as descendants.
 */
static unsigned int
normalize_after_adding(dial_node *node)
{
    dial_node *child;
    dial_node *child_aux;
    unsigned int result = 0;
    unsigned int result_aux = 0;

    if (node->act_ptr != NULL)
    {
        node->leaf = TRUE;
        return node->init_iters;
    }

    TAILQ_FOREACH_SAFE(child, &node->children, links, child_aux)
    {
        result_aux = normalize_after_adding(child);
        if (result_aux == 0)
        {
            TAILQ_REMOVE(&node->children, child, links);
            free(child);
        }

        assert(UINT_MAX - result_aux >= result);
        result = result + result_aux;
    }

    node->cur_iters = node->init_iters = result;
    node->leaf = FALSE;

    return result;
}

/*
 * If some iterations were chosen from a given act of original
 * scenario, add one or more acts describing them to the
 * constructed scenario.
 */
te_errno
act_to_scenario(act_chosen *act, testing_scenario *scenario)
{
    unsigned int i;
    unsigned int start = 0;
    te_bool started = FALSE;
    te_errno rc;

    for (i = act->first; i <= act->last; i++)
    {
        if (act->chosen[i - act->first])
        {
            if (!started)
            {
                start = i;
                started = TRUE;
            }
        }
        else if (started)
        {
            started = FALSE;
            rc = scenario_add_act(scenario, start, i - 1, act->flags, NULL);
            if (rc != 0)
                return rc;
        }

        if (i == UINT_MAX)
            break;
    }

    if (started)
    {
        rc = scenario_add_act(scenario, start, act->last, act->flags, NULL);
        if (rc != 0)
            return rc;
    }

    return 0;
}

/*
 * Add iterations from the original scenario as new leafs
 * to the selection tree. Then remove from it all nodes
 * not having new leafs as descendants. After that selection
 * tree can be used to choose randomly iterations belonging
 * to a given scenario (taking into account selection
 * weights at all levels).
 */
static te_errno
process_original_scenario(testing_scenario *scenario,
                          dial_node *root,
                          acts_chosen *iters,
                          uint64_t *total_iters)
{
    dial_node *cur_node;
    act_chosen *act_iters;
    testing_act *act;
    unsigned int i;
    te_errno rc = 0;
    uint64_t iters_count = 0;

    cur_node = root;
    TAILQ_FOREACH(act, scenario, links)
    {
        uint64_t num;

        num = ITERS_NUM(act);

        act_iters = TE_ALLOC(sizeof(*act_iters));
        act_iters->first = act->first;
        act_iters->last = act->last;
        act_iters->flags = act->flags;
        act_iters->chosen = TE_ALLOC(num * sizeof(*(act_iters->chosen)));

        iters_count += num;

        TAILQ_INSERT_TAIL(iters, act_iters, links);

        for (i = act->first; i <= act->last; i++)
        {
            /*
             * It should be fine for an iteration to be added more than
             * once to the selection tree (with references to different
             * scenario acts).
             */
            rc = add_from_scen(&cur_node, i, act_iters);
            if (rc != 0)
                return rc;

            if (i == UINT_MAX)
                break;
        }
    }

    normalize_after_adding(root);
    *total_iters = iters_count;

    return 0;
}

/*
 * Choose requested number of iterations from selection tree.
 * It is assumed here that selection tree leafs hold links to
 * constructed scenario acts where the choice can be saved.
 */
static te_errno
choose_iters(dial_node *root, uint64_t select_num)
{
    uint64_t i;
    act_chosen *act_iters;
    te_errno rc;

    dial_tree_print(root, TE_LL_INFO,
                    "before choosing and removing iterations");

    for (i = 0; i < select_num; i++)
    {
        unsigned int iter;
        unsigned int iter_idx;

        rc = select_test_iter(root, &iter, &act_iters);
        if (rc != 0)
        {
            ERROR("%s(): failed to choose test iteration", __FUNCTION__);
            return rc;
        }
        if (act_iters == NULL)
        {
            ERROR("%s(): chosen iteration is not linked to scenario act",
                  __FUNCTION__);
            return TE_ENOENT;
        }
        assert(iter >= act_iters->first && iter <= act_iters->last);

        iter_idx = iter - act_iters->first;
        if (act_iters->chosen[iter_idx])
        {
            ERROR("%s(): choosing the second time the same iteration from "
                  "the same act", __FUNCTION__);
            return TE_ENOENT;
        }

        act_iters->chosen[iter_idx] = 1;
    }

    dial_tree_print(root, TE_LL_INFO,
                    "after choosing and removing iterations");
    return 0;
}

/* See description in tester_run.h */
te_errno
scenario_apply_dial(testing_scenario *scenario,
                    const struct tester_cfgs *cfgs,
                    double dial)
{
    dial_node *root = NULL;
    acts_chosen iters;
    act_chosen *act_iters;
    uint64_t total_iters = 0;
    uint64_t select_num;
    te_errno rc = 0;

    TAILQ_INIT(&iters);

    /*
     * Create selection tree containing information about all
     * the existing iterations.
     */
    rc = dial_tree_construct(cfgs, &root);
    if (rc != 0)
        goto cleanup;

    /*
     * Add iterations from the original scenario as new leafs
     * to the selection tree (with references to act structures
     * of the newly constructed scenario).
     */
    rc = process_original_scenario(scenario, root, &iters,
                                   &total_iters);
    if (rc != 0)
        goto cleanup;

    /*
     * After final tree is constructed, set initial selection weights
     * for its nodes.
     */
    set_init_weights(root);

    /*
     * Compute exact number of iterations we should choose.
     * Choose this number of iterations randomly from the
     * selection tree.
     */
    select_num = total_iters * dial / 100.0;
    rc = choose_iters(root, select_num);
    if (rc != 0)
        goto cleanup;

    /*
     * Release original scenario and construct a new one in its place
     * containing only the chosen iterations (while keeping sequence
     * specified in the original scenario).
     */
    scenario_free(scenario);

    TAILQ_FOREACH(act_iters, &iters, links)
    {
        rc = act_to_scenario(act_iters, scenario);
        if (rc != 0)
            goto cleanup;
    }

cleanup:

    dial_tree_free(root);
    acts_chosen_free(&iters);
    return rc;
}
