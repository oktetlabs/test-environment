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

DEFINE_LGR_ENTITY("ACSE CLI");

static te_errno
cli_cpe_list(const char *args)
{
    int i;
    acse_epc_msg_t msg;
    acse_epc_config_data_t cfg_data;

    msg.opcode = EPC_CONFIG_CALL;
    msg.data = &cfg_data;
    msg.length = sizeof(cfg_data);

    for (i = 0; args[i] && !isspace(args[i]); i++)
        cfg_data.acs[i] = args[i];
    cfg_data.acs[i] = '\0';

    
    cfg_data.op.level = EPC_CFG_CPE;
    cfg_data.op.fun = EPC_CFG_LIST;
    cfg_data.oid[0] = '\0';
    cfg_data.cpe[0] = '\0';

    acse_epc_send(&msg);
    return 0;
}

static te_errno
cli_cpe_cr(const char *args)
{
    int i;
    te_errno rc;

    acse_epc_msg_t msg;
    acse_epc_cwmp_data_t c_data;

    msg.opcode = EPC_CONFIG_CALL;
    msg.data = &c_data;
    msg.length = sizeof(c_data);

    memset(&c_data, 0, sizeof(c_data));

    c_data.op = EPC_CONN_REQ;

    for (i = 0; args[i] && !isspace(args[i]) && (args[i] != '/'); i++)
        c_data.acs[i] = args[i];
    c_data.acs[i] = '\0';
    args += i;

    while((*args) && (isspace(*args) || (*args == '/')))
        args++;
    
    for (i = 0; args[i] && !isspace(args[i]) ; i++)
        c_data.cpe[i] = args[i];
    if (i == 0)
    {
        printf("Call CR fails, args '%s', CPE name not detected\n",
              args);
        return 1;
    }
    c_data.cpe[i] = '\0';

    rc = acse_epc_send(&msg);
    if (rc != 0)
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);

    return 0;
}

static te_errno
cli_parse_exec_cpe(const char *args)
{
    cpe_t *cpe;
    if (strncmp(args, "list ", 5) == 0)
        return cli_cpe_list(args + 5);
    if (strncmp(args, "cr ", 3) == 0)
        return cli_cpe_cr(args + 3);
    return 0;
}

static te_errno
cli_parse_exec_acs(const char *args)
{
    /* TODO */
    RING("%s():%d TODO", __FUNCTION__, __LINE__);
    return 0;
}

static te_errno
epc_parse_cli(const char *buf, size_t len)
{
    if (strncmp(buf, "cpe ", 4) == 0)
        return cli_parse_exec_cpe(buf + 4);

    if (strncmp(buf, "acs ", 4) == 0)
        return cli_parse_exec_acs(buf + 4);

    return 0;
}

static te_errno
print_cwmp_response(acse_epc_cwmp_data_t *cwmp_resp)
{
    RING("%s():%d TODO", __FUNCTION__, __LINE__);
    /* TODO */
    return 0;
}

static te_errno
print_config_response(acse_epc_config_data_t *cfg_resp)
{
    printf("Result: %s\n", cfg_resp->value);
    return 0;
}

#define BUF_SIZE 256

int 
main(int argc, char **argv)
{
    const char *msg_sock_name;
    te_errno rc;
    int rpoll;

    if (argc > 1)
        msg_sock_name = strdup(argv[1]);
    else 
        msg_sock_name = strdup(EPC_ACSE_SOCK);

    if ((rc = acse_epc_open(msg_sock_name, EPC_MMAP_AREA, ACSE_EPC_CLIENT))
        != 0)
    {
        ERROR("open EPC failed %r", rc);
        return 1;
    }

    /* main loop */
    while (1)
    {
        struct pollfd pfd[2];
        pfd[0].fd = 0; /* stdin */
        pfd[0].events = POLLIN;
        pfd[0].revents = 0;
        pfd[1].fd = acse_epc_sock();
        pfd[1].events = POLLIN;
        pfd[1].revents = 0;

        rpoll = poll(pfd, 2, -1);
        if (rpoll > 0)
        {
            if (pfd[0].revents)
            {
                char buf[BUF_SIZE];
                ssize_t r = read(pfd[0].fd, buf, BUF_SIZE);

                if (r < 0)
                {
                    perror("read fail");
                    break;
                }
                rc = epc_parse_cli(buf, r);
                if (rc != 0)
                    RING("parse error %r", rc);
            }
            if (pfd[1].revents)
            {
                acse_epc_msg_t *msg_resp;
                rc = acse_epc_recv(&msg_resp);
                if (rc != 0)
                    RING("EPC recv error %r", rc);
                else
                    switch (msg_resp->opcode)
                    {
                        case EPC_CONFIG_RESPONSE:
                            print_config_response(msg_resp->data);
                            break;
                        case EPC_CWMP_RESPONSE:
                            print_cwmp_response(msg_resp->data);
                            break;
                        default:
                            ERROR("Unexpected opcode 0x%x from EPC",
                                 msg_resp->opcode);
                    }
            }
        }
    }
    acse_epc_close();
    return 0;
}

