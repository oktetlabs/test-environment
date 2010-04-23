/** @file 
 * @brief Test Environment: RGT - log index faking utility
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

#define OUTPUT_BUF_SIZE 16384

/**
 * Write an index entry to a stream.
 *
 * @param output        The stream to write to.
 * @param offset        Message offset.
 * @param timestamp     Message timestamp.
 *
 * @return TRUE if the entry was written successfully, FALSE otherwise.
 */
static te_bool
write_entry(FILE *output, uint64_t offset, uint64_t timestamp)
{
    uint8_t buf[sizeof(offset) + sizeof(timestamp)];
    ssize_t i;

    for (i = sizeof(buf) - 1; i >= 0; i--, timestamp >>= 8)
        buf[i] = timestamp & 0xFF;
    for (i = sizeof(offset) - 1; i >= 0; i--, offset >>= 8)
        buf[i] = offset & 0xFF;

    if (fwrite(&buf, sizeof(buf), 1, output) != 1)
        return FALSE;

    return TRUE;
}


/** Output order */
enum order {
    ORDER_EQ,
    ORDER_INC,
    ORDER_DEC,
    ORDER_RAND
};


/**
 * First timestamp generation function prototype.
 *
 * @param length    Timestamp sequence length.
 *
 * @return First timestamp.
 */
typedef uint64_t ts_gen_first_fn(uint64_t length);

/**
 * Next timestamp generation function prototype.
 *
 * @param prev  Previous timestamp.
 *
 * @return Next timestamp.
 */
typedef uint64_t ts_gen_next_fn(uint64_t prev);

/** Timestamp generator */
typedef struct ts_gen {
    ts_gen_first_fn    *first;
    ts_gen_next_fn     *next;
} ts_gen;

uint64_t
ts_gen_eq_first(uint64_t length)
{
    (void)length;
    return 0;
}

uint64_t
ts_gen_eq_next(uint64_t prev)
{
    (void)prev;
    return 0;
}


uint64_t
ts_gen_inc_first(uint64_t length)
{
    (void)length;
    return 0;
}

uint64_t
ts_gen_inc_next(uint64_t prev)
{
    return prev + 1;
}


uint64_t
ts_gen_dec_first(uint64_t length)
{
    return length - 1;
}

uint64_t
ts_gen_dec_next(uint64_t prev)
{
    return prev - 1;
}


uint64_t
ts_gen_rand_first(uint64_t length)
{
    (void)length;
    return (uint64_t)random() << 32 | (uint64_t)random();
}

uint64_t
ts_gen_rand_next(uint64_t prev)
{
    (void)prev;
    return (uint64_t)random() << 32 | (uint64_t)random();
}


ts_gen  gen_list[] = {
#define GEN(_NAME, _name) \
    [ORDER_##_NAME] = {.first   = ts_gen_##_name##_first,   \
                       .next    = ts_gen_##_name##_next}
    GEN(EQ, eq),
    GEN(INC, inc),
    GEN(DEC, dec),
    GEN(RAND, rand),
};

static int
run(const char *output_name, uint64_t length,
    enum order order, unsigned int seed)
{
    int                 result      = 1;
    ts_gen_first_fn    *gen_first   = gen_list[order].first;
    ts_gen_next_fn     *gen_next    = gen_list[order].next;
    FILE               *output      = NULL;
    void               *output_buf  = NULL;
    uint64_t            offset;
    uint64_t            timestamp;

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

    /*
     * Generate
     */
    srandom(seed);
    for (offset = 0, timestamp = (*gen_first)(length);
         length > 0;
         offset++, timestamp = (*gen_next)(timestamp), length--)
        if (!write_entry(output, offset, timestamp))
            ERROR_CLEANUP("Failed writing an entry: %s", strerror(errno));

    /* Flush output */
    if (fflush(output) != 0)
        ERROR_CLEANUP("Failed flushing output: %s", strerror(errno));

    result = 0;

cleanup:

    if (output != NULL)
        fclose(output);
    free(output_buf);

    return result;
}


static int
usage(FILE *stream, const char *progname)
{
    return 
        fprintf(
            stream, 
            "Usage: %s [OPTION]... [OUTPUT_INDEX]\n"
            "Fake a timestamp index of a TE log file.\n"
            "\n"
            "With no OUTPUT, or when OUTPUT is -, write standard output.\n"
            "\n"
            "Options:\n"
            "  -h, --help           this help message\n"
            "  -l, --length=NUM     specify output length in entries\n"
            "  -o, --order=STRING   specify output order "
                                    "(eq|inc|dec|rand)\n"
            "  -s, --seed=NUM       specify seed for random order output\n"
            "\n"
            "The default options are -l 16 -o inc -s 1.\n"
            "\n",
            progname);
}


typedef enum opt_val {
    OPT_VAL_HELP        = 'h',
    OPT_VAL_LENGTH      = 'l',
    OPT_VAL_ORDER       = 'o',
    OPT_VAL_SEED        = 's',
} opt_val;


int
main(int argc, char * const argv[])
{
    static const struct option  long_opt_list[] = {
        {.name      = "help",
         .has_arg   = no_argument,
         .flag      = NULL,
         .val       = OPT_VAL_HELP},
        {.name      = "length",
         .has_arg   = required_argument,
         .flag      = NULL,
         .val       = OPT_VAL_LENGTH},
        {.name      = "order",
         .has_arg   = required_argument,
         .flag      = NULL,
         .val       = OPT_VAL_ORDER},
        {.name      = "seed",
         .has_arg   = required_argument,
         .flag      = NULL,
         .val       = OPT_VAL_SEED},
    };
    static const char          *short_opt_list = "hl:o:s:";

    int             c;
    uint64_t        length      = 16;
    enum order      order       = ORDER_INC;
    unsigned int    seed        = 1;
    const char     *output_name = "-";

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
            case OPT_VAL_LENGTH:
                length = strtoull(optarg, NULL, 0);
                break;
            case OPT_VAL_ORDER:
#define ORDER_IF_ELSE(_NAME) \
    if (strcasecmp(optarg, #_NAME) == 0)    \
        order = ORDER_##_NAME;              \
    else
                ORDER_IF_ELSE(EQ)
                ORDER_IF_ELSE(INC)
                ORDER_IF_ELSE(DEC)
                ORDER_IF_ELSE(RAND)
                    ERROR_USAGE_RETURN("Unknown order \"%s\"", optarg);
#undef ORDER_IF_ELSE
                break;
            case OPT_VAL_SEED:
                seed = strtol(optarg, NULL, 0);
                break;
            case '?':
                usage(stderr, program_invocation_short_name);
                return 1;
                break;
        }
    }

    if (optind < argc)
    {
        output_name = argv[optind++];
        if (optind < argc)
                ERROR_USAGE_RETURN("Too many arguments");
    }

    /*
     * Verify command line arguments
     */
    if (*output_name == '\0')
        ERROR_USAGE_RETURN("Empty output file name");

    /*
     * Run
     */
    return run(output_name, length, order, seed);
}



