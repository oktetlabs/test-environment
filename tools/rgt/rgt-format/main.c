/** @file 
 * @brief Test Environment: Engine of RGT format module.
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

#include <stdio.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#define assert(_x)
#endif

#define UTILITY_NAME "ABCD"
#include <ctype.h>
#include <libxml/parser.h>

#include <popt.h>

#include "rgt_tmpls_lib.h"

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

static const char *tmpl_files[RGT_TMPLS_NUM];

/** 
 * Pointer to the buffer that is allocated under template file names 
 * while command line options are processed.
 */
static char *str_buf = NULL;

static struct log_tmpl html_tmpls[RGT_TMPLS_NUM];

/** Name of the XML report file */
static const char *xml_file_name = NULL;

/* Output file name passed as an argument */
static const char *out_file_name = NULL;

/** Output file descriptor */
static FILE *out_fd = NULL;

/** Array of user specified variables */
static const char **user_vars = NULL;

enum e_element_type {
    XML_ELEMENT_START,
    XML_ELEMENT_END
};

enum e_rgt_xml2html_state {
    RGT_XML2HTML_STATE_INITIAL,
    RGT_XML2HTML_STATE_LOGS,
    RGT_XML2HTML_STATE_LOG_MSG,
    RGT_XML2HTML_STATE_MEM_DUMP,
    RGT_XML2HTML_STATE_MEM_DUMP_ROW,
    RGT_XML2HTML_STATE_MEM_DUMP_ELEM,
    RGT_XML2HTML_STATE_FILE,
};

enum e_log_row_colour {
    LOG_ROW_LIGHT,
    LOG_ROW_DARK,
};

/** 
 * The structure represents a global context that is used in all handlers
 * to determine the current state, values some element specific variables
 */
struct global_context {
    enum e_rgt_xml2html_state state; /**< Current state of processing */
    enum e_log_row_colour log_col; /**< The color of the current log row */
    struct mem_dump_info {
        int mem_width; /**< Number of elements in a memory row.
                         (Used only for mem-dump element processing) */
        int cur_num; /**< Current number of elements in memory row */
        int first_row;
    } mem_dump;
};

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static struct global_context global_ctx;


/**
 * Callback function that is called before parsing the document.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 *
 * @return Nothing
 */
static void
rgt_log_start_document(void *user_data ATTRIBUTE_UNUSED)
{
    UNUSED(user_data);

    rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_DOCUMENT_START], 
                         NULL, user_vars);
}

/**
 * Callback function that is called when XML parser reaches the end 
 * of the document.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 *
 * @return Nothing
 */
static void
rgt_log_end_document(void *user_data ATTRIBUTE_UNUSED)
{
    UNUSED(user_data);

    rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_DOCUMENT_END],
                         NULL, user_vars);
}

/**
 * Processes start/stop "msg" tag.
 *
 * @param  atts     An array of attribute name, attribute value pairs
 *                  (it is used only in start element processing)
 * @param  el_type  Determine whether to process start of end element
 *
 * @todo Delete ctx and name parameters.
 */
static void
process_log_msg(struct global_context *ctx, const xmlChar *name,
                const xmlChar **atts, enum e_element_type el_type)
{
    int         i;
    /* Initialize to zero all the array */
    /** @todo Fix hardcoded constant and make the implementation clearer */
    const char *new_atts[11] = { "row_class", };

    UNUSED(ctx);
    UNUSED(name);

    /* Add an additional row_class attribute */
    new_atts[1] = (global_ctx.log_col == LOG_ROW_LIGHT) ?
            "tdlight" : "tddark";

    for (i = 2; i < 10 && atts != NULL && atts[i - 2] != NULL; i++)
    {
        new_atts[i] = atts[i - 2];
    }
    assert(atts == NULL || atts[i - 2] == NULL);

    if (el_type == XML_ELEMENT_START)
    {
        rgt_tmpls_lib_output(out_fd,
                &html_tmpls[LOG_PART_LOG_MSG_START], new_atts, user_vars);
    }
    else
    {
        rgt_tmpls_lib_output(out_fd,
                &html_tmpls[LOG_PART_LOG_MSG_END], new_atts, user_vars);
        global_ctx.log_col = (global_ctx.log_col == LOG_ROW_LIGHT) ? 
                LOG_ROW_DARK : LOG_ROW_LIGHT;
    }
}

/**
 * Processes start/stop tag "mem-dump" for the mem-dump.
 *
 * @param  el_type  Determine whether to process start of end element
 *
 * @todo Delete ctx, name and atts parameters.
 */
static void
process_mem_dump(struct global_context *ctx, const xmlChar *name,
                 const xmlChar **atts, enum e_element_type el_type)
{
    UNUSED(ctx);
    UNUSED(name);
    UNUSED(atts);

    if (el_type == XML_ELEMENT_START)
        rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_MEM_DUMP_START],
                             NULL, user_vars);
    else
        rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_MEM_DUMP_END],
                             NULL, user_vars);
}

/**
 * Processes start/stop tag "row" for the mem-dump.
 *
 * @param  el_type  Determine whether to process start of end element
 *
 * @todo Delete ctx, name and atts parameters.
 */
static void
process_mem_row(struct global_context *ctx, const xmlChar *name,
                const xmlChar **atts, enum e_element_type el_type)
{
    UNUSED(name);
    UNUSED(atts);

    if (el_type == XML_ELEMENT_START)
        rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_MEM_ROW_START],
                             NULL, user_vars);
    else
    {
        int i;

        assert((ctx->mem_dump.mem_width - ctx->mem_dump.cur_num) >= 0);

        for (i = 0; i < (ctx->mem_dump.mem_width - ctx->mem_dump.cur_num); i++)
            rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_MEM_ELEM_EMPTY], 
                                 NULL, user_vars);

        rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_MEM_ROW_END],
                             NULL, user_vars);
    }
}

/**
 * Processes start/stop tag "elem" for the mem-dump.
 *
 * @param  el_type  Determine whether to process start of end element
 *
 * @todo Delete ctx, name and atts parameters.
 */
static void
process_mem_elem(struct global_context *ctx, const xmlChar *name,
                 const xmlChar **atts, enum e_element_type el_type)
{
    UNUSED(ctx);
    UNUSED(name);
    UNUSED(atts);

    if (el_type == XML_ELEMENT_START)
        rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_MEM_ELEM_START],
                             NULL, user_vars);
    else
        rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_MEM_ELEM_END],
                             NULL, user_vars);
}

/**
 * Callback function that is called when XML parser meets an opening tag.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  name       The element name
 * @param  atts       An array of attribute name, attribute value pairs
 *
 * @return Nothing
 */
static void
rgt_log_start_element(void *user_data, const xmlChar *name, const xmlChar **atts)
{
    struct global_context *ctx = (struct global_context *)user_data;

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_INITIAL:
            if (strcmp(name, "logs") == 0)
            {
                ctx->state = RGT_XML2HTML_STATE_LOGS;
            }
            else if (strcmp(name, "proteos:log_report") == 0)
            {
                /* Do nothing */
            }
            return;

        case RGT_XML2HTML_STATE_LOGS:
            process_log_msg(ctx, name, atts, XML_ELEMENT_START);
            ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            return;

        case RGT_XML2HTML_STATE_LOG_MSG:
            if (strcmp(name, "mem-dump") == 0)
            {
                process_mem_dump(ctx, name, atts, XML_ELEMENT_START);
                ctx->state = RGT_XML2HTML_STATE_MEM_DUMP;
                ctx->mem_dump.first_row = TRUE;
                ctx->mem_dump.mem_width = 0;
            }
            else if (strcmp(name, "file") == 0)
            {
                ctx->state = RGT_XML2HTML_STATE_FILE;
            }
            else if (strcmp(name, "br") == 0)
            {
                rgt_tmpls_lib_output(out_fd, &html_tmpls[LOG_PART_BR],
                                     NULL, user_vars);
            }
            return;

        case RGT_XML2HTML_STATE_MEM_DUMP:
            assert(strcmp(name, "row") == 0);
            process_mem_row(ctx, name, atts, XML_ELEMENT_START);
            ctx->mem_dump.cur_num = 0;
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ROW;
            return;

        case RGT_XML2HTML_STATE_MEM_DUMP_ROW:
            assert(strcmp(name, "elem") == 0);
            process_mem_elem(ctx, name, atts, XML_ELEMENT_START);
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ELEM;
            return;
            
        default:
            assert(0);
    }
}

/**
 * Callback function that is called when XML parser meets the end of 
 * an element.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  name       The element name
 *
 * @return Nothing
 */
static void
rgt_log_end_element(void *user_data, const xmlChar *name)
{
    struct global_context *ctx = (struct global_context *)user_data;

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_LOG_MSG:
            if (strcmp(name, "msg") == 0)
            {
                process_log_msg(ctx, name, NULL, XML_ELEMENT_END);
                ctx->state = RGT_XML2HTML_STATE_LOGS;
            }
            return;
        
        case RGT_XML2HTML_STATE_LOGS:
            ctx->state = RGT_XML2HTML_STATE_INITIAL;
            return;

        case RGT_XML2HTML_STATE_FILE:
            assert(strcmp(name, "file") == 0);
            ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            return;

        case RGT_XML2HTML_STATE_MEM_DUMP_ELEM:
            assert(strcmp(name, "elem") == 0);

            if (ctx->mem_dump.first_row)
            {
                ctx->mem_dump.mem_width++;
            }
            ctx->mem_dump.cur_num++;

            process_mem_elem(ctx, name, NULL, XML_ELEMENT_END);
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP_ROW;
            return;

        case RGT_XML2HTML_STATE_MEM_DUMP_ROW:
            assert(strcmp(name, "row") == 0);

            ctx->mem_dump.first_row = FALSE;
            process_mem_row(ctx, name, NULL, XML_ELEMENT_END);
            ctx->state = RGT_XML2HTML_STATE_MEM_DUMP;
            return;
        
        case RGT_XML2HTML_STATE_MEM_DUMP:
            assert(strcmp(name, "mem-dump") == 0);
            
            process_mem_dump(ctx, name, NULL, XML_ELEMENT_END);
            ctx->state = RGT_XML2HTML_STATE_LOG_MSG;
            return;

        case RGT_XML2HTML_STATE_INITIAL:
            
            return;

        default:
            assert(0);
    }
}

/**
 * Callback function that is called when XML parser meets character data.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  ch         Pointer to the string
 * @param  len        Number of the characters in the string
 *
 * @return Nothing
 */
static void
rgt_log_characters(void *user_data, const xmlChar *ch, int len)
{
    struct global_context *ctx = (struct global_context *)user_data;

    switch (ctx->state)
    {
        case RGT_XML2HTML_STATE_LOG_MSG:
        case RGT_XML2HTML_STATE_MEM_DUMP_ELEM:
        case RGT_XML2HTML_STATE_FILE:
        {
#define MAX_LEN 60
            char buf[MAX_LEN + 1];
            int cur_len = 0;

            buf[MAX_LEN] = '\0';
            while (len > MAX_LEN)
            {
                memcpy(buf, ch + cur_len, MAX_LEN);
                fprintf(out_fd, "%s", buf);
                cur_len += MAX_LEN;
                len -= MAX_LEN;
            }
            memcpy(buf, ch + cur_len, len);
            buf[len] = '\0';
            fprintf(out_fd, "%s", buf);
#undef MAX_LEN
            break;
        }

        default:
            break;
    }
}

/** The structure specifies all types callback routines that should be called */
static xmlSAXHandler sax_handler = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* attributeDeclDebug, */
    NULL, /* elementDeclDebug, */
    NULL, /* unparsedEntityDeclDebug, */
    NULL,
    rgt_log_start_document,
    rgt_log_end_document,
    rgt_log_start_element,
    rgt_log_end_element,
    NULL, /* referenceDebug, */
    rgt_log_characters,
    NULL, /* ignorableWhitespaceDebug, */
    NULL, /* processingInstructionDebug, */
    NULL, /* commentDebug, */
    NULL, /* warningDebug, */
    NULL, /* errorDebug, */
    NULL, /* fatalErrorDebug, */
    NULL, /* getParameterEntityDebug, */
    NULL, /* cdataBlockDebug, */
    NULL, /* externalSubsetDebug, */
    1
};

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
        "[<xml report file>] [<output file>]");
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
 *    The function updates the following global variables according to 
 *    the command line arguments specified in argv:
 *      html_tmpls    - Array of template file names
 *      xml_file_name - XML report file name
 *      out_file_name - Output file name (if out_fd is not stdout)
 *      out_fd        - Output file descriptor
 *
 *    Allocates memory under template file names and saves pointer to
 *    the memory in "str_buf" variable, which should be freed.
 *
 *    In the case of an error it calls exit() function with code 1.
 */
static void
process_cmd_line_opts(int argc, char **argv)
{
    const char  *opt_out_file_name = NULL; /* Output file name passed
                                              with -f option */
    const char  *tmpls_file_name = NULL;
    FILE        *fd;
    poptContext  optCon; /* Context for parsing command-line options */
    int          n_user_vars = 0;
    int          rc;
    
    /* Option Table */
    struct poptOption optionsTable[] = {
        { "tmpls-file", 't', POPT_ARG_STRING, NULL, 't',
          "Template suite file name.", "FILE" },

        { "xml-report-file", 'f', POPT_ARG_STRING, NULL, 'f', 
          "XML report file name.", "FILE" },

        { "output", 'o', POPT_ARG_STRING, NULL, 'o', 
          "Output file name.", "FILE" },
          
        { "with-var", 'w', POPT_ARG_STRING, NULL, 'w', 
           "Specify variable to be used in parsing context"
           "in form \"var_name@@var_value\", for example \"name@@oleg\"", NULL },

        { "version", 'v', POPT_ARG_NONE, NULL, 'v', 
          "Display version information.", NULL },

        POPT_AUTOHELP

        { NULL, 0, 0, NULL, 0, NULL, NULL },
    };

    /* Process command line options */
    optCon = poptGetContext(NULL, argc, (const char **)argv, optionsTable, 0);

    poptSetOtherOptionHelp(optCon,
                           "[OPTIONS...] [<xml report file>] [<output file>]");

    while ((rc = poptGetNextOpt(optCon)) >= 0)
    {
        if (rc == 't')
        {
            if ((tmpls_file_name = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify template suite file", NULL);
            }
        }
        else if (rc == 'f')
        {
            if ((xml_file_name = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify XML report file", NULL);
            }
        }
        else if (rc == 'o')
        {
            if ((opt_out_file_name = poptGetOptArg(optCon)) == NULL)
            {
                usage(optCon, 1, "Specify Output file name", NULL);
            }
        }
        else if (rc == 'w')
        {
            const char **tmp_ptr = user_vars;
            const char  *param;
            char        *var_val;            

            user_vars = realloc(user_vars,
                                ((++n_user_vars) * 2 + 1) * sizeof(char *));
            if (user_vars == NULL)
            {
                free(tmp_ptr);
                printf("Not enough memory\n");
                poptFreeContext(optCon);
                exit(1);            
            }
            if ((param = poptGetOptArg(optCon)) == NULL)
            {
                free(user_vars);
                usage(optCon, 1, "Specify variable name and value", NULL);
            }
            if ((var_val = strstr(param, "@@")) == NULL)
            {
                free(user_vars);
                printf("There is no value specified for %s variable\n"
                       "Variable value starts after \"@@\" marker\n", param);
                poptFreeContext(optCon);
                exit(1);
            }
            var_val[0] = '\0';
            var_val += 2;
            user_vars[n_user_vars * 2 - 2] = param;
            user_vars[n_user_vars * 2 - 1] = var_val;
            user_vars[n_user_vars * 2]     = NULL;
        }
        else if (rc == 'v')
        {
            printf("Package %s: %s version %s\n%s\n", 
                   PACKAGE, UTILITY_NAME, VERSION, TE_COPYRIGHT);
            free(user_vars);
            poptFreeContext(optCon);
            exit(0);
        }
    }

    if (rc < -1)
    {
        /* An error occurred during option processing */
        fprintf(stderr, "%s: %s\n",
                poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                poptStrerror(rc));
        poptFreeContext(optCon);
        free(user_vars);
        exit(1);
    }

    if (tmpls_file_name == NULL)
    {
        free(user_vars);
        usage(optCon, 1, "Specify template suite file", NULL);    
    }
    if ((fd = fopen(tmpls_file_name, "r")) == NULL)
    {
        perror(tmpls_file_name);
        free(user_vars);
        poptFreeContext(optCon);
        exit(1);
    }
    else
    {
        long  fsize;
        long  cur_len = 0;
        int   len;
        int   tmpl_index = 0;
        char *str;

        fseek(fd, 0L, SEEK_END);
        fsize = ftell(fd);
        fseek(fd, 0L, SEEK_SET);

        str_buf = (char *)malloc(fsize += 1 /* Under trailing '\0' */);
        assert(str_buf != NULL);

        while ((str = fgets(str_buf + cur_len, fsize - cur_len, fd)) != NULL)
        {
            if (tmpl_index == RGT_TMPLS_NUM)
                break;

            if (isspace(str[0]))
            {
                /* Skip lines started with space character */
                continue;
            }
            len = strlen(str);
            if (str[len - 1] == '\n')
            {
                str[len - 1] = '\0';
            }
            tmpl_files[tmpl_index++] = str;
            cur_len += len;
        }
        
        fclose(fd);

        if (tmpl_index < RGT_TMPLS_NUM)
        {
            free(str_buf);
            free(user_vars);
            fprintf(stderr, "Not all template files mentioned in "
                            "template suite file\n");
            poptFreeContext(optCon);
            exit(1);
        }
    }

    if (xml_file_name == NULL && (xml_file_name = poptGetArg(optCon)) == NULL)
    {
        free(str_buf);
        free(user_vars);
        usage(optCon, 1, "Specify XML report file", NULL);
    }

    /* Get output file name */
    if ((out_file_name = poptGetArg(optCon)) == NULL)
    {
        if (opt_out_file_name != NULL)
        {
            if ((out_fd = fopen(opt_out_file_name, "w")) == NULL)
            {
                free(str_buf);
                free(user_vars);
                perror(opt_out_file_name);
                poptFreeContext(optCon);
                exit(1);
            }
            out_file_name = opt_out_file_name;
        }
        else
        {
            out_fd = stdout;
        }
    }
    else
    {
        if (opt_out_file_name != NULL)
        {
            free(str_buf);
            free(user_vars);
            usage(optCon, 1, "Output file name specified twice: "
                  "with -f option and as a command line argument", NULL);
        }

        if ((out_fd = fopen(out_file_name, "w")) == NULL)
        {
            free(str_buf);
            free(user_vars);
            perror(out_file_name);
            poptFreeContext(optCon);
            exit(1);
        }
    }

    if (poptPeekArg(optCon) != NULL)
    {
        free(str_buf);
        free(user_vars);
        if (out_fd != stdout)
        {
            fclose(out_fd);
            unlink(out_file_name);
        }
        usage(optCon, 1, "Too many parameters specified", NULL);
    }

    poptFreeContext(optCon);
}

int
main(int argc, char **argv)
{
    int rc = 0;

    process_cmd_line_opts(argc, argv);

    if (rgt_tmpls_lib_parse(tmpl_files,  html_tmpls, RGT_TMPLS_NUM) != 0)
    {
        free(str_buf);
        free(user_vars);
        if (out_fd != stdout)
        {
            fclose(out_fd);
            unlink(out_file_name);
        }
        return 1;
    }
    free(str_buf);

    global_ctx.state = RGT_XML2HTML_STATE_INITIAL;
    global_ctx.log_col = LOG_ROW_LIGHT;

    if (xmlSAXUserParseFile(&sax_handler, &global_ctx, xml_file_name) != 0)
    {
        if (out_fd != stdout)
        {
            fclose(out_fd);
            unlink(out_file_name);
        }
        fprintf(stderr, "Cannot parse XML document\n");
        rc = 1;
    }
    else
    {
        fclose(out_fd);
    }

    free(user_vars);
    rgt_tmpls_lib_free(html_tmpls, RGT_TMPLS_NUM);
    xmlCleanupParser();

    return rc;
}
