/** @file
 * @brief ACSE CLI Utils
 *
 * Utilities for simple CLI tool
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include<string.h>
#include<ctype.h>


/**
 * Copy token from line to separate place.
 * Tokens are sperated by spaces.
 *
 * @return offset in @p line of the end of token, i.e. offset
 *              of the first symbol after token;
 *              or zero if there is no more token.
 */
static inline size_t 
cli_token_copy(const char *line, char *token)
{
    size_t  t /* ofs in target */, 
            s /* ofs in source */,
            b /* leading blanks*/;
    int quoted = 0;

    for (b = 0; isspace(line[b]); b++);
    line += b;

    for (t = 0, s = 0; line[s] && isprint(line[s]); s++)
    {
        if (line[s] == '"')
        {
            quoted = !quoted;
            continue;
        }
        if (line[s] == '\\')
        {
            s++;
        }
        else if (!quoted && isspace(line[s]))
            break;

        token[t] = line[s];
        t++;
    }
    if (0 == t)
        return 0;
    token[t] = '\0';

    for (; isspace(line[s]); s++);

    return s + b;
}

struct cli_cmd_descr_t;

/** Constants useful for CLI lib */
enum {

    /** Status codes */
    CLI_OK = 0, 
    CLI_E_WRONG_TAG,
    CLI_E_MISS_TAGS,
    CLI_E_SPECIFIC,
    CLI_E_EXEC,

    /** Size of error buffer */
    CLI_ERR_BUF = 256,
};


typedef int (*cli_cmd_method_fn)(int argc, const int *arg_tags,
                                 const char *rest_line, char *err_buf);

typedef struct cli_cmd_descr_t {
    const char *label;
    int         tag;
    const char *descr;

    cli_cmd_method_fn       func;

    struct cli_cmd_descr_t *sub_cmds;

} cli_cmd_descr_t;

#define END_CMD_ARRAY {NULL, -1, NULL, NULL, NULL}

/**
 * Perform entered command line according with command descriptors
 * tree, call lowest user callback.
 *
 * @param root_list     Root of command descriptor tree.
 * @param line          Command line from user.
 *
 * @return zero on success, -1 otherwise.
 */
extern int cli_perform_cmd(cli_cmd_descr_t *root_list, const char *line);

/**
 * Print help for this command and its subcommands. 
 */
extern void cli_print_cmd_help(cli_cmd_descr_t *cd);

