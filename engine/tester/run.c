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

#if 1
#define LOG_LEVEL   (ERROR_LVL | WARNING_LVL | RING_LVL | INFORMATION_LVL)
#else
#define LOG_LEVEL   0xff
#endif

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


/** Tester run path log user */
#define TESTER_FLOW         "FlowNoRGT"

/** Size of the Tester shell command buffer */
#define TESTER_CMD_BUF_SZ   1024

/** Size of the bulk used to allocate space for test parameters string */
#define TEST_PARAM_STR_BULK 32

#define TEST_RESULT(_rc) \
    (((_rc) == ETESTPASS) || \
     (((_rc) >= ETESTFAILMIN) && ((_rc) <= ETESTFAILMAX)))

/** Print string which may be NULL. */
#define PRINT_STRING(_str)  ((_str) ? : "")

/** ID of the main session of a Test Package */
#define TEST_ID_PKG_MAIN_SESSION    (-1)


/** Test parameters iteration entry */
typedef struct test_param_iteration {
    TAILQ_ENTRY(test_param_iteration)   links;  /**< List links */
    test_params                         params; /**< List of parameters */
    const struct test_param_iteration  *base;   /**< Base iteration */
} test_param_iteration;

/** List of test parameters iterations */
typedef TAILQ_HEAD(test_param_iterations, test_param_iteration)
    test_param_iterations;


/** Test parameters grouped in one pointer */
typedef struct test_thread_params {
    tester_ctx     *ctx;        /**< Tester context */
    run_item       *test;       /**< Test to be run */
    test_id         id;         /**< Test ID */
    test_params    *params;     /**< Test parameters */

#ifdef TESTER_TIMEOUT_SUPPORT
    int             pipe;       /**< One side of the pipe for notification */
#endif
    
    int             rc;         /**< Test result */
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

    TAILQ_INIT(&new_ctx->reqs);
    rc = test_requirements_clone(&ctx->reqs, &new_ctx->reqs);
    if (rc != 0)
    {
        ERROR("%s(): failed to clone requirements: %X", __FUNCTION__, rc);
        free(new_ctx);
        return NULL;
    }
    TAILQ_INIT(&new_ctx->paths);
    rc = tester_run_paths_clone(&ctx->paths, &new_ctx->paths);
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
 *
 * @return Poiner to allocated test parameter or NULL.
 */
static test_param *
test_param_new(const char *name, const char *value, te_bool clone)
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

    p->name = strdup(name);
    p->value = strdup(value);
    if (p->name == NULL || p->value == NULL)
    {
        ERROR("strdup() failed");
        free(p->name);
        free(p->value);
        free(p);
        EXIT("ENOMEM");
        return NULL;
    }
    p->clone = clone;
    VERB("New parameter: %s=%s", p->name, p->value);

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
            break;
        }
    }
    TAILQ_INSERT_TAIL(params, param, links);
}

/**
 * Clone a test parameter.
 *
 * @param p     Test parameter to be cloned
 *
 * @return Clone of the test parameter or NULL.
 */
static test_param *
test_param_clone(const test_param *p)
{
    test_param *pc = calloc(1, sizeof(*pc));

    ENTRY("%s=%s", p->name, p->value);

    if (pc == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(*pc));
        EXIT("ENOMEM");
        return NULL;
    }

    pc->name = strdup(p->name);
    pc->value = strdup(p->value);
    if (pc->name == NULL || pc->value == NULL)
    {
        ERROR("strdup() failed");
        free(pc->name);
        free(pc->value);
        free(pc);
        EXIT("ENOMEM");
        return NULL;
    }
    pc->clone = p->clone;

    return pc;
}

/**
 * Free a test parameter.
 *
 * @param p     Test parameter to be freed 
 */
static void
test_param_free(test_param *p)
{
    free(p->name);
    free(p->value);
    free(p);
}

/**
 * Free list of test parameters.
 *
 * @param params    List of test parameters to be freed    
 */
static void 
test_params_free(test_params *params)
{
    test_param *p;

    while ((p = params->tqh_first) != NULL)
    {
        TAILQ_REMOVE(params, p, links);
        test_param_free(p);
    }
}


/**
 * Allocate new test parameter iteration with empty set of parameters.
 *
 * @return Pointer to allocated memory or NULL.
 */
static test_param_iteration *
test_param_iteration_new(void)
{
    test_param_iteration *p = calloc(1, sizeof(*p));

    ENTRY();

    if (p == NULL)
    {
        ERROR("calloc(1, %u) failed", sizeof(*p));
        EXIT("ENOMEM");
        return NULL;
    }
    
    TAILQ_INIT(&p->params);

    return p;
}


/**
 * Clone existing test parameters iteration.
 *
 * @param i         Existing iteration
 *
 * @return Clone of the passed iteration or NULL.
 */
static test_param_iteration *
test_param_iteration_clone(const test_param_iteration *i)
{
    test_param_iteration *ic = test_param_iteration_new();
    test_param           *p;
    test_param           *pc;

    ENTRY("0x%x", (unsigned int)i);

    if (ic == NULL)
    {
        EXIT("ENOMEM");
        return NULL;
    }
    for (p = i->params.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->clone)
        {
            pc = test_param_clone(p);
            if (pc == NULL)
            {
                EXIT("ENOMEM");
                return NULL;
            }
            TAILQ_INSERT_TAIL(&ic->params, pc, links);
        }
    }
    ic->base = (i->base) ? : i;
    EXIT("OK");
    return ic;
}

/**
 * Free test parameters iteration.
 *
 * @param p     Iteration to be freed
 */
static void
test_param_iteration_free(test_param_iteration *p)
{
    test_params_free(&p->params);
    free(p);
}

/**
 * Free list of test parameters iterations.
 *
 * @param iters     List of test parameters iterations to be freed    
 */
static void 
test_param_iterations_free(test_param_iterations *iters)
{
    test_param_iteration *p;

    while ((p = iters->tqh_first) != NULL)
    {
        TAILQ_REMOVE(iters, p, links);
        test_param_iteration_free(p);
    }
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

    WARN("Failed to find value by reference '%s'", refer);
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
    assert(((value->value != NULL) + (value->refvalue != NULL) +
            (value->ext != NULL)) == 1);
    
    if (value->value != NULL)
        return value->value;
    else if (value->ext != NULL)
        return get_ref_value(value->ext, params);
    else if (value->refvalue != NULL)
        ERROR("Value to value references using 'refvalue' are not "
              "supported yet");
    else
        assert(0);
    
    return NULL;
}

/**
 * Get test session reffered variable value.
 *
 * @param var       Reffered variable
 * @param params    Parameters passed to test session
 *
 * @return Parameter value or NULL.
 */
static const char *
get_refvar_value(const test_session_var *var, const test_params *params)
{
    const char *refer;

    assert(var->type == TEST_SESSION_VAR_REFERRED);
    refer = (var->u.ref.attrs.refer) ? : var->name;

    return get_ref_value(refer, params);
}

/**
 * Get run item reffered argument value.
 *
 * @param arg       Reffered argument
 * @param param     Parameters context
 *
 * @return Parameter value or NULL.
 */
static const char *
get_refarg_value(const test_arg *arg, const test_params *params)
{
    const char *refer;

    assert(arg->type == TEST_ARG_REFERRED);
    refer = (arg->u.ref.attrs.refer) ? : arg->name;

    return get_ref_value(refer, params);
}


/**
 * Create a list of iterations based on session variables and passed 
 * to the session parameters (arguments).
 *
 * @param params    Current parameters
 * @param vars      Parent test session variables
 * @param iters     Head of the list of parameters iterations
 *
 * @return Status code.
 */
static int
vars_iterations(test_params *params, test_session_vars *vars,
                test_param_iterations *iters)
{
    test_param_iteration *i;
    test_param_iteration *i_clone;
    test_session_var     *var;
    test_var_arg_value   *value;
    test_param           *p;
    const char           *s;

    ENTRY();

    assert(vars != NULL);
    assert(iters != NULL);
    /* Variables iteration must start from empty list */
    assert(iters->tqh_first == NULL);
    
    i = test_param_iteration_new();
    if (i == NULL)
    {
        ERROR("Failed to create new test parameters iteration");
        EXIT("ENOMEM");
        return ENOMEM;
    }
    /* Insert one iteration with empty set of parametes */
    TAILQ_INSERT_TAIL(iters, i, links);

    /* Each for each set of iterations */
    for (var = vars->tqh_first; var != NULL; var = var->links.tqe_next)
    {
        for (i = iters->tqh_first; i != NULL; i = i->links.tqe_next)
        {
            if (var->type == TEST_SESSION_VAR_SIMPLE)
            {
                for (value = var->u.var.values.tqh_first;
                     value != NULL;
                     value = value->links.tqe_next)
                {
                    s = get_value(value, &i->base->params);
                    if (s == NULL)
                        return EINVAL;
                    p = test_param_new(var->name, s, var->handdown);
                    if (p == NULL)
                    {
                        ERROR("Failed to create new test parameter");
                        EXIT("ENOMEM");
                        return ENOMEM;
                    }
                    /* If one more value exist */
                    if (value->links.tqe_next != NULL)
                    {
                        /* Clone current iteration before update */
                        i_clone = test_param_iteration_clone(i);
                        if (i_clone == NULL)
                        {
                            ERROR("Cloning of the iteration failed");
                            EXIT("ENOMEM");
                            return ENOMEM;
                        }
                    }
                    else
                    {
                        /* 
                         * It is the last value, 
                         * there is no necessity to clone.
                         */
                        i_clone = NULL;
                    }
                    /* Update current iteration */
                    test_params_add(&i->params, p);
                    if (i_clone != NULL)
                    {
                        /* Insert clone after current iteration */
                        TAILQ_INSERT_AFTER(iters, i, i_clone, links);
                        i = i_clone;
                    }
                }
            }
            else if (var->type == TEST_SESSION_VAR_REFERRED)
            {
                s = get_refvar_value(var, params);
                if (s == NULL)
                    return EINVAL;
                p = test_param_new(var->name, s, var->handdown);
                if (p == NULL)
                {
                    ERROR("Failed to create new test parameter");
                    EXIT("ENOMEM");
                    return ENOMEM;
                }
                test_params_add(&i->params, p);
            }
            else
            {
                assert(FALSE);
            }
        }
    }

    EXIT("OK");
    return 0;
}


/**
 * Create a list of parameters iterations.
 *
 * @param base_iters    Base list of iterations
 * @param args          Test arguments
 * @param iters         Head of the list of parameters iterations
 *
 * @return Status code.
 */
static int
args_iterations(test_param_iterations *base_iters, test_args *args,
                test_param_iterations *iters)
{
    test_param_iteration *base_i;
    test_param_iteration *i;
    test_param_iteration *i_clone;
    test_arg             *arg;
    test_var_arg_value   *v;
    test_param           *p;
    const char           *s;

    ENTRY();

    assert(base_iters != NULL);
    assert(base_iters->tqh_first != NULL);
    assert(args != NULL);
    assert(iters != NULL);
    assert(iters->tqh_first == NULL);

    /* Clone base list of iterations */
    for (base_i = base_iters->tqh_first; 
         base_i != NULL; 
         base_i = base_i->links.tqe_next)
    {
        i = test_param_iteration_clone(base_i);
        if (i == NULL)
        {
            ERROR("Cloning of the test parameters iteration failed");
            return ENOMEM;
        }
        TAILQ_INSERT_TAIL(iters, i, links);
    }

    /* Each for each set of iterations */
    for (arg = args->tqh_first; arg != NULL; arg = arg->links.tqe_next)
    {
        for (i = iters->tqh_first; i != NULL; i = i->links.tqe_next)
        {
            if (arg->type == TEST_ARG_SIMPLE)
            {
                for (v = arg->u.arg.values.tqh_first;
                     v != NULL;
                     v = v->links.tqe_next)
                {
                    s = get_value(v, &i->base->params);
                    if (s == NULL)
                        return EINVAL;
                    p = test_param_new(arg->name, s, TRUE);
                    if (p == NULL)
                    {
                        ERROR("Failed to create new test parameter");
                        EXIT("ENOMEM");
                        return ENOMEM;
                    }
                    /* If one more value exist */
                    if (v->links.tqe_next != NULL)
                    {
                        /* Clone current iteration before update */
                        i_clone = test_param_iteration_clone(i);
                        if (i_clone == NULL)
                        {
                            ERROR("Cloning of the iteration failed");
                            EXIT("ENOMEM");
                            return ENOMEM;
                        }
                    }
                    else
                    {
                        /* 
                         * It is the last value, there is no necessity 
                         * to clone.
                         */
                        i_clone = NULL;
                    }
                    /* Update current iteration */
                    test_params_add(&i->params, p);
                    if (i_clone != NULL)
                    {
                        /* Insert clone after current iteration */
                        TAILQ_INSERT_AFTER(iters, i, i_clone, links);
                        i = i_clone;
                    }
                }
            }
            else if (arg->type == TEST_ARG_REFERRED)
            {
                s = get_refarg_value(arg, &i->base->params);
                if (s == NULL)
                    return EINVAL;
                p = test_param_new(arg->name, s, TRUE);
                if (p == NULL)
                {
                    ERROR("Failed to create new test parameter");
                    EXIT("ENOMEM");
                    return ENOMEM;
                }
                test_params_add(&i->params, p);
            }
            else
            {
                assert(FALSE);
            }
        }
    }

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
    return strdup("");
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
            char *nv = malloc(len += TEST_PARAM_STR_BULK);

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
            rest += TEST_PARAM_STR_BULK;
            v = nv;
        }
        rest -= sprintf(v + (len - rest), " \"%s=%s\"", p->name, p->value);
    }
    if (v != NULL)
        VERB("%s(): %s", __FUNCTION__, v);

    return v;
}

/**
 * Log test (script, package, session) result.
 *
 * @param parent    Parent ID
 * @param test      Test ID
 * @param result    Test result
 */
static void
log_test_result(test_id parent, test_id test, int result, te_bool verbose)
{
    const char *verdict;

    if (result == ETESTPASS)
    {
        verdict = "PASSED";
        LOG_RING(TESTER_FLOW, "$T%d $T%d PASSED", parent, test);
    }
    else
    {
        switch (result)
        {
            case ETESTKILL:
                LOG_RING(TESTER_FLOW, "$T%d $T%d KILLED", parent, test);
                verdict = "KILLED";
                break;

            case ETESTCORE:
                LOG_RING(TESTER_FLOW, "$T%d $T%d DUMPED", parent, test);
                verdict = "DUMPED";
                break;

            case ETESTSKIP:
                LOG_RING(TESTER_FLOW, "$T%d $T%d SKIPPED", parent, test);
                verdict = "SKIPPED";
                break;

            case ETESTFAKE:
                LOG_RING(TESTER_FLOW, "$T%d $T%d FAKE", parent, test);
                verdict = "FAKE";
                break;

            default:
            {
                const char *reason;

                verdict = "FAILED";
                switch (result)
                {
                    case ETESTFAIL:
                        reason = "TODO - from test";
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
                LOG_RING(TESTER_FLOW, "$T%d $T%d FAILED %s",
                         parent, test, reason);
                break;
            }
        }
    }

    if (verbose)
    {
        printf("%s\n", verdict);
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
    te_bool     ctx_cloned = FALSE;
    int         result = 0;
    int         rc;
    char       *params_str;
    char        cmd[TESTER_CMD_BUF_SZ];
    char        shell[256] = "";
    char        postfix[32] = "";
    char        gdb_init[32] = "";

    ENTRY();

    if (ctx->paths.tqh_first != NULL && !(ctx->flags & TESTER_CTX_INLOGUE))
    {
        ctx = tester_ctx_clone(ctx);
        if (ctx == NULL)
        {
            ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
            return ENOMEM;
        }
        ctx_cloned = TRUE;

        rc = tester_run_path_forward(ctx, script->name);
        if (rc != 0)
        {
            tester_ctx_free(ctx);
            if (rc == ENOENT)
            {
                /* Silently ignore nodes not of the path */
                return 0;
            }
            else
            {
                ERROR("%s(): tester_run_path_forward() failed", __FUNCTION__);
                return rc;
            }
        }
    }

    params_str = test_params_to_string(params);

    if (ctx->flags & TESTER_CTX_GDB)
    {
        FILE *f;

        if (snprintf(gdb_init, sizeof(gdb_init),
                     TESTER_GDB_FILENAME_FMT, id) >=
                (int)sizeof(gdb_init))
        {
            ERROR("Too short buffer is reserved for GDB init file name");
            return ETESMALLBUF;
        }
        /* TODO Clean up */
        f = fopen(gdb_init, "w");
        if (f == NULL)
        {
            ERROR("Failed to create GDB init file: %s", strerror(errno));
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
            ERROR("Too short buffer is reserved for shell command prefix");
            return ETESMALLBUF;
        }
    }
    else if (ctx->flags & TESTER_CTX_VALGRIND)
    {
        if (snprintf(shell, sizeof(shell),
                     "valgrind --num-callers=16 --leak-check=yes "
                     "--show-reachable=yes ") >= (int)sizeof(shell))
        {
            ERROR("Too short buffer is reserved for shell command prefix");
            return ETESMALLBUF;
        }
        if (snprintf(postfix, sizeof(postfix),
                     " 2>" TESTER_VG_FILENAME_FMT, id) >= (int)sizeof(postfix))
        {
            ERROR("Too short buffer is reserved for test script "
                  "command postfix");
            return ETESMALLBUF;
        }
    }

    if (snprintf(cmd, sizeof(cmd), "%s%s%s%s", shell, script->execute,
                 (ctx->flags & TESTER_CTX_GDB) ? "" : PRINT_STRING(params_str),
                 postfix) >= (int)sizeof(cmd))
    {
        ERROR("Too short buffer is reserved for test script command "
              "line");
        return ETESMALLBUF;
    }

    /* Log call */
    if (ctx->flags & TESTER_CTX_VERBOSE)
    {
        if (ctx->flags & TESTER_CTX_VVERB)
            printf("Starting %d:%d test %s ... ", ctx->id, id, script->name);
        else
            printf("Starting test %s ... ", script->name);
        fflush(stdout);
    }
    LOG_RING(TESTER_FLOW, "$T%d $T%d TEST %s \"%s\" ARGs%s",
             ctx->id, id, script->name, PRINT_STRING(script->descr),
             PRINT_STRING(params_str));
    free(params_str);

    if (!tester_is_run_required(ctx, &script->reqs, params))
    {
        result = ETESTSKIP;
    }
    else if (ctx->flags & TESTER_CTX_FAKE)
    {
        result = ETESTFAKE;
    }
    else
    {
        ERROR("$T%d system(%s)", id, cmd);
        rc = system(cmd);
        if (rc == -1)
        {
            ERROR("system(%s) failed: %X", cmd, errno);
            if (ctx_cloned)
                tester_ctx_free(ctx);
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
            ERROR("$T%d was killed by signal %d", id, WTERMSIG(rc));
            /* ETESTCORE may be already set */
            if (result == 0)
                result = ETESTKILL;
        }
        else if (!WIFEXITED(rc))
        {
            ERROR("$T%d was abnormally terminated", id);
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

    log_test_result(ctx->id, id, result,
                    !!(ctx->flags & TESTER_CTX_VERBOSE));

    EXIT("%X", result);

    if (ctx_cloned)
        tester_ctx_free(ctx);

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
    test_id                 parent_id = ctx->id; 
    int                     result = ETESTPASS;
    int                     rc;
    char                   *params_str;
    run_item               *test;
    test_param_iterations   iters;

    ENTRY();

    ctx = tester_ctx_clone(ctx);
    if (ctx == NULL)
    {
        ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
        return ENOMEM;
    }
    ctx->id = id;

    if (session->name != NULL && ctx->paths.tqh_first != NULL &&
        !(ctx->flags & TESTER_CTX_INLOGUE))
    {
        rc = tester_run_path_forward(ctx, session->name);
        if (rc != 0)
        {
            tester_ctx_free(ctx);
            if (rc == ENOENT)
            {
                /* Silently ignore nodes not of the path */
                return 0;
            }
            else
            {
                ERROR("%s(): tester_run_path_forward() failed", __FUNCTION__);
                return rc;
            }
        }
    }

    /* Create list of iterations */
    TAILQ_INIT(&iters);
    rc = vars_iterations(params, &session->vars, &iters);
    if (rc != 0)
    {
        ERROR("Failed to create a list of parameters iterations");
        tester_ctx_free(ctx);
        return rc;
    }
    if (iters.tqh_first == NULL)
    {
        ERROR("Empty list of parameters iterations");
        tester_ctx_free(ctx);
        return EINVAL;
    }

    if (id != TEST_ID_PKG_MAIN_SESSION)
    {
        if (ctx->flags & TESTER_CTX_VVERB)
        {
            printf("Starting %d:%d session ...\n", parent_id, id);
        }
        /* Log test session bindings and parameters */
        params_str = test_params_to_string(params);
        LOG_RING(TESTER_FLOW, "$T%d $T%d SESSION ARGs%s",
                 parent_id, id, PRINT_STRING(params_str));
        free(params_str);
    }

    /* If fake flag is already set in context */
    if (ctx->flags & TESTER_CTX_FAKE)
    {
        result = ETESTFAKE;
    }

    do {
        if (session->prologue != NULL)
        {
            if (ctx->flags & TESTER_CTX_NOLOGUES)
            {
                VERB("Prologues are disabled globally");
            }
            else
            {
                VERB("Running test session prologue...");
                ctx->flags |= TESTER_CTX_INLOGUE;
                rc = iterate_test(ctx, session->prologue, &iters);
                if ((rc != 0) && (rc != ETESTFAKE))
                {
                    result = ETESTPROLOG;
                    break;
                }
                ctx->flags &= ~TESTER_CTX_INLOGUE;
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
            if ((rc != 0) && (rc != ETESTFAKE) && (rc != ETESTSKIP))
            {
                /* 
                 * Other results except success and skip are mapped 
                 * to ETESTFAIL. 
                 */
                result = ETESTFAIL;
            }
        }

        if (session->epilogue != NULL)
        {
            if (ctx->flags & TESTER_CTX_NOLOGUES)
            {
                VERB("Epilogues are disabled globally");
            }
            else
            {
                VERB("Running test session epilogue...");
                ctx->flags |= TESTER_CTX_INLOGUE;
                rc = iterate_test(ctx, session->epilogue, &iters);
                if ((rc != 0) && (rc != ETESTFAKE))
                {
                    result = ETESTEPILOG;
                    break;
                }
                ctx->flags &= ~TESTER_CTX_INLOGUE;
            }
        }
        else
        {
            VERB("Test session has no epilogue");
        }
    } while (0);

    if (id != TEST_ID_PKG_MAIN_SESSION)
    {
        if (ctx->flags & TESTER_CTX_VVERB)
        {
            printf("Done %d:%d session ... ", ctx->id, id);
        }
        log_test_result(parent_id, id, result,
                        !!(ctx->flags & TESTER_CTX_VVERB));
    }

    EXIT("%d", result);

    test_param_iterations_free(&iters);
    tester_ctx_free(ctx);

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
    test_id parent_id = ctx->id;
    int     rc;
    int     result;
    char   *authors;
    char   *params_str;

    ENTRY();

    assert(!(ctx->flags & TESTER_CTX_INLOGUE));

    ctx = tester_ctx_clone(ctx);
    if (ctx == NULL)
    {
        ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
        return ENOMEM;
    }
    ctx->id = id;

    if (ctx->paths.tqh_first != NULL)
    {
        rc = tester_run_path_forward(ctx, pkg->name);
        if (rc != 0)
        {
            tester_ctx_free(ctx);
            if (rc == ENOENT)
            {
                /* Silently ignore nodes not of the path */
                return 0;
            }
            else
            {
                ERROR("%s(): tester_run_path_forward() failed", __FUNCTION__);
                return rc;
            }
        }
    }

    if (ctx->flags & TESTER_CTX_VERBOSE)
    {
        if (ctx->flags & TESTER_CTX_VVERB)
            printf("Starting %d:%d package %s ...\n",
                   parent_id, id, pkg->name);
        else
            printf("Starting package %s ...\n", pkg->name);
        fflush(stdout);
    }

    /* Log name, description, author(s) and parameters */
    authors = persons_info_to_string(&pkg->authors);
    params_str = test_params_to_string(params);
    LOG_RING(TESTER_FLOW, "$T%d $T%d PACKAGE %s \"%s\"%s%s ARGs%s",
             parent_id, id, pkg->name, PRINT_STRING(pkg->descr),
             (authors != NULL) ? " authours " : "", PRINT_STRING(authors),
             PRINT_STRING(params_str));
    free(authors);
    free(params_str);

    /* Check requirements */
    if (tester_is_run_required(ctx, &pkg->reqs, params))
    {
        /* Run test session provided by the package */
        result = run_test_session(ctx, &pkg->session, 
#if 1
                                  TEST_ID_PKG_MAIN_SESSION,
#else
                                  tester_get_id(),
#endif
                                  params);
    }
    else
    {
        /* Log that test package is skipped because of 'reason' */
        VERB("Test Package '%s' skipped", pkg->name);
        result = ETESTSKIP;
    }

    if (ctx->flags & TESTER_CTX_VERBOSE)
    {
        if (ctx->flags & TESTER_CTX_VVERB)
            printf("Done %d:%d package %s ... ", parent_id, id, pkg->name);
        else
            printf("Done package %s ... ", pkg->name);
    }
    log_test_result(parent_id, id, result,
                    !!(ctx->flags & TESTER_CTX_VERBOSE));
    
    tester_ctx_free(ctx);

    return result;
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
 * @param params        Test parameters
 *
 * @return Status code.
 */
static int
run_test(tester_ctx *ctx, run_item *test, test_params *params)
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
        thr_params.id     = tester_get_id(); 
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
    int                     result = ETESTPASS;
    test_param_iterations   aux_base_iters;
    test_param_iterations   iters;
    test_param_iteration   *i;
    char                   *backup_name;

    ENTRY();

    if (test->name != NULL && ctx->paths.tqh_first != NULL &&
        !(ctx->flags & TESTER_CTX_INLOGUE))
    {
        ctx = tester_ctx_clone(ctx);
        if (ctx == NULL)
        {
            ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
            return ENOMEM;
        }
        ctx_cloned = TRUE;

        rc = tester_run_path_forward(ctx, test->name);
        if (rc != 0)
        {
            tester_ctx_free(ctx);
            if (rc == ENOENT)
            {
                /* Silently ignore nodes not of the path */
                return 0;
            }
            else
            {
                ERROR("%s(): tester_run_path_forward() failed", __FUNCTION__);
                return rc;
            }
        }
    }

    TAILQ_INIT(&aux_base_iters);

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
        TAILQ_INSERT_TAIL(&aux_base_iters, i, links);
        base_iters = &aux_base_iters;
    }

    /* Create list of iterations */
    TAILQ_INIT(&iters);
    rc = args_iterations(base_iters, &test->args, &iters);
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

    /* Iterate parameters */
    for (i = iters.tqh_first; i != NULL; i = i->links.tqe_next)
    {
        if (test->attrs.track_conf)
        {
            /* Make Configuration backup, if required */
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

        /* Run test with specified parameters */
        rc = run_test(ctx, test, &(i->params));
        if (!TEST_RESULT(rc))
        {
            ERROR("run_test() failed: %X", rc);
            result = ETESTUNEXP;
        }
        else if (result < rc)
        {
            result = rc;
        }

        if (test->attrs.track_conf)
        {
            /* Check configuration backup */
            rc = cfg_verify_backup(backup_name);
            if (TE_RC_GET_ERROR(rc) == ETEBACKUP)
            {
                /* If backup is not OK, restore it */
                WARN("Current configuration differs from backup - restore");
                rc = cfg_restore_backup(backup_name);
                if (rc != 0)
                {
                    ERROR("Cannot restore configuration backup: %X", rc);
                    result = rc;
                }
                else
                {
                    INFO("Configuration successfully restored using backup");
                    RC_UPDATE(result, ETESTCONF);
                }
            }
            else if (rc != 0)
            {
                ERROR("Cannot verify configuration backup: %X", rc);
                result = rc;
            }
            free(backup_name);
        }
    } while (0);

    test_param_iterations_free(&aux_base_iters);
    test_param_iterations_free(&iters);
    if (ctx_cloned)
        tester_ctx_free(ctx);

    return result;
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
    const char *maintainer_name;
    const char *maintainer_mailto;
    run_item   *test;

    ENTRY();

    assert(cfg->maintainers.tqh_first != NULL);
    maintainer_name   = cfg->maintainers.tqh_first->name;
    maintainer_mailto = cfg->maintainers.tqh_first->mailto;
    assert(maintainer_mailto != NULL);

    INFO("Running configuration:\n"
         "File:        %s\n"
         "Maintainer:  %s%smailto:%s\n"
         "Description: %s",
         cfg->filename,
         maintainer_name ? : "", maintainer_name ? " " : "",
         maintainer_mailto,
         cfg->descr ? : "(no description)");

    /* Clone Tester context */
    ctx = tester_ctx_clone(ctx);
    if (ctx == NULL)
    {
        ERROR("%s(): tester_ctx_clone() failed", __FUNCTION__);
        return ENOMEM;
    }
    /* Add configuration to context requirements */
    rc = test_requirements_clone(&cfg->reqs, &ctx->reqs);
    if (rc != 0)
    {
        ERROR("%s(): test_requirements_clone() failed", __FUNCTION__);
        tester_ctx_free(ctx);
        return ENOMEM;
    }

    /*
     * Process options. Put options in appropriate context.
     */
    WARN("Options in Tester configuration files are ignored.");

    /*
     * Run all listed tests.
     */
    for (test = cfg->runs.tqh_first;
         test != NULL;
         test = test->links.tqe_next)
    {
        rc = iterate_test(ctx, test, NULL);
        if (rc != 0)
        {
            ERROR("TODO");
            tester_ctx_free(ctx);
            return rc;
        }
    }

    tester_ctx_free(ctx);

    return 0;
}
