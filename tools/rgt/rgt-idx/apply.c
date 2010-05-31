/** @file 
 * @brief Test Environment: RGT - log index application utility
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

#include "common.h"

#define INDEX_BUF_SIZE  16384
#define INPUT_BUF_SIZE  4096
#define OUTPUT_BUF_SIZE 16384

#define MIN_BUF_SIZE    4096

/**
 * Grow a buffer to accomodate specified contents length.
 *
 * @param pbuf  Location of/for the buffer pointer.
 * @param psize Location of/for the buffer size.
 * @param len   Contents length to fit into the buffer.
 *
 * @return TRUE if grown successfully, FALSE otherwise
 */
static te_bool
grow_buf(uint8_t **pbuf, size_t *psize, size_t len)
{
    uint8_t    *buf     = *pbuf;
    size_t      size    = *psize;

    if (len <= size)
        return TRUE;

    if (size < MIN_BUF_SIZE)
        size = MIN_BUF_SIZE;

    while (size < len)
        size *= 2;

    buf = realloc(buf, size);
    if (buf == NULL)
        return FALSE;

    *pbuf = buf;
    *psize = size;

    return TRUE;
}


static read_message_rc
read_message(FILE *input, uint8_t **pbuf, size_t *psize, size_t *plen)
{
    te_log_version  ver;
    size_t          pos;
    size_t          req_var_field_num;
    te_log_nfl      flen;

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
     * Grow the buffer to accomodate fixed message part plus next field
     * length.
     */
    *plen = 1 +                     /* Version */
            8 +                     /* Timestamp */
            sizeof(te_log_level) +  /* Level */
            sizeof(te_log_id) +     /* ID */
            sizeof(flen);           /* entity name length */
    if (!grow_buf(pbuf, psize, *plen))
        return READ_MESSAGE_RC_ERR;

    /* Start output */
    pos = 0;

    /* Output version */
    (*pbuf)[pos++] = ver;

    /* Read timestamp, level, ID and entity name length */
    if (fread(*pbuf + pos, *plen - pos, 1, input) != 1)
        return READ_MESSAGE_RC_ERR;
    pos = *plen;

    /* This includes entity name, user name and format specification */
    req_var_field_num = 3;
    while (TRUE)
    {
        /* Extract next field length */
#if SIZEOF_TE_LOG_NFL == 4
        flen = ntohl(*(te_log_nfl *)(*pbuf + pos - sizeof(te_log_nfl)));
#elif SIZEOF_TE_LOG_NFL == 2
        flen = ntohs(*(te_log_nfl *)(*pbuf + pos - sizeof(te_log_nfl)));
#elif SIZEOF_TE_LOG_NFL != 1
#error Unexpected value of SIZEOF_TE_LOG_NFL
#endif
        /* If it is a required field */
        if (req_var_field_num > 0)
            req_var_field_num--;
        /* Else if it is a special (terminating) field length */
        else if (flen == TE_LOG_RAW_EOR_LEN)
            break;

        /* Read field contents and next field's length */
        *plen += flen + sizeof(te_log_nfl);
        if (!grow_buf(pbuf, psize, *plen))
            return READ_MESSAGE_RC_ERR;
        if (fread(*pbuf + pos, *plen - pos, 1, input) != 1)
            return READ_MESSAGE_RC_ERR;
        pos = *plen;
    }

    return READ_MESSAGE_RC_OK;
}


/**
 * Read an index entry offset from a stream, position the stream at the next
 * entry.
 *
 * @param index     The stream to read from.
 * @param poffset   Location for the offset.
 *
 * @return TRUE if the entry was read successfully, FALSE if an error
 *         occurred or EOF was reached.
 */
static te_bool
read_entry_offset(FILE *index, uint64_t *poffset)
{
    uint64_t    offset;
    uint8_t     buf[sizeof(offset) + sizeof(uint64_t)];
    ssize_t     i;

    if (fread(&buf, sizeof(buf), 1, index) != 1)
        return FALSE;

    for (offset = 0, i = 0; i <= 7; i++)
        offset = (offset << 8) | buf[i];

    *poffset = offset;

    return TRUE;
}


static int
run(const char *input_name, const char *index_name, const char *output_name)
{
    int                 result      = 1;
    FILE               *input       = NULL;
    void               *input_buf   = NULL;
    FILE               *index       = NULL;
    void               *index_buf   = NULL;
    FILE               *output      = NULL;
    void               *output_buf  = NULL;
    uint8_t             version;
    uint64_t            offset;
    uint8_t            *buf         = NULL;
    size_t              size        = 0;
    size_t              len;
    read_message_rc     read_rc;

    /* Open input */
    input = fopen(input_name, "r");
    if (input == NULL)
        ERROR_CLEANUP("Failed to open \"%s\": %s",
                      input_name, strerror(errno));

    /* Set input buffer */
    input_buf = malloc(INPUT_BUF_SIZE);
    setvbuf(input, input_buf, _IOFBF, INPUT_BUF_SIZE);


    /* Open index */
    if (index_name[0] == '-' && index_name[1] == '\0')
        index = stdin;
    else
    {
        index = fopen(index_name, "r");
        if (index == NULL)
            ERROR_CLEANUP("Failed to open \"%s\": %s",
                          index_name, strerror(errno));
    }

    /* Set index buffer */
    index_buf = malloc(INDEX_BUF_SIZE);
    setvbuf(index, index_buf, _IOFBF, INDEX_BUF_SIZE);

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

    /* Write log file version to the output */
    if (fwrite(&version, sizeof(version), 1, output) != 1)
        ERROR_CLEANUP("Failed to write log file version to the output: %s",
                      strerror(errno));

    while (TRUE)
    {
        /* Read an index entry offset */
        if (!read_entry_offset(index, &offset))
        {
            if (feof(index))
                break;
            else
                ERROR_CLEANUP("Failed reading index entry: %s",
                              strerror(errno));
        }
        /* Check offset range */
        if (offset > OFF_T_MAX)
            ERROR_CLEANUP("Index entry contains "
                          "unsupported offset %" PRIu64, offset);

        /* Seek the input log */
        if (fseeko(input, (off_t)offset, SEEK_SET) != 0)
            ERROR_CLEANUP("Failed to seek to input position "
                          "%" PRIu64 ": %s", offset, strerror(errno));

        /* Read a message from the input */
        read_rc = read_message(input, &buf, &size, &len);
        if (read_rc < READ_MESSAGE_RC_OK)
        {
            if (read_rc == READ_MESSAGE_RC_WRONG_VER)
                ERROR("Message with unsupported version encountered "
                      "at position %" PRIu64, offset);
            else
            {
                int read_errno = errno;

                ERROR("Failed reading input message "
                      "(starting at %" PRIu64 ") at %" PRId64 ": %s",
                      offset, (int64_t)ftello(input),
                      read_rc == READ_MESSAGE_RC_EOF
                          ? "unexpected EOF"
                          : strerror(read_errno));
            }
            goto cleanup;
        }

        /* Write the message to the output */
        if (fwrite(buf, len, 1, output) != 1)
            ERROR_CLEANUP("Failed writing message to the output: %s",
                          strerror(errno));
    }

    if (fflush(output) != 0)
        ERROR_CLEANUP("Failed flushing output: %s", strerror(errno));

    result = 0;

cleanup:

    free(buf);
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
            "Usage: %s [OPTION]... INPUT_LOG [INPUT_INDEX [OUTPUT_LOG]]\n"
            "Apply a log index to a TE log, "
            "outputting it in the index order.\n"
            "\n"
            "With no INPUT_INDEX, or when INPUT_INDEX is -, "
            "read standard input.\n"
            "With no OUTPUT_LOG, or when OUTPUT_LOG is -, "
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
    const char *input_name  = NULL;
    const char *index_name  = "-";
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
            index_name = argv[optind++];
            if (optind < argc)
            {
                output_name = argv[optind++];
                if (optind < argc)
                    ERROR_USAGE_RETURN("Too many arguments");
            }
        }
    }

    /*
     * Verify command line arguments
     */
    if (input_name == NULL)
        ERROR_USAGE_RETURN("Input log is not specified");
    if (*input_name == '\0')
        ERROR_USAGE_RETURN("Empty input log file name");
    if (*index_name == '\0')
        ERROR_USAGE_RETURN("Empty index file name");
    if (*output_name == '\0')
        ERROR_USAGE_RETURN("Empty output log file name");

    /*
     * Run
     */
    return run(input_name, index_name, output_name);
}


