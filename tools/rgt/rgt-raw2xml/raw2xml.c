/** @file 
 * @brief Test Environment: RGT - log dumping utility
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
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
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#include <getopt.h>
#include <ctype.h>

#include "te_defs.h"
#include "te_raw_log.h"

#define ERROR(_fmt, _args...) fprintf(stderr, _fmt "\n", ##_args)

#define ERROR_CLEANUP(_fmt, _args...) \
    do {                                \
        ERROR(_fmt, ##_args);           \
        goto cleanup;                   \
    } while (0)

#define ERROR_USAGE_RETURN(_fmt, _args...) \
    do {                                                \
        ERROR(_fmt, ##_args);                           \
        usage(stderr, program_invocation_short_name);   \
        return 1;                                       \
    } while (0)


#define INPUT_BUF_SIZE  16384
#define OUTPUT_BUF_SIZE 16384

#define MESSAGE_ARG_LIST_MIN_SIZE   8
#define MESSAGE_ARG_LIST_THRES_SIZE 128

#define SCRAP_MIN_SIZE  16384


static void    *scrap_buf   = NULL;
static size_t   scrap_size  = 0;

te_bool
scrap_grow(size_t size)
{
    size_t  new_scrap_size  = scrap_size;
    void   *new_scrap_buf;

    if (size <= scrap_size)
        return TRUE;

    if (new_scrap_size < SCRAP_MIN_SIZE)
        new_scrap_size = SCRAP_MIN_SIZE;

    while (new_scrap_size < size)
        new_scrap_size += new_scrap_size / 2;

    new_scrap_buf = realloc(scrap_buf, new_scrap_size);
    if (new_scrap_buf == NULL)
        return FALSE;

    scrap_buf = new_scrap_buf;
    scrap_size = new_scrap_size;

    return TRUE;
}


void
scrap_clear(void)
{
    free(scrap_buf);
    scrap_size = 0;
}


/** Message variable-length field */
typedef struct message_fld {
    uint8_t    *buf;
    te_log_nfl  len;
} message_fld;


static te_bool
message_fld_valid(const message_fld *f)
{
    return f != NULL &&
           (f->buf != NULL || f->len == 0);
}


static te_bool
message_fld_list_valid(const message_fld *list, size_t num)
{
    if (list == NULL && num > 0)
        return FALSE;

    for (; num > 0; list++, num--)
        if (!message_fld_valid(list))
            return FALSE;

    return TRUE;
}


/** Message */
typedef struct message {
    te_log_version      version;
    te_log_ts_sec       ts_secs;
    te_log_ts_usec      ts_usecs;
    te_log_level        level;
    te_log_id           id;
    message_fld         entity_name;
    message_fld         user_name;
    message_fld         format;
    message_fld        *arg_list;
    size_t              arg_size;
    size_t              arg_num;
} message;


static te_bool
message_valid(const message *m)
{
    return m != NULL &&
           message_fld_valid(&m->entity_name) &&
           message_fld_valid(&m->user_name) &&
           message_fld_valid(&m->format) &&
           m->arg_num <= m->arg_size &&
           message_fld_list_valid(m->arg_list, m->arg_num);
}


static te_bool
message_init(message *m)
{
    memset(m, 0, sizeof(*m));

    m->arg_list     = malloc(sizeof(*m->arg_list) *
                             MESSAGE_ARG_LIST_MIN_SIZE);
    if (m->arg_list == NULL)
        return FALSE;

    m->arg_size     = MESSAGE_ARG_LIST_MIN_SIZE;

    return TRUE;
}


static void
message_clnp(message *m)
{
    size_t  i;

    assert(message_valid(m));

    free(m->entity_name.buf);
    free(m->user_name.buf);
    free(m->format.buf);

    for (i = 0; i < m->arg_num; i++)
        free(m->arg_list[i].buf);

    free(m->arg_list);
}


static void
message_arg_list_clear(message *m)
{
    assert(message_valid(m));

    for (; m->arg_num > 0; m->arg_num--)
        free(m->arg_list[m->arg_num - 1].buf);

    if (m->arg_size > MESSAGE_ARG_LIST_THRES_SIZE)
    {
        m->arg_list = realloc(m->arg_list,
                              sizeof(*m->arg_list) *
                              MESSAGE_ARG_LIST_THRES_SIZE);
        m->arg_size = MESSAGE_ARG_LIST_THRES_SIZE;
    }
}


static te_bool
message_arg_list_adda(message *m, uint8_t *buf, te_log_nfl len)
{
    assert(message_valid(m));
    assert(buf != NULL || len == 0);

    if (m->arg_num >= m->arg_size)
    {
        message_fld    *new_arg_list;
        size_t          new_arg_size;

        new_arg_size = m->arg_size + m->arg_size / 2;
        new_arg_list = realloc(m->arg_list,
                               new_arg_size * sizeof(*new_arg_list));
        if (new_arg_list == NULL)
            return FALSE;
        m->arg_list = new_arg_list;
        m->arg_size = new_arg_size;
    }

    m->arg_list[m->arg_num].buf = buf;
    m->arg_list[m->arg_num].len = len;
    m->arg_num++;

    return TRUE;
}


/** Message reading result code */
typedef enum read_message_rc {
    READ_MESSAGE_RC_ERR         = -2,   /**< A reading error occurred or
                                             unexpected EOF was reached */
    READ_MESSAGE_RC_WRONG_VER   = -1,   /**< A message of unsupported
                                             version was encountered */
    READ_MESSAGE_RC_EOF         = 0,    /**< The EOF was encountered instead
                                             of the message */
    READ_MESSAGE_RC_OK          = 1,    /**< The message was read
                                              successfully */
} read_message_rc;


/**
 * Read a message from a stream.
 *
 * @param input The stream to read from.
 * @param m     Message to write into.
 *
 * @return Result code.
 */
static read_message_rc
read_message(FILE *input, message *m)
{
    te_log_version  ver;
    te_log_nfl      len;
    uint8_t        *buf;
    /* NOTE: reverse order */
    message_fld    *req_fld_list[]    = {&m->format,
                                         &m->user_name,
                                         &m->entity_name};
    size_t          req_fld_num       = sizeof(req_fld_list) /
                                        sizeof(*req_fld_list);

    assert(input != NULL);
    assert(message_valid(m));

    /* Read and verify log message version */
    if (fread(&ver, sizeof(ver), 1, input) != 1)
    {
        if (feof(input))
            return READ_MESSAGE_RC_EOF;
        return READ_MESSAGE_RC_ERR;
    }
    if (ver != TE_LOG_VERSION)
        return READ_MESSAGE_RC_WRONG_VER;
    /* Store version */
    m->version = ver;

    /*
     * Read timestamp, level and ID
     */
    if (fread(&m->ts_secs, sizeof(m->ts_secs), 1, input) != 1 ||
        fread(&m->ts_usecs, sizeof(m->ts_usecs), 1, input) != 1 ||
        fread(&m->level, sizeof(m->level), 1, input) != 1 ||
        fread(&m->id, sizeof(m->id), 1, input) != 1)
        return READ_MESSAGE_RC_ERR;

    /*
     * Convert timestamp, level and ID
     */
    m->ts_secs = ntohl(m->ts_secs);
    m->ts_usecs = ntohl(m->ts_usecs);
#if SIZEOF_TE_LOG_LEVEL == 4
    m->level = ntohl(m->level);
#elif SIZEOF_TE_LOG_LEVEL == 2
    m->level = ntohs(m->level);
#elif SIZEOF_TE_LOG_LEVEL != 1
#error Unexpected value of SIZEOF_TE_LOG_LEVEL
#endif
#if SIZEOF_TE_LOG_ID == 4
    m->id = ntohl(m->id);
#elif SIZEOF_TE_LOG_ID == 2
    m->id = ntohs(m->id);
#elif SIZEOF_TE_LOG_ID != 1
#error Unexpected value of SIZEOF_TE_LOG_ID
#endif

    /* Reset message format argument list */
    message_arg_list_clear(m);

    /*
     * Read remaining required variable-length fields and optional format
     * arguments.
     */
    while (TRUE)
    {
        /* Read and convert the field length */
        if (fread(&len, sizeof(len), 1, input) != 1)
            return READ_MESSAGE_RC_ERR;
#if SIZEOF_TE_LOG_NFL == 4
        len = ntohl(len);
#elif SIZEOF_TE_LOG_NFL == 2
        len = ntohs(len);
#elif SIZEOF_TE_LOG_NFL != 1
#error Unexpected value of SIZEOF_TE_LOG_NFL
#endif

        /*
         * If it is not a required field and it is a special (terminating)
         * field length.
         */
        if (req_fld_num == 0 && len == TE_LOG_RAW_EOR_LEN)
            break;

        /*
         * Read the field
         */
        if (len == 0)
            buf = NULL;
        else
        {
            buf = malloc(len);
            if (buf == NULL)
                return READ_MESSAGE_RC_ERR;
            if (fread(buf, len, 1, input) != 1)
            {
                free(buf);
                return READ_MESSAGE_RC_ERR;
            }
        }

        /*
         * Store the field
         */
        if (req_fld_num > 0)
        {
            req_fld_num--;
            free(req_fld_list[req_fld_num]->buf);
            req_fld_list[req_fld_num]->buf = buf;
            req_fld_list[req_fld_num]->len = len;
        }
        else if (!message_arg_list_adda(m, buf, len))
        {
            free(buf);
            return READ_MESSAGE_RC_ERR;
        }
    }

    return READ_MESSAGE_RC_OK;
}


static int
run(unsigned long max_mem, const char *input_name, const char *output_name)
{
    /* Create Lua instance */
    /* Require rgt.msg */
    /* Require rgt.sink */
    /* Push max_mem */
    /* Call rgt.sink to create sink instance */
    /* Retrieve sink instance "take_file" method */
    /* Copy sink instance to the top */
    /* Open/push output file */
    /* Call "take_file" instance method to supply the sink with the file */
    /* Retrieve sink instance "start" method */
    /* Copy sink instance to the top */
    /* Call "start" instance method */
    /* Retrieve sink instance "put" method */

    /* Read log file version */

    while (TRUE)
    {
        /* Copy "put" sink instance method to the top */
        /* Copy sink instance to the top */
        /* Copy rgt.msg to the top */
        /* Call read_message to supply the arguments */
        /* If there are no more messages */
            /* Break */
        /* If an error occurred */
            /* Abort */
        /* Call rgt.msg with the arguments to create message instance */
        /* Call "put" sink instance method to feed the message */
    }

    /* Retrieve sink instance "finish" method */
    /* Copy sink instance to the top */
    /* Call sink instance "finish" method */
    /* Retrieve sink instance "yield_file" method */
    /* Copy sink instance to the top */
    /* Call sink instance "yield_file" method */
    /* Retrieve file "close" method */
    /* Copy file to the top */
    /* Call file "close" method */
}


static int
usage(FILE *stream, const char *progname)
{
    return 
        fprintf(
            stream, 
            "Usage: %s [OPTION]... [INPUT_RAW [OUTPUT_XML]]\n"
            "Convert a raw TE log file to XML.\n"
            "\n"
            "With no INPUT_RAW, or when INPUT_RAW is -, "
            "read standard input.\n"
            "With no OUTPUT_XML, or when OUTPUT_XML is -, "
            "write standard output.\n"
            "\n"
            "Options:\n"
            "  -h, --help       this help message\n"
            "  -m, --max-mem=MB maximum memory to use for output (MB)\n"
            "                   (default: RAM size / 4, maximum: 4096)\n"
            "\n",
            progname);
}


typedef enum opt_val {
    OPT_VAL_HELP        = 'h',
    OPT_VAL_MAX_MEM     = 'm',
} opt_val;


int
main(int argc, char * const argv[])
{
    static const struct option  long_opt_list[] = {
        {.name      = "help",
         .has_arg   = no_argument,
         .flag      = NULL,
         .val       = OPT_VAL_HELP},
        {.name      = "max-mem",
         .has_arg   = required_argument,
         .flag      = NULL,
         .val       = OPT_VAL_MAX_MEM},
        {.name      = NULL,
         .has_arg   = 0,
         .flag      = NULL,
         .val       = 0}
    };
    static const char          *short_opt_list = "hm:";

    int             c;
    const char     *input_name  = "-";
    const char     *output_name = "-";
    struct sysinfo  si;
    unsigned long   max_mem;

    /*
     * Set default max_mem
     */
    sysinfo(&si);
    max_mem = (unsigned long)
                (((uint64_t)(si.totalram) * mem_unit) / (4*1024*1024));
    if (max_mem > 4096)
        max_mem = 4096;

    /*
     * Read command line arguments
     */
    while ((c = getopt_long(argc, argv,
                            short_opt_list, long_opt_list, NULL)) >= 0)
    {
        switch (c)
        {
            case OPT_VAL_HELP:
                usage(stdout, program_invocation_short_name);
                return 0;
                break;
            case OPT_VAL_MAX_MEM:
                {
                    const char *end;

                    errno = 0;
                    max_mem = strtoul(optarg, &end, 0);
                    for (; isspace(*end); end++);
                    if (errno != 0 || *end != '\0' || max_mem > 4096)
                        ERROR_USAGE_RETURN("Invalid maximum memory option value");
                }
                break;
            case '?':
                usage(stderr, program_invocation_short_name);
                return 1;
                break;
        }
    }

    if (optind < argc)
    {
        input_name  = argv[optind++];
        if (optind < argc)
        {
            output_name = argv[optind++];
            if (optind < argc)
                ERROR_USAGE_RETURN("Too many arguments");
        }
    }

    /*
     * Verify command line arguments
     */
    if (*input_name == '\0')
        ERROR_USAGE_RETURN("Empty input file name");
    if (*output_name == '\0')
        ERROR_USAGE_RETURN("Empty output file name");

    /*
     * Run
     */
    return run(max_mem, input_name, output_name);
}


