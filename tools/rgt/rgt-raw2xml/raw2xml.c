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
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_raw_log.h"
#include "logger_defs.h"

#include "lua_rgt_msg.h"

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

#define SCRAP_MIN_SIZE  16384

static void    *scrap_buf   = NULL;
static size_t   scrap_size  = 0;

te_bool
scrap_grow(size_t size)
{
    size_t  new_scrap_size  = scrap_size;
    void   *new_scrap_buf;

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

#define SCRAP_CHECK_GROW(size) \
    (((size) <= scrap_size) ? TRUE : scrap_grow(size))

void
scrap_clear(void)
{
    free(scrap_buf);
    scrap_size = 0;
}


/** Message reading result code */
typedef enum read_message_rc {
    READ_MESSAGE_RC_ERR         = -1,   /**< A reading error occurred or
                                             unexpected EOF was reached */
    READ_MESSAGE_RC_EOF         = 0,    /**< The EOF was encountered instead
                                             of the message */
    READ_MESSAGE_RC_OK          = 1,    /**< The message was read
                                              successfully */
} read_message_rc;


/**
 * Read message variable length fields from a stream and place them into the
 * scrap buffer.
 *
 * @param input The stream to read from.
 * @param msg   The message to output field references to.
 *
 * @return Result code.
 */
static read_message_rc
read_message_flds(FILE *input, rgt_msg *msg)
{
    static size_t       entity_pos;
    static size_t       user_pos;
    static size_t       fmt_pos;
    static size_t       args_pos;
    static size_t      *fld_pos[]   = {&entity_pos, &user_pos,
                                       &fmt_pos, &args_pos};
    size_t              fld_idx     = 0;

    size_t              size        = 0;
    te_log_nfl          len;
    te_log_nfl          buf_len;
    size_t              fld_size;
    size_t              remainder;
    size_t              new_size;
    rgt_msg_fld        *fld;

    do {
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

        /* If it is an optional field and it is the terminating length */
        if (fld_idx >= (sizeof(fld_pos) / sizeof(*fld_pos) - 1) &&
            len == TE_LOG_RAW_EOR_LEN)
            buf_len = 0;
        else
            buf_len = len;

        /* Calculate field size, with alignment */
        fld_size = sizeof(*fld) + buf_len;
        remainder = fld_size % sizeof(*fld);
        if (remainder != 0)
            fld_size += sizeof(*fld) - remainder;

        /* Calculate new size and grow the buffer */
        new_size += size + fld_size;
        if (!SCRAP_CHECK_GROW(new_size))
            return READ_MESSAGE_RC_ERR;

        /* If it is a directly referenced field */
        if (fld_idx < sizeof(fld_pos) / sizeof(*fld_pos))
            /* Output the field position */
            *fld_pos[fld_idx] = size;

        /* Calculate field pointer */
        fld = (rgt_msg_fld *)((char *)scrap_buf + size);

        /* Fill-in and read-in the field */
        fld->size = fld_size;
        fld->len = len;
        if (buf_len > 0 && fread(fld->buf, buf_len, 1, input) != 1)
            return READ_MESSAGE_RC_ERR;

        /* Seal the field */
        size = new_size;
    } while (
         /* While it is a mandatory field or not a terminating length */
         fld_idx++ < (sizeof(fld_pos) / sizeof(*fld_pos) - 1) ||
         len != TE_LOG_RAW_EOR_LEN);

    /*
     * Output field pointers
     */
    msg->entity = (rgt_msg_fld *)((char *)scrap_buf + entity_pos);
    msg->user   = (rgt_msg_fld *)((char *)scrap_buf + user_pos);
    msg->fmt    = (rgt_msg_fld *)((char *)scrap_buf + fmt_pos);
    msg->args   = (rgt_msg_fld *)((char *)scrap_buf + args_pos);

    return READ_MESSAGE_RC_OK;
}


/**
 * Read a message from a stream and place variable length fields into the
 * scrap buffer.
 *
 * @param input The stream to read from.
 * @param msg   The message to output to.
 *
 * @return Result code.
 */
static read_message_rc
read_message(FILE *input, rgt_msg *msg)
{
    te_log_version      version;
    te_log_ts_sec       ts_secs;
    te_log_ts_usec      ts_usecs;
    te_log_level        level;
    te_log_id           id;

    /* Read and verify log message version */
    if (fread(&version, sizeof(version), 1, input) != 1)
        return feof(input) ? READ_MESSAGE_RC_EOF : READ_MESSAGE_RC_ERR;
    if (version != TE_LOG_VERSION)
    {
        errno = EINVAL;
        ERROR("Unknown log message version %u", version);
        return READ_MESSAGE_RC_ERR;
    }

    /*
     * Transfer timestamp
     */
    /* Read the timestamp fields */
    if (fread(&ts_secs, sizeof(ts_secs), 1, input) != 1 ||
        fread(&ts_usecs, sizeof(ts_usecs), 1, input) != 1)
        return READ_MESSAGE_RC_ERR;
    msg->ts_secs = ntohl(ts_secs);
    msg->ts_usecs = ntohl(ts_usecs);

    /*
     * Transfer level
     */
    if (fread(&level, sizeof(level), 1, input) != 1)
        return READ_MESSAGE_RC_ERR;
#if SIZEOF_TE_LOG_LEVEL == 4
    msg->level = ntohl(level);
#elif SIZEOF_TE_LOG_LEVEL == 2
    msg->level = ntohs(level);
#elif SIZEOF_TE_LOG_LEVEL != 1
#error Unexpected value of SIZEOF_TE_LOG_LEVEL
#endif

    /*
     * Transfer ID
     */
    if (fread(&id, sizeof(id), 1, input) != 1)
        return READ_MESSAGE_RC_ERR;
#if SIZEOF_TE_LOG_ID == 4
    msg->id = ntohl(id);
#elif SIZEOF_TE_LOG_ID == 2
    msg->id = ntohs(id);
#elif SIZEOF_TE_LOG_ID != 1
#error Unexpected value of SIZEOF_TE_LOG_ID
#endif

    /*
     * Transfer variable-length fields
     */
    return read_message_flds(input, msg);
}


/* Taken from lua.c */
static int
l_traceback(lua_State *L)
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


#if 0
static int
l_te_rc_to_str(lua_State *L)
{
    lua_Number  rc  = luaL_checknumber(L, 1);
    const char *src;
    const char *str;

    src = te_rc_mod2str(rc);
    str = te_rc_err2str(rc);

    if (*src == '\0')
        lua_pushstring(L, str);
    else
        lua_pushfstring(L, "%s-%s", src, str);

    return 1;
}


static int
l_xtmpfile(lua_State *L)
{
    const char *tmpdir;
    char       *path;
    int         fd;
    FILE       *file;
    FILE      **pfile;

    tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL || *tmpdir == '\0')
        tmpdir = "/tmp";

    if (asprintf(&path, "%s/raw2xml_XXXXXX", tmpdir) < 0)
        return luaL_error(L, "Failed to format temporary file template: %s",
                          strerror(errno));

    fd = mkstemp(path);
    if (fd < 0)
    {
        free(path);
        if (errno == EMFILE || errno == ENFILE)
        {
            /*
             * Have to resort to hardcoded error description to avoid locale
             * dependence
             */
            return luaL_error(L, "Failed to create temporary file: %s",
                              "too many open files");
        }
        else
            return luaL_error(L, "Failed to create temporary file: %s",
                              strerror(errno));
    }

    if (unlink(path) < 0)
    {
        free(path);
        return luaL_error(L, "Failed to unlink temporary file: %s",
                         strerror(errno));
    }

    free(path);

    file = fdopen(fd, "w+");
    if (file == NULL)
        return luaL_error(L, "Failed to convert temporary file "
                             "descriptor to a FILE: %s", strerror(errno));

    /* Create the file userdata */
    pfile = (FILE **)lua_newuserdata(L, sizeof(*pfile));
    *pfile = file;
    luaL_getmetatable(L, LUA_FILEHANDLE);
    lua_setmetatable(L, -2);
    /*
     * A hack: get io.close environment and assign it to the file to make
     * closing work
     */
    lua_getglobal(L, "io");
    lua_getfield(L, -1, "close");
    lua_remove(L, -2);
    lua_getfenv(L, -1);
    lua_remove(L, -2);
    lua_setfenv(L, -2);
    return 1;
}
#endif


#define LUA_PCALL(_nargs, _nresults) \
    do {                                                            \
        if (lua_pcall(L, _nargs, _nresults, traceback_idx) != 0)    \
            ERROR_CLEANUP("%s", lua_tostring(L, -1));               \
    } while (0)

#define LUA_REQUIRE(_mod) \
    do {                                \
        lua_getglobal(L, "require");    \
        lua_pushliteral(L, _mod);       \
        LUA_PCALL(1, 1);                \
    } while (0)


static te_bool
run_input_and_output(FILE *input,
                     lua_State *L, int traceback_idx,
                     int sink_idx, int sink_put_idx)
{
    te_bool             result      = FALSE;
    off_t               offset;
    rgt_msg             msg;
    int                 msg_idx;
    read_message_rc     read_rc;

    /* Create message reference userdata */
    lua_rgt_msg_wrap(L, &msg);
    msg_idx = lua_gettop(L);

    while (TRUE)
    {
        /* Retrieve current offset */
        offset = ftello(input);

        /* Read a message */
        read_rc = read_message(input, &msg);
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

        /* Copy "put" sink instance method to the top */
        lua_pushvalue(L, sink_put_idx);
        /* Copy sink instance to the top */
        lua_pushvalue(L, sink_idx);
        /* Copy message to the top */
        lua_pushvalue(L, msg_idx);

        /* Call "put" sink instance method to feed the message */
        if (lua_pcall(L, 2, 0, traceback_idx) != 0)
            ERROR_CLEANUP("Failed to output message starting at %lld:\n%s",
                          offset, lua_tostring(L, -1));
    }

    result = TRUE;

cleanup:

    /* Remove the message reference userdata, since it becomes invalid */
    lua_remove(L, msg_idx);

    /* Free scrap buffer used to hold message variable-length fields */
    scrap_clear();

    return result;
}


static te_bool
run_input(FILE *input, const char *output_name, unsigned long max_mem)
{
    te_bool     result          = FALSE;
    lua_State  *L               = NULL;
    int         traceback_idx;
    int         sink_idx;
    int         sink_put_idx;
#ifdef RGT_WITH_LUA_PROFILER
    int         profiler_idx;
#endif

    /*
     * Setup Lua
     */
    /* Create Lua instance */
    L = luaL_newstate();
    if (L == NULL)
        ERROR_CLEANUP("Failed to create Lua state");
    /* Load standard libraries */
    luaL_openlibs(L);

    /* Push traceback function */
    lua_pushcfunction(L, l_traceback);
    traceback_idx = lua_gettop(L);

    /* Require "strict" module */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "strict");
    LUA_PCALL(1, 0);

#ifdef RGT_WITH_LUA_PROFILER
    /* Require "profiler" module */
    LUA_REQUIRE("profiler");
    profiler_idx = lua_gettop(L);
#endif

    /* Require rgt.msg */
    lua_getglobal(L, "require");
    lua_pushliteral(L, LUA_RGT_MSG_NAME);
    LUA_PCALL(1, 0);

    /*
     * Create sink instance
     */
    /* Call rgt.sink(max_mem) to create sink instance */
    LUA_REQUIRE("rgt.sink");
    lua_pushnumber(L, (lua_Number)max_mem * 1024 * 1024);
    LUA_PCALL(1, 1);
    sink_idx = lua_gettop(L);

    /*
     * Supply sink instance with the output file
     */
    lua_getfield(L, sink_idx, "take_file");
    lua_pushvalue(L, sink_idx);

    /* Open/push output file */
    /* Get "io" table */
    lua_getglobal(L, "io");
    if (output_name[0] == '-' && output_name[1] == '\0')
        lua_getfield(L, -1, "stdout");
    else
    {
        lua_getfield(L, -1, "open");
        lua_pushstring(L, output_name);
        lua_pushliteral(L, "w");
        LUA_PCALL(2, 1);
    }
    /* Remove "io" table */
    lua_remove(L, -2);

    /* Call "take_file" instance method to supply the sink with the file */
    LUA_PCALL(2, 0);

#ifdef RGT_WITH_LUA_PROFILER
    /* Start profiling */
    lua_getfield(L, profiler_idx, "start");
    LUA_PCALL(0, 0);
#endif

    /*
     * Start sink output
     */
    lua_getfield(L, sink_idx, "start");
    lua_pushvalue(L, sink_idx);
    LUA_PCALL(1, 0);

    /*
     * Run with input file and Lua state
     */
    /* Retrieve sink instance "put" method */
    lua_getfield(L, sink_idx, "put");
    sink_put_idx = lua_gettop(L);

    if (!run_input_and_output(input, L, traceback_idx,
                              sink_idx, sink_put_idx))
        goto cleanup;

    /*
     * Finish sink output
     */
    lua_getfield(L, sink_idx, "finish");
    lua_pushvalue(L, sink_idx);
    LUA_PCALL(1, 0);

#ifdef RGT_WITH_LUA_PROFILER
    /* Stop profiling */
    lua_getfield(L, profiler_idx, "stop");
    LUA_PCALL(0, 0);
#endif

    /*
     * Take the output file from the sink
     */
    lua_getfield(L, sink_idx, "yield_file");
    lua_pushvalue(L, sink_idx);
    LUA_PCALL(1, 1);

    /*
     * Close the output
     */
    lua_getfield(L, -1, "close");
    lua_pushvalue(L, -2);
    LUA_PCALL(1, 0);

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

    /*
     * Run with input file
     */
    if (!run_input(input, output_name, max_mem))
        goto cleanup;

    /*
     * Close the input
     */
    fclose(input);
    input = NULL;

    /* Success */
    result = 0;

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
                (((uint64_t)(si.totalram) * si.mem_unit) / (4*1024*1024));
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
                    max_mem = strtoul(optarg, (char **)&end, 0);
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


