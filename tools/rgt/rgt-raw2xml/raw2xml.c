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

/* Taken from lua.c */
static int
traceback(lua_State *L)
{
    /* 'message' not a string? */
    if (!lua_isstring(L, 1))
        /* Keep it intact */
        return 1;

    lua_getglobal(L, "debug");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return 1;
    }

    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        return 1;
    }

    /* Pass error message */
    lua_pushvalue(L, 1);
    /* Skip this function and traceback */
    lua_pushinteger(L, 2);
    /* Call debug.traceback */
    lua_call(L, 2, 1);

    return 1;
}

#define LUA_PCALL_CLEANUP(_nargs, _nresults) \
    do {                                                            \
        if (lua_pcall(L, _nargs, _nresults, traceback_idx) != 0)    \
            ERROR_CLEANUP("%s", lua_tostring(L, -1, &len));         \
    } while (0)


/** Message reading result code */
typedef enum read_message_rc {
    READ_MESSAGE_RC_ERR         = -1,   /**< A reading error occurred or
                                             unexpected EOF was reached */
    READ_MESSAGE_RC_EOF         = 0,    /**< The EOF was encountered instead
                                             of the message */
    READ_MESSAGE_RC_OK          = 1,    /**< The message was read
                                              successfully */
} read_message_rc;


#if SIZEOF_TE_LOG_NFL == 4
#define LENTOH(_len) ntohl(len)
#elif SIZEOF_TE_LOG_NFL == 2
#define LENTOH(_len) ntohs(len)
#elif SIZEOF_TE_LOG_NFL != 1
#error Unexpected value of SIZEOF_TE_LOG_NFL
#endif

static read_message_rc
read_arg(FILE *input, void **pbuf, size_t *plen)
{
    read_message_rc     result  = READ_MESSAGE_RC_ERR;
    void               *buf     = NULL;
    te_log_nfl          len;

    /* Read and convert the field length */
    if (fread(&len, sizeof(len), 1, input) != 1)
        goto cleanup;
    len = LENTOH(len);
    if (len == TE_LOG_RAW_EOR_LEN)
    {
        result = READ_MESSAGE_RC_EOF;
        goto cleanup;
    }
    /* Allocate the buffer */
    buf = malloc(len);
    if (buf == NULL)
        goto cleanup;
    /* Read the field */
    if (fread(buf, len, 1, input) != 1)
        goto cleanup;

    if (pbuf != NULL)
    {
        *pbuf = buf;
        buf = NULL;
    }

    if (plen != NULL)
        *plen = len;

    result = READ_MESSAGE_RC_OK;

cleanup:

    free(buf);

    return result;
}


/**
 * Read and format a message text.
 */
static read_message_rc
read_text(FILE *input, const char *fmt, size_t len, lua_State *L)
{
    read_message_rc     result      = READ_MESAGE_RC_ERR;
    luaL_Buffer         buf;
    const char         *prevp;
    const char         *p;
    read_message_rc     arg_rc;
    void               *arg_buf     = NULL;
    size_t              arg_len;

    luaL_buffinit(L, &buf);

    prevp = fmt;
    p = fmt;

    for (; len > 0 && *p != '%'; p++, len--);
    if (p > prevp)
        luaL_addlstring(&buf, prevp, p - prevp);
    p++;

    /* If it is an escaped '%' */
    if (*p == '%')
    {
        prevp = p;
        continue;
    }

    /* If it is an incomplete format specification */
    if (*p == '\0')
    {
        p--
        goto finish;
    }

#define READ_ARG \
    do {                                            \
        read_message_rc rc;                         \
                                                    \
        rc = read_arg(input, &arg_buf, &arg_len);   \
        if (rc == READ_MESSAGE_RC_ERR)              \
            goto cleanup;                           \
        else if (rc == READ_MESSAGE_RC_EOF)         \
            goto finish;                            \
    } while (0)

    switch (*p)
    {
        case 'c':
            {
                uint32_t    val;

                READ_ARG;
                if (arg_len != sizeof(val))
                    ERROR_CLEANUP("Invalid %%c format argument");
                val = ntohl(*(uint32_t *)arg_buf);
                luaL_addchar(&buf, (char)val);
                free(buf);
                buf = NULL;
            }
            break;

        case 's':
            {
                READ_ARG;
                luaL_addstring(&buf, (const char *)arg_buf);
                free(buf);
                buf = NULL;
            }

        case 'd':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
    }

#undef READ_ARG


    luaL_pushresult(&buf);

cleanup:

    free(arg_buf);

    return result;
}

/**
 * Read a message from a stream and place its parts on a Lua stack.
 *
 * @param input         The stream to read from.
 * @param L             Lua state to put the message parts to.
 * @param traceback_idx Error traceback function stack index.
 * @param ts_class_idx  Timestamp class stack index.
 *
 * @return Result code.
 */
static read_message_rc
read_message(FILE *input, lua_State *L, int traceback_idx, int ts_class_idx)
{
    read_message_rc     result      = READ_MESSAGE_RC_ERR;
    te_log_version      version;
    te_log_ts_sec       ts_secs;
    te_log_ts_usec      ts_usecs;
    te_log_level        level;
    const char         *str;
    te_log_nfl          len;
    uint8_t            *buf;

    /*
     * Transfer version
     */
    /* Read and verify log message version */
    if (fread(&version, sizeof(version), 1, input) != 1)
    {
        if (feof(input))
            result = READ_MESSAGE_RC_EOF;
        goto cleanup;
    }
    if (version != TE_LOG_VERSION)
    {
        errno = EINVAL;
        ERROR_CLEANUP("Unknown log message version %u", version);
    }
    /* Push version */
    lua_pushnumber(L, ver);

    /*
     * Transfer timestamp
     */
    /* Read the timestamp fields */
    if (fread(&ts_secs, sizeof(ts_secs), 1, input) != 1 ||
        fread(&ts_usecs, sizeof(ts_usecs), 1, input) != 1)
        goto cleanup;
    ts_secs = ntohl(ts_secs);
    ts_usecs = ntohl(ts_usecs);
    /* Create timestamp instance */
    lua_pushvalue(L, ts_class_idx);
    lua_pushnumber(ts_secs);
    lua_pushnumber(ts_usecs);
    LUA_PCALL_CLEANUP(2, 1);

    /*
     * Transfer level
     */
    if (fread(level, sizeof(level), 1, input) != 1)
        goto cleanup;
#if SIZEOF_TE_LOG_LEVEL == 4
    level = ntohl(level);
#elif SIZEOF_TE_LOG_LEVEL == 2
    level = ntohs(level);
#elif SIZEOF_TE_LOG_LEVEL != 1
#error Unexpected value of SIZEOF_TE_LOG_LEVEL
#endif
    str = te_log_level2str(level);
    if (str == NULL)
    {
        errno = EINVAL;
        ERROR_CLEANUP("Unknown log level %u", level);
    }
    lua_pushstring(L, str);

    /*
     * Transfer ID
     */
    if (fread(id, sizeof(id), 1, input) != 1)
        goto cleanup;
#if SIZEOF_TE_LOG_ID == 4
    id = ntohl(id);
#elif SIZEOF_TE_LOG_ID == 2
    id = ntohs(id);
#elif SIZEOF_TE_LOG_ID != 1
#error Unexpected value of SIZEOF_TE_LOG_ID
#endif
    lua_pushnumber(L, id);

#define READ_FLD \
    do {                                                \
        /* Read and convert the field length */         \
        if (fread(&len, sizeof(len), 1, input) != 1)    \
            goto cleanup;                               \
        len = LENTOH(len);                              \
        /* Allocate the buffer */                       \
        buf = malloc(len);                              \
        if (buf == NULL)                                \
            goto cleanup;                               \
        /* Read the field */                            \
        if (fread(buf, len, 1, input) != 1)             \
            goto cleanup;                               \
    } while (0)

    /*
     * Transfer entity
     */
    READ_FLD;
    lua_pushlstring(L, buf, len);
    free(buf);
    buf = NULL;

    /*
     * Transfer user
     */
    READ_FLD;
    lua_pushlstring(L, buf, len);
    free(buf);
    buf = NULL;

    /*
     * Transfer text
     */
    READ_FLD;
    result = read_text(input, buf, len, L);
    free(buf);
    buf = NULL;

#undef READ_FLD

cleanup:

    free(buf);

    return result;
}


static te_bool
run_input_and_output(FILE *input,
                     lua_State *L, int traceback_idx,
                     int ts_class_idx, int msg_class idx
                     int sink_idx, int sink_put_idx)
{
    te_bool             result      = FALSE;
    off_t               offset;
    read_message_rc     read_rc;

    while (TRUE)
    {
        /* Copy "put" sink instance method to the top */
        lua_pushvalue(L, sink_put_idx);
        /* Copy sink instance to the top */
        lua_pushvalue(L, sink_idx);
        /* Copy rgt.msg to the top */
        lua_pushvalue(L, msg_class_idx);

        /* Retrieve current offset */
        offset = ftello(input);

        /* Call read_message to supply the arguments */
        read_rc = read_message(input, L, traceback_idx, ts_class_idx);
        if (read_rc < READ_MESSAGE_RC_OK)
        {
            if (read_rc == READ_MESSAGE_RC_EOF)
                break;
            else
            {
                int read_errno = errno;

                ERROR_CLEANUP("Failed reading input message "
                              "(starting at %lld) at %lld: %s",
                              (long long int)offset,
                              (long long int)ftello(input),
                              feof(input)
                                  ? "unexpected EOF"
                                  : strerror(read_errno));
            }
        }

        /* Call rgt.msg with the arguments to create message instance */
        LUA_PCALL_CLEANUP(7, 1);

        /* Call "put" sink instance method to feed the message */
        LUA_PCALL_CLEANUP(2, 0);
    }

    result = TRUE;

cleanup:

    return result;
}


static te_bool
run_input(FILE *input, const char *output_name, unsigned long max_mem)
{
    te_bool     result          = FALSE;
    lua_State  *L               = NULL;
    int         traceback_idx;
    int         ts_class_idx;
    int         msg_class_idx;
    int         sink_idx;
    int         sink_put_idx;

    /*
     * Setup Lua
     */
    /* Create Lua instance */
    L = luaL_newstate();
    if (L == NULL)
        ERROR_CLEANUP("Failed to create Lua state");

    /* Push traceback function */
    lua_pushcfunction(L, traceback);
    traceback_idx = lua_gettop(L);

    /* Require rgt.ts */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "rgt.ts");
    LUA_PCALL_CLEANUP(1, 1);
    ts_class_idx = lua_gettop(L);

    /* Require rgt.msg */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "rgt.msg");
    LUA_PCALL_CLEANUP(1, 1);
    msg_class_idx = lua_gettop(L);

    /*
     * Create sink instance
     */
    /* Call rgt.sink(max_mem) to create sink instance */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "rgt.sink");
    LUA_PCALL_CLEANUP(1, 1);
    lua_pushnumber((lua_Number)max_mem * 1024 * 1024);
    LUA_PCALL_CLEANUP(2, 1);
    sink_idx = lua_gettop(L);

    /*
     * Supply sink instance with the output file
     */
    /* Retrieve sink instance "take_file" method */
    lua_getfield(L, sink_idx, "take_file");
    /* Copy sink instance to the top */
    lua_pushvalue(L, sink_idx);

    /* Open/push output file */
    lua_getglobal(L, "io");
    if (output_name[0] == '-' && output_name[1] == '\0')
        lua_getfield(L, -1, "stdout");
    else
    {
        lua_getfield(L, -1, "open");
        lua_pushstring(L, input_name);
        lua_pushliteral(L, "w");
        LUA_PCALL_CLEANUP(2, 1);
    }
    lua_remove(L, -2);

    /* Call "take_file" instance method to supply the sink with the file */
    LUA_PCALL_CLEANUP(2, 0);

    /*
     * Start sink output
     */
    /* Retrieve sink instance "start" method */
    lua_getfield(L, sink_idx, "start");
    /* Copy sink instance to the top */
    lua_pushvalue(L, sink_idx);

    /* Call "start" instance method */
    LUA_PCALL_CLEANUP(1, 0);

    /* Retrieve sink instance "put" method */
    lua_getfield(L, sink_idx, "put");
    sink_put_idx = lua_gettop(L);

    if (!run_input_and_output(input, L, traceback_idx,
                              ts_class_idx, msg_class_idx,
                              sink_idx, sink_put_idx))
        goto cleanup;

    /*
     * Finish sink output
     */
    /* Retrieve sink instance "finish" method */
    lua_getfield(L, sink_idx, "finish");
    /* Copy sink instance to the top */
    lua_pushvalue(L, sink_idx);
    /* Call sink instance "finish" method */
    LUA_PCALL_CLEANUP(1, 0);

    /*
     * Take the output file from the sink
     */
    /* Retrieve sink instance "yield_file" method */
    lua_getfield(L, sink_idx, "yield_file");
    /* Copy sink instance to the top */
    lua_pushvalue(L, sink_idx);
    /* Call sink instance "yield_file" method */
    LUA_PCALL_CLEANUP(1, 1);

    /*
     * Close the output
     */
    /* Retrieve file "close" method */
    lua_getfield(L, -1, "close");
    /* Copy file to the top */
    lua_pushvalue(L, -2);
    /* Call file "close" method */
    LUA_PCALL_CLEANUP(2, 0);

    result = TRUE;

cleanup:

    if (L != NULL)
        lua_close(L);

    return result;
}


static int
run(const char *input_name, const char *output_name, unsigned long max_mem)
{
    int                 result          = 1;
    FILE               *input           = NULL;
    void               *input_buf       = NULL;
    te_log_version      version;

    /*
     * Setup input
     */
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

    /* Read and verify log file version */
    if (fread(&version, sizeof(version), 1, input) != 1)
        ERROR_CLEANUP("Failed to read log file version: %s",
                      feof(input) ? "unexpected EOF" : strerror(errno));
    if (version != 1)
        ERROR_CLEANUP("Unsupported log file version %hhu", version);

    if (!run_input(input, output_name, max_mem))

    /*
     * Close the input
     */
    fclose(input);

cleanup:

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
    return run(input_name, output_name, max_mem);
}


