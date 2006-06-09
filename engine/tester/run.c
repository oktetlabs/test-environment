/** @file
 * @brief Tester Subsystem
 *
 * Code dealing with running of the requested configuration.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#define TE_LGR_USER     "Run"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE
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

#include "tester_conf.h"
#include "tester_term.h"
#include "tester_run.h"


/** Define it to enable support of timeouts in Tester */
#undef TESTER_TIMEOUT_SUPPORT

/** Format string for Valgrind output filename */
#define TESTER_VG_FILENAME_FMT  "vg.test.%d"

/** Format string for GDB init filename */
#define TESTER_GDB_FILENAME_FMT "gdb.%d"

/** Tester run path log user */
#define TESTER_CONTROL             "Control"
/** Prefix of all messages from Tester:Flow user */
#define TESTER_CONTROL_MSG_PREFIX  "%u %u "

/** Size of the Tester shell command buffer */
#define TESTER_CMD_BUF_SZ           32768

/** Size of the bulk used to allocate space for a string */
#define TESTER_STR_BULK 64

/** Is return code a test result or TE error? */
#define TEST_RESULT(_rc) \
    (((TE_RC_GET_ERROR(_rc)) >= TE_ETESTRESULTMIN) && \
     (TE_RC_GET_ERROR(_rc) <= TE_ETESTRESULTMAX))

/** Print string which may be NULL. */
#define PRINT_STRING(_str)  ((_str) ? : "")


/** Tester context */
typedef struct tester_ctx {
    LIST_ENTRY(tester_ctx)  links;  /**< List links */

    unsigned int        flags;          /**< Flags (enum tester_flags) */

    te_errno            group_status;   /**< Status code of whole group
                                             executed in this context */
    te_errno            status;         /**< Status code */

    unsigned int        parent_id;      /**< ID of the test parent */
    unsigned int        child_id;       /**< ID of the test to execute */
    
    const reqs_expr    *targets;        /**< Target requirements
                                             expression */
    te_bool             targets_free;   /**< Should target requirements
                                             be freed? */

    test_requirements   reqs;           /**< List of collected sticky
                                              requirements */

    char               *backup;         /**< Configuration backup name */
    te_bool             backup_ok;      /**< Optimization to avoid
                                             duplicate (subsequent)
                                             verifications */

    test_iter_arg      *args;           /**< Test iteration arguments */
    unsigned int        n_args;         /**< Number of arguments */

    struct tester_ctx  *keepalive_ctx;  /**< Keep-alive context */
} tester_ctx;

/**
 * Opaque data for all configuration traverse callbacks.
 */
typedef struct tester_run_data {
    unsigned int                flags;      /**< Flags */
    const reqs_expr            *targets;    /**< Target requirements
                                                 expression specified
                                                 in command line */

    const testing_act          *act;        /**< Current testing act */
    unsigned int                act_id;     /**< Configuration ID of
                                                 the current test to run */

    LIST_HEAD(, tester_ctx)     ctxs;       /**< Stack of contexts */

} tester_run_data;


/**
 * Get unique test ID.
 */
static test_id
tester_get_id(void)
{
    static test_id id = 0;

    return id++;
}

/**
 * Free Tester context.
 *
 * @param ctx       Tester context to be freed
 */
static void
tester_ctx_free(tester_ctx *ctx)
{
    if (ctx == NULL)
        return;

    if (ctx->targets_free)
    {
        /*
         * It is OK to discard 'const' qualifier here, since it exactly
         * specified that it should be freed.
         */
        tester_reqs_expr_free_nr((reqs_expr *)ctx->targets);
    }
    test_requirements_free(&ctx->reqs);
    tester_ctx_free(ctx->keepalive_ctx);
    free(ctx);
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
    new_ctx->flags = ctx->flags;
    new_ctx->group_status = TE_RC(TE_TESTER, TE_ETESTEMPTY);
    /* new_ctx->status = 0; */
    new_ctx->parent_id = ctx->parent_id;
    new_ctx->child_id = ctx->child_id;
    new_ctx->targets = ctx->targets;
    new_ctx->targets_free = FALSE;

    TAILQ_INIT(&new_ctx->reqs);
    rc = test_requirements_clone(&ctx->reqs, &new_ctx->reqs);
    if (rc != 0)
    {
        ERROR("%s(): failed to clone requirements: %r", __FUNCTION__, rc);
        free(new_ctx);
        return NULL;
    }

    /* new_ctx->backup = NULL; */
    /* new_ctx->backup_ok = FALSE; */

    /* new_ctx->args = NULL; */

    /* new_ctx->keepalive_ctx = NULL; */

    return new_ctx;
}

/**
 * Allocate a new (the first) tester context.
 *
 * @param data          Global Tester context data
 *
 * @return Allocated context.
 */
static tester_ctx *
tester_run_new_ctx(tester_run_data *data)
{
    tester_ctx *new_ctx = calloc(1, sizeof(*new_ctx));
    
    if (new_ctx == NULL)
        return NULL;

    new_ctx->flags = data->flags;
    new_ctx->group_status = TE_RC(TE_TESTER, TE_ETESTEMPTY);
    /* new_ctx->status = 0; */
    new_ctx->parent_id = tester_get_id();
    /* new_ctx->child_id = 0; */
    new_ctx->targets = data->targets;
    new_ctx->targets_free = FALSE;
    TAILQ_INIT(&new_ctx->reqs);
    /* new_ctx->backup = NULL; */
    /* new_ctx->backup_ok = FALSE; */
    /* new_ctx->args = NULL; */

    assert(data->ctxs.lh_first == NULL);
    LIST_INSERT_HEAD(&data->ctxs, new_ctx, links);

    VERB("Initial context: flags=0x%x parent_id=%u",
         new_ctx->flags, new_ctx->parent_id);

    return new_ctx;
}

/**
 * Clone the most recent (current) Tester context.
 *
 * @param data          Global Tester context data
 *
 * @return Allocated context.
 */
static tester_ctx *
tester_run_clone_ctx(tester_run_data *data)
{
    tester_ctx *new_ctx;

    assert(data->ctxs.lh_first != NULL);
    new_ctx = tester_ctx_clone(data->ctxs.lh_first);
    if (new_ctx == NULL)
    {
        data->ctxs.lh_first->status = TE_RC(TE_TESTER, TE_ENOMEM);
        return NULL;
    }
    
    LIST_INSERT_HEAD(&data->ctxs, new_ctx, links);

    VERB("Tester context %p clonned %p: flags=0x%x parent_id=%u "
         "child_id=%u", new_ctx->links.le_next, new_ctx, new_ctx->flags,
         new_ctx->parent_id, new_ctx->child_id);

    return new_ctx;
}

/**
 * Destroy the most recent (current) Tester context.
 *
 * @param data          Global Tester context data
 *
 * @return Status code.
 */
static void
tester_run_destroy_ctx(tester_run_data *data)
{
    tester_ctx *curr = data->ctxs.lh_first;
    tester_ctx *prev;

    assert(curr != NULL);
    prev = curr->links.le_next;
    if (prev != NULL)
        prev->status = curr->group_status;

    VERB("Tester context %p deleted: flags=0x%x parent_id=%u child_id=%u "
         "status=%r", curr, curr->flags, curr->parent_id, curr->child_id,
         curr->status);

    LIST_REMOVE(curr, links);
    tester_ctx_free(curr);
}

/**
 * Update test group status.
 * 
 * @param group_status  Group status
 * @param iter_status   Test iteration status
 *
 * @return Updated group status.
 */
static te_errno
tester_group_status(te_errno group_status, te_errno iter_status)
{
    if (!TEST_RESULT(iter_status))
    {
        group_status = iter_status;
    }
    else if (group_status < iter_status)
    {
        if (TE_RC_GET_ERROR(iter_status) == TE_ETESTSRCH)
            group_status = TE_RC(TE_TESTER, TE_ETESTFAIL);
        else
            group_status = iter_status;
    }
    return group_status;
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
                /* Discard just printed */
                s[0] = '\0';
                /* We are going to extend buffer */
                rest += total;
                total <<= 1;
                res = realloc(res, total);
                if (res == NULL)
                {
                    ERROR("%s(): Memory allocation failure", __FUNCTION__);
                    return NULL;
                }
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
 * @param arg           Test argument
 */
static size_t
test_param_space(const test_iter_arg *arg)
{
    return 1 /* space */ + strlen(arg->name) + 1 /* = */ +
           1 /* " */ + strlen(arg->value) + 1 /* " */ + 1 /* \0 */;
}

/**
 * Convert test parameters to string representation.
 * The first symbol is a space, if the result is not NULL.
 *
 * @param str           Initial string to append parameters or NULL
 * @param n_args        Number of arguments
 * @param args          Current arguments context
 *
 * @return Allocated string or NULL.
 */
static char *
test_params_to_string(char *str, const unsigned int n_args,
                      const test_iter_arg *args)
{
    unsigned int         i;
    const test_iter_arg *p;

    char       *v = str;
    size_t      len = (str == NULL) ? 0 : (strlen(str) + 1);
    size_t      rest = (len == 0) ? 0 : 1;


    if (args == NULL)
        return v;

    for (i = 0, p = args; i < n_args; ++i, ++p)
    {
        size_t req = test_param_space(p);

        VERB("%s(): parameter %s=%s", __FUNCTION__, p->name, p->value);
        while (rest < req)
        {
            char *nv;

            len += TESTER_STR_BULK;
            nv = realloc(v, len);
            if (nv == NULL)
            {
                ERROR("realloc(%p, %u) failed", v, len);
                return NULL;
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
 * @param ri            Run item
 * @param parent        Parent ID
 * @param test          Test ID
 * @param args          Array with test iteration arguments
 */
static void
log_test_start(const run_item *ri, test_id parent, test_id test,
               const test_iter_arg *args)
{
    char   *params_str;
    char   *authors;

    params_str = test_params_to_string(NULL, ri->n_args, args);
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
    if (TE_RC_GET_ERROR(result) == TE_ETESTPASS)
    {
        LOG_RING(TESTER_CONTROL, TESTER_CONTROL_MSG_PREFIX "PASSED",
                 parent, test);
    }
    else
    {
        switch (TE_RC_GET_ERROR(result))
        {
            case TE_ETESTKILL:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "KILLED",
                         parent, test);
                break;

            case TE_ETESTCORE:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "CORED",
                         parent, test);
                break;

            case TE_ETESTSKIP:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "SKIPPED",
                         parent, test);
                break;

            case TE_ETESTFAKE:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "FAKED",
                         parent, test);
                break;

            case TE_ETESTEMPTY:
                LOG_RING(TESTER_CONTROL,
                         TESTER_CONTROL_MSG_PREFIX "EMPTY",
                         parent, test);
                break;

            default:
            {
                const char *reason;

                switch (TE_RC_GET_ERROR(result))
                {
                    case TE_ETESTFAIL:
                        /* TODO Reason from test */
                        reason = "";
                        break;

                    case TE_ETESTCONF:
                        reason = "Unexpected configuration changes";
                        break;

                    case TE_ETESTSRCH:
                        reason = "Executable not found";
                        break;

                    case TE_ETESTPROLOG:
                        reason = "Session prologue failed";
                        break;

                    case TE_ETESTEPILOG:
                        reason = "Session epilogue failed";
                        break;

                    case TE_ETESTALIVE:
                        reason = "Keep-alive validation failed";
                        break;

                    case TE_ETESTEXCEPT:
                        reason = "Exception handler failed";
                        break;

                    case TE_ETESTUNEXP:
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
 * @param exec_id       Test execution ID
 * @param script        Test script to run
 * @param n_args        Number of arguments
 * @param args          Arguments to be passed
 * @param flags         Flags
 *
 * @return Status code.
 */
static te_errno
run_test_script(test_script *script, test_id exec_id,
                const unsigned int n_args, const test_iter_arg *args,
                const unsigned int flags)
{
    te_errno    result = 0;
    int         ret;
    char       *params_str = NULL;
    char       *cmd = NULL;
    char        shell[256] = "";
    char        gdb_init[32] = "";
    char        postfix[32] = "";
    char        vg_filename[32] = "";

    ENTRY("name=%s exec_id=%u n_args=%u arg=%p flags=0x%x",
          script->name, exec_id, n_args, args, flags);

    if (asprintf(&params_str, " te_test_id=%u", exec_id) < 0)
    {
        ERROR("%s(): asprintf() failed", __FUNCTION__);
        return TE_RC(TE_TESTER, TE_ENOMEM);
    }
    params_str = test_params_to_string(params_str, n_args, args);

    if (flags & TESTER_GDB)
    {
        FILE *f;

        if (snprintf(gdb_init, sizeof(gdb_init),
                     TESTER_GDB_FILENAME_FMT, exec_id) >=
                (int)sizeof(gdb_init))
        {
            ERROR("Too short buffer is reserved for GDB init file "
                  "name");
            free(params_str);
            return TE_RC(TE_TESTER, TE_ESMALLBUF);
        }
        f = fopen(gdb_init, "w");
        if (f == NULL)
        {
            ERROR("Failed to create GDB init file: %s",
                  strerror(errno));
            free(params_str);
            return TE_OS_RC(TE_TESTER, errno);
        }
        fprintf(f, "set args %s\n", params_str);
        free(params_str);
        params_str = NULL;
        if (fclose(f) != 0)
        {
            ERROR("fclose() failed");
            return TE_OS_RC(TE_TESTER, errno);;
        }
        if (snprintf(shell, sizeof(shell),
                     "gdb -x %s ", gdb_init) >=
                (int)sizeof(shell))
        {
            ERROR("Too short buffer is reserved for shell command "
                  "prefix");
            return TE_RC(TE_TESTER, TE_ESMALLBUF);
        }
    }
    else if (flags & TESTER_VALGRIND)
    {
        if (snprintf(shell, sizeof(shell),
                     "valgrind --num-callers=16 --leak-check=yes "
                     "--show-reachable=yes --tool=memcheck ") >=
                (int)sizeof(shell))
        {
            ERROR("Too short buffer is reserved for shell command prefix");
            free(params_str);
            return TE_RC(TE_TESTER, TE_ESMALLBUF);
        }
        if (snprintf(vg_filename, sizeof(vg_filename),
                     TESTER_VG_FILENAME_FMT, exec_id) >=
                (int)sizeof(vg_filename))
        {
            ERROR("Too short buffer is reserved for Vagrind output "
                  "filename");
            free(params_str);
            return TE_RC(TE_TESTER, TE_ESMALLBUF);
        }
        if (snprintf(postfix, sizeof(postfix), " 2>%s", vg_filename) >=
                (int)sizeof(postfix))
        {
            ERROR("Too short buffer is reserved for test script "
                  "command postfix");
            free(params_str);
            return TE_RC(TE_TESTER, TE_ESMALLBUF);
        }
    }

    cmd = malloc(TESTER_CMD_BUF_SZ);
    if (cmd == NULL)
    {
        ERROR("%s():%u: malloc(%u) failed",
              __FUNCTION__, __LINE__, TESTER_CMD_BUF_SZ);
        free(params_str);
        return TE_OS_RC(TE_TESTER, errno);;
    }
    if (snprintf(cmd, TESTER_CMD_BUF_SZ, "%s%s%s%s", shell, script->execute,
                 (flags & TESTER_GDB) ? "" : PRINT_STRING(params_str),
                 postfix) >= TESTER_CMD_BUF_SZ)
    {
        ERROR("Too short buffer is reserved for test script command "
              "line");
        free(cmd);
        free(params_str);
        return TE_RC(TE_TESTER, TE_ESMALLBUF);
    }
    free(params_str);

    if (flags & TESTER_FAKE)
    {
        result = TE_RC(TE_TESTER, TE_ETESTFAKE);
    }
    else
    {
        VERB("ID=%d system(%s)", exec_id, cmd);
        ret = system(cmd);
        if (ret == -1)
        {
            ERROR("system(%s) failed: errno %d", cmd, errno);
            free(cmd);
            return TE_OS_RC(TE_TESTER, errno);;
        }

#ifdef WCOREDUMP
        if (WCOREDUMP(ret))
        {
            ERROR("Command '%s' executed in shell dumped core", cmd);
            result = TE_RC(TE_TESTER, TE_ETESTCORE);
        }
#endif
        if (WIFSIGNALED(ret))
        {
            if (WTERMSIG(ret) == SIGINT)
            {
                result = TE_RC(TE_TESTER, TE_ESHUTDOWN);
                ERROR("ID=%d was interrupted by SIGINT, shut down",
                      exec_id);
            }
            else
            {
                ERROR("ID=%d was killed by the signal %d : %s", exec_id,
                      WTERMSIG(ret), strsignal(WTERMSIG(ret)));
                /* TE_ETESTCORE may already be set */
                if (result == 0)
                    result = TE_RC(TE_TESTER, TE_ETESTKILL);
            }
        }
        else if (!WIFEXITED(ret))
        {
            ERROR("ID=%d was abnormally terminated", exec_id);
            /* TE_ETESTCORE may already be set */
            if (result == 0)
                result = TE_RC(TE_TESTER, TE_ETESTUNEXP);
        }
        else
        {
            if (result != 0)
                ERROR("Unexpected return value of system() call");

            switch (WEXITSTATUS(ret))
            {
                case EXIT_FAILURE:
                    result = TE_RC(TE_TESTER, TE_ETESTFAIL);
                    break;

                case EXIT_SUCCESS:
                    result = TE_RC(TE_TESTER, TE_ETESTPASS);
                    break;

                case TE_EXIT_SIGINT:
                    result = TE_RC(TE_TESTER, TE_ESHUTDOWN);
                    ERROR("ID=%d was interrupted by SIGINT, shut down",
                          exec_id);
                    break;

                case TE_EXIT_NOT_FOUND:
                    result = TE_RC(TE_TESTER, TE_ETESTSRCH);
                    ERROR("ID=%d was not run, executable not found",
                          exec_id);
                    break;

                default:
                    result = TE_RC(TE_TESTER, TE_ETESTUNEXP);
            }
        }
        if (flags & TESTER_VALGRIND)
        {
            te_log_message(__FILE__, __LINE__,
                           TE_LL_INFO, TE_LGR_ENTITY, TE_LGR_USER,
                           "Standard error output of the script with "
                           "ID=%d:\n%Tf", exec_id, vg_filename);
        }
    }

    free(cmd);

    EXIT("%r", result);

    return result;
}


static tester_cfg_walk_ctl
run_script(run_item *ri, test_script *script,
           unsigned int cfg_id_off, void *opaque)
{
    tester_run_data        *gctx = opaque;
    tester_ctx             *ctx;
    tester_cfg_walk_ctl     ctl;

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u script=%s", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id, script->name);

    assert(ri != NULL);
    assert(ri->n_args == ctx->n_args);
    ctx->status = run_test_script(script, ctx->child_id,
                                  ctx->n_args, ctx->args,
                                  gctx->act == NULL ? 0 : /* FIXME */
                                      gctx->act->flags);

    switch (TE_RC_GET_ERROR(ctx->status))
    {
        case TE_ETESTSKIP:
        case TE_ETESTFAKE:
        case TE_ETESTPASS:
        case TE_ETESTSRCH:
        case TE_ETESTFAIL:
            ctl = TESTER_CFG_WALK_CONT;
            break;

        case TE_ETESTKILL:
        case TE_ETESTCORE:
        case TE_ETESTUNEXP:
            ctl = TESTER_CFG_WALK_EXC;
            break;

        case TE_ESHUTDOWN:
            ctl = TESTER_CFG_WALK_STOP;
            /* Override status as KILLED */
            ctx->status = TE_RC(TE_TESTER, TE_ETESTKILL);
            break;

        default:
            ctl = TESTER_CFG_WALK_FAULT;
            break;
    }

    EXIT("%u", ctl);
    return ctl;
}


/**
 * Create (if it is necessary) configuration backup in specified context.
 *
 * @param ctx           Context
 * @param track_conf    'track_conf' attribute of the run item
 *
 * @return Status code.
 *
 * @note Status code in context is updated as well.
 */
static te_errno
run_create_cfg_backup(tester_ctx *ctx, tester_track_conf track_conf)
{
    if ((~ctx->flags & TESTER_NO_CFG_TRACK) &&
        (track_conf != TESTER_TRACK_CONF_NO))
    {
        te_errno rc;

        /* Create backup to be verified after each iteration */
        rc = cfg_create_backup(&ctx->backup);
        if (rc != 0)
        {
            ERROR("Cannot create configuration backup: %r", rc);
            if (TEST_RESULT(ctx->group_status))
                ctx->group_status = rc;
            EXIT("FAULT");
            return rc;
        }
        ctx->backup_ok = TRUE;
    }

    return 0;
}

/**
 * Verify configuration backup in the specified context.
 *
 * @param ctx           Context
 * @param track_conf    'track_conf' attribute of the run item
 *
 * @note Status code in context is updated in the case of failure.
 */
static void
run_verify_cfg_backup(tester_ctx *ctx, tester_track_conf track_conf)
{
    te_errno    rc;

    if (!ctx->backup_ok && ctx->backup != NULL)
    {
        /* Check configuration backup */
        rc = cfg_verify_backup(ctx->backup);
        if (TE_RC_GET_ERROR(rc) == TE_EBACKUP)
        {
            if (track_conf == TESTER_TRACK_CONF_YES)
            {
                /* If backup is not OK, restore it */
                WARN("Current configuration differs from backup - "
                     "restore");
            }
            rc = cfg_restore_backup(ctx->backup);
            if (rc != 0)
            {
                ERROR("Cannot restore configuration backup: %r", rc);
                if (TEST_RESULT(ctx->status))
                    ctx->status = rc;
            }
            else if (track_conf == TESTER_TRACK_CONF_YES)
            {
                RING("Configuration successfully restored using backup");
                if (TEST_RESULT(ctx->status) &&
                    (ctx->status < TE_RC(TE_TESTER, TE_ETESTCONF)))
                {
                    ctx->status = TE_RC(TE_TESTER, TE_ETESTCONF);
                }
            }
            else
            {
                ctx->backup_ok = TRUE;
            }
        }
        else if (rc != 0)
        {
            ERROR("Cannot verify configuration backup: %r", rc);
            if (TEST_RESULT(ctx->status))
                ctx->status = rc;
        }
        else
        {
            ctx->backup_ok = TRUE;
        }
    }
}

/**
 * Release configuration backup in the specified context.
 *
 * @param ctx           Context
 *
 * @return Status code.
 *
 * @note Status code in context is updated in the case of failure.
 */
static te_errno
run_release_cfg_backup(tester_ctx *ctx)
{
    te_errno    rc = 0;

    if (ctx->backup != NULL)
    {
        rc = cfg_release_backup(&ctx->backup);
        if (rc != 0)
        {
            ERROR("cfg_release_backup() failed: %r", rc);
            if (TEST_RESULT(ctx->group_status))
                ctx->group_status = rc;
            ctx->backup = NULL;
        }
        else
        {
            assert(ctx->backup == NULL);
        }
    }
    return rc;
}


static tester_cfg_walk_ctl
run_cfg_start(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    char               *maintainers;

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    if (gctx->act_id >= cfg_id_off + cfg->total_iters)
    {
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    maintainers = persons_info_to_string(&cfg->maintainers);
    RING("Running configuration:\n"
         "File:        %s\n"
         "Maintainers:%s\n"
         "Description: %s",
         cfg->filename, maintainers,
         cfg->descr != NULL ? cfg->descr : "(no description)");
    free(maintainers);

    /*
     * Process options. Put options in appropriate context.
     */
    if (cfg->options.tqh_first != NULL)
        WARN("Options in Tester configuration files are ignored.");

    /* Clone Tester context */
    ctx = tester_run_clone_ctx(gctx);
    if (ctx == NULL)
        return TESTER_CFG_WALK_FAULT;

    if (cfg->targets != NULL)
    {
        if (ctx->targets != NULL)
        {
            /* Add configuration to context requirements */
            ctx->targets = reqs_expr_binary(TESTER_REQS_EXPR_AND,
                                            (reqs_expr *)ctx->targets,
                                            cfg->targets);
            if (ctx->targets == NULL)
            {
                tester_run_destroy_ctx(gctx);
                ctx->status = TE_RC(TE_TESTER, TE_ENOMEM);
                return TESTER_CFG_WALK_FAULT;
            }
            ctx->targets_free = TRUE;
        }
        else
        {
            ctx->targets = cfg->targets;
            ctx->targets_free = FALSE;
        }
    }

    EXIT();
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_cfg_end(tester_cfg *cfg, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(cfg);

    assert(gctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    tester_run_destroy_ctx(gctx);

    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

/**
 * Should this run item be run?
 *
 * @param cfg_id_off    Current offset of the configuration ID
 * @param act_id        Action ID to be run
 * @param weight        Weight of single iterations
 * @param n_iters       Number of iterations
 *
 * @retval TESTING_STOP         This item should be run
 * @retval TESTING_BACKWARD     Something before this item should be run
 * @retval TESTING_FORWARD      Something after this item should be run
 */
static testing_direction
run_this_item(const unsigned int cfg_id_off, const unsigned int act_id,
              const unsigned int weight, const unsigned int n_iters)
{
    VERB("%s(): act_id=%u cfg_id_off=%u weight=%u n_iters=%u",
         __FUNCTION__, act_id, cfg_id_off, weight, n_iters);

    if (act_id < cfg_id_off)
        return TESTING_BACKWARD;
    else if (act_id >= cfg_id_off + n_iters * weight)
        return TESTING_FORWARD;
    else
        return TESTING_STOP;
}

static tester_cfg_walk_ctl
run_item_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
               void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            rc;

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    if (~flags & TESTER_CFG_WALK_SERVICE)
    {
        if (tester_sigint_received)
        {
            ctx->status = TE_RC(TE_TESTER, TE_ETESTKILL);
            return TESTER_CFG_WALK_STOP;
        }

        switch (run_this_item(cfg_id_off, gctx->act_id,
                              ri->weight, ri->n_iters))
        {
            case TESTING_FORWARD:
                EXIT("SKIP");
                return TESTER_CFG_WALK_SKIP;

            case TESTING_BACKWARD:
                EXIT("BACK");
                return TESTER_CFG_WALK_BACK;

            case TESTING_STOP:
                /* Run here */
                break;

            default:
                assert(FALSE);
                ctx->status = TE_RC(TE_TESTER, TE_EFAULT);
                return TESTER_CFG_WALK_FAULT;
        }

        if (ctx->backup == NULL)
        {
            rc = run_create_cfg_backup(ctx, test_get_attrs(ri)->track_conf);
        }
    }

    assert(ctx->args == NULL);
    if (ri->n_args > 0)
    {
        unsigned int    i;

        ctx->args = calloc(ri->n_args, sizeof(*ctx->args));
        if (ctx->args == NULL)
        {
            ctx->status = TE_RC(TE_TESTER, TE_ENOMEM);
            return TESTER_CFG_WALK_FAULT;
        }
        ctx->n_args = ri->n_args;
        for (i = 0; i < ctx->n_args; ++i)
            TAILQ_INIT(&ctx->args[i].reqs);
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_item_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
             void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);
    UNUSED(flags);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    ctx->n_args = 0;
    free(ctx->args);
    ctx->args = NULL;

    if (run_release_cfg_backup(ctx) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_pkg_start(run_item *ri, test_package *pkg,
              unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            rc;

    UNUSED(ri);

    assert(gctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    ctx = tester_run_clone_ctx(gctx);
    if (ctx == NULL)
        return TESTER_CFG_WALK_FAULT;

    assert(~ctx->flags & TESTER_INLOGUE);

    ctx->parent_id = ctx->child_id;

    rc = tester_get_sticky_reqs(&ctx->reqs, &pkg->reqs);
    if (rc != 0)
    {
        ERROR("%s(): tester_get_sticky_reqs() failed: %r",
              __FUNCTION__, rc);
        ctx->status = rc;
        return TESTER_CFG_WALK_FAULT;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_session_start(run_item *ri, test_session *session,
                  unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);
    UNUSED(session);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u, (%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    /*
     * 'parent_id' is equal to 'child_id', if context is cloned in
     * package_start callback.
     */
    if (ctx->parent_id != ctx->child_id)
    {
        ctx = tester_run_clone_ctx(gctx);
        if (ctx == NULL)
            return TESTER_CFG_WALK_FAULT;
        ctx->parent_id = ctx->child_id;
    }
    ctx->child_id = 0; /* Just to catch errors */

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_session_end(run_item *ri, test_session *session,
                unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);
    UNUSED(session);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    tester_run_destroy_ctx(gctx);

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_prologue_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    if (ctx->flags & TESTER_NO_LOGUES)
    {
        WARN("Prologues are disabled globally");
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    ctx = tester_run_clone_ctx(gctx);
    if (ctx == NULL)
        return TESTER_CFG_WALK_FAULT;

    VERB("Running test session prologue...");
    ctx->flags |= TESTER_INLOGUE;

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_prologue_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            status;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    assert(ctx->flags & TESTER_INLOGUE);
    status = ctx->group_status;
    tester_run_destroy_ctx(gctx);
    
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    if ((TE_RC_GET_ERROR(status) != TE_ETESTPASS) && 
        (TE_RC_GET_ERROR(status) != TE_ETESTFAKE))
    {
        if ((TE_RC_GET_ERROR(status) == TE_ETESTSKIP) ||
            (TE_RC_GET_ERROR(status) == TE_ENOENT))
            ctx->group_status = TE_RC(TE_TESTER, TE_ETESTSKIP);
        else
            ctx->group_status = TE_RC(TE_TESTER, TE_ETESTPROLOG);
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_epilogue_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    if (ctx->flags & TESTER_NO_LOGUES)
    {
        WARN("Epilogues are disabled globally");
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    ctx = tester_run_clone_ctx(gctx);
    if (ctx == NULL)
        return TESTER_CFG_WALK_FAULT;

    VERB("Running test session epilogue...");
    ctx->flags |= TESTER_INLOGUE;

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_epilogue_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            status;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    assert(ctx->flags & TESTER_INLOGUE);
    status = ctx->status;
    tester_run_destroy_ctx(gctx);
    
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    if ((TE_RC_GET_ERROR(status) != TE_ETESTPASS) && 
        (TE_RC_GET_ERROR(status) != TE_ETESTFAKE))
    {
        if ((TE_RC_GET_ERROR(status) == TE_ETESTSKIP) ||
            (TE_RC_GET_ERROR(status) == TE_ENOENT))
            ctx->group_status = TE_RC(TE_TESTER, TE_ETESTSKIP);
        else
            ctx->group_status = TE_RC(TE_TESTER, TE_ETESTEPILOG);
        EXIT("SKIP");
        return TESTER_CFG_WALK_SKIP;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_keepalive_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    if (ctx->keepalive_ctx == NULL)
    {
        ctx->keepalive_ctx = tester_ctx_clone(ctx);
        if (ctx->keepalive_ctx == NULL)
        {
            ctx->status = TE_RC(TE_TESTER, TE_ENOMEM);
            return TESTER_CFG_WALK_FAULT;
        }
    }
    ctx = ctx->keepalive_ctx;
    
    if (run_create_cfg_backup(ctx, test_get_attrs(ri)->track_conf) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    LIST_INSERT_HEAD(&gctx->ctxs, ctx, links);

    VERB("Running test session keep-alive validation...");

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_keepalive_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            status;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    /* Release configuration backup, since it may differ the next time */
    if (run_release_cfg_backup(ctx) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    /* Remember status of the keep-alive validation */
    status = ctx->group_status;

    /* Remove keep-alive context (it is still stored in keepalive_ctx) */
    LIST_REMOVE(ctx, links);
 
    /* Get current context and update its group status */
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    if ((TE_RC_GET_ERROR(status) != TE_ETESTPASS) && 
        (TE_RC_GET_ERROR(status) != TE_ETESTFAKE))
    {
        ERROR("Keep-alive validation failed: %r", status);
        ctx->group_status =
            tester_group_status(ctx->group_status, 
                                TE_RC(TE_TESTER, TE_ETESTALIVE));
        EXIT("INTR");
        return TESTER_CFG_WALK_INTR;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_exception_start(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    assert(gctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    /* Exceptin handler is always run in a new context */
    ctx = tester_run_clone_ctx(gctx);
    if (ctx == NULL)
        return TESTER_CFG_WALK_FAULT;

    /* Create configuration backup in a new context */
    if (run_create_cfg_backup(ctx, test_get_attrs(ri)->track_conf) != 0)
    {
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    VERB("Running test session exception handler...");

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_exception_end(run_item *ri, unsigned int cfg_id_off, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    te_errno            status;

    UNUSED(ri);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    /* Release configuration backup, since it may differ the next time */
    if (run_release_cfg_backup(ctx) != 0)
    {
        tester_run_destroy_ctx(gctx);
        EXIT("FAULT");
        return TESTER_CFG_WALK_FAULT;
    }

    /* Remember status of the exception handler */
    status = ctx->group_status;

    /* Destroy exception handler context */
    tester_run_destroy_ctx(gctx);
 
    /* Get current context and update its group status */
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);

    if ((TE_RC_GET_ERROR(status) != TE_ETESTPASS) && 
        (TE_RC_GET_ERROR(status) != TE_ETESTFAKE))
    {
        ERROR("Exception handler failed: %r", status);
        ctx->group_status =
            tester_group_status(ctx->group_status, 
                                TE_RC(TE_TESTER, TE_ETESTEXCEPT));
        EXIT("INTR");
        return TESTER_CFG_WALK_INTR;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}


/**
 * Get plain value of the aggregated argument value.
 *
 * @param value         Aggregated argument value
 * @param args          Context arguments
 * @param n_args        Number of arguments in the context
 * @param reqs          List for collected requirements
 */
static const char *
run_get_value(const test_entity_value *value,
              const test_iter_arg *args, const unsigned int n_args,
              test_requirements *reqs)
{
    if (test_requirements_clone(&value->reqs, reqs) != 0)
        return NULL;

    if (value->plain != NULL)
    {
        return value->plain;
    }
    else if (value->ref != NULL)
    {
        return run_get_value(value->ref, args, n_args, reqs);
    }
    else if (value->ext != NULL)
    {
        unsigned int i;

        for (i = 0;
             i < n_args && strcmp(value->ext, args[i].name) != 0;
             ++i);

        if (i < n_args)
        {
            if (test_requirements_clone(&args[i].reqs, reqs) != 0)
                return NULL;

            return args[i].value;
        }
        else
        {
            ERROR("Failed to get argument value by external "
                  "reference '%s'", value->ext);
            return NULL;
        }
    }
    else
    {
        assert(FALSE);
        return NULL;
    }
}

/** Data to be passed as opaque to run_prepare_arg_value_cb() function. */
typedef struct run_prepare_arg_value_cb_data {
    const test_iter_arg    *ctx_args;   /**< Context of arguments */
    unsigned int            ctx_n_args; /**< Number of arguments
                                             in the context */

    const run_item *ri;         /**< Run item context */
    unsigned int    i_value;    /**< Index of the required value */
    unsigned int    i;          /**< Index of the current value */
    test_iter_arg  *arg;        /**< The next argument to be filled in */
} run_prepare_arg_value_cb_data;

/**
 * Function to be called for each singleton value of the run item
 * argument (explicit or inherited) to calculate total number of values.
 *
 * The function complies with test_entity_value_enum_cb prototype.
 */
static te_errno
run_prepare_arg_value_cb(const test_entity_value *value, void *opaque)
{
    run_prepare_arg_value_cb_data  *data = opaque;

    if (data->i < data->i_value)
    {
        data->i++;
        return 0;
    }

    data->arg->value = run_get_value(value, data->ctx_args,
                                     data->ctx_n_args, &data->arg->reqs);
    if (data->arg->value == NULL)
    {
        ERROR("Unable to get value of the argument of the test '%s'",
              test_get_name(data->ri));
        return TE_RC(TE_TESTER, TE_ESRCH);
    }

    return TE_RC(TE_TESTER, TE_EEXIST);
}

/** Information about length of the list */
typedef struct run_prepare_arg_list_data {
    LIST_ENTRY(run_prepare_arg_list_data)   links;  /**< List links */

    const char     *name;   /**< Name of the list */
    unsigned int    index;  /**< Index of the value */
} run_prepare_arg_list_data;

/** Data to be passed as opaque to run_prepare_arg_cb() function. */
typedef struct run_prepare_arg_cb_data {
    const test_iter_arg    *ctx_args;   /**< Context of arguments */
    unsigned int            ctx_n_args; /**< Number of arguments
                                             in the context */

    const run_item *ri;         /**< Run item context */
    unsigned int    n_iters;    /**< Remaining number of iterations */
    unsigned int    i_iter;     /**< Index of the required iteration */
    test_iter_arg  *arg;        /**< The next argument to be filled in */

    LIST_HEAD(, run_prepare_arg_list_data)  lists;  /**< Lists */

} run_prepare_arg_cb_data;

/**
 * Function to be called for each argument (explicit or inherited) to
 * calculate total number of iterations.
 *
 * The function complies with test_var_arg_enum_cb prototype.
 */
static te_errno
run_prepare_arg_cb(const test_var_arg *va, void *opaque)
{
    run_prepare_arg_cb_data        *data = opaque;
    test_var_arg_list              *ri_list = NULL;
    run_prepare_arg_list_data      *iter_list = NULL;
    unsigned int                    n_values;
    unsigned int                    i_value;
    run_prepare_arg_value_cb_data   value_data;
    te_errno                        rc;

    data->arg->name = va->name;

    if (va->list != NULL)
    {
        for (ri_list = data->ri->lists.lh_first;
             ri_list != NULL && strcmp(ri_list->name, va->list) != 0;
             ri_list = ri_list->links.le_next);

        assert(ri_list != NULL);

        for (iter_list = data->lists.lh_first;
             iter_list != NULL && strcmp(iter_list->name, va->list) != 0;
             iter_list = iter_list->links.le_next);
    }

    if (iter_list != NULL)
    {
        i_value = iter_list->index;
        VERB("%s(): Index of the value of '%s' to get is '%u' because of "
             "the list '%s'", __FUNCTION__, va->name, i_value,
             iter_list->name);
    }
    else
    {
        n_values = (ri_list == NULL) ? test_var_arg_values(va)->num :
                                       ri_list->len;
                       
        assert(data->n_iters % n_values == 0);
        data->n_iters /= n_values;

        i_value = data->i_iter / data->n_iters;
        data->i_iter %= data->n_iters;

        if (ri_list != NULL)
        {
            iter_list = malloc(sizeof(*iter_list));
            if (iter_list == NULL)
                return TE_RC(TE_TESTER, TE_ENOMEM);

            iter_list->name = ri_list->name;
            iter_list->index = i_value;
            LIST_INSERT_HEAD(&data->lists, iter_list, links);
        }
        VERB("%s(): Index of the value of '%s' to get is %u",
             __FUNCTION__, va->name, i_value);
    }

    if ((iter_list != NULL) && (i_value >= test_var_arg_values(va)->num))
    {
        if (va->preferred == NULL)
        {
            i_value = 0;
        }
        else
        {
            data->arg->value = run_get_value(va->preferred,
                                             data->ctx_args,
                                             data->ctx_n_args,
                                             &data->arg->reqs);
            if (data->arg->value == NULL)
            {
                ERROR("Unable to get preferred value of the argument "
                      "'%s' of the test '%s'", va->name,
                      test_get_name(data->ri));
                return TE_RC(TE_TESTER, TE_ESRCH);
            }
            goto exit_ok;
        }
    }

    value_data.ctx_args = data->ctx_args;
    value_data.ctx_n_args = data->ctx_n_args;
    value_data.ri = data->ri;
    value_data.i_value = i_value;
    value_data.i = 0;
    value_data.arg = data->arg;

    rc = test_var_arg_enum_values(data->ri, va, run_prepare_arg_value_cb,
                                  &value_data);
    if (rc == 0)
    {
        ERROR("Impossible happened - value by index not found");
        return TE_RC(TE_TESTER, TE_ENOENT);
    }
    else if (TE_RC_GET_ERROR(rc) != TE_EEXIST)
    {
        ERROR("%s(): Unexpected failure %r", __FUNCTION__, rc);
        return rc;
    }

exit_ok:
    /* Move to the next argument */
    data->arg++;

    return 0;
}

/**
 * Prepare arguments to be passed in executable.
 *
 * @param ri            Run item
 * @param i_iter        Index of the iteration to prepare
 * @param args          Location for arguments
 *
 * @return Status code.
 */
static te_errno
run_prepare_args(const test_iter_arg *ctx_args,
                 const unsigned int ctx_n_args,
                 const run_item *ri, unsigned i_iter,
                 test_iter_arg *args)
{
    run_prepare_arg_cb_data     data;
    run_prepare_arg_list_data  *p;
    te_errno                    rc;

    data.ctx_args = ctx_args;
    data.ctx_n_args = ctx_n_args;
    data.ri = ri;
    data.n_iters = ri->n_iters;
    data.i_iter = i_iter;
    data.arg = args;
    LIST_INIT(&data.lists);

    rc = test_run_item_enum_args(ri, run_prepare_arg_cb, &data);
    if (rc != 0 && TE_RC_GET_ERROR(rc) != TE_ENOENT)
    {
        return rc;
    }

    while ((p = data.lists.lh_first) != NULL)
    {
        LIST_REMOVE(p, links);
        free(p);
    }

    return 0;
}

static tester_cfg_walk_ctl
run_iter_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
               unsigned int iter, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    tester_ctx         *parent_ctx;
    te_errno            rc;

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    parent_ctx = ctx->links.le_next;
    if (flags & TESTER_CFG_WALK_SERVICE)
    {
        assert(parent_ctx != NULL);
        parent_ctx = parent_ctx->links.le_next;
    }
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    if (~flags & TESTER_CFG_WALK_SERVICE)
    {
        if (tester_sigint_received)
        {
            ctx->status = TE_RC(TE_TESTER, TE_ETESTKILL);
            return TESTER_CFG_WALK_STOP;
        }

        switch (run_this_item(cfg_id_off, gctx->act_id,
                              ri->weight, 1))
        {
            case TESTING_FORWARD:
                EXIT("SKIP");
                return TESTER_CFG_WALK_SKIP;

            case TESTING_BACKWARD:
                EXIT("BACK");
                return TESTER_CFG_WALK_BACK;

            case TESTING_STOP:
                /* Run here */
                break;

            default:
                assert(FALSE);
                ctx->status = TE_RC(TE_TESTER, TE_EFAULT);
                return TESTER_CFG_WALK_FAULT;
        }
    }

    /* Fill in arguments */
    assert(ri->n_args == ctx->n_args);
    assert((ctx->args == NULL) == (ctx->n_args == 0));
    if (ctx->n_args > 0)
    {
        rc = run_prepare_args(parent_ctx != NULL ? parent_ctx->args : NULL,
                              parent_ctx != NULL ? parent_ctx->n_args : 0,
                              ri, iter, ctx->args);
        if (rc != 0)
        {
            ctx->status = rc;
            return TESTER_CFG_WALK_FAULT; 
        }
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_iter_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
             unsigned int iter, void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    unsigned int        i;

    UNUSED(flags);
    UNUSED(iter);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    /* TODO: Optimize arguments fill in */
    assert(ri->n_args == ctx->n_args);
    for (i = 0; i < ctx->n_args; ++i)
        test_requirements_free(&ctx->args[i].reqs);

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}


static tester_cfg_walk_ctl
run_repeat_start(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
                 void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;

    UNUSED(flags);

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    /* FIXME: Optimize */
    if ((ctx->flags & TESTER_QUIET_SKIP) &&
        !tester_is_run_required(ctx->targets, &ctx->reqs,
                                ri, ctx->args, ctx->flags, TRUE))
    {
        /* Silently skip without any logs */
        ctx->status = TE_RC(TE_TESTER, TE_ENOENT);
        EXIT("SKIP - ENOENT");
        return TESTER_CFG_WALK_SKIP;
    }

    ctx->child_id = tester_get_id();

    /* Test is considered here as run, if such event is logged */
    tester_term_out_start(ri->type, test_get_name(ri),
                          ctx->parent_id, ctx->child_id, ctx->flags);
    log_test_start(ri, ctx->parent_id, ctx->child_id, ctx->args);

    /* FIXME: Optimize */
    if ((~ctx->flags & TESTER_QUIET_SKIP) &&
        !tester_is_run_required(ctx->targets, &ctx->reqs,
                                ri, ctx->args, ctx->flags, FALSE))
    {
        ctx->status = TE_RC(TE_TESTER, TE_ETESTSKIP);
        EXIT("SKIP - ETESTSKIP");
        return TESTER_CFG_WALK_SKIP;
    }
    else
    {
        ctx->backup_ok = FALSE;
    }

    EXIT("CONT");
    return TESTER_CFG_WALK_CONT;
}

static tester_cfg_walk_ctl
run_repeat_end(run_item *ri, unsigned int cfg_id_off, unsigned int flags,
               void *opaque)
{
    tester_run_data    *gctx = opaque;
    tester_ctx         *ctx;
    unsigned int        step;

    assert(gctx != NULL);
    ctx = gctx->ctxs.lh_first;
    assert(ctx != NULL);
    ENTRY("cfg_id_off=%u act=(%d,%d,0x%x) act_id=%u", cfg_id_off,
          gctx->act != NULL ? (int)gctx->act->first : -1,
          gctx->act != NULL ? (int)gctx->act->last : -1,
          gctx->act != NULL ? gctx->act->flags : 0,
          gctx->act_id);

    if (TE_RC_GET_ERROR(ctx->status) != TE_ENOENT)
    {
        run_verify_cfg_backup(ctx, test_get_attrs(ri)->track_conf);

        log_test_result(ctx->parent_id, ctx->child_id, ctx->status);
        tester_term_out_done(ri->type, test_get_name(ri),
                             ctx->parent_id, ctx->child_id, ctx->flags,
                             ctx->status);

        /* Update result of the group */
        ctx->group_status =
            tester_group_status(ctx->group_status, ctx->status);
        if (!TEST_RESULT(ctx->group_status))
        {
            EXIT("FAULT");
            return TESTER_CFG_WALK_FAULT;
        }
    }

    if (~flags & TESTER_CFG_WALK_SERVICE)
    {
        if (tester_sigint_received)
        {
            ctx->status = TE_RC(TE_TESTER, TE_ETESTKILL);
            return TESTER_CFG_WALK_STOP;
        }

        if (ri->type == RUN_ITEM_SCRIPT)
        {
            step = 1;
        }
        else if (TE_RC_GET_ERROR(ctx->status) == TE_ETESTSKIP ||
                 TE_RC_GET_ERROR(ctx->status) == TE_ENOENT ||
                 TE_RC_GET_ERROR(ctx->status) == TE_ETESTPROLOG)
        {
            step = ri->weight;
        }
        else
        {
            step = 0;
        }
        if (scenario_step(&gctx->act, &gctx->act_id, step) == TESTING_STOP)
        {
            /* End of testing scenario */
            EXIT("FIN");
            return TESTER_CFG_WALK_FIN;
        }
        else
        {
            switch (run_this_item(cfg_id_off, gctx->act_id,
                                  ri->weight, 1))
            {
                case TESTING_STOP:
                    EXIT("CONT");
                    return TESTER_CFG_WALK_CONT;

                case TESTING_FORWARD:
                    EXIT("BREAK");
                    return TESTER_CFG_WALK_BREAK;

                case TESTING_BACKWARD:
                    EXIT("BACK");
                    return TESTER_CFG_WALK_BACK;

                default:
                    assert(FALSE);
                    return TESTER_CFG_WALK_FAULT;
            }
        }
    }
    else
    {
        EXIT("BREAK");
        return TESTER_CFG_WALK_BREAK;
    }
}


/* See the description in tester_run.h */
te_errno
tester_run(const testing_scenario *scenario,
           const reqs_expr        *targets,
           const tester_cfgs      *cfgs,
           const unsigned int      flags)
{
    te_errno                rc;
    tester_run_data         data;
    tester_cfg_walk_ctl     ctl;
    const tester_cfg_walk   cbs = {
        run_cfg_start,
        run_cfg_end,
        run_pkg_start,
        NULL,
        run_session_start,
        run_session_end,
        run_prologue_start,
        run_prologue_end,
        run_epilogue_start,
        run_epilogue_end,
        run_keepalive_start,
        run_keepalive_end,
        run_exception_start,
        run_exception_end,
        run_item_start,
        run_item_end,
        run_iter_start,
        run_iter_end,
        run_repeat_start,
        run_repeat_end,
        run_script,
    };

    if (scenario->tqh_first == NULL)
    {
        ERROR("Testing scenario is empty");
        return TE_RC(TE_TESTER, TE_ENOENT);
    }

    memset(&data, 0, sizeof(data));
    data.flags = flags;
    data.targets = targets;
    data.act = scenario->tqh_first;
    data.act_id = data.act->first;

    if (tester_run_new_ctx(&data) == NULL)
        return TE_RC(TE_TESTER, TE_ENOMEM);

    ctl = tester_configs_walk(cfgs, &cbs, 0, &data);
    switch (ctl)
    {
        case TESTER_CFG_WALK_CONT:
            ERROR("Unexpected 'continue' at the end of walk");
            rc = TE_RC(TE_TESTER, TE_EFAULT);
            break;

        case TESTER_CFG_WALK_FIN:
            rc = data.ctxs.lh_first->group_status;
            if (TEST_RESULT(rc))
                rc = 0;
            break;

        case TESTER_CFG_WALK_SKIP:
            ERROR("Unexpected 'skip' at the end of walk");
            rc = TE_RC(TE_TESTER, TE_EFAULT);
            break;

        case TESTER_CFG_WALK_INTR:
            rc = data.ctxs.lh_first->status;
            assert(rc != 0);
            ERROR("Execution of testing scenario interrupted: %r", rc);
            break;

        case TESTER_CFG_WALK_STOP:
            ERROR("Execution of testing scenario interrupted by user");
            rc = TE_RC(TE_TESTER, TE_EINTR);
            break;

        case TESTER_CFG_WALK_FAULT:
            rc = data.ctxs.lh_first->status;
            assert(rc != 0);
            break;

        default:
            assert(FALSE);
            rc = TE_RC(TE_TESTER, TE_EFAULT);
    }

    tester_run_destroy_ctx(&data);

    return rc;
}
