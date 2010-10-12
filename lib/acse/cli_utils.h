/** @file
 * @brief ACSE CLI Utils
 *
 * Utilities for simple CLI tool
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id: acse_cli.c 64634 2010-06-07 18:46:13Z konst $
 */

#include<string.h>
#include<ctype.h>


/**
 * Copy token from line to separate place.
 * Tokens are sperated by spaces.
 *
 * @return offset in @p line of the end of token, i.e. offset
 *              of the first symbol after token.
 */
static inline size_t 
cli_token_copy(const char *line, char *token)
{
    size_t t, s;

    for (s = 0; isspace(line[s]); s++);
    line += s;

    for (t = 0; line[t] && (!isspace(line[t])); t++)
        token[t] = line[t];
    token[t] = '\0';

    return s + t;
}

struct cli_cmd_descr_t;

typedef int (*cli_cmd_method_fn)(int argc, const int *arg_tags,
                                 const char *rest_line, void *opaque);

typedef struct cli_cmd_descr_t {
    const char *label;
    int         tag;
    const char *descr;

    cli_cmd_method_fn       func;

    struct cli_cmd_descr_t *sub_cmds;

} cli_cmd_descr_t;

#define END_CMD_ARRAY {NULL, -1, NULL, NULL, NULL}

extern int cli_perform_cmd(cli_cmd_descr_t *root_list, const char *line);


