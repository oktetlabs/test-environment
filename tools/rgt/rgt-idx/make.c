/** @file 
 * @brief Test Environment: RGT - log index creation utility
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
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <arpa/inet.h>

#include "te_defs.h"
#include "te_raw_log.h"

#include "common.h"

#define INPUT_BUF_SIZE  16384
#define OUTPUT_BUF_SIZE 16384

/**
 * Read a message timestamp from a stream, position the stream at next
 * message.
 *
 * @param input         The stream to read from.
 * @param pntimestamp   Location for the message timestamp in network order.
 *
 * @return Result code.
 */
static read_message_rc
read_message_ts(FILE *input, uint64_t *pntimestamp)
{
    static uint8_t  skip_buf[MAX(sizeof(te_log_level) + sizeof(te_log_id),
                                 4096)];

    te_log_version  ver;
    te_log_nfl      len;
    size_t          req_var_field_num;

    /* Read and verify log message version */
    if (fread(&ver, sizeof(ver), 1, input) != 1)
    {
        if (feof(input))
            return READ_MESSAGE_RC_EOF;
        return READ_MESSAGE_RC_ERR;
    }
    if (ver != TE_LOG_VERSION)
        return READ_MESSAGE_RC_WRONG_VER;

    /*
     * Read timestamp as a single 64 bit integer in the network byte order.
     */
    if (fread(pntimestamp, sizeof(*pntimestamp), 1, input) != 1)
        return READ_MESSAGE_RC_ERR;

    /* Skip level and ID */
    if (fread(&skip_buf,
              (sizeof(te_log_level) + sizeof(te_log_id)), 1, input) != 1)
        return READ_MESSAGE_RC_ERR;

    /*
     * Skip remaining required variable-length fields and optional format
     * arguments.
     */
    /* This includes entity name, user name and format specification */
    req_var_field_num = 3;
    while (TRUE)
    {
        if (fread(&len, sizeof(len), 1, input) != 1)
            return READ_MESSAGE_RC_ERR;
#if SIZEOF_TE_LOG_NFL == 4
        len = ntohl(len);
#elif SIZEOF_TE_LOG_NFL == 2
        len = ntohs(len);
#elif SIZEOF_TE_LOG_NFL != 1
#error Unexpected value of SIZEOF_TE_LOG_NFL
#endif

        /* If it is a required field */
        if (req_var_field_num > 0)
            req_var_field_num--;
        /* Else if it is a special (terminating) field length */
        else if (len == TE_LOG_RAW_EOR_LEN)
            break;

        /* While the remaining contents is bigger than the skip buffer */
        for (; len > sizeof(skip_buf); len -= sizeof(skip_buf))
            /* Read a portion the size of the skip buffer */
            if (fread(skip_buf, sizeof(skip_buf), 1, input) != 1)
                return READ_MESSAGE_RC_ERR;

        /* Read the tail smaller than the skip buffer */
        if (len > 0 && fread(skip_buf, len, 1, input) != 1)
            return READ_MESSAGE_RC_ERR;
    }

    return READ_MESSAGE_RC_OK;
}


/**
 * Write an index entry to a stream.
 *
 * @param output        The stream to write to.
 * @param offset        Message offset in the host byte order.
 * @param ntimestamp    Message timestamp in the network byte order.
 *
 * @return TRUE if the entry was written successfully, FALSE otherwise.
 */
static te_bool
write_entry(FILE *output, uint64_t offset, uint64_t ntimestamp)
{
    uint8_t buf[sizeof(offset) + sizeof(ntimestamp)];
    ssize_t i;

    memcpy(buf + sizeof(offset), &ntimestamp, sizeof(ntimestamp));
    for (i = sizeof(offset) - 1; i >= 0; i--, offset >>= 8)
        buf[i] = offset & 0xFF;

    if (fwrite(&buf, sizeof(buf), 1, output) != 1)
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
    uint8_t             version;
    read_message_rc     read_rc;
    off_t               offset;
    uint64_t            ntimestamp;

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

    while (TRUE)
    {
        /* Retrieve current offset */
        /* Not checking the result - what could possibly go wrong? */
        offset = ftello(input);

        /* Read message timestamp */
        read_rc = read_message_ts(input, &ntimestamp);
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

        /* Output index entry */
        if (!write_entry(output, offset, ntimestamp))
            ERROR_CLEANUP("Failed writing output: %s", strerror(errno));
    }

    if (fflush(output) != 0)
        ERROR_CLEANUP("Failed flushing output: %s", strerror(errno));

    result = 0;

cleanup:

    if (output != NULL)
        fclose(output);
    free(output_buf);
    if (input != NULL)
        fclose(input);
    free(input_buf);

    return result;
}


static int
usage(FILE *stream, const char *progname)
{
    return 
        fprintf(
            stream, 
            "Usage: %s [OPTION]... [INPUT_LOG [OUTPUT_INDEX]]\n"
            "Generate a timestamp index of a TE log file.\n"
            "\n"
            "With no INPUT_LOG, or when INPUT_LOG is -, read standard input.\n"
            "With no OUTPUT_INDEX, or when OUTPUT_INDEX is -, "
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


