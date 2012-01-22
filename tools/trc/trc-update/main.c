/** @file
 * @brief Testing Results Comparator
 *
 * TRC update tool
 *  
 *
 * Copyright (C) 2011 Test Environment authors (see file AUTHORS
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

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for TRC tool
#endif

#include "te_defs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "te_trc.h"
#include "trc_tools.h"
#include "log_parse.h"
#include "re_subst.h"
#include "logic_expr.h"
#include "te_string.h"
#include "tq_string.h"

#include "trc_report.h"
#include "trc_update.h"

#ifdef HAVE_LIBPERL
#define PERL_NO_GET_CONTEXT
#include <EXTERN.h>
#include <perl.h>
#endif

#include <unistd.h>

DEFINE_LGR_ENTITY("TRC UPD");

/** TRC update tool command line options */
enum {
    TRC_UPDATE_OPT_VERSION = 1,     /**< Display version information */
    TRC_UPDATE_OPT_CMD,             /**< Used to pass command with which
                                         tool was actually runned */
    TRC_UPDATE_OPT_DB,              /**< Specify TRC DB path */
    TRC_UPDATE_OPT_TEST_NAME,       /**< Specify full name of the test */
    TRC_UPDATE_OPT_TAGS,            /**< Specify tags expression */
    TRC_UPDATE_OPT_LOG,             /**< Tester run log in XML format */
    TRC_UPDATE_OPT_RULES,           /**< Path to file with updating
                                         rules to be applied */
    TRC_UPDATE_OPT_RULES_SAVE,      /**< Where to save generated updating
                                         rules */
    TRC_UPDATE_OPT_RULES_ALL,       /**< Generate updating rules for old
                                         iterations in TRC (including
                                         those for which there is no
                                         conflicting results in logs) */
    TRC_UPDATE_OPT_NO_USE_IDS,      /**< Do not set user_attr attribute
                                         to updating rule ID in
                                         generated TRC XML */
    TRC_UPDATE_OPT_COPY_OLD,        /**< Duplicate old results in <news>
                                         section of updating rule */
    TRC_UPDATE_OPT_COPY_CONFLS,     /**< Duplicate conflicting results in
                                         <news> section of updating rule */
    TRC_UPDATE_OPT_NO_WILDS,        /**< Do not generate wildcards */
    TRC_UPDATE_OPT_LOG_WILDS,       /**< Generate wildcards for results
                                         from logs, not from TRC DB */
    TRC_UPDATE_OPT_FAKE_LOG,        /**< Fake Tester run log */
    TRC_UPDATE_OPT_OLD_MATCH_EXPR,  /**< Expression to match iterations
                                         in TRC with iterations in logs */
    TRC_UPDATE_OPT_OLD_MATCH_PERL,  /**< Path to iterations matching perl
                                         script */
    TRC_UPDATE_OPT_OLD_MATCH_OTH,   /**< Path to iteration matching
                                         program */
    TRC_UPDATE_OPT_TRC_SAVE,        /**< Path to file where resulting TRC
                                         should be saved */
};

/** Name of the file with expected testing result database */
static char *db_fn = NULL;

/** Name of the file in which resulting TRC should be saved */
static char *trc_save_to = NULL;

/** Expression to match iterations in TRC with iterations in logs */
static char *perl_expr = NULL;
/** Perl script to match iterations in TRC with iterations in logs */
static char *perl_script = NULL;
/** Program script to match iterations in TRC with iterations in logs */
static char *oth_prog = NULL;

/** TRC update context */
static trc_update_ctx ctx;

#ifdef HAVE_LIBPERL
/** Perl interpreter */
static PerlInterpreter *perl_interp;
#endif

/**
 * Add new group of logs.
 *
 * @param tags_str  Tag expression
 *
 * @return
 *      Pointer to structure describing group of logs or NULL
 */
static trc_update_tag_logs *add_new_tag_logs(char *tags_str)
{
    trc_update_tag_logs     *tag_logs;

    tag_logs = TE_ALLOC(sizeof(*tag_logs));

    if (tag_logs == NULL)
    {
        ERROR("Failed to allocate memory");
        return NULL;
    }

    tag_logs_init(tag_logs);

    tag_logs->tags_str = tags_str;
    if (logic_expr_parse(tag_logs->tags_str,
                         &tag_logs->tags_expr) != 0)
    {
        ERROR("Incorrect tag expression: %s",
              tag_logs->tags_str);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&ctx.tags_logs, tag_logs, links);

    return tag_logs;
}

/**
 * Process command line options and parameters specified in argv.
 * The procedure contains "Option table" that should be updated 
 * if so me new options are going to be added.
 *
 * @param argc  Number of elements in array "argv".
 * @param argv  Array of strings that represents all command line arguments.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
static int
trc_update_process_cmd_line_opts(int argc, char **argv)
{
    int             result = EXIT_FAILURE;
    poptContext     optCon;
    int             opt;

    trc_update_tag_logs     *tag_logs;
    tqe_string              *tqe_str;

    char        *s;
    te_bool      no_use_ids = FALSE;

    struct poptOption options_table[] = {
        { "test-name", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_TEST_NAME,
          "Specify full name of the test", NULL },

        { "old-match-expr", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_OLD_MATCH_EXPR,
          "Specify expression to match old iterations from TRC with "
          "new ones (it makes sense when set of parameters was changed)",
          NULL },

        { "old-match-perl", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_OLD_MATCH_PERL,
          "Specify path to a perl script matching old iterations from "
          "TRC with new ones", NULL },

        { "old-match-prog", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_OLD_MATCH_OTH,
          "Specify path to a program matching old iterations from TRC "
          "with new ones", NULL },

        { "rules", 'r', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_RULES,
          "Specify updating rules file in XML format", NULL },

        { "rules-save", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_RULES_SAVE,
          "Specify where to save updating rules for editing", NULL },

        { "rules-all", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_RULES_ALL,
          "Create updating rules for all results (not only for those "
          "which are to be merged with new ones)", NULL },

        { "no-use-ids", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_USE_IDS,
          "Do not use id attribute of rules and user_attr attribute "
          "of test iterations to match rules and iterations in "
          "\"raw\" TRC generated by this tool", NULL },

        { "copy-old", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_COPY_OLD,
          "Copy results from existing TRC DB into <news> section  "
          "of updating rule", NULL },

        { "copy-confls", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_COPY_CONFLS,
          "Copy conflicting results from logs into <news> section  "
          "of updating rule", NULL },

        { "no-wilds", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_NO_WILDS,
          "Do not generate wildcards in resulting TRC",
          NULL },

        { "log-wilds", '\0', POPT_ARG_NONE, NULL,
          TRC_UPDATE_OPT_LOG_WILDS,
          "Generate wildcards for results from logs, not from TRC DB",
          NULL },

        { "log", 'l', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_LOG,
          "Specify log file", NULL },

        { "tags", 't', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_TAGS,
          "Specify tag expression", NULL },

        { "fake-log", '\0', POPT_ARG_STRING | POPT_ARGFLAG_DOC_HIDDEN,
          NULL, TRC_UPDATE_OPT_FAKE_LOG,
          "Specify log file of fake Tester run in XML format", NULL },

        { "trc-save", '\0', POPT_ARG_STRING, NULL,
          TRC_UPDATE_OPT_TRC_SAVE,
          "Specify file to save resulting TRC", NULL },

        { "cmd", '\0', POPT_ARG_STRING | POPT_ARGFLAG_DOC_HIDDEN,
          NULL, TRC_UPDATE_OPT_CMD, NULL, NULL },

        { "db", 'd', POPT_ARG_STRING, NULL, TRC_UPDATE_OPT_DB,
          "Specify name of the file with expected testing results "
          "database.",
          "FILENAME" },

        { "version", '\0', POPT_ARG_NONE, NULL, TRC_UPDATE_OPT_VERSION, 
          "Display version information.", NULL },

        POPT_AUTOHELP
        POPT_TABLEEND
    };

    optCon = poptGetContext(NULL, argc, (const char **)argv,
                            options_table, 0);

    while ((opt = poptGetNextOpt(optCon)) >= 0)
    {
        switch (opt)
        {
            case TRC_UPDATE_OPT_TEST_NAME:
                tqe_str = TE_ALLOC(sizeof(tqe_string));
                tqe_str->v = (char *)poptGetOptArg(optCon);
                s = strchr(tqe_str->v, ':');
                if (s != NULL)
                    *s = '\0';
                s = strchr(tqe_str->v, '%');
                if (s != NULL)
                    *s = '\0';
                s = tqe_str->v + strlen(tqe_str->v) - 1;
                if (*s == '/')
                    *s = '\0';
                TAILQ_INSERT_TAIL(&ctx.test_names, tqe_str, links);
                break;

            case TRC_UPDATE_OPT_TAGS:
                tag_logs = add_new_tag_logs(poptGetOptArg(optCon));
                if (tag_logs == NULL)
                    goto exit;
                break;

            case TRC_UPDATE_OPT_LOG:
                if (TAILQ_EMPTY(&ctx.tags_logs))
                {
                    tag_logs = add_new_tag_logs(strdup("UNSPEC"));
                    if (tag_logs == NULL)
                        goto exit;
                }
                else
                    tag_logs = TAILQ_LAST(&ctx.tags_logs,
                                          trc_update_tags_logs);

                tqe_str = malloc(sizeof(tqe_string));
                tqe_str->v = poptGetOptArg(optCon);

                TAILQ_INSERT_TAIL(&tag_logs->logs, tqe_str, links);
                break;

            case TRC_UPDATE_OPT_FAKE_LOG:
                ctx.fake_log = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_OLD_MATCH_EXPR:
                perl_expr = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_OLD_MATCH_PERL:
                perl_script = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_OLD_MATCH_OTH:
                oth_prog = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_RULES:
                ctx.rules_load_from = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_RULES_SAVE:
                ctx.rules_save_to = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_COPY_OLD:
                ctx.flags |= TRC_LOG_PARSE_COPY_OLD;
                break;

            case TRC_UPDATE_OPT_COPY_CONFLS:
                ctx.flags |= TRC_LOG_PARSE_COPY_CONFLS;
                break;

            case TRC_UPDATE_OPT_RULES_ALL:
                ctx.flags |= TRC_LOG_PARSE_RULES_ALL;
                break;

            case TRC_UPDATE_OPT_NO_USE_IDS:
                no_use_ids = TRUE;
                break;

            case TRC_UPDATE_OPT_NO_WILDS:
                ctx.flags |= TRC_LOG_PARSE_NO_GEN_WILDS;
                break;

            case TRC_UPDATE_OPT_LOG_WILDS:
                ctx.flags |= TRC_LOG_PARSE_LOG_WILDS;
                break;

            case TRC_UPDATE_OPT_CMD:
                ctx.cmd = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_DB:
                db_fn = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_TRC_SAVE:
                trc_save_to = poptGetOptArg(optCon);
                break;

            case TRC_UPDATE_OPT_VERSION:
                printf("Test Environment: %s\n\n%s\n", PACKAGE_STRING,
                       TE_COPYRIGHT);
                goto exit;

            default:
                ERROR("Unexpected option number %d", opt);
                goto exit;
        }
    }

    if (!no_use_ids)
        ctx.flags |= TRC_LOG_PARSE_USE_RULE_IDS;

    if (opt != -1)
    {
        ERROR("%s:%s", poptBadOption(optCon, 0), poptStrerror(opt));
        goto exit;
    }

    if (poptGetArg(optCon) != NULL)
    {
        ERROR("Unexpected arguments in command line");
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:
    poptFreeContext(optCon);
    return result;
}

/**
 * Function to match iteration in TRC with iteration from logs
 * using one of perl_expr, perl_script, oth_prog.
 *
 * @param db_args   Arguments of iteration in TRC
 * @param n_args    Number of arguments of iteration from log
 * @param args      Arguments of iteration from log
 * @param data      TRC Update data attached to iteration
 *
 * @return ITER_NO_MATCH if not matching, ITER_WILD_MATCH if
 *         matching.
 */
int
func_args_match(const trc_test_iter_args *db_args,
                unsigned int n_args, trc_report_argument *args,
                void *data)
{
    trc_update_test_iter_data *iter_data = data;
    trc_test_iter_arg         *arg;

#ifdef HAVE_LIBPERL
    HV  *olds;
    HV  *news;
    HV  *commons;
    HV  *uncomm_olds;
    HV  *uncomm_news;
    SV  *val;
    SV  *res;
#endif
    
    te_string   te_str;
    int         i = 0;

    tqh_strings     arg_names;
    tqh_strings     common_args;
    tqe_string     *arg_name;

    TAILQ_INIT(&arg_names);
    TAILQ_INIT(&common_args);

    if (iter_data != NULL)
        return ITER_NO_MATCH;

    if (perl_expr != NULL || perl_script != NULL)
    {
#ifdef HAVE_LIBPERL
        dTHX;

        olds = get_hv("olds", GV_ADD);
        hv_clear(olds);

        TAILQ_FOREACH(arg, &db_args->head, links)
        {
            val = newSVpv(arg->value, strlen(arg->value));
            if (hv_store(olds, arg->name, strlen(arg->name),
                         val, 0) == NULL)
            {
                ERROR("%s(): hv_store() failed");
                return ITER_NO_MATCH;
            }

            tq_strings_add_uniq(&arg_names, arg->name);
        }

        news = get_hv("news", GV_ADD);
        hv_clear(news);
        commons = get_hv("commons", GV_ADD);
        hv_clear(commons);

        uncomm_news = get_hv("uncomm_news", GV_ADD);
        hv_clear(uncomm_news);
        uncomm_olds = get_hv("uncomm_olds", GV_ADD);
        hv_clear(uncomm_olds);

        for (i = 0; i < (int)n_args; i++)
        {
            val = newSVpv(args[i].value, strlen(args[i].value));
            if (hv_store(news, args[i].name, strlen(args[i].name),
                         val, 0) == NULL)
            {
                ERROR("%s(): hv_store() failed");
                return ITER_NO_MATCH;
            }

            if (tq_strings_add_uniq(&arg_names, args[i].name) == 1)
            {
                val = newSViv(1);
                if (hv_store(commons, args[i].name,
                             strlen(args[i].name), val, 0) == NULL)
                {
                    ERROR("%s(): hv_store() failed");
                    return ITER_NO_MATCH;
                }
            }
        }

        eval_pv("sub uncomm_old"
                "{"
                "   $uncomm_olds{$_[0]} = 1;"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub uncomm_new"
                "{"
                "   $uncomm_news{$_[0]} = 1;"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub uncomm"
                "{"
                "   $uncomm_olds{$_[0]} = 1;"
                "   $uncomm_news{$_[0]} = 1;"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub comm_inc"
                "{"
                "   my $arg;"
                "   foreach $arg (keys %commons)"
                "   {"
                "       $commons{$arg} = 0;"
                "   }"
                "   foreach $arg (@_)"
                "   {"
                "       $commons{$arg} = 1;"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub comm_exc"
                "{"
                "   my $arg;"
                "   foreach $arg (@_)"
                "   {"
                "       if (exists($commons{$arg})) "
                "       {"
                "           $commons{$arg} = 0;"
                "       }"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub comm_eq"
                "{"
                "   my $rc = 1;"
                "   my $arg;"
                "   foreach $arg (keys %commons)"
                "   {"
                "       if ($commons{$arg} == 1)"
                "       {"
                "           $rc = $rc && (($olds{$arg} eq "
                "                          $news{$arg}) || "
                "                         (exists($olds{$arg}) && "
                "                          length($olds{$arg}) == 0));"
                "       }"
                "   }"
                "   return $rc;"
                "}",
                TRUE);

        eval_pv("sub uncomm_chk"
                "{"
                "   my $arg;"
                "   foreach $arg (keys %olds)"
                "   {"
                "       if (!exists($commons{$arg}) &&"
                "           $uncomm_olds{$arg} != 1)"
                "       {"
                "           return 0;"
                "       }"
                "   }"
                "   foreach $arg (keys %news)"
                "   {"
                "       if (!exists($commons{$arg}) &&"
                "           $uncomm_news{$arg} != 1)"
                "       {"
                "           return 0;"
                "       }"
                "   }"
                "   return 1;"
                "}",
                TRUE);

        eval_pv("sub olds { return $olds{$_[0]}; }", TRUE);
        eval_pv("sub news { return $news{$_[0]}; }", TRUE);
        eval_pv("sub olds_e { return exists($olds{$_[0]}); }", TRUE);
        eval_pv("sub news_e { return exists($news{$_[0]}); }", TRUE);

        TAILQ_FOREACH(arg_name, &arg_names, links)
        {
            memset(&te_str, 0, sizeof(te_str));
            te_str.ptr = NULL;

            te_string_append(&te_str, "sub %s { return \"%s\"; }",
                             arg_name->v, arg_name->v);
            eval_pv(te_str.ptr, TRUE);
            te_string_free(&te_str);
        }

        memset(&te_str, 0, sizeof(te_str));
        te_str.ptr = NULL;

        if (perl_expr != NULL)
        {
            te_string_append(&te_str, "$rc = (%s) && comm_eq() "
                             "&& uncomm_chk();",
                             perl_expr);
            eval_pv(te_str.ptr, TRUE);
        }
        else
        {
            te_string_append(&te_str,
                             "open my $fh, \"<\", \"%s\" or die $!;\n"
                             "local $/;\n"
                             "my $perl_code = <$fh>;\n"
                             "close $fh;\n"
                             "eval($perl_code);\n"
                             "$rc = $rc && comm_eq() && uncomm_chk();\n",
                             perl_script);
            eval_pv(te_str.ptr, TRUE);
        }

        te_string_free(&te_str);

        eval_pv("$res = $rc ? 1 : 0;", TRUE);
        res = get_sv("res", 0);

        tq_strings_free(&arg_names, NULL);
        tq_strings_free(&common_args, NULL);

        if (SvIV(res) == 1)
            return ITER_WILD_MATCH;
        else
            return ITER_NO_MATCH;
#endif
    }
    else if (oth_prog != NULL)
    {
        int   status = 0;
        pid_t child = fork();

        if (child != 0)
        {
            waitpid(child, &status, 0);
            if (status == 0)
                return ITER_WILD_MATCH;
            else
                return ITER_NO_MATCH;
        }
        else
        {
            char **argv;
            int    j = 0;

            TAILQ_FOREACH(arg, &db_args->head, links)
                i++;

            argv = TE_ALLOC(sizeof(*args) * (i + n_args + 2));

            j = 0;
            argv[j++] = oth_prog;

            TAILQ_FOREACH(arg, &db_args->head, links)
            {
                memset(&te_str, 0, sizeof(te_str));
                te_str.ptr = NULL;
                te_string_append(&te_str, "--old-%s=%s",
                                 arg->name, arg->value);
                argv[j++] = te_str.ptr;
            }

            for (i = 0; i < (int)n_args; i++)
            {
                memset(&te_str, 0, sizeof(te_str));
                te_str.ptr = NULL;
                te_string_append(&te_str, "--new-%s=%s",
                                 args[i].name, args[i].value);
                argv[j++] = te_str.ptr;
            }

            execv(oth_prog, argv);
        }
    }

    return ITER_NO_MATCH;
}

int
main(int argc, char **argv, char **envp)
{
    int     result = EXIT_FAILURE;
    char   *perl_embed_params[] = { "", "-e", "0" };

#ifdef HAVE_LIBPERL
    PERL_SYS_INIT3(&argc, &argv, &envp);
#else
    fprintf(stderr, "WARNING: libperl is missed. You cannot use "
            "--old-match-expr and --old-match-perl.\n");
#endif

    trc_update_init_ctx(&ctx);

    if (trc_update_process_cmd_line_opts(argc, argv) != EXIT_SUCCESS)
        goto exit;

    if (ctx.cmd == NULL)
    {
        te_string   te_str;
        int         i;

        memset(&te_str, 0, sizeof(te_str));
        te_str.ptr = NULL;
    
        te_string_append(&te_str, "%s", argv[0]);

        for (i = 1; i < argc; i++)
            te_string_append(&te_str, " %s", argv[i]);

        ctx.cmd = te_str.ptr;
    }

    if (db_fn == NULL)
    {
        ERROR("Missing name of the file with TRC database");
        goto exit;
    }

    if (ctx.flags & TRC_LOG_PARSE_LOG_WILDS)
    {
        if (trc_db_init(&ctx.db) != 0)
        {
            ERROR("Failed to initialize a new TRC database");
            goto exit;
        }
    }
    else if (trc_db_open(db_fn, &ctx.db) != 0)
    {
        ERROR("Failed to open TRC database '%s'", db_fn);
        goto exit;
    }

    /* Allocate TRC database user ID */
    ctx.db_uid = trc_db_new_user(ctx.db);

    if (perl_expr != NULL || perl_script != NULL || oth_prog != NULL)
        ctx.func_args_match = func_args_match;
    
#ifdef HAVE_LIBPERL
    /* Allocate and initialize perl interpreter */
    perl_interp = perl_alloc();
    perl_construct(perl_interp);
    perl_parse(perl_interp, NULL, 3, perl_embed_params, NULL);
    dTHX;
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
#endif

    if (trc_update_process_logs(&ctx) != 0)
    {
        ERROR("Failed to process XML logs");
        goto exit;
    }

    if (trc_save_to == NULL)
        trc_save_to = "temp_trc_db.xml";

    if (trc_db_save(ctx.db, trc_save_to,
                    TRC_SAVE_REMOVE_OLD | TRC_SAVE_RESULTS |
                    TRC_SAVE_GLOBALS,
                    ctx.db_uid, &trc_update_is_to_save,
                    (ctx.rules_load_from == NULL &&
                     (ctx.flags & TRC_LOG_PARSE_USE_RULE_IDS)) ?
                            &trc_update_set_user_attr : NULL,
                    ctx.cmd) != 0)
    {
        ERROR("Failed to save TRC database '%s'", db_fn);
        goto exit;
    }

    result = EXIT_SUCCESS;

exit:

    if (ctx.db != NULL)
    {
        trc_db_free_user_data(ctx.db, ctx.db_uid, free,
                              (void (*)(void *))
                              &trc_update_free_test_iter_data);
        trc_db_free_user(ctx.db, ctx.db_uid);
        trc_db_close(ctx.db);
    }

#ifdef HAVE_LIBPERL
    perl_destruct(perl_interp);
    perl_free(perl_interp);
    PERL_SYS_TERM();
#endif

    trc_update_free_ctx(&ctx);

    free(db_fn);
    free(perl_expr);
    free(perl_script);
    free(oth_prog);

    return result;
}
