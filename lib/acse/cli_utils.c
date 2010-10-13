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
    char token[200];
    size_t ofs = 0;
    unsigned level = 0;

    int cd_tags[MAX_LEVELS];
    cli_cmd_descr_t *cd_stack[MAX_LEVELS];

    cli_cmd_descr_t *cd = root_list;

    cli_cmd_method_fn func = NULL;

    do {
        ofs = cli_token_copy(line, token);
        if (0 == ofs) 
            break; /* there is no more token */
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

        if (NULL == cd->sub_cmds)
            break;

        cd = cd->sub_cmds; 
    } while (1);

    if (NULL != func)
    {
        func(level, cd_tags, line, NULL);
    }
    else /* unsupported or syntax error */
    {
        unsigned i;
        printf("Unexpected, '");
        for (i = 0; i < level; i++)
            printf("%s ", cd_stack[i]->label);
        printf("'; last label '%s', descr: %s\n", cd->label, cd->descr);
    }
    return 0;
}

