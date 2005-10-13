/** @file 
 * @brief Test Environment: rgt-core implementation.
 *    Implementation of main and usage/help functions.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
/* Define just for passing as a parameter in "free_resources" function */
#define SIGINT (!(0))
#endif

#include <popt.h>
#include <stdio.h>
#include <setjmp.h>

#include "rgt_common.h"
#include "log_msg.h"
#include "log_format.h"
#include "flow_tree.h"
#include "filter.h"
#include "io.h"
#include "memory.h"
#include "live_mode.h"
#include "postponed_mode.h"

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
 * @param  argc   Number of elements in array "argv".
 * @param  argv   Array of strings that represents all command line
 *                arguments.
 *
 * @return  Nothing.
 *
 * @se
 *    The function can update some of the following global variables 
 *    according to the command line arguments specified in argv.
 *    
 *      fltr_file_name   - Name of the TCL filter file.
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
    
    /* Option Table */
    struct poptOption optionsTable[] = {
        { "filter", 'f', POPT_ARG_STRING, NULL, 'f',
          "TCL filter file.", "FILE" },

        { "mode", 'm', POPT_ARG_STRING, NULL, 'm',
          "Mode of operation, can be " 
          RGT_OP_MODE_LIVE_STR " or " RGT_OP_MODE_POSTPONED_STR ". "
          "By default " RGT_OP_MODE_DEFAULT_STR " mode is used.", "MODE" },

        { "no-cntrl-msg", '\0', POPT_ARG_NONE, NULL, 'n',
          "Process TESTER control messages as ordinary: do not process "
          "test flow structure.", NULL },

        { NULL, 'V', POPT_ARG_NONE, NULL, 'V',
          "Verbose trace.", NULL },
          
        { "version", 'v', POPT_ARG_NONE, NULL, 'v', 
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
        if (rc == 'f')
        {
            if ((ctx->fltr_fname = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify TCL filter file", NULL);
            }
        }
        else if (rc == 'm')
        {
            if ((ctx->op_mode_str = poptGetOptArg(optCon)) == NULL ||
                (strcmp(ctx->op_mode_str, RGT_OP_MODE_LIVE_STR) != 0 && 
                 strcmp(ctx->op_mode_str, RGT_OP_MODE_POSTPONED_STR) != 0))
            {
                usage(optCon, 1, "Specify mode of operation", 
                      RGT_OP_MODE_LIVE_STR " or "
                      RGT_OP_MODE_POSTPONED_STR);
            }
            ctx->op_mode = 
                strcmp(ctx->op_mode_str, RGT_OP_MODE_LIVE_STR) == 0 ?
                RGT_OP_MODE_LIVE : RGT_OP_MODE_POSTPONED;
        }
        else if (rc == 'v')
        {
            printf("Package %s: rgt-core version %s\n%s\n", 
                   PACKAGE, VERSION, TE_COPYRIGHT);
            poptFreeContext(optCon);
            exit(0);
        }
        else if (rc == 'n')
        {
            /* User do not want us to process control messages */
            ctx->proc_cntrl_msg = FALSE;
        }
        else if (rc == 'V')
        {
            ctx->verb = TRUE;
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
    if ((ctx->rawlog_fname = poptGetArg(optCon)) == NULL)
    {
        usage(optCon, 1, "Specify RAW log file", NULL);
    }

    /* Try to open Raw log file */
    if ((ctx->rawlog_fd = fopen(ctx->rawlog_fname, "r")) == NULL)
    {
        perror(ctx->rawlog_fname);
        poptFreeContext(optCon);
        exit(1);
    }

    if (ctx->op_mode == RGT_OP_MODE_POSTPONED)
    {
        fseek(ctx->rawlog_fd, 0L, SEEK_END);
        ctx->rawlog_size = ftell(ctx->rawlog_fd);
        fseek(ctx->rawlog_fd, 0L, SEEK_SET);
    }

    ctx->out_fd = stdout;
    if ((ctx->out_fname = poptGetArg(optCon)) != NULL)
    {
        /* Try to open file */
        if ((ctx->out_fd = fopen(ctx->out_fname, "w")) == NULL)
        {
            perror(ctx->out_fname);
            fclose(ctx->rawlog_fd);
            poptFreeContext(optCon);
            exit(1);
        }
    }

    if (poptPeekArg(optCon) != NULL)
    {
        if (ctx->out_fd != stdout)
            unlink(ctx->out_fname);
        usage(optCon, 1, "Too many parameters specified", NULL);
    }

    poptFreeContext(optCon);

    switch (ctx->op_mode)
    {
        case RGT_OP_MODE_LIVE:
            ctx->io_mode = RGT_IO_MODE_BLK;
            live_mode_init(ctrl_msg_proc, &reg_msg_proc);
            break;

        case RGT_OP_MODE_POSTPONED:
            ctx->io_mode = RGT_IO_MODE_NBLK;
            postponed_mode_init(ctrl_msg_proc, &reg_msg_proc);
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
 * arriving SIGINT signal to the programm.
 *
 * @param  signo  Signal number by which the programm is terminated.
 *                If signo is equal to zero it means an error was occured 
 *                during the programm operation. We use an assertion that 
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

    /* Exit 0 in the case of CTRL^C or normal completion */
    exit(!signo);
}

/**
 * Processes command line options.
 * Checks if specified files really exist in file system.
 * Creates TCL interpreter and feeds TCL filter file for it.
 * Processes all messages in order they are placed in RLF.
 *
 * @param argc  Number of arguments passed in command line
 * @param argv  Array of command line argumetns
 * 
 */
int
main(int argc, char **argv)
{
    log_msg       *msg = NULL;
    char          *err_msg;

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
        /* Log message processing loop */
        while (1)
        {
            rgt_update_progress_bar(&rgt_ctx);

            if (rgt_ctx.fetch_log_msg(&msg, &rgt_ctx) == 0)
                break;
            
            if (!rgt_ctx.proc_cntrl_msg)
            {
                /* We do not need to care about Log ID */
                msg->id = TE_LOG_ID_UNDEFINED;
            }

            rgt_core_process_log_msg(msg);
        }

        if (rgt_ctx.op_mode == RGT_OP_MODE_POSTPONED)
        {
            /* Process flow tree (call callback routines for each node) */
            postponed_process_open();
            flow_tree_trace();
            postponed_process_close();
        }

        /* Successful competion */
        free_resources(SIGINT);
    }
    else
    {
        free_resources(0);
    }

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
    ctx->proc_cntrl_msg = TRUE;
    ctx->verb = FALSE;
}

/**
 * Output progress bar processing status
 *
 * @param ctx  Rgt utility context
 */
static void
rgt_update_progress_bar(rgt_gen_ctx_t *ctx)
{
    long offset;

    if (ctx->op_mode != RGT_OP_MODE_POSTPONED || !ctx->verb)
        return;

    offset = ftell(ctx->rawlog_fd);
    fprintf(stderr, "\r%ld%%",
            (long)(((long long)offset * 100L) / ctx->rawlog_size));
}
