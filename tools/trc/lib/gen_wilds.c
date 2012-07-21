/** @file
 * @brief Testing Results Comparator: update tool
 *
 * Implementation of agrorithms used to generate wildcards
 * from full subset structure build for a given test. Full subset
 * structure contains for each possible iteration record (including
 * wildcards) the set of iterations described by it. The task is to
 * select as small as possible number of (wildcard) iteration records
 * enough to describe the test.
 *
 * Copyright (C) 2005-2012 Test Environment authors (see file AUTHORS
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include "gen_wilds.h"
#include "te_defs.h"

/** Cell of table structure used by DLX algorithm */
typedef struct dlx_cell dlx_cell;
struct dlx_cell {
    dlx_cell    *L;     /**< Left neighbor */
    dlx_cell    *R;     /**< Right neighbor */
    dlx_cell    *U;     /**< Upper neighbor */
    dlx_cell    *D;     /**< Down neighbor */
    dlx_cell    *C;     /**< Head of column elements list */

    int     set_id;     /**< Number of set in array of all sets */
    int     elm_id;     /**< Number of element */
};

/**
 * Generate problem representation required to use DLX algorithm.
 *
 * @param p     Default representation of problem
 *
 * @return Pointer to the head of table used by DLX algorithm.
 */
static dlx_cell *
gen_dlx(problem *p)
{
    dlx_cell    *h;
    dlx_cell    *c;
    dlx_cell    *prev_e;
    dlx_cell    *prev_u = NULL;
    dlx_cell    *e;

    int i;
    int j;

    h = calloc(1, sizeof(*h));

    h->U = NULL;
    h->D = NULL;
    h->L = h;
    h->R = h;
    h->set_id = -1;

    for (i = p->elm_num - 1; i >= 0; i--)
    {
        c = calloc(1, sizeof(*c));
        c->U = c->D = c;
        c->L = h;
        c->R = h->R;
        c->R->L = c;
        c->L->R = c;
        c->C = c;

        c->set_id = -1;
        c->elm_id = i;
    }

    for (i = 0; i < p->set_num; i++)
    {
        prev_e = NULL;
        for (j = 0; j < p->sets[i].num; j++)
        {
            e = calloc(1, sizeof(*e)); 
            e->set_id = i;
            e->elm_id = p->sets[i].els[j];

            if (prev_e != NULL)
            {
                e->L = prev_e;
                e->R = prev_e->R;
                e->L->R = e;
                e->R->L = e;
            }
            else
            {
                e->L = e;
                e->R = e;
            }

            c = h;

            do {
                c = c->R;
                if (c == h)
                    break;

                if (c->elm_id == e->elm_id)
                    break;
            } while (1);

            assert(c != h);

            e->C = c;
            
            for (prev_u = c; prev_u->D != c; prev_u = prev_u->D);

            e->U = prev_u;
            e->D = prev_u->D;
            e->U->D = e;
            e->D->U = e;

            prev_e = e;
        }
    }

    return h;
}

/**
 * Release memory in which problem representation used by DLX algorithm
 * was stored.
 *
 * @param h     Head of table used by DLX algorithm pointer
 */
static void
free_dlx(dlx_cell *h)
{
    dlx_cell    *c;
    dlx_cell    *e;
    dlx_cell    *tmp;

    for (c = h->R; c != h; )
    {
        for (e = c->D; e != c; )
        {
            e->R->L = e->L;
            e->L->R = e->R;
            e->D->U = e->U;
            e->U->D = e->D;
            tmp = e;
            e = e->D;
            free(tmp);
        }

        tmp = c;
        c = c->R;
        free(tmp);
    }

    free(h);
}
 
/**
 * Release memory in which set of numbers was stored.
 *
 * @param s     Set structure pointer
 */
static void
set_free(set *s)
{
    free(s->els);
}

/** Described in gen_wilds.h */
void
problem_free(problem *p)
{
    int i;

    for (i = 0; i < p->set_num; i++)
        set_free(&p->sets[i]);

    free(p->sets);
    free(p->sol);
    p->set_num = p->set_max = p->elm_num = p->sol_num = 0;
}

/** In this array currently found solution is stored */
static int *O = NULL;
/** In this array the best solution found by now is stored */
static int *O_min = NULL;
/** Number of elements in "O" array */
static int  N = 0;
/** Number of elements in "O_min" array */
static int  N_min = 0;
/** Number of solutions found by the moment */
static unsigned long int  solutions_found;
/** When to stop DLX algorithm */
static struct timeval *time_to_stop = NULL;
/**
 * Whether DLX algorithm looked through all the possible solutions
 * by the moment or not
 */
static te_bool work_done = FALSE;

/**
 * Cover column in DLX table. It means excluding the element
 * covered by a selected set from further consideration.
 *
 * @param c     Column head
 */
static void
column_cover(dlx_cell *c)
{
    dlx_cell *i;
    dlx_cell *j;

    c = c->C;

    c->L->R = c->R;
    c->R->L = c->L;

    for (i = c->D; i != c; i = i->D)
    {
        for (j = i->R; j != i; j = j->R)
        {
            j->D->U = j->U;
            j->U->D = j->D;
        }
    }
}

/**
 * Restore column in DLX table. It used for rollback of
 * previously made decision.
 *
 * @param c     Column head
 */
static void
column_uncover(dlx_cell *c)
{
    dlx_cell *i;
    dlx_cell *j;

    c = c->C;

    for (i = c->U; i != c; i = i->U)
    {
        for (j = i->L; j != i; j = j->L)
        {
            j->D->U = j;
            j->U->D = j;
        }
    }

    c->L->R = c;
    c->R->L = c;
}

/**
 * Implementation of Knuth's DLX alrorithm for solving minimum exact
 * set cover problem.
 *
 * @param h     Head of table used to represent a problem
 * @param k     Number of sets already included in solution
 *
 * @return
 *          @c -2   Timeout happened
 *          @c 0    Terminated successfully
 */
static int
dlx(dlx_cell *h, int k)
{
    dlx_cell *c;
    dlx_cell *r;
    dlx_cell *j;

    struct timeval tmv;

    int rc = 0;

    if (time_to_stop != NULL && solutions_found % 10000L == 0)
    {
        gettimeofday(&tmv, NULL);
        if ((time_to_stop->tv_sec - tmv.tv_sec) * 1000000L +
            (time_to_stop->tv_usec - tmv.tv_usec) < 0)
            return -2;
    }

    if (h->R == h)
    {
        /*
         * Correct solution was found - all the elements
         * are covered.
         */
        if (N_min == 0 || k < N_min)
        {
            /*
             * If solution is better than the best solution found
             * previously - update information about the best solution
             */
            free(O_min);
            O_min = calloc(N, sizeof(*O_min));
            memcpy(O_min, O, N * sizeof(*O_min));
            N_min = k;
        }

        solutions_found++;
        if (k == 0)
            work_done = TRUE;
        return 0;
    }

    /*
     * Every time we just select the first element
     */
    c = h->R;
    column_cover(c);

    /*
     * Look through all the sets covering selected element.
     */
    for (r = c->D; r != c; r = r->D)
    {
        O[k] = r->set_id;
        
        /*
         * For each set, try to select it, then delete all
         * the sets having some element in common with it and
         * rule out all newly covered elements from table
         */
        for (j = r->R; j != r; j = j->R)
            column_cover(j);

        /*
         * Apply DLX alrorithm to reduced table.
         */
        rc = dlx(h, k + 1);

        /*
         * Rollback changes in table.
         */
        for (j = r->L; j != r; j = j->L)
            column_uncover(j);

        if (rc < 0)
            break;
    }

    /* Rollback decision of selecting the first element */
    column_uncover(c);
    if (k == 0 && rc >= 0)
        work_done = TRUE; /* All the possible solutions were processed */
    return rc;
}

/**
 * Implementation of greedy algorithm for solving minimum exact
 * set cover problem.
 *
 * @param Problem representation
 *
 * @return Status code
 */
static te_errno
greedy(problem *prb)
{
    int i;
    int max_i;
    int j;
    int k;
    int *avail;
    int  n_avail = prb->set_num;

    avail = calloc(prb->set_num, sizeof(*avail));
    O_min = calloc(prb->set_num, sizeof(*O_min));
    N_min = 0;

    while (n_avail > 0)
    {
        max_i = -1;
        for (i = 0; i < prb->set_num; i++)
        {
            if (avail[i] == 0 &&
                (max_i == -1 || prb->sets[max_i].num < prb->sets[i].num))
                max_i = i;
        }

        avail[max_i] = -1;
        n_avail--;
        O_min[N_min] = max_i;
        N_min++;

        for (i = 0; i < prb->set_num; i++)
        {
            if (avail[i] == 0)
            {
                for (j = 0; j < prb->sets[i].num; j++)
                {
                    for (k = 0; k < prb->sets[max_i].num; k++)
                    {
                        if (prb->sets[max_i].els[k] == prb->sets[i].els[j])
                            break;
                    }
                    
                    if (k < prb->sets[max_i].num)
                        break;
                }

                if (j < prb->sets[i].num)
                {
                    avail[i] = -1;
                    n_avail--;
                }
            }
        }
    }

    free(avail);
    return 0;
}

/**
 * Implementation of greedy algorithm for solving set cover problem.
 *
 * @param prb   Problem representation
 *
 * @return Status code
 */
static te_errno
greedy_set_cov(problem *prb)
{
    int i;
    int max_i;
    int k;
    int *avail;
    int  n_avail = prb->set_num;
    int  *elms_cov;

    avail = calloc(prb->set_num, sizeof(*avail));
    elms_cov = calloc(prb->elm_num, sizeof(*elms_cov));
    O_min = calloc(prb->set_num, sizeof(*O_min));
    N_min = 0;

    for (i = 0; i < prb->elm_num; i++)
    {
        elms_cov[i] = 0;
    }

    for (i = 0; i < prb->set_num; i++)
    {
        prb->sets[i].n_diff = prb->sets[i].num;
    }

    while (n_avail > 0)
    {
        max_i = -1;
        for (i = 0; i < prb->set_num; i++)
        {
            if (avail[i] == 0 &&
                (max_i == -1 || prb->sets[max_i].n_diff < prb->sets[i].n_diff))
                max_i = i;
        }

        avail[max_i] = -1;
        n_avail--;
        O_min[N_min] = max_i;
        N_min++;

        for (k = 0; k < prb->sets[max_i].num; k++)
        {
            elms_cov[prb->sets[max_i].els[k]] = 1;
        }

        for (i = 0; i < prb->set_num; i++)
        {
            prb->sets[i].n_diff = 0;

            if (avail[i] == 0)
            {
                for (k = 0; k < prb->sets[i].num; k++)
                {
                    assert(prb->sets[i].els[k] < prb->elm_num);
                    if (elms_cov[prb->sets[i].els[k]] == 0)
                        prb->sets[i].n_diff++;
                }
                    
                if (prb->sets[i].n_diff == 0)
                {
                    avail[i] = -1;
                    n_avail--;
                }
            }
        }
    }

    free(avail);
    free(elms_cov);
    return 0;
}

/** Described in gen_wilds.h */
te_errno
get_fss_solution(problem *p, alg_type at)
{
    int              n;
    int              m;
    dlx_cell        *dlx_instance;
    struct timeval   tv_before;

    int     *sol_dlx = NULL;
    int      sol_dlx_n = 0;
    int     *sol_greedy = NULL;
    int      sol_greedy_n = 0;

    n = p->elm_num;
    m = p->set_num;

    work_done = FALSE;
    if (at == ALG_EXACT_COV_DLX || at == ALG_EXACT_COV_BOTH)
    {
        dlx_instance = gen_dlx(p);
        N = n + m;
        O = calloc(n + m, sizeof(*O));
        O_min = NULL;
        N_min = 0;
        solutions_found = 0;
        gettimeofday(&tv_before, NULL);
        time_to_stop = calloc(1, sizeof(*time_to_stop));
        time_to_stop->tv_sec = tv_before.tv_sec + 1;
        time_to_stop->tv_usec = tv_before.tv_usec;
        dlx(dlx_instance, 0);
        free_dlx(dlx_instance);
        free(time_to_stop);
    }

    if (work_done || at == ALG_EXACT_COV_DLX)
    {
        if (at == ALG_EXACT_COV_DLX && !work_done)
        {
            p->sol = NULL;
            p->sol_num = -1;
        }
        else
        {
            p->sol = O_min;
            p->sol_num = N_min;
        }

        return 0;
    }

    if (at == ALG_EXACT_COV_DLX || at == ALG_EXACT_COV_BOTH)
    {
        sol_dlx = calloc(N_min, sizeof(*sol_dlx));
        memcpy(sol_dlx, O_min, N_min * sizeof(*sol_dlx));
        sol_dlx_n = N_min;
    }

    if (at == ALG_EXACT_COV_GREEDY || at == ALG_EXACT_COV_BOTH)
        greedy(p);
    else
        greedy_set_cov(p);

    sol_greedy = O_min;
    sol_greedy_n = N_min;

    if (sol_greedy_n < sol_dlx_n || at == ALG_EXACT_COV_GREEDY ||
        at == ALG_SET_COV_GREEDY)
    {
        p->sol = sol_greedy;
        p->sol_num = sol_greedy_n;
        free(sol_dlx);
    }
    else
    {
        p->sol = sol_dlx;
        p->sol_num = sol_dlx_n;
        free(sol_greedy);
    }

    return 0;
}


