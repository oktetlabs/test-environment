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

/** Glabal variables */

/** TCL filter file name */
static const char *fltr_file_name = NULL;
/** Raw log file name */
static const char *raw_file_name = NULL;
/** Output file name */
static const char *output_file_name = NULL;

/** Raw log and output file descriptors */
FILE *output_fd = NULL;
FILE *rawlog_fd = NULL;

/** Rgt operation mode in string and numerical representations */
enum e_rgt_op_mode  rgt_op_mode     = RGT_OP_MODE_DEFAULT;
const char         *rgt_op_mode_str = RGT_OP_MODE_DEFAULT_STR;

/** Whether Rgt should process control messages or not */
int                 process_control_msg = FALSE; /* @todo Change to TRUE!*/

/**
 * Operation mode (live or postponed) influences on desirable read behaviour
 * that can be blocking or nonblocking.
 * This global variable contains current rgt reading mode.
 */
enum read_mode      rgt_rmode;

/**
 * rgt_fetch_log_msg is a pointer to a function that should be used for 
 * extraction log messages from a raw log file.
 * Actually this variable is set to an appropriate function according to
 * RLF version determined from the first byte of the RLF.
 */
static f_fetch_log_msg rgt_fetch_log_msg;

/** 
 * The stack context of the main procedure.
 * It is used for exception generations
 */
jmp_buf rgt_mainjmp;


/**
 * Print "usage" how to.
 *
 * @param  optCon    Context for parsing command line arguments.
 * @param  exitcode  Code that is passed to the "exit" call.
 * @param  error     Error message string.
 * @param  addl      Additional notes that is output.
 *
 * @return  Nothing.
 *
 * @se  Frees popt Context (specified in optCon) and exits with specified code.
 */
static void
usage(poptContext optCon, int exitcode, char *error, char *addl)
{
    poptSetOtherOptionHelp(optCon, 
        "<filter file> <raw log file> [<output file>]");
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
 * @param  argv   Array of strings that represents all command line arguments.
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
 *      rgt_op_mode      - The mode of the rgt operation in numerical format.
 *      rgt_rmode        - Read mode that should be used in READ calls.
 *
 *    In the case of an error it calls exit() function with code 1.
 */
static void
process_cmd_line_opts(int argc, char **argv)
{
    poptContext  optCon; /* context for parsing command-line options */
    int          rc;
    
    /* Option Table */
    struct poptOption optionsTable[] = {
        { "mode", 'm', POPT_ARG_STRING, NULL, 'm',
          "Mode of operation, can be " 
          RGT_OP_MODE_LIVE_STR " or " RGT_OP_MODE_POSTPONED_STR ". "
          "By default " RGT_OP_MODE_DEFAULT_STR " mode is used.", "MODE" },

        { "no-cntrl-msg", '\0', POPT_ARG_NONE, NULL, 'n',
          "Process TESTER control messages as ordinary: do not process "
          "test flow structure.", NULL },
          
        { "version", 'v', POPT_ARG_NONE, NULL, 'v', 
          "Display version information.", 0 },
          
        POPT_AUTOHELP
        
        { NULL, 0, 0, NULL, 0, NULL, 0 },
    };
    
    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv, optionsTable, 0);
  
    poptSetOtherOptionHelp(optCon, 
        "[OPTION...] <filter file> [<raw log file>] [<output file>]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 'm')
        {
            if ((rgt_op_mode_str = poptGetOptArg(optCon)) == NULL ||
                (strcmp(rgt_op_mode_str, RGT_OP_MODE_LIVE_STR) != 0 && 
                 strcmp(rgt_op_mode_str, RGT_OP_MODE_POSTPONED_STR) != 0))
            {
                usage(optCon, 1, "Specify mode of operation", 
                      RGT_OP_MODE_LIVE_STR " or " RGT_OP_MODE_POSTPONED_STR);
            }
            rgt_op_mode = 
                strcmp(rgt_op_mode_str, RGT_OP_MODE_LIVE_STR) == 0 ?
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
            process_control_msg = TRUE; /* @todo Change to FALSE!*/
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

    /* Get <filter file> name */
    if ((fltr_file_name = poptGetArg(optCon)) == NULL)
    {
        usage(optCon, 1, "Specify TCL filter file", NULL);
    }
    
    /* Get <raw log file> names */
    if ((raw_file_name = poptGetArg(optCon)) == NULL)
    {
        rawlog_fd = stdin;
    }
    else
    {
        /* Try to open Raw log file */
        if ((rawlog_fd = fopen(raw_file_name, "r")) == NULL)
        {
            perror(raw_file_name);
            poptFreeContext(optCon);
            exit(1);
        }
    }
    
    if ((output_file_name = poptGetArg(optCon)) == NULL)
    {
        output_fd = stdout;
    }
    else
    {
        /* Try to open file */
        if ((output_fd = fopen(output_file_name, "w")) == NULL)
        {
            perror(output_file_name);
            fclose(rawlog_fd);
            poptFreeContext(optCon);
            exit(1);
        }
    }

    if (poptPeekArg(optCon) != NULL)
    {
        if (output_fd != stdout)
            unlink(output_file_name);
        usage(optCon, 1, "Too many parameters specified", NULL);
    }

    poptFreeContext(optCon);

    if (rgt_op_mode == RGT_OP_MODE_LIVE)
    {
        rgt_rmode = RMODE_BLOCKING;
        live_mode_init(ctrl_msg_proc, &reg_msg_proc);
    }
    else if (rgt_op_mode == RGT_OP_MODE_POSTPONED)
    {
        rgt_rmode = RMODE_NONBLOCKING;
        postponed_mode_init(ctrl_msg_proc, &reg_msg_proc);
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
    fclose(rawlog_fd);
    fclose(output_fd);

    if (signo == 0)
    {
        unlink(output_file_name);
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
    log_msg *msg = NULL;
    char    *err_msg;

    process_cmd_line_opts(argc, argv);

#ifdef HAVE_SIGNAL_H
    /* Set signal handler for catching CTRL^C interruption */
    signal(SIGINT, free_resources);
#endif

    if (rgt_filter_init(fltr_file_name) < 0)
    {
        /* This function never returns */
        free_resources(0);
    }

    /* Determine version of the format of raw log file */
    if ((rgt_fetch_log_msg = rgt_define_rlf_format(rawlog_fd, 
                                                   &err_msg)) == NULL)
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
            if (rgt_fetch_log_msg(&msg, rawlog_fd) == 0)
                break;

            rgt_core_process_log_msg(msg);
        }

        if (rgt_op_mode == RGT_OP_MODE_POSTPONED)
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
 * @param  msg   Pointer to message to be processed.
 *
 * @return  Nothing.
 */
static void
rgt_core_process_log_msg(log_msg *msg)
{
    /* 
     * Check if it is a control message.
     * Control messages come from Tester and have well-known pair of 
     * Entity and User names.
     */
    if (strcmp(msg->entity, CMSG_ENTITY_NAME) == 0 &&
        strcmp(msg->user, CMSG_USER_NAME) == 0 &&
        process_control_msg)
    {
        rgt_process_control_message(msg);
    }
    else
    {
        rgt_process_regular_message(msg);
    }
}
