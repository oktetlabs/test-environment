/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with running of the requested configuration.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "conf_api.h"

#include "internal.h"
#include "iters.h"


/** Tester run path log user */
#define TESTER_CONTROL             "Control"
/** Prefix of all messages from Tester:Flow user */
#define TESTER_CONTROL_MSG_PREFIX  "%u %u "

/** Size of the Tester shell command buffer */
#define TESTER_CMD_BUF_SZ   1024

/** Size of the bulk used to allocate space for a string */
#define TESTER_STR_BULK 64

/** Is return code a test result or TE error? */
#define TEST_RESULT(_rc) \
    (((_rc) == ETESTPASS) || \
     (((_rc) >= ETESTRESULTMIN) && ((_rc) <= ETESTRESULTMAX)))

/** Print string which may be NULL. */
#define PRINT_STRING(_str)  ((_str) ? : "")


/** Test parameters grouped in one pointer */
typedef struct test_thread_params {
    tester_ctx     *ctx;    /**< Tester context */
    run_item       *test;   /**< Test to be run */
    test_id         id;     /**< Test ID */
    test_params    *params; /**< Test parameters */

#ifdef TESTER_TIMEOUT_SUPPORT
    int             pipe;   /**< One side of the pipe for notification */
#endif
    
    int             rc;     /**< Test result */
} test_thread_params;


static int iterate_test(tester_ctx *ctx, run_item *test,
                        test_param_iterations *iters);


/**
 * Get unique test ID.
 */
test_id
tester_get_id(void)
{
    static test_id id;

    return id++;
}

/**
 * Clone Tester context.
 *
 * @param ctx       Tester context to be cloned.
 *
 * @return Pointer to new Tester context.
 */
static tester_ctx *
tester_ctx_clone(const tester_ctx *ctx)
{
    int         rc;
    tester_ctx *new_ctx = calloc(1, sizeof(*new_ctx));

    if (new_ctx == NULL)
    {
        ERROR("%s(): calloc(1, %u)", __FUNCTION__, sizeof(*new_ctx));
        return NULL;
    }
    new_ctx->id      = ctx->id;
    new_ctx->flags   = ctx->flags;
    new_ctx->timeout = ctx->timeout;
    new_ctx->targets = ctx->targets;
    new_ctx->path    = ctx->path;

    TAILQ_INIT(&new_ctx->reqs);
    rc = test_requirements_clone(&ctx->reqs, &new_ctx->reqs);
    if (rc != 0)
    {
        ERROR("%s(): failed to clone requirements: %X", __FUNCTION__, rc);
        free(new_ctx);
        return NULL;
    }

    return new_ctx;
}

/**
 * Create new test parameter.
 *
 * @param name      Parameter name
 * @param value     Parameter value
 * @param clone     Should the parameter be cloned?
 * @param reqs      Associated requirements
 *
 * @return Poiner to allocated test parameter or NULL.
 */
static test_param *
test_param_new(const char *name, const char *value, te_bool clone,
               const test_requirements *reqs)
{
    test_param *p;

    assert(name != NULL);
    assert(value != NULL);

    ENTRY("name=%s, value=%s, clone=%d", name, value, clone);
    
    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(*p));
        EXIT("ENOMEM");
        return NULL;
    }

    p->name = name;
    p->value = strdup(value);
    if (p->value == NULL)
    {
        ERROR("strdup() failed");
        free(p);
        EXIT("ENOMEM");
        return NULL;
    }
    p->reqs = reqs;
    p->clone = clone;
    VERB("New parameter: %s=%s, clone=%d, reqs=%p",
         p->name, p->value, p->clone, p->reqs);

    return p;
}

/**
 * Add new parameter in the list of parameters.
 * If parameter with the same name already exists, remove it.
 *
 * @param params    List of parameters
 * @param param     New parameter to add
 */
static void
test_params_add(test_params *params, test_param *param)
{
    test_param *p;

    assert(params != NULL);
    assert(param != NULL);
    assert(param->name != NULL);
    assert(param->value != NULL);
    for (p = params->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (strcmp(p->name, param->name) == 0)
        {
            TAILQ_REMOVE(params, p, links);
            test_param_free(p);
            break;
        }
    }
    TAILQ_INSERT_TAIL(params, param, links);
}



/**
 * Get parameter value by reference.
 *
 * @param refer     Reference name
 * @param params    Parameters context
 *
 * @return Parameter value or NULL.
 */
static const char *
get_ref_value(const char *refer, const test_params *params)
{
    test_param *p;

    for (p = params->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (strcmp(refer, p->name) == 0)
            return p->value;
    }

    ERROR("Failed to find value by reference '%s'", refer);
    return NULL;
}

/**
 * Get 'const char *' value of the argument/variable value.
 *
 * @param value     Variable/argument value
 * @param params    Parameters context
 *
 * @return Parameter value or NULL.
 */
static const char *
get_value(const test_var_arg_value *value, const test_params *params)
{
    assert(value != NULL);
    assert(((value->value != NULL) + (value->ref != NULL) +
            (value->ext != NULL)) == 1);
    
    if (value->value != NULL)
        return value->value;
    else if (value->ext != NULL)
        return get_ref_value(value->ext, params);
    else if (value->ref != NULL)
    {
        assert(value->ref != value);
        return get_value(value->ref, params);
    }
    else
        assert(0);
    
    return NULL;
}

/**
 * Get i-th value from the list.
 * 
 * @param values        List of values
 * @param value_i       Index of the value to get
 * @param preferred     Preferred value, if 'value_i' is too big
 * @param params        Current parameters context
 * @param more          Whether more values present
 *
 * @return Value or NULL.
 */
static const char *
get_ith_value(const test_var_arg_values *values, unsigned int value_i,
              const test_var_arg_value *preferred,
              const test_params *params, te_bool *more)
{
    const test_var_arg_value   *v;
    unsigned int                i;

    assert(values != NULL);
    assert(preferred != NULL);
    assert(more != NULL);

    for (i = 0, v = values->tqh_first;
         i < value_i && v != NULL;
         ++i, v = v->links.tqe_next);
    *more = (v != NULL) && (v->links.tqe_next != NULL);
    if (v == NULL)
        v = preferred;
    return get_value(v, params);
}

/**
 * Get argument preferred value.
 *
 * @param p     Variable/argument pointer
 */
static const test_var_arg_value *
var_arg_preferred_value(const test_var_arg *p)
{
    assert(p != NULL);

    return (p->attrs.preferred) ? : p->values.tqh_first;
}

/**
 * Free list of allocated strings.
 *
 * @param head      Head of the list
 */
static void
tq_strings_free(tqh_strings *head)
{
    tqe_string *p;

    while ((p = head->tqh_first) != NULL)
    {
        TAILQ_REMOVE(head, p, links);
        free(p);
    }
}

/**
 * Create a list of parameters iterations.
 *
 * @param vas       Test variables or arguments
 * @param iters     Head of the list of parameters iterations
 *
 * @return Status code.
 */
static int
vars_args_iterations(test_vars_args        *vas,
                     test_param_iterations *iters)
{
    test_param_iteration       *i;
    test_param_iteration       *i_clone;
    const test_var_arg         *va;
    const test_var_arg_value   *v;
    const test_var_arg_value   *value;
    test_param                 *tp;
    const char                 *s;
    tqh_strings                 processed_lists;
    tqe_string                 *list;
    unsigned int                value_i;
    const test_var_arg         *va2;
    te_bool                     more_in_list;
    te_bool                     more_in_arg;

    ENTRY();

    assert(vas != NULL);
    assert(iters != NULL);
    assert(iters->tqh_first != NULL);

    TAILQ_INIT(&processed_lists);

    /* Each for each set of iterations */
    for (va = vas->tqh_first; va != NULL; va = va->links.tqe_next)
    {
        if (va->attrs.list != NULL)
        {
            /* Check that such list is not processed yet */
            for (list = processed_lists.tqh_first;
                 list != NULL && strcmp(list->v, va->attrs.list) != 0;
                 list = list->links.tqe_next);
            if (list != NULL)
            {
                /* This argument is from already processed list */
                continue;
            }
            /* Remember that the list is processed */
            list = calloc(1, sizeof(*list));
            if (list == NULL)
            {
                tq_strings_free(&processed_lists);
                EXIT("ENOMEM");
                return ENOMEM;
            }
            list->v = va->attrs.list;
            TAILQ_INSERT_HEAD(&processed_lists, list, links);
        }
        else
        {
            list = NULL;
        }
        for (i = iters->tqh_first; i != NULL; i = i->links.tqe_next)
        {
            for (v = va->values.tqh_first, value_i = 0,
                     more_in_list = FALSE, i_clone = NULL;
                 v != NULL || (list != NULL && more_in_list);
                 ++value_i)
            {
                value = (v) ? : var_arg_preferred_value(va);
                /* All the times except to first, move to the clone */
                if (i_clone != NULL)
                    i = i_clone;
                /* Clone current iteration before update */
                i_clone = test_param_iteration_clone(i, TRUE);
                if (i_clone == NULL)
                {
                    tq_strings_free(&processed_lists);
                    ERROR("Cloning of the iteration failed");
                    EXIT("ENOMEM");
                    return ENOMEM;
                }
                /* Insert clone after current iteration */
                TAILQ_INSERT_AFTER(iters, i, i_clone, links);
                
                /* Get argument value and create parameter */
                s = get_value(value, i->base);
                if (s == NULL)
                {
                    tq_strings_free(&processed_lists);
                    return EINVAL;
                }
                tp = test_param_new(va->name, s, va->handdown,
                                    (value->reqs.tqh_first == NULL) ?
                                        NULL : &value->reqs);
                if (tp == NULL)
                {
                    tq_strings_free(&processed_lists);
                    ERROR("Failed to create new test parameter");
                    EXIT("ENOMEM");
                    return ENOMEM;
                }
                i->has_reqs = (i->has_reqs || (tp->reqs != NULL));
                /* Update current iteration */
                test_params_add(&i->params, tp);
                
                /* Add values for all parameters from the same list */
                if (list != NULL)
                {
                    for (va2 = va->links.tqe_next, more_in_list = FALSE;
                         va2 != NULL;
                         va2 = va2->links.tqe_next)
                    {
                        if (va2->attrs.list == NULL ||
                            strcmp(va->attrs.list, va2->attrs.list) != 0)
                        {
                            continue;
                        }
                        /* Argument from the same list */

                        /* Get its i-th value */
                        s = get_ith_value(&va2->values, value_i,
                                          var_arg_preferred_value(va2),
                                          i->base, &more_in_arg);
                        if (s == NULL)
                        {
                            tq_strings_free(&processed_lists);
                            return EINVAL;
                        }
                        more_in_list = (more_in_list || more_in_arg);
                        /* Create parameter and add it in iteration */
                        /* FIXME: Requirement here */
                        tp = test_param_new(va2->name, s, va->handdown,
                                            NULL);
                        if (tp == NULL)
                        {
                            tq_strings_free(&processed_lists);
                            ERROR("Failed to create new test parameter");
                            EXIT("ENOMEM");
                            return ENOMEM;
                        }
                        test_params_add(&i->params, tp);
                    }
                }
                if (v != NULL)
                    v = v->links.tqe_next;
            }
            /* Free the last not used iteration clone */
            if (i_clone != NULL)
            {
                TAILQ_REMOVE(iters, i_clone, links);
                test_param_iteration_free(i_clone);
            }
        }
    }

    tq_strings_free(&processed_lists);

    EXIT("OK");
    return 0;
}


/**
 * Convert information about group of persons to string representation.
 *
 * @param persons   Information about persons
 *
 * @return Allocated string or NULL.
 */
static char *
persons_info_to_string(const persons_info *persons)
{
    size_t              total = TESTER_STR_BULK;
    char               *res = malloc(total);
    char               *s = res;
    size_t              rest = total;
    const person_info  *p;
    int                 printed;
    te_bool             again;

    if (res == NULL)
    {
        ERROR("%s(): Memory allocation failure", __FUNCTION__);
        return NULL;
    }

    s[0] = '\0';
    for (p = persons->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        do  {
            printed = snprintf(s, rest, " %s%smailto:%s",
                               PRINT_STRING(p->name),
                               (p->name == NULL) ? "" : " ",
                               PRINT_STRING(p->mailto));
            assert(printed > 0);
            if ((size_t)printed >= rest)
            {
                char *t;
                
                /* Discard just printed */
                s[0] = '\0';
                /* We are going to extend buffer */
                rest += total;
                total <<= 1;
                t = malloc(total);
                if (t == NULL)
                {
                    ERROR("%s(): Memory allocation failure", __FUNCTION__);
                    return res;
                }
                /* Copy early printed to new buffer */
                strcpy(t, res);
                /* Substitute */
                free(res);
                res = t;
                /* Locate current pointer */
                s = res + strlen(res);
                /* Print the last item again */
                again = TRUE;
            }
            else
            {
                rest -= printed;
                s += printed;
                again = FALSE;
            }
        } while (again);
    }
    return res;
}


/**
 * Space in the string required for test parameter.
 *
 * @param param     Test parameter
 */
static size_t
test_param_space(const test_param *param)
{
    return 1 /* space */ + 1 /* " */ + strlen(param->name) +
           1 /* = */ + strlen(param->value) + 1 /* " */ + 1 /* \0 */;
}

/**
 * Convert test parameters to string representation.
 * The first symbol is a space, if the result is not NULL.
 *
 * @param params    Test parameters to convert
 *
 * @return Allocated string or NULL.
 */
static char *
test_params_to_string(const test_params *params)
{
    test_param *p;
    char       *v = NULL;
    size_t      len = 0;
    size_t      rest = len;


    if (params == NULL)
        return v;

    for (p = params->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        size_t req = test_param_space(p);

        VERB("%s(): parameter %s=%s", __FUNCTION__, p->name, p->value);
        while (rest < req)
        {
            char *nv = malloc(len += TESTER_STR_BULK);

            if (nv == NULL)
            {
                free(v);
                ERROR("malloc(%u) failed", len);
                return NULL;
            }
            if (v != NULL)
            {
                strcpy(nv, v);
                free(v);
            }
            rest += TESTER_STR_BULK;
            v = nv;
        }
        rest -= sprintf(v + (len - rest), " %s=\"%s\"", p->name, p->value);
    }
    if (v != NULL)
        VERB("%s(): %s", __FUNCTION__, v);

    return v;
}

/**
 * Log test (script, package, session) start.
 *
 * @param parent    Parent ID
 * @param test      Test ID
 * @param params    List of parameters
 */
static void
log_test_start(const run_item *ri, test_id parent, test_id test,
               const test_params *params)
{
    char   *params_str;
    char   *authors;

    params_str = test_params_to_string(params);
    switch (ri->type)
    {
        case RUN_ITEM_SCRIPT:
            LOG_RING(TESTER_CONTROL, TESTER_CONTROL_MSG_PREFIX
                     "TEST %s \"%s\" ARGs%s",
                     parent, test, ri->u.script.name,
                     PRINT_STRING(ri->u.script.objective),
                     PRINT_STRING(params_str));
            break;

        case RUN_ITEM_SESSION:
            LOG_RING(TESTER_CONTROL, TESTER_CONTROL_MSG_PREFIX
                     "SESSION ARGs%s",
                     parent, test, PRINT_STRING(params_str));
            break;

        case RUN_ITEM_PACKAGE:
            authors = persons_info_to_string(&ri->u.package->authors);
            if (authors == NULL)
            {
                LOG_RING(TESTER_CONTROL, TESTER_CONTROL_MSG_PREFIX
                         "PACKAGE %s \"%s\" ARGs%s",
                         parent, test, ri->u.package->name,
                         PRINT_STRING(ri->u.package->objective),
                         PRINT_STRING(params_str));
            }
            else
            {
                LOG_RING(TESTER_CONTROL, TESTER_CONTROL_MSG_PREFIX
                         "PACKAGE %s \"%s\" AUTHORS%s ARGs%s",
                         parent, test, ri->u.package->name,
                         PRINT_STRING(ri->u.package->objective),
                         PRINT_STRING(authors),
                         PRINT_STRING(params_str));
            }
            free(authors);
            break;

        default:
            ERROR("Invalid run item type %d", ri->type);
    }
    free(params_str);
}

/**
 * Log test (script, package, session) result.
 *
 * @param parent    Parent ID
 * @param test      Test ID
 * @param result    Test result
 */
static void
log_test_result(test_id parent, test_id test, int result)
{
    if (result == ETESTPASS)
    {
        LOG_RING(TESTER_CONTROL, TESTER_CONTROL_MSG_PREFIX "PASSED",
                 parent, test);
    }
    else
    {
        switch (result)
        {
            case ETESTKILL:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "KILLED",
                         parent, test);
                break;

            case ETESTCORE:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "CORED",
                         parent, test);
                break;

            case ETESTSKIP:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "SKIPPED",
                         parent, test);
                break;

            case ETESTFAKE:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "FAKED",
                         parent, test);
                break;

            case ETESTEMPTY:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "EMPTY",
                         parent, test);
                break;

            default:
            {
                const char *reason;

                switch (result)
                {
                    case ETESTFAIL:
                        /* TODO Reason from test */
                        reason = "";
                        break;

                    case ETESTCONF:
                        reason = "Unexpected configuration changes";
                        break;

                    case ETESTPROLOG:
                        reason = "Session prologue failed";
                        break;

                    case ETESTEPILOG:
                        reason = "Session epilogue failed";
                        break;

                    case ETESTUNEXP:
                        reason = "Unexpected failure type";
                        break;

                    default:
                        reason = "Unknown test result";
                }
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "FAILED %s",
                         parent, test, reason);
                break;
            }
        }
    }
}


/**
 * Run test script in provided context with specified parameters.
 *
 * @param ctx       Tester context
 * @param id        Test ID
 * @param script    Test script to run
 * @param params    Parameters to be passed
 *
 * @return Status code.
 */
static int
run_test_script(tester_ctx *ctx, test_script *script, test_id id,
                test_params *params)
{
    int         result = 0;
    int         rc;
    char       *params_str;
    char        cmd[TESTER_CMD_BUF_SZ];
    char        shell[256] = "";
    char        postfix[32] = "";
    char        gdb_init[32] = "";

    ENTRY();

    params_str = test_params_to_string(params);

    if (ctx->flags & TESTER_GDB)
    {
        if (params_str != NULL)
        {
            FILE *f;

            if (snprintf(gdb_init, sizeof(gdb_init),
                         TESTER_GDB_FILENAME_FMT, id) >=
                    (int)sizeof(gdb_init))
            {
                ERROR("Too short buffer is reserved for GDB init file "
                      "name");
                return ETESMALLBUF;
            }
            /* TODO Clean up */
            f = fopen(gdb_init, "w");
            if (f == NULL)
            {
                ERROR("Failed to create GDB init file: %s",
                      strerror(errno));
                return errno;
            }
            fprintf(f, "set args %s\n", params_str);
            if (fclose(f) != 0)
            {
                ERROR("fclose() failed");
                return errno;
            }
            if (snprintf(shell, sizeof(shell),
                         "gdb -x %s ", gdb_init) >=
                    (int)sizeof(shell))
            {
                ERROR("Too short buffer is reserved for shell command "
                      "prefix");
                return ETESMALLBUF;
            }
        }
        else
        {
            if (snprintf(shell, sizeof(shell), "gdb ") >=
                    (int)sizeof(shell))
            {
                ERROR("Too short buffer is reserved for shell command "
                      "prefix");
                return ETESMALLBUF;
            }
        }
    }
    else if (ctx->flags & TESTER_VALGRIND)
    {
        if (snprintf(shell, sizeof(shell),
                     "valgrind --num-callers=16 --leak-check=yes "
                     "--show-reachable=yes --tool=memcheck ") >=
                (int)sizeof(shell))
        {
            ERROR("Too short buffer is reserved for shell command prefix");
            return ETESMALLBUF;
        }
        if (snprintf(postfix, sizeof(postfix),
                     " 2>" TESTER_VG_FILENAME_FMT, id) >=
                (int)sizeof(postfix))
        {
            ERROR("Too short buffer is reserved for test script "
                  "command postfix");
            return ETESMALLBUF;
        }
    }

    if (snprintf(cmd, sizeof(cmd), "%s%s%s%s", shell, script->execute,
                 (ctx->flags & TESTER_GDB) ? "" : PRINT_STRING(params_str),
                 postfix) >= (int)sizeof(cmd))
    {
        ERROR("Too short buffer is reserved for test script command "
              "line");
        return ETESMALLBUF;
    }
    free(params_str);

    if (ctx->flags & TESTER_FAKE)
    {
        result = ETESTFAKE;
    }
    else
    {
        VERB("ID=%d system(%s)", id, cmd);
        rc = system(cmd);
        if (rc == -1)
        {
            ERROR("system(%s) failed: %X", cmd, errno);
            return errno;
        }

#ifdef WCOREDUMP
        if (WCOREDUMP(rc))
        {
            ERROR("Command '%s' executed in shell dumped core", cmd);
            result = ETESTCORE;
        }
#endif
        if (WIFSIGNALED(rc))
        {
            /* @todo Signal-to-string */
            ERROR("ID=%d was killed by signal %d", id, WTERMSIG(rc));
            /* ETESTCORE may be already set */
            if (result == 0)
                result = ETESTKILL;
        }
        else if (!WIFEXITED(rc))
        {
            ERROR("ID=%d was abnormally terminated", id);
            /* ETESTCORE may be already set */
            if (result == 0)
                result = ETESTUNEXP;
        }
        else
        {
            if (result != 0)
                ERROR("Unexpected return value of system() call");

            switch (WEXITSTATUS(rc))
            {
                case EXIT_FAILURE:
                    result = ETESTFAIL;
                    break;

                case ETESTPASS: /* EXIT_SUCCESS */
                case ETESTFAIL:
                case ETEENV:
                    result = WEXITSTATUS(rc);
                    break;

                default:
                    result = ETESTUNEXP;
            }
        }
    }

    EXIT("%X", result);

    return result;
}


/**
 * Run test session.
 *
 * @param ctx       Tester context
 * @param id        Test ID
 * @param session   Test session
 * @param params    Set of parameters
 *
 * @return Status code.
 */
static int
run_test_session(tester_ctx *ctx, test_session *session, test_id id,
                 test_params *params)
{
    int                     result = ETESTEMPTY;
    int                     rc;
    test_param_iteration   *base_i;
    test_param_iterations   iters;
    run_item               *test;


    ENTRY();

    ctx->id = id;

    /* Create base list of iterations with only one iteration */
    TAILQ_INIT(&iters);
    base_i = test_param_iteration_new();
    if (base_i == NULL)
        return ENOMEM;
    base_i->base = params;
    /* FIXME: base_i.reqs */
    TAILQ_INSERT_TAIL(&iters, base_i, links);

    /* Create list of iterations */
    rc = vars_args_iterations(&session->vars, &iters);
    if (rc != 0)
    {
        ERROR("Failed to create a list of parameters iterations "
              "for ID=%u", id);
        return rc;
    }
    if (iters.tqh_first == NULL)
    {
        ERROR("Empty list of parameters iterations");
        return EINVAL;
    }

    do {
        if (session->prologue != NULL)
        {
            if (ctx->flags & TESTER_NOLOGUES)
            {
                WARN("Prologues are disabled globally");
            }
            else
            {
                VERB("Running test session prologue...");
                ctx->flags |= TESTER_INLOGUE;
                rc = iterate_test(ctx, session->prologue, &iters);
                if ((rc != 0) && (rc != ETESTFAKE))
                {
                    result = ETESTPROLOG;
                    break;
                }
                ctx->flags &= ~TESTER_INLOGUE;
            }
        }
        else
        {
            VERB("Test session has no prologue");
        }

        if (session->keepalive == NULL)
        {
            VERB("Test session has no keep-alive validation");
        }

        for (test = session->run_items.tqh_first;
             test != NULL;
             test = test->links.tqe_next)
        {
            if (session->keepalive != NULL)
            {
                VERB("Running test session keep-alive validation...");
                rc = iterate_test(ctx, session->keepalive, &iters);
                if ((rc != 0) && (rc != ETESTFAKE))
                {
                    result = ETESTALIVE;
                    break;
                }
            }

            rc = iterate_test(ctx, test, &iters);
            if ((rc != 0) && (rc != ETESTEMPTY) &&
                (rc != ETESTFAKE) && (rc != ETESTSKIP))
            {
                /* 
                 * Other results except success and skip are mapped 
                 * to ETESTFAIL. 
                 */
                result = ETESTFAIL;
            }
            else if (result == ETESTEMPTY)
            {
                result = rc;
            }
        }

        if (session->epilogue != NULL)
        {
            if (ctx->flags & TESTER_NOLOGUES)
            {
                WARN("Epilogues are disabled globally");
            }
            else
            {
                VERB("Running test session epilogue...");
                ctx->flags |= TESTER_INLOGUE;
                rc = iterate_test(ctx, session->epilogue, &iters);
                if ((rc != 0) && (rc != ETESTFAKE))
                {
                    result = ETESTEPILOG;
                    break;
                }
                ctx->flags &= ~TESTER_INLOGUE;
            }
        }
        else
        {
            VERB("Test session has no epilogue");
        }
    } while (0);

    EXIT("%d", result);

    test_param_iterations_free(&iters);

    return result;
}


/**
 * Run test package.
 *
 * @param ctx       Tester context
 * @param id        Test ID
 * @param pkg       Test package
 * @param params    Set of parameters
 *
 * @return Status code.
 */
static int
run_test_package(tester_ctx *ctx, test_package *pkg, test_id id,
                 test_params *params)
{
    ENTRY();

    assert(!(ctx->flags & TESTER_INLOGUE));

    ctx->id = id;

    return run_test_session(ctx, &pkg->session, id, params);
}


#ifdef TESTER_NO_TIMEOUT_SUPPORT
/**
 * Notify manager that thread becomes joinable.
 *
 * @param thr_params    Test thread parameters
 */
static void
test_thread_joinable(test_thread_params *thr_params)
{
    int  rc;
    char msg[] = "OK";

    ENTRY("0x%x", (unsigned int)pthread_self());

    rc = write(thr_params->pipe, msg, sizeof(msg));
    if (rc != sizeof(msg))
    {
        ERROR("Failed to notify that thread becomes joinable: %X", errno);
    }
}
#endif

/**
 * Test thread entry point.
 *
 * @param args      Test thread parameters
 *
 * @return Test thread parameters with set 'rc' field.
 */
static void *
run_test_thread(void *args)
{
    test_thread_params *thr_params = (test_thread_params *)args;
    int                 rc;

    ENTRY("0x%x", (unsigned int)pthread_self());

    switch (thr_params->test->type)
    {
        case RUN_ITEM_SCRIPT:
            rc = run_test_script(thr_params->ctx,
                                 &thr_params->test->u.script,
                                 thr_params->id,
                                 thr_params->params);
            break;

        case RUN_ITEM_SESSION:
            rc = run_test_session(thr_params->ctx,
                                  &thr_params->test->u.session,
                                  thr_params->id,
                                  thr_params->params);
            break;

        case RUN_ITEM_PACKAGE:
            rc = run_test_package(thr_params->ctx,
                                  thr_params->test->u.package,
                                  thr_params->id,
                                  thr_params->params);
            break;

        default:
            rc = EINVAL;
    }

    thr_params->rc = rc;

#ifdef TESTER_TIMEOUT_SUPPORT
    test_thread_joinable(thr_params);
#endif

    EXIT("0x%x", (unsigned int)pthread_self());

#ifdef TESTER_TIMEOUT_SUPPORT
    log_client_close();
#endif

    return NULL;
}

/**
 * Run test with specified set of parameters.
 *
 * @param ctx           Tester context
 * @param test          Test to be run
 * @param id            Assigned test ID
 * @param params        Test parameters
 *
 * @return Status code.
 */
static int
run_test(tester_ctx *ctx, run_item *test, test_id id, test_params *params)
{
    int rc;
#ifdef TESTER_TIMEOUT_SUPPORT
    int pipefds[2];
#endif


    ENTRY();

#ifdef TESTER_TIMEOUT_SUPPORT
    /* Create a pipe to be used for join notification */
    if (pipe(pipefds) != 0)
    {
        ERROR("pipe() failed: %X", errno);
        return errno;
    }
#endif

    do {
        test_thread_params  thr_params;
#ifdef TESTER_TIMEOUT_SUPPORT
        pthread_t           th;
        int                 waitfd;
        fd_set              rd;
        struct timeval      timeout;
#endif


        /* Prepare test arguments */
        memset(&thr_params, 0, sizeof(thr_params));
        thr_params.ctx    = ctx;
        thr_params.test   = test;
        thr_params.id     = id; 
        thr_params.params = params;

#ifdef TESTER_TIMEOUT_SUPPORT
        thr_params.pipe   = pipefds[1];

        /* Run thread with the test */
        VERB("Running test thread...");
        rc = pthread_create(&th, NULL, run_test_thread, &thr_params);
        if (rc != 0)
        {
            rc = errno;
            ERROR("pthread_create() failed: %X", rc);
            break;
        }

        /* Wait on select() specified timeout */
        waitfd = pipefds[0];
        FD_ZERO(&rd);
        FD_SET(waitfd, &rd);
        timeout = test->attrs.timeout;

        VERB("Waiting for test thread 0x%x becomes joinable...",
             (unsigned int)th);
        do {
            rc = select(waitfd + 1, &rd, NULL, NULL, &timeout);
            VERB("Waiting for test thread 0x%x done: rc=%d, errno=%d, "
                 "FD_ISSET=%d", (unsigned int)th, rc, errno,
                 FD_ISSET(waitfd, &rd));
        } while ((rc == -1) && (errno == EINTR));

        if (rc > 1)
        {
            ERROR("Unexpected select() return value %d", rc);
        }
        if ((rc > 0) && (FD_ISSET(waitfd, &rd)))
        {
            VERB("Test thread 0x%x has become joinable - join",
                 (unsigned int)th);
            /* Test thread is joinable */
            rc = pthread_join(th, NULL);
            if (rc != 0)
            {
                ERROR("pthread_join() failed: %X", errno);
            }
            rc = thr_params.rc;
        }
        else
        {
            VERB("Test thread 0x%x has not become joinable before "
                 "timeout - kill", (unsigned int)th);
            /* Timeout - test thread is not joinable */
            rc = pthread_kill(th, SIGINT);
            if (rc != 0)
            {
                ERROR("pthread_kill() failed: %X", errno);
            }
        }
#else
        run_test_thread(&thr_params);
        rc = thr_params.rc;
#endif
    } while (0);

#ifdef TESTER_TIMEOUT_SUPPORT
    if (close(pipefds[0]) != 0)
    {
        ERROR("close() of pipefds[0] failed: %X", errno);
        RC_UPDATE(rc, errno);
    }
    if (close(pipefds[1]) != 0)
    {
        ERROR("close() of pipefds[1] failed: %X", errno);
        RC_UPDATE(rc, errno);
    }
#endif

    EXIT("%d", rc);

    return rc;
}


/**
 * Get run item name.
 *
 * @param test          Test to run
 *
 * @return Name.
 */
static const char *
tester_run_item_name(const run_item *test)
{
    switch (test->type)
    {
        case RUN_ITEM_SCRIPT:
            return test->u.script.name;

        case RUN_ITEM_SESSION:
            return test->u.session.name;

        case RUN_ITEM_PACKAGE:
            return test->u.package->name;

        default:
            ERROR("Invalid run item type %d", test->type);
            return "";
    }
}


/**
 * Run test with iteration of its parameters.
 *
 * @param ctx           Tester context
 * @param test          Test to run
 * @param base_iters    List of iterations based on test session variables
 *
 * @return Status code.
 */
static int
iterate_test(tester_ctx *ctx, run_item *test,
             test_param_iterations *base_iters)
{
    te_bool                 ctx_cloned = FALSE;
    int                     rc;
    int                     test_result = ETESTPASS;
    int                     all_result = ETESTEMPTY;
    unsigned int            run_iters = 0;
    te_bool                 test_skipped = FALSE;
    test_param_iterations   iters;
    test_param_iteration   *i;
    char                   *backup_name = NULL;
    const char             *run_item_name = test->name;
    const char             *test_name = tester_run_item_name(test);

    ENTRY();

    if ((run_item_name != NULL || test_name != NULL) &&
        (ctx->path->paths.tqh_first != NULL))
    {
        ctx = tester_ctx_clone(ctx);
        if (ctx == NULL)
        {
            ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
            return ENOMEM;
        }
        ctx_cloned = TRUE;

        if (run_item_name != NULL)
        {
            rc = tester_run_path_forward(ctx, run_item_name);
            if (rc != 0)
            {
                if (rc == ENOENT)
                {
                    /* Silently ignore nodes not of the path */
                    if (~ctx->flags & TESTER_INLOGUE)
                    {
                        tester_ctx_free(ctx);
                        return all_result;
                    }
                }
                else
                {
                    tester_ctx_free(ctx);
                    ERROR("%s(): tester_run_path_forward() failed",
                          __FUNCTION__);
                    return rc;
                }
            }
        }
        if (test_name != NULL)
        {
            rc = tester_run_path_forward(ctx, test_name);
            if (rc != 0)
            {
                if (rc == ENOENT)
                {
                    /* Silently ignore nodes not of the path */
                    if (~ctx->flags & TESTER_INLOGUE)
                    {
                        tester_ctx_free(ctx);
                        return all_result;
                    }
                }
                else
                {
                    tester_ctx_free(ctx);
                    ERROR("%s(): tester_run_path_forward() failed",
                          __FUNCTION__);
                    return rc;
                }
            }
        }
    }

    TAILQ_INIT(&iters);

    /* Configuration file provided empty list of base iterations */
    if (base_iters == NULL)
    {
        i = test_param_iteration_new();
        if (i == NULL)
        {
            if (ctx_cloned)
                tester_ctx_free(ctx);
            return ENOMEM;
        }
        TAILQ_INSERT_TAIL(&iters, i, links);
    }
    else
    {
        test_param_iteration   *base_i;

        /* Clone base list of iterations */
        for (base_i = base_iters->tqh_first; 
             base_i != NULL; 
             base_i = base_i->links.tqe_next)
        {
            i = test_param_iteration_clone(base_i, FALSE);
            if (i == NULL)
            {
                ERROR("Cloning of the test parameters iteration failed");
                if (ctx_cloned)
                    tester_ctx_free(ctx);
                return ENOMEM;
            }
            TAILQ_INSERT_TAIL(&iters, i, links);
        }
    }

    /* Create list of iterations */
    rc = vars_args_iterations(&test->args, &iters);
    if (rc != 0)
    {
        ERROR("Failed to create a list of parameters iterations");
        test_param_iterations_free(&iters);
        if (ctx_cloned)
            tester_ctx_free(ctx);
        return rc;
    }
    if (iters.tqh_first == NULL)
    {
        ERROR("Empty list of parameters iterations");
        if (ctx_cloned)
            tester_ctx_free(ctx);
        return EINVAL;
    }

    /* Create backup to be verified after each iteration */
    if (!(ctx->flags & (TESTER_NO_CS | TESTER_NOCFGTRACK)) &&
        test->attrs.track_conf != TESTER_TRACK_CONF_NO)
    {
        rc = cfg_create_backup(&backup_name);
        if (rc != 0)
        {
            ERROR("Cannot create configuration backup: %X", rc);
            test_param_iterations_free(&iters);
            if (ctx_cloned)
                tester_ctx_free(ctx);
            return rc;
        }
    }

    /* Iterate parameters */
    for (i = iters.tqh_first; i != NULL; i = i->links.tqe_next)
    {
        test_id     id = tester_get_id();
        te_bool     test_ctx_cloned = FALSE;
        tester_ctx *test_ctx = ctx;

        if (ctx->path->params.tqh_first != NULL &&
            (~(ctx->flags) & TESTER_INLOGUE))
        {
            rc = tester_run_path_params_match(test_ctx, &(i->params));
            if (rc != 0)
            {
                if (rc == ENOENT)
                {
                    /* Silently ignore nodes not of the path */
                    continue;
                }
                else
                {
                    ERROR("%s(): tester_run_path_params_forward() failed",
                          __FUNCTION__);
                    all_result = rc;
                    break;
                }
            }
        }

        if ((~test_ctx->flags & TESTER_INLOGUE) &&
            (test_ctx->flags & TESTER_QUIET_SKIP) &&
            !tester_is_run_required(test_ctx, test, &(i->params), TRUE))
        {
            /* Silently skip without any logs */
            if (test_ctx_cloned)
                tester_ctx_free(test_ctx);
            test_skipped = TRUE;
            continue;
        }

        /* Test is considered here as run, if such event is logged */
        run_iters++;

        tester_out_start(test->type, tester_run_item_name(test),
                         ctx->id, id, ctx->flags);
        log_test_start(test, ctx->id, id, &(i->params));

        if ((~test_ctx->flags & TESTER_INLOGUE) &&
            (~test_ctx->flags & TESTER_QUIET_SKIP) &&
            !tester_is_run_required(test_ctx, test, &(i->params), FALSE))
        {
            test_result = ETESTSKIP;
        }
        else
        {
            if (test->type != RUN_ITEM_SCRIPT)
            {
                test_ctx = tester_ctx_clone(ctx);
                if (test_ctx == NULL)
                {
                    ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
                    all_result = ENOMEM;
                    break;
                }
                test_ctx_cloned = TRUE;
            }

            /* Run test with specified parameters */
            test_result = run_test(test_ctx, test, id, &(i->params));
            if (!TEST_RESULT(test_result))
            {
                ERROR("run_test() failed: %X", rc);
            }

            if (backup_name != NULL)
            {
                /* Check configuration backup */
                rc = cfg_verify_backup(backup_name);
                if (TE_RC_GET_ERROR(rc) == ETEBACKUP)
                {
                    if (test->attrs.track_conf == TESTER_TRACK_CONF_YES)
                    {
                        /* If backup is not OK, restore it */
                        WARN("Current configuration differs from "
                             "backup - restore");
                    }
                    rc = cfg_restore_backup(backup_name);
                    if (rc != 0)
                    {
                        ERROR("Cannot restore configuration backup: "
                              "%X", rc);
                        if (TEST_RESULT(test_result))
                            test_result = rc;
                    }
                    else if (test->attrs.track_conf ==
                                 TESTER_TRACK_CONF_YES)
                    {
                        RING("Configuration successfully restored "
                             "using backup");
                        RC_UPDATE(test_result, ETESTCONF);
                    }
                }
                else if (rc != 0)
                {
                    ERROR("Cannot verify configuration backup: %X", rc);
                    if (TEST_RESULT(test_result))
                        test_result = rc;
                }
            }
        }

        log_test_result(ctx->id, id, test_result);
        tester_out_done(test->type, tester_run_item_name(test),
                        ctx->id, id, ctx->flags, test_result);

        if (test_ctx_cloned)
            tester_ctx_free(test_ctx);
        
        /* Update result of all iterations */
        if (!TEST_RESULT(test_result))
        {
            all_result = test_result;
            break;
        }
        else if ((all_result < test_result) || (all_result == ETESTEMPTY))
        {
            all_result = test_result;
        }
    }

    if (backup_name != NULL)
    {
        rc = cfg_release_backup(&backup_name);
        if (rc != 0)
        {
            ERROR("cfg_release_backup() failed: %X", rc);
            if (TEST_RESULT(all_result))
                all_result = rc;
        }
    }

    test_param_iterations_free(&iters);
    if (ctx_cloned)
        tester_ctx_free(ctx);

    if (test_skipped && ((all_result == ETESTPASS) ||
                         (all_result == ETESTEMPTY)))
        all_result = ETESTSKIP;

    return all_result;
}


/**
 * Run tests configuration.
 *
 * @param ctx       Tester context
 * @param cfg       Tester configuration
 *
 * @return Status code.
 */
int
tester_run_config(tester_ctx *ctx, tester_cfg *cfg)
{
    int         rc;
    int         result = 0;
    char       *maintainers;
    reqs_expr  *targets_root = NULL;
    run_item   *test;

    ENTRY();

    maintainers = persons_info_to_string(&cfg->maintainers);
    RING("Running configuration:\n"
         "File:        %s\n"
         "Maintainers:%s\n"
         "Description: %s",
         cfg->filename, maintainers, cfg->descr ? : "(no description)");
    free(maintainers);

    /* Clone Tester context */
    ctx = tester_ctx_clone(ctx);
    if (ctx == NULL)
    {
        ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
        return ENOMEM;
    }
    if (cfg->targets != NULL)
    {
        if (ctx->targets != NULL)
        {
            /* Add configuration to context requirements */
            ctx->targets = targets_root =
                reqs_expr_binary(TESTER_REQS_EXPR_AND,
                                 ctx->targets, cfg->targets);
            if (ctx->targets == NULL)
            {
                tester_ctx_free(ctx);
                return ENOMEM;
            }
        }
        else
        {
            ctx->targets = cfg->targets;
        }
    }
    /* Add flags set for the root element of the run path */
    ctx->flags |= ctx->path->flags;

    /*
     * Process options. Put options in appropriate context.
     */
    if (cfg->options.tqh_first != NULL)
        WARN("Options in Tester configuration files are ignored.");

    /*
     * Run all listed tests.
     */
    for (test = cfg->runs.tqh_first;
         test != NULL;
         test = test->links.tqe_next)
    {
        rc = iterate_test(ctx, test, NULL);
        if (!TEST_RESULT(rc))
        {
            ERROR("iterate_test() failed: %X", rc);
            result = ETESTUNEXP;
        }
        else if (result < rc)
        {
            result = rc;
        }
    }

    tester_reqs_expr_free_nr(targets_root);
    tester_ctx_free(ctx);

    return result;
}
