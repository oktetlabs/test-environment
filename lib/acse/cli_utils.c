/** @file
 * @brief ACSE CLI
 *
 * ACS Emulator CLI tool
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include<string.h>
#include<stdio.h>
#include<ctype.h>

#include "cli_utils.h"

#define MAX_LEVELS 20


int
cli_perform_cmd(cli_cmd_descr_t *root_list, const char *line)
{
    int rc = CLI_E_WRONG_TAG;

    char     err_buf[CLI_ERR_BUF] = "";
    char     token[200] = {0,};
    size_t   ofs = 0;
    unsigned level = 0;

    int               cd_tags[MAX_LEVELS];
    cli_cmd_descr_t  *cd_stack[MAX_LEVELS];
    cli_cmd_descr_t  *cd = root_list;

    cli_cmd_method_fn func = NULL;

    do {
        ofs = cli_token_copy(line, token);
        if (0 == ofs)
            break; /* there is no token */
        while (NULL != cd->label)
        {
            if (strcmp(cd->label, token) == 0)
                break;
            cd ++;
        }
        if (NULL == cd->label) /* token was not found */
            break;

        if (NULL != cd->func)
            func = cd->func;
        cd_stack[level] = cd;
        cd_tags[level] = cd->tag;
        level++;
        line += ofs;
        token[0] = '\0';

        if (NULL == cd->sub_cmds)
            break;

        cd = cd->sub_cmds;
    } while (1);

    if (0 == ofs && 0 == level) /* Line contain only space symbols. */
        return 0;

    if (NULL != func)
        rc = func(level, cd_tags, line, err_buf);
    else if (0 == token[0])
        rc = CLI_E_MISS_TAGS;

    if (rc != CLI_OK)/* unsupported or syntax error */
    {
        unsigned i;
        switch (rc)
        {
            case CLI_E_WRONG_TAG:
                if (token[0] != 0)
                    printf("Unexpected token '%s'; ", token);

                /* fall through ... */
            case CLI_E_MISS_TAGS:
                if (level > 0)
                    cd = cd_stack[level-1]->sub_cmds;
                else
                    cd = root_list;
                if (NULL == cd)
                {
                    printf("Not implemented. Sorry.\n");
                    break;
                }

                printf("expected:\n");
                while (cd->label != NULL)
                {
                    printf("    ");
                    for (i = 0; i < level; i++)
                        printf("%s ", cd_stack[i]->label);
                    printf("%s ... \n\tdescr: %s\n", cd->label, cd->descr);
                    cd++;
                }

                break;
            case CLI_E_SPECIFIC:
                printf("parse error, expected:\n\t");
                for (i = 0; i < level; i++)
                    printf("%s ", cd_stack[i]->label);
                printf("%s\n", err_buf);
                break;
            default:
                printf("Internal error, unexpected status %d\n", rc);
        }
    }
    return 0;
}

void
cli_print_cmd_help(cli_cmd_descr_t *cd)
{
    return;
}
