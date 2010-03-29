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

#include<string.h>
#include<ctype.h>
#include<popt.h>


#include "acse_epc.h"
#include "acse_internal.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "te_cwmp.h"
#include "logger_api.h"

#include "logger_file.h"

DEFINE_LGR_ENTITY("ACSE CLI");

/* TODO some normal way to parse command line?.. */

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

static te_errno
cli_args_acs_cpe(const char *args, size_t *offset, char *acs, char *cpe)
{
    int i;
    const char *start_args = args;

    if (!(args && offset && acs && cpe))
        return TE_EINVAL;

    while (isspace(*args)) args++;

    for (i = 0; args[i] && (!isspace(args[i])) && (args[i] != '/'); i++)
        acs[i] = args[i];
    acs[i] = '\0';
    args += i;

    while((*args) && (isspace(*args) || (*args == '/')))
        args++;
    
    if ((i = cli_token_copy(args, cpe)) == 0)
    {
        fprintf(stderr, "Call CR fails, args '%s', CPE name not detected\n",
              start_args);
        return TE_EFAIL;
    }
    args += i;

    *offset = args - start_args;

    return 0;
}

static te_errno
cli_cfg_list(const char *args, acse_cfg_level_t level)
{
    acse_epc_msg_t msg;
    acse_epc_config_data_t cfg_data;

    msg.opcode = EPC_CONFIG_CALL;
    msg.data.cfg = &cfg_data;
    msg.length = sizeof(cfg_data);
    msg.status = 0;

    if (level == EPC_CFG_CPE)
        cli_token_copy(args, cfg_data.acs);
    
    cfg_data.op.magic = EPC_CONFIG_MAGIC;
    cfg_data.op.level = level;
    cfg_data.op.fun = EPC_CFG_LIST;

    acse_epc_send(&msg);
    return 0;
}


static te_errno
cli_cfg_add(const char *args, acse_cfg_level_t level)
{
    acse_epc_msg_t         msg;
    acse_epc_config_data_t cfg_data;

    msg.opcode = EPC_CONFIG_CALL;
    msg.data.cfg = &cfg_data;
    msg.length = sizeof(cfg_data);
    msg.status = 0;

    args += cli_token_copy(args, cfg_data.acs);
    
    if (level == EPC_CFG_CPE)
        cli_token_copy(args, cfg_data.cpe);
    else
        cfg_data.cpe[0] = '\0';

    cfg_data.op.magic = EPC_CONFIG_MAGIC;
    cfg_data.op.level = level;
    cfg_data.op.fun = EPC_CFG_ADD;

    acse_epc_send(&msg);

    return 0;
}

static te_errno
cli_acs_config(const char *args, acse_cfg_op_t fun)
{
    acse_epc_msg_t         msg;
    acse_epc_config_data_t cfg_data;

    msg.opcode = EPC_CONFIG_CALL;
    msg.data.cfg = &cfg_data;
    msg.length = sizeof(cfg_data);
    msg.status = 0;

    args += cli_token_copy(args, cfg_data.acs);

    args += cli_token_copy(args, cfg_data.oid);

    cli_token_copy(args, cfg_data.value);

    cfg_data.op.magic = EPC_CONFIG_MAGIC;
    cfg_data.op.level = EPC_CFG_ACS;
    cfg_data.op.fun = fun;
    cfg_data.cpe[0] = '\0';

    acse_epc_send(&msg);

    return 0;
}

static te_errno
cli_cpe_config(const char *args, acse_cfg_op_t fun)
{
    te_errno    rc;
    size_t      offset = 0;

    acse_epc_msg_t         msg;
    acse_epc_config_data_t cfg_data;

    msg.opcode = EPC_CONFIG_CALL;
    msg.data.cfg = &cfg_data;
    msg.length = sizeof(cfg_data);
    msg.status = 0;

    rc = cli_args_acs_cpe(args, &offset, cfg_data.acs, cfg_data.cpe);
    if (rc != 0)
    {
        fprintf(stderr, "Parse error 0x%x\n", rc);
        return rc;
    }
    args += offset;

    args += cli_token_copy(args, cfg_data.oid);

    if (fun == EPC_CFG_MODIFY)
        cli_token_copy(args, cfg_data.value);
    else
        cfg_data.value[0] = '\0';

    cfg_data.op.magic = EPC_CONFIG_MAGIC;
    cfg_data.op.level = EPC_CFG_CPE;
    cfg_data.op.fun = fun;

    acse_epc_send(&msg);
    return 0;
}

static te_errno
cli_cpe_cr(const char *args)
{
    te_errno rc;
    size_t offset = 0;

    acse_epc_msg_t msg;
    acse_epc_cwmp_data_t c_data;

    msg.opcode = EPC_CWMP_CALL;
    msg.data.cwmp = &c_data;
    msg.length = sizeof(c_data);

    memset(&c_data, 0, sizeof(c_data));

    if (strncmp(args, "call ", 5) == 0)
        c_data.op = EPC_CONN_REQ;
    else if (strncmp(args, "show ", 5) == 0)
        c_data.op = EPC_CONN_REQ_CHECK;
    else
    {
        printf("unsupported command for 'cpe cr'\n");
        return TE_EFAIL;
    }
    args += 5;

    rc = cli_args_acs_cpe(args, &offset, c_data.acs, c_data.cpe);
    if (rc != 0)
    {
        fprintf(stderr, "Parse error 0x%x\n", rc);
        return rc;
    }
    args += offset;

    rc = acse_epc_send(&msg);
    if (rc != 0)
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);

    return 0;
}


static te_errno
cli_cpe_rpc(const char *args)
{
    te_errno rc;
    size_t offset = 0;

    acse_epc_msg_t msg;
    acse_epc_cwmp_data_t c_data;

    msg.opcode = EPC_CWMP_CALL;
    msg.data.cwmp = &c_data;
    msg.length = sizeof(c_data);

    memset(&c_data, 0, sizeof(c_data));

    if (strncmp(args, "call ", 5) == 0)
    {
        args += 5;
        c_data.op = EPC_RPC_CALL ;
        
        /* todo full parsing */
        c_data.hold_requests = 0;
        c_data.rpc_cpe = CWMP_RPC_get_rpc_methods; 
        c_data.to_cpe.p = NULL; 
    }
    else if (strncmp(args, "show ", 5) == 0)
    {
        args += 5;
        c_data.op = EPC_RPC_CHECK;
        c_data.index = atoi(args);

        while (isdigit(*args)) args++;
        while (isspace(*args)) args++;
    }
    else
    {
        printf("unsupported command for 'cpe cr'\n");
        return TE_EFAIL;
    }

    rc = cli_args_acs_cpe(args, &offset, c_data.acs, c_data.cpe);
    if (rc != 0)
    {
        fprintf(stderr, "Parse error 0x%x\n", rc);
        return rc;
    }
    args += offset;

    rc = acse_epc_send(&msg);
    if (rc != 0)
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);

    return 0;
}


static te_errno
cli_cpe_inform(const char *args)
{
    te_errno rc;
    size_t offset = 0;

    acse_epc_msg_t msg;
    acse_epc_cwmp_data_t c_data;

    msg.opcode = EPC_CWMP_CALL;
    msg.data.cwmp = &c_data;
    msg.length = sizeof(c_data);

    memset(&c_data, 0, sizeof(c_data));

    c_data.op = EPC_GET_INFORM;
    c_data.index = atoi(args);

    while (isdigit(*args)) args++;
    while (isspace(*args)) args++;

    rc = cli_args_acs_cpe(args, &offset, c_data.acs, c_data.cpe);
    if (rc != 0)
    {
        fprintf(stderr, "Parse error 0x%x\n", rc);
        return rc;
    }
    args += offset;

    rc = acse_epc_send(&msg);
    if (rc != 0)
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);

    return 0;
}

static te_errno
cli_parse_exec_cpe(const char *args)
{
    if (strncmp(args, "add ", 4) == 0)
        return cli_cfg_add(args + 4, EPC_CFG_CPE);
    if (strncmp(args, "list ", 5) == 0)
        return cli_cfg_list(args + 5, EPC_CFG_CPE);
    if (strncmp(args, "cr ", 3) == 0)
        return cli_cpe_cr(args + 3);
    if (strncmp(args, "inform ", 7) == 0)
        return cli_cpe_inform(args + 7);
    if (strncmp(args, "rpc ", 4) == 0)
        return cli_cpe_rpc(args + 4);
    if (strncmp(args, "modify ", 7) == 0)
        return cli_cpe_config(args + 7, EPC_CFG_MODIFY);
    if (strncmp(args, "obtain ", 7) == 0)
        return cli_cpe_config(args + 7, EPC_CFG_OBTAIN);
    return 0;
}

static te_errno
cli_parse_exec_acs(const char *args)
{
    if (strncmp(args, "add ", 4) == 0)
        return cli_cfg_add(args + 4, EPC_CFG_ACS);
    if (strncmp(args, "list", 4) == 0)
        return cli_cfg_list(args + 4, EPC_CFG_ACS);
    if (strncmp(args, "modify ", 7) == 0)
        return cli_acs_config(args + 7, EPC_CFG_MODIFY);
    if (strncmp(args, "obtain ", 7) == 0)
        return cli_acs_config(args + 7, EPC_CFG_OBTAIN);
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

static const char *
rpc_cpe_to_string(te_cwmp_rpc_cpe_t rpc_cpe)
{
    switch (rpc_cpe)
    {
    case CWMP_RPC_NONE:
        return "NONE";
    case CWMP_RPC_get_rpc_methods: 
        return "get_rpc_methods";
    case CWMP_RPC_set_parameter_values: 
        return "set_parameter_values";
    case CWMP_RPC_get_parameter_values: 
        return "get_parameter_values";
    case CWMP_RPC_get_parameter_names: 
        return "get_parameter_names";
    case CWMP_RPC_set_parameter_attributes: 
        return "set_parameter_attributes";
    case CWMP_RPC_get_parameter_attributes: 
        return "get_parameter_attributes";
    case CWMP_RPC_add_object: 
        return "add_object";
    case CWMP_RPC_delete_object: 
        return "delete_object";
    case CWMP_RPC_reboot: 
        return "reboot";
    case CWMP_RPC_download: 
        return "download";
    case CWMP_RPC_upload: 
        return "upload";
    case CWMP_RPC_factory_reset: 
        return "factory_reset";
    case CWMP_RPC_get_queued_transfers: 
        return "get_queued_transfers";
    case CWMP_RPC_get_all_queued_transfers: 
        return "get_all_queued_transfers";
    case CWMP_RPC_schedule_inform: 
        return "schedule_inform";
    case CWMP_RPC_set_vouchers: 
        return "set_vouchers";
    case CWMP_RPC_get_options: 
        return "get_options";
    }
    return "???";
}


static void
print_rpc_response(acse_epc_cwmp_data_t *cwmp_resp)
{
    switch (cwmp_resp->rpc_cpe)
    {
    case CWMP_RPC_get_rpc_methods: 
    {
        MethodList *mlist;
        if ((mlist = cwmp_resp->from_cpe.get_rpc_methods_r->MethodList_)
                != NULL)
        {
            int i;
            printf("RPC methods: ");
            for (i = 0; i < mlist->__size; i++)
                printf("'%s', ", mlist->__ptrstring[i]);
            printf("\n");
        }
    }
        break;
    case CWMP_RPC_NONE:
    case CWMP_RPC_set_parameter_values: 
    case CWMP_RPC_get_parameter_values: 
    case CWMP_RPC_get_parameter_names: 
    case CWMP_RPC_set_parameter_attributes: 
    case CWMP_RPC_get_parameter_attributes: 
    case CWMP_RPC_add_object: 
    case CWMP_RPC_delete_object: 
    case CWMP_RPC_reboot: 
    case CWMP_RPC_download: 
    case CWMP_RPC_upload: 
    case CWMP_RPC_factory_reset: 
    case CWMP_RPC_get_queued_transfers: 
    case CWMP_RPC_get_all_queued_transfers: 
    case CWMP_RPC_schedule_inform: 
    case CWMP_RPC_set_vouchers: 
    case CWMP_RPC_get_options: 
        printf("TODO... \n");
        break;
    }
}


static te_errno
print_cwmp_response(te_errno status, acse_epc_cwmp_data_t *cwmp_resp)
{
    switch(cwmp_resp->op)
    {
    case EPC_CONN_REQ:
    case EPC_CONN_REQ_CHECK:
        printf("Connection request to %s/%s, state %d\n",
                cwmp_resp->acs, cwmp_resp->cpe,
                (int)cwmp_resp->from_cpe.cr_state);
        break;
    case EPC_RPC_CALL:
        printf("RPC call'%s' to %s/%s, index %d\n",
                rpc_cpe_to_string(cwmp_resp->rpc_cpe),
                cwmp_resp->acs, cwmp_resp->cpe, cwmp_resp->index);
        break;
    case EPC_RPC_CHECK:
        printf("RPC check, '%s' to %s/%s, status %s\n",
                rpc_cpe_to_string(cwmp_resp->rpc_cpe),
                cwmp_resp->acs, cwmp_resp->cpe,
                te_rc_err2str(status));
        if (status == 0)
            print_rpc_response(cwmp_resp);
        break;
    case EPC_GET_INFORM:
        {
            _cwmp__Inform *inform = cwmp_resp->from_cpe.inform;
            printf("Get Inform from %s/%s, index %d\n",
                    cwmp_resp->acs, cwmp_resp->cpe, cwmp_resp->index);
            if (status != 0)
            {
                printf("failed, status '%s'\n", te_rc_err2str(status));
                break;
            }
            printf("Device OUI: '%s'\n", inform->DeviceId->OUI);
            if (inform->Event != NULL)
            {
                int i;
                for(i = 0; i < inform->Event->__size; i++)
                {
                    cwmp__EventStruct *ev =
                        inform->Event->__ptrEventStruct[i];
                    if (ev != NULL)
                        printf("Event[%d]: '%s'\n", i, ev->EventCode);
                }
            }
        }
        break;
    }
    return 0;
}

static te_errno
print_config_response(te_errno status, acse_epc_config_data_t *cfg_resp)
{ 

    if (status != 0)
        printf("ERROR in response: %s\n",
            te_rc_err2str(status));
    else 
        printf("Result: %s\n", cfg_resp->value);
    return 0;
}

#define BUF_SIZE 256

static void
cli_exit_handler(void)
{
    RING("Normal exit from CLI");
    acse_epc_close();
}

char *epc_sock_name = NULL;
char *script_name = NULL;

struct poptOption acse_cli_opts[] = 
{
    {"epc-socket", 'e', POPT_ARG_STRING, &epc_sock_name, 0,
            "filename for EPC socket", "EPC socket"},
    {"script",     's', POPT_ARG_STRING, &script_name, 0, 
            "filename with list of commands to perform before operation",
            "script"},
    {NULL, 0, 0, NULL, 0, NULL, NULL}
};



int 
main(int argc, const char **argv)
{
    te_errno rc;
    int rpoll, rpopt;
    poptContext cont;

    cont = poptGetContext(NULL, argc, argv, acse_cli_opts, 0);
    
    rpopt = poptGetNextOpt(cont); /* this really parse all command line */

    if ((rc = acse_epc_open(epc_sock_name, NULL, ACSE_EPC_CLIENT))
        != 0)
    {
        ERROR("open EPC failed %r", rc);
        return 1;
    }
    atexit(&cli_exit_handler);

    /* TODO process command line script */
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
                if (r == 0) /* The end of input */
                    break;

                buf[r] = '\0';
                rc = epc_parse_cli(buf, r);
                if (rc != 0)
                    RING("parse error %r", rc);
            }
            if (pfd[1].revents)
            {
                acse_epc_msg_t *msg_resp;
                rc = acse_epc_recv(&msg_resp);
                if (TE_RC_GET_ERROR(rc) == TE_ENOTCONN)
                    break;
                else if (rc != 0)
                    RING("EPC recv error %r", rc);
                switch (msg_resp->opcode)
                {
                    case EPC_CONFIG_RESPONSE:
                        print_config_response(msg_resp->status,
                                              msg_resp->data.cfg);
                        break;
                    case EPC_CWMP_RESPONSE:
                        print_cwmp_response(msg_resp->status,
                                            msg_resp->data.cwmp);
                        break;
                    default:
                        ERROR("Unexpected opcode 0x%x from EPC",
                             msg_resp->opcode);
                }
                printf("\n> ");
            }
        }
    }
    return 0;
}

