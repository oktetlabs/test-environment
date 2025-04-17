/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test Environment: rgt-core implementation.
 *
 * Implementation of main and usage/help functions.
 */

#include "rgt_common.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
/* Define just for passing as a parameter in "free_resources" function */
#define SIGINT (!(0))
#endif

#include <popt.h>
#include <stdio.h>
#include <setjmp.h>

#include "log_msg.h"
#include "log_format.h"
#include "flow_tree.h"
#include "filter.h"
#include "io.h"
#include "memory.h"
#include "live_mode.h"
#include "postponed_mode.h"
#include "index_mode.h"
#include "junit_mode.h"

/*
 * Define PACKAGE, VERSION and TE_COPYRIGHT just for the case it's build
 * with an ugly method.
 */
#ifndef PACKAGE
#define PACKAGE "rgt"
#endif

#ifndef VERSION
#define VERSION ""
#endif

#ifndef TE_COPYRIGHT
#define TE_COPYRIGHT ""
#endif

static void rgt_core_process_log_msg(log_msg *msg);
static void rgt_ctx_set_defaults(rgt_gen_ctx_t *ctx);
static void rgt_update_progress_bar(rgt_gen_ctx_t *ctx);

/** Global RGT context */
rgt_gen_ctx_t rgt_ctx;

/**
 * The stack context of the main procedure.
 * It is used for exception generations
 */
jmp_buf rgt_mainjmp;


/**
 * Print "usage" how to.
 *
 * @param optCon    Context for parsing command line arguments.
 * @param exitcode  Code that is passed to the "exit" call.
 * @param error     Error message string.
 * @param addl      Additional notes that is output.
 *
 * @se Frees popt Context (specified in optCon) and exits with specified
 *     code.
 */
static void
usage(poptContext optCon, int exitcode, char *error, char *addl)
{
    poptSetOtherOptionHelp(optCon,
        "<raw log file> [<output file>]");
    poptPrintUsage(optCon, stderr, 0);
    if (error)
    {
        fprintf(stderr, "%s", error);
        if (addl != NULL)
            fprintf(stderr, ": %s", addl);
        fprintf(stderr, "\n");
    }

    poptFreeContext(optCon);

    exit(exitcode);
}

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated if some new
 * options are going to be added.
 *
 * @param[in]       argc            Number of elements in array "argv".
 * @param[in]       argv            Array of strings that represents all
                                    command line arguments.
 * @param[in,out]   ctx             Context to setup.
 *
 * @return  Nothing.
 *
 * @se
 *    The function can update some of the following global variables
 *    according to the command line arguments specified in argv.
 *
 *      fltr_file_name   - Name of the XML filter file.
 *      raw_file_name    - Name of the Raw log file.
 *      output_file_name - Name of the output file.
 *      rgt_op_mode_str  - The mode of the rgt operation in string format.
 *      rgt_op_mode      - The mode of the rgt operation in numerical
 *                         format.
 *      rgt_rmode        - Read mode that should be used in READ calls.
 *
 *    In the case of an error it calls exit() function with code 1.
 */
static void
process_cmd_line_opts(int argc, char **argv, rgt_gen_ctx_t *ctx)
{
    poptContext  optCon; /* context for parsing command-line options */
    int          rc;

    const char *rawlog_fname = NULL;
    const char *out_fname = NULL;

    enum {
        RGT_OPT_FILTER = 1,
        RGT_OPT_MODE,
        RGT_OPT_NO_CNTRL_MSG,
        RGT_OPT_MI_META,
        RGT_OPT_INCOMPLETE_LOG,
        RGT_OPT_TMPDIR,
        RGT_OPT_STOP_AT_ENTITY,
        RGT_OPT_VERBOSE,
        RGT_OPT_VERSION,
    };

    /* Option Table */
    struct poptOption optionsTable[] = {
        { "filter", 'f', POPT_ARG_STRING, NULL, RGT_OPT_FILTER,
          "XML filter file.", "FILE" },

        { "mode", 'm', POPT_ARG_STRING, NULL, RGT_OPT_MODE,
          "Mode of operation, can be "
          RGT_OP_MODE_LIVE_STR ", " RGT_OP_MODE_POSTPONED_STR
          ", " RGT_OP_MODE_INDEX_STR " or " RGT_OP_MODE_JUNIT_STR ". "
          "By default " RGT_OP_MODE_DEFAULT_STR " mode is used.", "MODE" },

        { "no-cntrl-msg", '\0', POPT_ARG_NONE, NULL, RGT_OPT_NO_CNTRL_MSG,
          "Process TESTER control messages as ordinary: do not process "
          "test flow structure.", NULL },

        { "mi-meta", '\0', POPT_ARG_NONE, NULL, RGT_OPT_MI_META,
          "Include MI artifacts in <meta> section of XML log",
          NULL },

        { "incomplete-log", '\0', POPT_ARG_NONE, NULL,
          RGT_OPT_INCOMPLETE_LOG,
          "Do not shout on truncated log report, but complete it "
          "automatically.", NULL },

        { "tmpdir", 't', POPT_ARG_STRING, NULL, RGT_OPT_TMPDIR,
          "Temporary directory for message queues offloading.", "PATH" },

        { "stop-at-entity", '\0', POPT_ARG_STRING, NULL,
          RGT_OPT_STOP_AT_ENTITY,
          "Stop processing at the first message with a given entity.",
          "ENTITY" },

        { NULL, 'V', POPT_ARG_NONE, NULL, RGT_OPT_VERBOSE,
          "Verbose trace.", NULL },

        { "version", 'v', POPT_ARG_NONE, NULL, RGT_OPT_VERSION,
          "Display version information.", 0 },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv, optionsTable,
                            0);

    poptSetOtherOptionHelp(optCon,
        "[OPTION...] [<raw log file>] [<output file>]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        switch (rc)
        {
            case RGT_OPT_FILTER:
                if ((ctx->fltr_fname = poptGetOptArg(optCon)) == NULL)
                {
                    usage(optCon, 1, "Specify XML filter file", NULL);
                }
                break;

            case RGT_OPT_TMPDIR:
                if ((ctx->tmp_dir = poptGetOptArg(optCon)) == NULL)
                {
                    usage(optCon, 1, "Specify temporary directory path",
                          NULL);
                }
                break;

            case RGT_OPT_MODE:
                if ((ctx->op_mode_str = poptGetOptArg(optCon)) == NULL ||
                    (strcmp(ctx->op_mode_str,
                            RGT_OP_MODE_LIVE_STR) != 0 &&
                     strcmp(ctx->op_mode_str,
                            RGT_OP_MODE_POSTPONED_STR) != 0 &&
                     strcmp(ctx->op_mode_str,
                            RGT_OP_MODE_INDEX_STR) != 0 &&
                     strcmp(ctx->op_mode_str,
                            RGT_OP_MODE_JUNIT_STR) != 0))
                {
                    usage(optCon, 1, "Specify mode of operation",
                          RGT_OP_MODE_LIVE_STR ", "
                          RGT_OP_MODE_POSTPONED_STR ", "
                          RGT_OP_MODE_INDEX_STR ", "
                          RGT_OP_MODE_JUNIT_STR);
                }

                if (strcmp(ctx->op_mode_str, RGT_OP_MODE_LIVE_STR) == 0)
                    ctx->op_mode = RGT_OP_MODE_LIVE;
                else if (strcmp(ctx->op_mode_str,
                                RGT_OP_MODE_POSTPONED_STR) == 0)
                    ctx->op_mode = RGT_OP_MODE_POSTPONED;
                else if (strcmp(ctx->op_mode_str,
                                RGT_OP_MODE_INDEX_STR) == 0)
                    ctx->op_mode = RGT_OP_MODE_INDEX;
                else
                    ctx->op_mode = RGT_OP_MODE_JUNIT;

                break;

            case RGT_OPT_VERSION:
                printf("Package %s: rgt-core version %s\n%s\n",
                       PACKAGE, VERSION, TE_COPYRIGHT);
                poptFreeContext(optCon);
                exit(0);
                break;

            case RGT_OPT_NO_CNTRL_MSG:
                /* User do not want us to process control messages */
                ctx->proc_cntrl_msg = false;
                break;

            case RGT_OPT_MI_META:
                ctx->mi_meta = true;
                break;

            case RGT_OPT_INCOMPLETE_LOG:
                /* User ask us to complete log report automatically */
                ctx->proc_incomplete = true;
                break;

            case RGT_OPT_STOP_AT_ENTITY:
                if ((ctx->stop_at_entity = poptGetOptArg(optCon)) == NULL)
                    usage(optCon, 1, "Specify log entity", NULL);

                break;

            case RGT_OPT_VERBOSE:
                ctx->verb = true;
                break;

            default:
                assert(0);
                break;
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        exit(1);
    }

    /* Get <raw log file> name */
    if ((rawlog_fname = poptGetArg(optCon)) == NULL)
    {
        usage(optCon, 1, "Specify RAW log file", NULL);
    }

    /* Try to open Raw log file */
    if ((ctx->rawlog_fd = fopen(rawlog_fname, "r")) == NULL)
    {
        perror(rawlog_fname);
        poptFreeContext(optCon);
        exit(1);
    }

    if (ctx->op_mode != RGT_OP_MODE_LIVE)
    {
        fseeko(ctx->rawlog_fd, 0LL, SEEK_END);
        ctx->rawlog_size = ftello(ctx->rawlog_fd);
        fseeko(ctx->rawlog_fd, 0LL, SEEK_SET);
    }

    ctx->out_fd = stdout;
    if ((out_fname = poptGetArg(optCon)) != NULL)
    {
        /* Try to open file */
        if ((ctx->out_fd = fopen(out_fname, "w")) == NULL)
        {
            perror(out_fname);
            fclose(ctx->rawlog_fd);
            poptFreeContext(optCon);
            exit(1);
        }
    }

    if (poptPeekArg(optCon) != NULL)
    {
        if (ctx->out_fd != stdout)
            unlink(out_fname);
        usage(optCon, 1, "Too many parameters specified", NULL);
    }

    /*
     * poptGetArg() returns an internal pointer that
     * becomes invalid after calling poptFreeContext().
     */
    ctx->rawlog_fname = strdup(rawlog_fname);

    if (out_fname != NULL)
        ctx->out_fname = strdup(out_fname);

    poptFreeContext(optCon);

    switch (ctx->op_mode)
    {
        case RGT_OP_MODE_LIVE:
            ctx->io_mode = RGT_IO_MODE_BLK;
            live_mode_init(ctrl_msg_proc, &reg_msg_proc,
                           log_root_proc);
            break;

        case RGT_OP_MODE_POSTPONED:
            ctx->io_mode = RGT_IO_MODE_NBLK;
            postponed_mode_init(ctrl_msg_proc, &reg_msg_proc,
                                log_root_proc);
            break;

        case RGT_OP_MODE_INDEX:
            ctx->io_mode = RGT_IO_MODE_NBLK;
            index_mode_init(ctrl_msg_proc, &reg_msg_proc,
                            log_root_proc);
            break;

        case RGT_OP_MODE_JUNIT:
            ctx->io_mode = RGT_IO_MODE_NBLK;
            junit_mode_init(ctrl_msg_proc, &reg_msg_proc,
                            log_root_proc);
            break;

        default:
            assert(0);
    }
}

/**
 * Frees all global resources are used by rgt-core:
 *   Destroys flow tree module;
 *   Destroys filter module;
 *   Closes all file descriptors.
 * This function can be called explicitly or as a callback function on
 * arriving SIGINT signal to the program.
 *
 * @param  signo  Signal number by which the program is terminated.
 *                If signo is equal to zero it means an error was occurred
 *                during the program operation. We use an assertion that
 *                no one signal has code equal to zero.
 *
 * @return  Nothing.
 */
static void free_resources(int signo)
{
    /* log_parser_free_resources(); */
    flow_tree_destroy();
    rgt_filter_destroy();
    destroy_node_info_pool();
    destroy_log_msg_pool();
    fclose(rgt_ctx.rawlog_fd);
    fclose(rgt_ctx.out_fd);

    if (signo == 0)
    {
        unlink(rgt_ctx.out_fname);
    }

    free(rgt_ctx.tmp_dir);

    /* Exit 0 in the case of CTRL^C or normal completion */
    exit(!signo);
}

/**
 * Processes command line options.
 * Checks if specified files really exist in file system.
 * Parses XML filter file.
 * Processes all messages in order they are placed in RLF.
 *
 * @param argc  Number of arguments passed in command line
 * @param argv  Array of command line arguments
 *
 */
int
main(int argc, char **argv)
{
    log_msg       *msg = NULL;
    char          *err_msg;
    uint32_t       latest_ts[2] = { 0, 0 };

    rgt_ctx_set_defaults(&rgt_ctx);
    process_cmd_line_opts(argc, argv, &rgt_ctx);

#ifdef HAVE_SIGNAL_H
    /* Set signal handler for catching CTRL^C interruption */
    signal(SIGINT, free_resources);
#endif

    if (rgt_filter_init(rgt_ctx.fltr_fname) < 0)
    {
        /* This function never returns */
        free_resources(0);
    }

    /* Determine version of the format of raw log file */
    rgt_ctx.fetch_log_msg = rgt_define_rlf_format(&rgt_ctx, &err_msg);
    if (rgt_ctx.fetch_log_msg == NULL)
    {
        fprintf(stderr, "%s", err_msg);
        free_resources(0);
    }

    /* Initialize internal data structures in flow tree module */
    flow_tree_init();
    initialize_node_info_pool();
    initialize_log_msg_pool();

    if (setjmp(rgt_mainjmp) == 0)
    {
        if (log_root_proc[CTRL_EVT_START] != NULL)
            log_root_proc[CTRL_EVT_START]();

        /* Log message processing loop */
        while (1)
        {
            rgt_update_progress_bar(&rgt_ctx);

            if (rgt_ctx.fetch_log_msg(&msg, &rgt_ctx) == 0)
            {
                if (rgt_ctx.op_mode != RGT_OP_MODE_LIVE)
                    break;

                fclose(rgt_ctx.rawlog_fd);
                rgt_ctx.rawlog_fd = NULL;

                rgt_ctx.rawlog_fd = fopen(rgt_ctx.rawlog_fname, "r");
                if (rgt_ctx.rawlog_fd == NULL)
                {
                    fprintf(stderr, "Can not open new tmp_raw_log file");
                    free_resources(0);
                }
                else
                {
                    rgt_ctx.fetch_log_msg = rgt_define_rlf_format(&rgt_ctx, &err_msg);
                    if (rgt_ctx.fetch_log_msg == NULL)
                    {
                        fprintf(stderr, "%s", err_msg);
                        free_resources(0);
                    }

                    continue;
                }
            }


            if (!rgt_ctx.proc_cntrl_msg)
            {
                /* We do not need to care about Log ID */
                msg->id = TE_LOG_ID_UNDEFINED;
            }

            /* Update the latest timestamp value when needed */
            if (rgt_ctx.proc_incomplete &&
                TIMESTAMP_CMP(latest_ts, msg->timestamp) < 0)
            {
                memcpy(&latest_ts, &(msg->timestamp), sizeof(latest_ts));
            }

            rgt_core_process_log_msg(msg);

            if (rgt_ctx.stop_at_entity != NULL && msg->entity != NULL &&
                strcmp(msg->entity, rgt_ctx.stop_at_entity) == 0)
            {
                break;
            }
        }

        if (rgt_ctx.op_mode == RGT_OP_MODE_POSTPONED ||
            rgt_ctx.op_mode == RGT_OP_MODE_JUNIT)
        {
            if (rgt_ctx.proc_incomplete)
                rgt_emulate_accurate_close(latest_ts);

            /* Process flow tree (call callback routines for each node) */
            flow_tree_trace();
        }

        if (log_root_proc[CTRL_EVT_END] != NULL)
            log_root_proc[CTRL_EVT_END]();

        /* Successful completion */
        free_resources(SIGINT);
    }
    else
    {
        free_resources(0);
    }

    free(rgt_ctx.rawlog_fname);
    free(rgt_ctx.out_fname);

    return 0;
}

/**
 * Verifies if a message is control or regular one and calls appropriate
 * message processing function.
 *
 * @param msg  Pointer to message to be processed
 *
 * @return  Nothing.
 */
static void
rgt_core_process_log_msg(log_msg *msg)
{
    /*
     * Check if it is a control message.
     * Control messages have well-known User name.
     */
    if (rgt_ctx.proc_cntrl_msg &&
        strcmp(msg->user, TE_LOG_CMSG_USER) == 0 &&
        strcmp(msg->entity, TE_LOG_CMSG_ENTITY_TESTER) == 0)
    {
        rgt_process_tester_control_message(msg);
    }
    else
    {
        rgt_process_regular_message(msg);
    }
}

/**
 * Set default values into rgt context data structure
 *
 * @param ctx  context to be updated
 */
static void
rgt_ctx_set_defaults(rgt_gen_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->op_mode = RGT_OP_MODE_DEFAULT;
    ctx->op_mode_str = RGT_OP_MODE_DEFAULT_STR;
    ctx->stop_at_entity = NULL;
    ctx->proc_cntrl_msg = true;
    ctx->mi_meta = false;
    ctx->proc_incomplete = false;
    ctx->verb = false;
    ctx->tmp_dir = NULL;
    ctx->current_nest_lvl = 0;
}

/**
 * Output progress bar processing status
 *
 * @param ctx  Rgt utility context
 */
static void
rgt_update_progress_bar(rgt_gen_ctx_t *ctx)
{
    off_t offset;

    if (ctx->op_mode == RGT_OP_MODE_LIVE || !ctx->verb)
        return;

    offset = ftello(ctx->rawlog_fd);
    fprintf(stderr, "\r%ld%%",
            (long)(((long long)offset * 100L) / ctx->rawlog_size));
}

