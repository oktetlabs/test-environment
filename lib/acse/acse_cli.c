/** @file
 * @brief ACSE CLI
 *
 * ACS Emulator CLI tool
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */
#include "te_config.h"

#include <string.h>
#include <ctype.h>


#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "logger_api.h"
#include "acse_epc.h"
#include "acse_internal.h"

#include "logger_file.h"

DEFINE_LGR_ENTITY("ACSE");

static te_errno
epc_cpe_list(const char *args)
{
    int i;
    acse_epc_msg_t pars;

    for (i = 0; args[i] && !isspace(args[i]); i++)
        pars.acs[i] = args[i];
    pars.acs[i] = '\0';
    pars.op = ACSE_PC_CPE_LIST;
    pars.oid[0] = '\0';
    pars.cpe[0] = '\0';

    acs_cpe_list(&pars);
    printf("CPE list under '%s': %s\n", pars.acs, pars.list);
    return 0;
}

static te_errno
epc_cpe_cr(const char *args)
{
    int i;
    cpe_t *cpe;
    char acs_name[100] = "";
    char cpe_name[100] = "";
    te_errno rc;

    for (i = 0; args[i] && !isspace(args[i]) && (args[i] != '/'); i++)
        acs_name[i] = args[i];
    acs_name[i] = '\0';
    args += i;

    while((*args) && (isspace(*args) || (*args == '/')))
        args++;
    
    for (i = 0; args[i] && !isspace(args[i]) ; i++)
        cpe_name[i] = args[i];
    if (i == 0)
    {
        printf("Call CR fails, args '%s', CPE name not detected\n");
        return 1;
    }
    cpe_name[i] = '\0';

    cpe = db_find_cpe(NULL, acs_name, cpe_name);
    if (cpe == NULL)
    {
        printf("Call connection request fails, '%s':'%s' not found\n",
               acs_name, cpe_name);
        return 2;
    }

    rc = acse_init_connection_request(cpe);
    if (rc != 0)
    {
        ERROR("Conn Req failed: %r", rc);
        return 3;
    }
    return 0;
}

static te_errno
epc_parse_exec_cpe(const char *args)
{
    cpe_t *cpe;
    if (strncmp(args, "list ", 5) == 0)
        return epc_cpe_list(args + 5);
    if (strncmp(args, "cr ", 3) == 0)
        return epc_cpe_cr(args + 3);
    return 0;
}

static te_errno
epc_parse_exec_acs(const char *args)
{
    return 0;
}

static te_errno
epc_parse_cli(const char *buf, size_t len)
{
    if (strncmp(buf, "cpe ", 4) == 0)
        return epc_parse_exec_cpe(buf + 4);

    if (strncmp(buf, "acs ", 4) == 0)
        return epc_parse_exec_cpe(buf + 4);

    return 0;
}



int 
main(int argc, char **argv)
{
    return 0;
}

