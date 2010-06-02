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

#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <arpa/inet.h>

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


#define ESCAPE_CHAR_LIST(_M) \
    _M('\\', '\\')                  \
    _M('\r', 'r')                   \
    _M('\n', 'n')                   \
    _M('\t', 't')                   \
    _M('\"', '"')                   \
    _M('\0', '0')

/**
 * Determine number of characters required for escaped output of a byte.
 *
 * @param b Byte to escape.
 *
 * @return Number of characters required for output.
 */
static size_t
escape_eval_byte(uint8_t b)
{
    switch (b)
    {
#define CHAR(_val, _char) case _val: return 2;
        ESCAPE_CHAR_LIST(CHAR)
#undef CHAR
        default:
            return (b >= 0x20 && b <= 0x7E) ? 1 : 4;
    }
}


/**
 * Escape a byte and output it to a buffer.
 *
 * @param buf   Buffer to output the escape sequence to.
 * @param b     Byte to escape.
 *
 * @return      Number of output characters.
 */
static size_t
escape_fmt_byte(char *buf, uint8_t b)
{
    static const char   hex_digit[16]   = "0123456789ABCDEF";

    switch (b)
    {
#define CHAR(_val, _char) \
        case _val:          \
            buf[0] = '\\';  \
            buf[1] = _char; \
            return 2;
        ESCAPE_CHAR_LIST(CHAR)
#undef CHAR

        default:
            if (b >= 0x20 && b <= 0x7E)
            {
                *buf = b;
                return 1;
            }
            else
            {
                *buf++  = '\\';
                *buf++  = 'x';
                *buf++  = hex_digit[b >> 4];
                *buf    = hex_digit[b & 0xF];
                return 4;
            }
    }
}


/**
 * Determine the number of characters required for escaped output of a
 * buffer.
 *
 * @param buf           Pointer to the buffer to escape.
 * @param len           Length of the buffer to escape.
 * @param pgot_space    Location for "got space" flag - set to TRUE if a
 *                      space character was found in the buffer; could be
 *                      NULL.
 *
 * @return Number of characters required for output.
 */
static size_t
escape_eval_buf(void *buf, size_t len, te_bool *pgot_space)
{
    size_t      esc_len     = 0;
    te_bool     got_space   = FALSE;
    uint8_t    *p;

    for (p = (uint8_t *)buf; len > 0; p++, len--)
    {
        if (*p == ' ')
            got_space = TRUE;
        esc_len += escape_eval_byte(*p);
    }

    if (pgot_space != NULL)
        *pgot_space = got_space;

    return esc_len;
}


/**
 * Escape a buffer.
 *
 * @param out_buf   Output buffer pointer; could be NULL.
 * @param in_buf    Input buffer pointer.
 * @param in_len    Input buffer length.
 *
 * @return Number of output characters.
 */
static size_t
escape_fmt_buf(char *out_buf, void *in_buf, size_t in_len)
{
    char   *in_p;
    char   *out_p;

    for (in_p = in_buf, out_p = out_buf; in_len > 0; in_p++, in_len--)
        out_p += escape_fmt_byte(out_p, *in_p);

    return (out_p - out_buf);
}


/**
 * Write a message field dump to a stream.
 *
 * @note Uses global scrap buffer.
 *
 * @param output    Stream to write to.
 * @param f         Message field to dump.
 *
 * @return True if dumped successfully, false otherwise.
 */
static te_bool
write_dump_field(FILE *output, const message_fld *f)
{
    te_bool     quoted;
    size_t      out_len;
    char       *p;

    assert(output != NULL);
    assert(message_fld_valid(f));

    /* Check if the string needs quoting and calculate output length */
    if (f->len > 0)
        out_len = escape_eval_buf(f->buf, f->len, &quoted);
    else
    {
        out_len = 0;
        quoted = TRUE;
    }
    out_len += quoted ? 3 : 1;

    /* Grow the scrap buffer if necessary */
    if (!scrap_grow(out_len))
        return FALSE;

    /* Output to the buffer */
    p = (char *)scrap_buf;
    *p++ = ' ';
    if (quoted)
        *p++ = '"';
    p += escape_fmt_buf(p, f->buf, f->len);
    if (quoted)
        *p = '"';

    /* Output to the file */
    return (fwrite(scrap_buf, out_len, 1, output) == 1);
}


/**
 * Write a message dump to a stream.
 *
 * @note Uses global scrap buffer.
 *
 * @param output    Stream to write to.
 * @param m         Message to dump.
 *
 * @return True if dumped successfully, false otherwise.
 */
static te_bool
write_dump(FILE *output, const message *m)
{
    size_t  i;

    assert(output != NULL);
    assert(message_valid(m));

    if (fprintf(output,
                "%" PRIu8 " %" PRIu32 ".%.6" PRIu32 " %u %u",
                m->version, m->ts_secs, m->ts_usecs,
                (unsigned int)m->level, (unsigned int)m->id) < 0)
        return FALSE;

    if (!write_dump_field(output, &m->entity_name) ||
        !write_dump_field(output, &m->user_name) ||
        !write_dump_field(output, &m->format))
        return FALSE;

    for (i = 0; i < m->arg_num; i++)
        if (!write_dump_field(output, &m->arg_list[i]))
            return FALSE;

    if (fputc('\n', output) == EOF)
        return FALSE;

    return TRUE;
}


static int
run(const char *input_name, const char *output_name)
{
    int                 result      = 1;
    FILE               *input       = NULL;
    void               *input_buf   = NULL;
    FILE               *output      = NULL;
    void               *output_buf  = NULL;
    te_log_version      version;
    off_t               offset;
    read_message_rc     read_rc;
    message             m;

    /* Initialize message structure */
    if (!message_init(&m))
    {
        ERROR("Failed to initialize a message: %s", strerror(errno));
        return result;
    }

    /* Open input */
    if (input_name[0] == '-' && input_name[1] == '\0')
        input = stdin;
    else
    {
        input = fopen(input_name, "r");
        if (input == NULL)
            ERROR_CLEANUP("Failed to open \"%s\": %s",
                          input_name, strerror(errno));
    }

    /* Set input buffer */
    input_buf = malloc(INPUT_BUF_SIZE);
    setvbuf(input, input_buf, _IOFBF, INPUT_BUF_SIZE);

    /* Open output */
    if (output_name[0] == '-' && output_name[1] == '\0')
        output = stdout;
    else
    {
        output = fopen(output_name, "w");
        if (output == NULL)
            ERROR_CLEANUP("Failed to open \"%s\": %s",
                          output_name, strerror(errno));
    }

    /* Set output buffer */
    output_buf = malloc(OUTPUT_BUF_SIZE);
    setvbuf(output, output_buf, _IOFBF, OUTPUT_BUF_SIZE);

    /* Read and verify log file version */
    if (fread(&version, sizeof(version), 1, input) != 1)
        ERROR_CLEANUP("Failed to read log file version: %s",
                      feof(input) ? "unexpected EOF" : strerror(errno));
    if (version != 1)
        ERROR_CLEANUP("Unsupported log file version %hhu", version);

    /* Output log file version */
    if (fprintf(output, "%" PRIu8 "\n", version) < 0)
        ERROR_CLEANUP("Failed to write log file version: %s",
                      strerror(errno));


    while (TRUE)
    {
        /* Retrieve current offset */
        /* Not checking the result - what could possibly go wrong? */
        offset = ftello(input);

        /* Read message */
        read_rc = read_message(input, &m);
        if (read_rc < READ_MESSAGE_RC_OK)
        {
            if (read_rc == READ_MESSAGE_RC_EOF)
                break;
            else if (read_rc == READ_MESSAGE_RC_WRONG_VER)
                ERROR("Message with unsupported version encountered "
                      "at %lld", (long long int)offset);
            else if (read_rc == READ_MESSAGE_RC_ERR)
            {
                int read_errno = errno;

                ERROR("Failed reading input message "
                      "(starting at %lld) at %lld: %s",
                      (long long int)offset,
                      (long long int)ftello(input),
                      feof(input)
                          ? "unexpected EOF"
                          : strerror(read_errno));
            }
            goto cleanup;
        }

        /* Write message dump */
        if (!write_dump(output, &m))
            ERROR_CLEANUP("Failed writing output: %s", strerror(errno));
    }

    if (fflush(output) != 0)
        ERROR_CLEANUP("Failed flushing output: %s", strerror(errno));

    result = 0;

cleanup:

    message_clnp(&m);
    if (output != NULL)
        fclose(output);
    free(output_buf);
    if (input != NULL)
        fclose(input);
    free(input_buf);
    scrap_clear();

    return result;
}


static int
usage(FILE *stream, const char *progname)
{
    return 
        fprintf(
            stream, 
            "Usage: %s [OPTION]... [INPUT_LOG [OUTPUT_DUMP]]\n"
            "Dump a TE log file to human-readable text format.\n"
            "\n"
            "With no INPUT_LOG, or when INPUT_LOG is -, read standard input.\n"
            "With no OUTPUT_DUMP, or when OUTPUT_DUMP is -, "
            "write standard output.\n"
            "\n"
            "Options:\n"
            "  -h, --help       this help message\n"
            "\n",
            progname);
}


typedef enum opt_val {
    OPT_VAL_HELP        = 'h',
} opt_val;


int
main(int argc, char * const argv[])
{
    static const struct option  long_opt_list[] = {
        {.name      = "help",
         .has_arg   = no_argument,
         .flag      = NULL,
         .val       = OPT_VAL_HELP},
        {.name      = NULL,
         .has_arg   = 0,
         .flag      = NULL,
         .val       = 0}
    };
    static const char          *short_opt_list = "h";

    int         c;
    const char *input_name  = "-";
    const char *output_name = "-";

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
    return run(input_name, output_name);
}


