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
#include <sys/types.h>
#include <sys/wait.h>



#include "acse_epc.h"
#include "acse_internal.h"
#include "acse_user.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_defs.h"
#include "te_cwmp.h"
#include "logger_api.h"

#include "logger_file.h"

#include "cli_utils.h"
#include "cwmp_utils.h"

DEFINE_LGR_ENTITY("ACSE");

#ifdef TE_LGR_USER
#undef TE_LGR_USER
#endif
#define TE_LGR_USER     "CLI"

#define NUM_OF(_arr) (sizeof(_arr)/sizeof(_arr [0]))



static char acs_def_name[30] = "";
static char cpe_def_name[50] = "";
static int timeout_def = 20;
static int request_id = 0;

static te_errno print_config_response(te_errno status,
                                      acse_epc_config_data_t *cfg_resp);


static te_errno print_cwmp_response(te_errno status,
                                    acse_epc_cwmp_data_t *cwmp_resp);

enum cli_codes {
    CMD_PARAM = 0x1000, 
    CMD_RPC, 
    CMD_CR, 
    CMD_ENV, 
    CMD_HTTP, 

    PARAM_OBTAIN = 0x1010, 
    PARAM_MODIFY, 
    PARAM_LIST, 
    PARAM_ADD, 
    PARAM_DEL, 

    RPC_SEND = 0x1020, 
    RPC_CHECK, 

    CR_SEND = 0x1030, 
    CR_CHECK, 

    ENV_ACS = 0x1040, 
    ENV_CPE, 
    ENV_TIMEOUT, 
};


#define PARSE_ERROR(_format) \
    do { \
        strcpy(err_buf, _format); \
        return CLI_E_SPECIFIC; \
    } while (0)


static int
param_cmd_access(int argc, const int *arg_tags,
                 const char *rest_line, char *err_buf)
{
    acse_epc_config_data_t *cfg_data;
    te_errno    rc;
    size_t      ofs;

    /* Command here:
       'param acs|cpe modify|obtain <param_name> <value>'
    */
    if (argc < 3)
        return CLI_E_MISS_TAGS;

    acse_conf_prepare(arg_tags[2] /* function */, &cfg_data);

    strncpy(cfg_data->acs, acs_def_name, sizeof(cfg_data->acs));
    if (EPC_CFG_ACS == arg_tags[1])
        cfg_data->cpe[0] = '\0';
    else 
        strncpy(cfg_data->cpe, cpe_def_name, sizeof(cfg_data->cpe));

    
    ofs = cli_token_copy(rest_line, cfg_data->oid);
    if (0 == ofs)
    {
        if (EPC_CFG_MODIFY == cfg_data->op.fun)
            PARSE_ERROR(" <parameter_name> <value>");
        else
            PARSE_ERROR(" <parameter_name>");
    }
    rest_line += ofs;

    cfg_data->op.level = arg_tags[1];

    if (EPC_CFG_MODIFY == cfg_data->op.fun)
    {
        ofs = cli_token_copy(rest_line, cfg_data->value);
        if (0 == ofs)
            PARSE_ERROR(" <parameter_name> <value>");
    }
    else
        cfg_data->value[0] = '\0';

    rc = acse_conf_call(&cfg_data);

    if (TE_RC_GET_ERROR(rc) == TE_ENOTCONN)
    {
        printf("Connection broken\n");
        return -1;
    }
    print_config_response(rc, cfg_data);

    return 0;
}

static int
param_cmd_wait(int argc, const int *arg_tags,
               const char *rest_line, char *err_buf)
{
    acse_epc_config_data_t *cfg_data;
    te_errno    rc;
    size_t      ofs;
    char        exp_value[RCF_MAX_VAL];
    char        var_name[RCF_MAX_ID];
    int         timeout = timeout_def;

    /* Command here:
       'param acs|cpe wait <param_name> <value>'
    */
    if (argc < 3)
        return CLI_E_MISS_TAGS;

    printf("%s(): rest_line <%s>\n", __FUNCTION__, rest_line);
    
    ofs = cli_token_copy(rest_line, var_name);
    if (0 == ofs)
        PARSE_ERROR(" <parameter_name> <expected_value>");
    rest_line += ofs;

    printf("%s(): rest_line <%s>, var_name <%s>\n",
            __FUNCTION__, rest_line, var_name);

    ofs = cli_token_copy(rest_line, exp_value);
    if (0 == ofs)
        PARSE_ERROR(" <parameter_name> <exp_value>");

    do {
        acse_conf_prepare(arg_tags[2] /* function */, &cfg_data);

        strncpy(cfg_data->acs, acs_def_name, sizeof(cfg_data->acs));
        if (EPC_CFG_ACS == arg_tags[1])
            cfg_data->cpe[0] = '\0';
        else 
            strncpy(cfg_data->cpe, cpe_def_name, sizeof(cfg_data->cpe));

        strcpy(cfg_data->oid, var_name);

        cfg_data->op.level = arg_tags[1];


        cfg_data->value[0] = '\0';

        rc = acse_conf_call(&cfg_data);

        if (TE_RC_GET_ERROR(rc) == TE_ENOTCONN)
        {
            printf("Connection broken\n");
            return -1;
        }

        print_config_response(rc, cfg_data);

    } while ((timeout--) > 0 &&
             strcmp(exp_value, cfg_data->value) != 0 &&
             sleep(1) == 0);

    return 0;
}
static int
param_cmd_list(int argc, const int *arg_tags,
               const char *rest_line, char *err_buf)
{ 
    UNUSED(argc);
    UNUSED(arg_tags);
    UNUSED(rest_line);
    printf("cfg param list unsupported\n");
    return 0;
}

static int
param_cmd_ad(int argc, const int *arg_tags,
              const char *rest_line, char *err_buf)
{
    UNUSED(argc);
    UNUSED(arg_tags);
    UNUSED(rest_line);
    printf("cfg param add/delete unsupported\n");
    return 0;
}


/**
 * Put words in line into string array.
 */
static string_array_t *
parse_string_array(const char *line)
{
    static char buf[300];
    size_t tok_len;
    string_array_t *names = cwmp_str_array_alloc(NULL, NULL);

    while ((tok_len = cli_token_copy(line, buf)) > 0 )
    {
        cwmp_str_array_add(names, buf, "", VA_END_LIST);
        line += tok_len;
    }
    return names;
}

/**
 * Parse 
 */
static ParameterNames *
parse_cwmp_ParameterNames(const char *line)
{
    static ParameterNames par_names;

    string_array_t *names = parse_string_array(line);

    par_names.__ptrstring = names->items;
    par_names.__size      = names->size;

    free(names);

    return &par_names;
}

/**
 * Fill to_cpe field in cwmp_data, get text human inserted info from line.
 *
 * Expects that rpc_cpe is set correctly.
 */
static te_errno
parse_cwmp_rpc_args(acse_epc_cwmp_data_t *cwmp_data,
                    const char *line, char *err_buf)
{
    static char buf[300];

    assert(cwmp_data != NULL);

    switch(cwmp_data->rpc_cpe)
    {
        case CWMP_RPC_get_rpc_methods:
        case CWMP_RPC_NONE:
            cwmp_data->to_cpe.p = NULL;
            break;
        case CWMP_RPC_set_parameter_values: 
        {
            static cwmp_set_parameter_values_t req;
            static ParameterValueList pv_list;
            size_t tok_len; 
            char val_buf[200];
            char type_buf[20];
            int type;

            cwmp_values_array_t *val_arr;

            req.ParameterList = &pv_list; 
            val_arr = cwmp_val_array_alloc(NULL, NULL);
            /* TODO: good parse here with error check */

            strcpy(err_buf, "(<param_name> <type> <value> )+");

            while ((tok_len = cli_token_copy(line, buf)) > 0 )
            {
                line += tok_len;
                if (0 == (tok_len = cli_token_copy(line, type_buf)))
                    return TE_EFAIL;
                line += tok_len;
                tok_len = cli_token_copy(line, val_buf);
                if (0 == (tok_len = cli_token_copy(line, type_buf)))
                    return TE_EFAIL;
                line += tok_len;
                type = cwmp_val_type_s2i(type_buf);

                if (type == SOAP_TYPE_string)
                    cwmp_val_array_add(val_arr, buf, "", type, val_buf,
                                       VA_END_LIST);
                else /* we have int-like type */
                    cwmp_val_array_add(val_arr, buf, "",
                                       type, atoi(val_buf), VA_END_LIST);
            }

            pv_list.__ptrParameterValueStruct = val_arr->items;
            pv_list.__size                    = val_arr->size;
            cwmp_data->to_cpe.set_parameter_values = &req;
        }
        break;
        case CWMP_RPC_get_parameter_values: 
        {
            static _cwmp__GetParameterValues  req;

            strcpy(err_buf, "(<param_name> )+");

            req.ParameterNames_ = parse_cwmp_ParameterNames(line); 
            if (0 == req.ParameterNames_->__size)
                return TE_EFAIL;
            cwmp_data->to_cpe.get_parameter_values = &req;
        }
        break;
        case CWMP_RPC_get_parameter_names: 
        {
            static char name[256];
            static char *name_ptr = name;
            static cwmp_get_parameter_names_t req;
            size_t ofs;

            cwmp_data->to_cpe.get_parameter_names = &req;

            strcpy(err_buf, "{0|1} <param_name>");

            req.ParameterPath = &name_ptr;
            ofs= cli_token_copy(line, buf);
            if (0 == ofs) return TE_EFAIL;
            line += ofs;
            req.NextLevel = atoi(buf);

            ofs = cli_token_copy(line, name);
            if (0 == ofs) return TE_EFAIL;
        }
        break;
        case CWMP_RPC_get_parameter_attributes: 
        {
            static _cwmp__GetParameterAttributes  req;

            strcpy(err_buf, "(<param_name> )+");

            req.ParameterNames_ = parse_cwmp_ParameterNames(line); 
            if (0 == req.ParameterNames_->__size)
                return TE_EFAIL;
            cwmp_data->to_cpe.get_parameter_attributes = &req;
        }
        break;
        case CWMP_RPC_set_parameter_attributes: 
        {
            static char name[256];
            static char buf[256];
            string_array_t *ac_list = NULL;
            int notif = -1; 
            size_t ofs = 0;

            /* Parse only one parameter */

            strcpy(err_buf,
                   "<param_name> [<notification>] [<access_entity>]");

            ofs = cli_token_copy(line, name);
            if (0 == ofs) return TE_EFAIL;
            line += ofs;

            while ((ofs = cli_token_copy(line, buf)) != 0 &&
                    NULL == ac_list)
            {
                if (isdigit(buf[0]))
                    notif = atoi(buf);
                else if (buf[0] == '"')
                    ac_list = cwmp_str_array_alloc(NULL, NULL);
                else
                    ac_list = parse_string_array(line);
                line += ofs;
            }

            cwmp_data->to_cpe.set_parameter_attributes =
                        cwmp_set_attrs_alloc(name, notif, ac_list);
        }
        break;
        case CWMP_RPC_download: 
        {
            cwmp_download_t *download;
            static char num[256];
            static char url[256];
            static char len[256] = "0";
            size_t ofs, flen = 0;

            strcpy(err_buf,
                   "<num_of_file_type:1|2|3> <url> [length]");

            ofs = cli_token_copy(line, num);
            if (0 == ofs) return TE_EFAIL;
            line += ofs;

            ofs = cli_token_copy(line, url);
            if (0 == ofs) return TE_EFAIL;
            line += ofs;

            ofs = cli_token_copy(line, len);
            if (ofs > 0)
                flen = atoi(len);

            download = cwmp_download_alloc("Test From ACSE_CLI", 
                                           atoi(num), flen, url);

            cwmp_data->to_cpe.download = download;
        }
        break;
        case CWMP_RPC_add_object: 
        {
            static char add_name[256];
            static char *add_name_ptr = add_name;
            static cwmp_add_object_t req;
            size_t ofs;

            cwmp_data->to_cpe.add_object = &req;

            strcpy(err_buf, "<parentname>");

            req.ObjectName = add_name; 
            req.ParameterKey = "ACSE CLI"; 
            ofs = cli_token_copy(line, add_name);
            if (0 == ofs) return TE_EFAIL;
        }
        break;
        case CWMP_RPC_delete_object: 
        {
            static char del_name[256];
            static char *del_name_ptr = del_name;
            static cwmp_delete_object_t req;
            size_t ofs;

            cwmp_data->to_cpe.delete_object = &req;

            strcpy(err_buf, "<instancename>");

            req.ObjectName = del_name; 
            req.ParameterKey = "ACSE CLI"; 
            ofs = cli_token_copy(line, del_name);
            if (0 == ofs) return TE_EFAIL;
        }
        break;
        default:
            printf("parse input, RPC %s is not supported yet :(\n", 
                cwmp_rpc_cpe_string(cwmp_data->rpc_cpe));
    }
    return 0;
}

static int
rpc_send(int argc, const int *arg_tags,
         const char *rest_line, char *err_buf)
{
    te_errno rc, status;
    acse_epc_cwmp_data_t *cwmp_data = NULL;

    if (argc < 3)
        return CLI_E_MISS_TAGS;

    /* command here: rpc send <rpcname> [<rpc args>...] */

    rc = acse_cwmp_prepare(acs_def_name, cpe_def_name,
                           EPC_RPC_CALL, &cwmp_data);
    if (0 != rc)
    {
        printf("cwmp prepare failed: %s\n", te_rc_err2str(rc));
        return CLI_E_EXEC;
    }

    cwmp_data->rpc_cpe = arg_tags[2]; 

    rc = parse_cwmp_rpc_args(cwmp_data, rest_line, err_buf);
    if (rc != 0)
    {
#if 1
        fprintf(stderr, "parse cwmp data failed: %s\n", te_rc_err2str(rc));
#endif
        return CLI_E_SPECIFIC;
    }

    rc = acse_cwmp_call(&status, NULL, &cwmp_data);
    if (0 != rc)
    {
        printf("cwmp call failed: %s\n", te_rc_err2str(rc));
        return CLI_E_EXEC;
    }
    else
    { 
        printf("status%s, request_id %d\n",
               te_rc_err2str(status), cwmp_data->request_id);
        request_id = cwmp_data->request_id;
    }

    return 0;
}

static int
rpc_check(int argc, const int *arg_tags,
          const char *rest_line, char *err_buf)
{
    te_errno rc, status;
    acse_epc_cwmp_data_t *cwmp_data = NULL;

    UNUSED(arg_tags);

    if (argc < 2)
        return CLI_E_MISS_TAGS;

    acse_cwmp_prepare(acs_def_name, cpe_def_name,
                      EPC_RPC_CHECK, &cwmp_data);

    while (isspace(*rest_line)) rest_line++;

    if (isdigit(rest_line[0]))
        cwmp_data->request_id = atoi(rest_line);
    else if (rest_line[0] == 0)
        cwmp_data->request_id = request_id;
    else 
        PARSE_ERROR("[<request_id>]");


    rc = acse_cwmp_call(&status, NULL, &cwmp_data);
    if (0 != rc)
    {
        printf("ACSE check failed: %s\n", te_rc_err2str(rc));
        return -1;
    }
    else
        print_cwmp_response(status, cwmp_data);

    return 0;
}



static int
rpc_wait(int argc, const int *arg_tags,
         const char *rest_line, char *err_buf)
{
    te_errno rc, status;
    acse_epc_cwmp_data_t *cwmp_data = NULL;
    int exp_request_id = 0;
    int timeout = timeout_def;

    UNUSED(arg_tags);

    if (argc < 2)
        return CLI_E_MISS_TAGS;

    while (isspace(*rest_line)) rest_line++;

    if (isdigit(rest_line[0]))
        exp_request_id = atoi(rest_line);
    else if (rest_line[0] == 0)
        exp_request_id = request_id;
    else 
        PARSE_ERROR("[<request_id>]");

    do {
        acse_cwmp_prepare(acs_def_name, cpe_def_name,
                          EPC_RPC_CHECK, &cwmp_data);

        cwmp_data->request_id = exp_request_id;


        rc = acse_cwmp_call(&status, NULL, &cwmp_data);
        if (0 != rc)
        {
            printf("ACSE check failed: %s\n", te_rc_err2str(rc));
            return -1;
        }
        else
            print_cwmp_response(status, cwmp_data);

    } while (((timeout--) > 0) &&
             (TE_EPENDING == TE_RC_GET_ERROR(status)) &&
             (sleep(1) == 0) );

    return 0;
}


static int
rpc_get_acs(int argc, const int *arg_tags,
            const char *rest_line, char *err_buf)
{
    UNUSED(argc);
    UNUSED(arg_tags);
    UNUSED(rest_line);
    UNUSED(err_buf);
    printf("get ACS RPC unsupported\n");
    return 0;
}

static int
cr_cmd(int argc, const int *arg_tags,
       const char *rest_line, char *err_buf)
{
    te_errno rc, status = 0;
    acse_epc_cwmp_data_t  *cwmp_data;

    UNUSED(rest_line);

    if (argc != 2)
        return CLI_E_MISS_TAGS;

    acse_cwmp_prepare(acs_def_name, cpe_def_name,
                      arg_tags[1], &cwmp_data);

    rc = acse_cwmp_call(&status, NULL, &cwmp_data);
    if (0 != rc)
    {
        printf("CWMP call failed: %s\n", te_rc_err2str(rc));
        return CLI_E_EXEC;
    }
    print_cwmp_response(status, cwmp_data);
    return 0;
}


static int
env_set(int argc, const int *arg_tags,
        const char *rest_line, char *err_buf)
{
    size_t len;
    char new_value[100];
    len = cli_token_copy(rest_line, new_value);

    if (argc < 2)
        return CLI_E_MISS_TAGS;

    if (len > 0) /* Is there new value? */
        switch (arg_tags[1])
        {
            case ENV_ACS:
                strncpy(acs_def_name, new_value, sizeof(acs_def_name));
                break;
            case ENV_CPE:
                strncpy(cpe_def_name, new_value, sizeof(cpe_def_name));
                break;
            case ENV_TIMEOUT:
                timeout_def = atoi(new_value);
                break;
            default:
                printf("env_set: wrong tag!\n");
        }
    else
        switch (arg_tags[1])
        {
            case ENV_ACS:
                printf("%s\n", acs_def_name);
                break;
            case ENV_CPE:
                printf("%s\n", cpe_def_name);
                break;
            case ENV_TIMEOUT:
                printf("%d\n", timeout_def);
                break;
            default:
                printf("env_set: wrong tag!\n");
        }
    return 0;
}


static int
http_resp(int argc, const int *arg_tags,
          const char *rest_line, char *err_buf)
{
    char code_buf[100];
    char direction_buf[200] = {0,};
    int http_code;
    size_t ofs;

    UNUSED(argc);
    UNUSED(arg_tags);

    ofs = cli_token_copy(rest_line, code_buf);
    if (0 == ofs || !isdigit(code_buf[0]))
        PARSE_ERROR("<http_code> [<direction URL>]");
    rest_line += ofs;
    rest_line += cli_token_copy(rest_line, direction_buf);
    http_code = atoi(code_buf);
    RING("http response %d to '%s'", http_code, direction_buf);
    acse_http_code(acs_def_name, cpe_def_name, http_code, direction_buf);
    return 0;
}

static cli_cmd_descr_t cmd_param_actions[] = {
    {"obtain", EPC_CFG_OBTAIN, "Obtain ACS/CPE config param",
            param_cmd_access, NULL},
    {"modify", EPC_CFG_MODIFY, "Modify ACS/CPE config param",
            param_cmd_access, NULL},
    {"wait", EPC_CFG_OBTAIN, "wait particular value of ACS config param",
            param_cmd_wait, NULL},
    {"list", EPC_CFG_LIST, "List config params",
            param_cmd_list, NULL},
    {"add", EPC_CFG_ADD, "add config param",
            param_cmd_ad, NULL},
    {"del", EPC_CFG_DEL, "del config param",
            param_cmd_ad, NULL},
    END_CMD_ARRAY
};

static cli_cmd_descr_t cmd_param_lev[] = {
    {"acs", EPC_CFG_ACS, "ACS config commands", NULL, cmd_param_actions},
    {"cpe", EPC_CFG_CPE, "CPE config commands", NULL, cmd_param_actions},
    END_CMD_ARRAY
};

static cli_cmd_descr_t cmd_rpc_cpe_kinds[] = {
    {"fin",          CWMP_RPC_NONE, "HTTP 204, finish CWMP session",
                                NULL, NULL},
    {"get_rpc_m",    CWMP_RPC_get_rpc_methods, "GetRPCMethods", NULL, NULL},
    {"get_par_vals", CWMP_RPC_get_parameter_values, 
                    "GetParameterValues", NULL, NULL},
    {"set_par_vals", CWMP_RPC_set_parameter_values,
                    "SetParameterValues", NULL, NULL},
    {"get_names",    CWMP_RPC_get_parameter_names,
                    "GetParameterNames", NULL, NULL},
    {"get_attrs",    CWMP_RPC_get_parameter_attributes,
                    "GetParameterAttributes", NULL, NULL},
    {"set_attrs",    CWMP_RPC_set_parameter_attributes,
                    "SetParameterAttributes", NULL, NULL},
    {"add",          CWMP_RPC_add_object,
                    "AddObject", NULL, NULL},
    {"delete",       CWMP_RPC_delete_object,
                    "DeleteObject", NULL, NULL},
    {"download",     CWMP_RPC_download,
                    "Download", NULL, NULL},
    END_CMD_ARRAY
};

static cli_cmd_descr_t cmd_rpc_actions[] = {
    {"send",  EPC_RPC_CALL,  "Send CWMP RPC", rpc_send, cmd_rpc_cpe_kinds},
    {"check", EPC_RPC_CHECK, "Check RPC status", rpc_check, NULL},
    {"get",   EPC_RPC_CHECK, "Get CWMP ACS RPC", rpc_get_acs, NULL},
    {"wait",  EPC_RPC_CHECK, "Wait RPC OK or Fault", rpc_wait, NULL},
    END_CMD_ARRAY
};

static cli_cmd_descr_t cmd_cr_actions[] = {
    {"send",  EPC_CONN_REQ, "Send ConnectionRequest", NULL, NULL},
    {"check", EPC_CONN_REQ_CHECK, "Check Conn.Request", NULL, NULL},
    END_CMD_ARRAY
};

static cli_cmd_descr_t cmd_env[] = {
    {"acs",     ENV_ACS, "default ACS name", NULL, NULL},
    {"cpe",     ENV_CPE, "default CPE name", NULL, NULL},
    {"timeout", ENV_TIMEOUT, "CPE config commands", NULL, NULL},
    END_CMD_ARRAY
};

static cli_cmd_descr_t acse_cmd_list[] = {
    {"param",CMD_PARAM,"config parameters", NULL, cmd_param_lev},
    {"rpc",  CMD_RPC,  "CWMP RPC commands", NULL, cmd_rpc_actions},
    {"cr",   CMD_CR,   "Connection Req. commands", cr_cmd, cmd_cr_actions},
    {"env",  CMD_ENV,  "Current environment", env_set, cmd_env},
    {"http", CMD_HTTP, "Unusual HTTP response", http_resp, NULL},
    END_CMD_ARRAY
};


static void
print_rpc_response(acse_epc_cwmp_data_t *cwmp_resp)
{
    int i;
    switch (cwmp_resp->rpc_cpe)
    {
    case CWMP_RPC_get_rpc_methods: 
    {
        MethodList *mlist;
        if ((mlist = cwmp_resp->from_cpe.get_rpc_methods_r->MethodList_)
                != NULL)
        {
            printf("RPC methods: ");
            for (i = 0; i < mlist->__size; i++)
                printf("'%s', ", mlist->__ptrstring[i]);
            printf("\n");
        }
    }
        break;
    case CWMP_RPC_set_parameter_values: 
        printf("Set status: %d\n", 
               cwmp_resp->from_cpe.set_parameter_values_r->Status);
        break;
    case CWMP_RPC_get_parameter_values: 
    {
        char buf[300];
        ParameterValueList *pv_list =
            cwmp_resp->from_cpe.get_parameter_values_r->ParameterList;

        for (i = 0; i < pv_list->__size; i++)
        {
            snprint_ParamValueStruct(buf, sizeof(buf),
                            pv_list->__ptrParameterValueStruct[i]);
            printf("  %s\n", buf);
        }
    }
    break;
    case CWMP_RPC_get_parameter_names: 
    {
        struct cwmp__ParameterInfoStruct *item;
        ParameterInfoList *pi_list =
            cwmp_resp->from_cpe.get_parameter_names_r->ParameterList;

        for (i = 0; i < pi_list->__size; i++)
        {
            item = pi_list->__ptrParameterInfoStruct[i];
            printf("  (%c) %s\n", item->Writable ? 'W' : '-', item->Name);
        }
    }
    break;
    case CWMP_RPC_set_parameter_attributes: 
        /* empty response, do nothing */
        break;
    case CWMP_RPC_get_parameter_attributes: 
    {
        struct cwmp__ParameterAttributeStruct *pa_item;
        ParameterAttributeList *pa_list = 
            cwmp_resp->from_cpe.get_parameter_attributes_r->ParameterList;

        for (i = 0; i < pa_list->__size; i++)
        {
            pa_item = pa_list->__ptrParameterAttributeStruct[i];
            printf(" Attrubutes of %s: \n\tNotification %d\n\tAccessList ",
                    pa_item->Name, (int)pa_item->Notification);
            if (NULL != pa_item->AccessList_ && 
                0    != pa_item->AccessList_->__size)
            {
                int j;
                for (j = 0; j < pa_item->AccessList_->__size; j++)
                    printf("%s; ", pa_item->AccessList_->__ptrstring[j]);
                printf("\n");
            }
            else
                printf("(empty)");
        }

    }
    break;
    case CWMP_RPC_download: 
        printf("Download status: %d\n", 
               cwmp_resp->from_cpe.download_r->Status);
    break;
    case CWMP_RPC_add_object: 
        printf("Add status: %d, instance %d\n", 
               cwmp_resp->from_cpe.add_object_r->Status,
               cwmp_resp->from_cpe.add_object_r->InstanceNumber);
    break;

    case CWMP_RPC_delete_object: 
        printf("Delete status: %d\n", 
               cwmp_resp->from_cpe.delete_object_r->Status);
    case CWMP_RPC_NONE:
    case CWMP_RPC_reboot: 
    case CWMP_RPC_upload: 
    case CWMP_RPC_factory_reset: 
    case CWMP_RPC_get_queued_transfers: 
    case CWMP_RPC_get_all_queued_transfers: 
    case CWMP_RPC_schedule_inform: 
    case CWMP_RPC_set_vouchers: 
    case CWMP_RPC_get_options: 
    case CWMP_RPC_FAULT: 
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
        printf("RPC call '%s' to %s/%s, id %d\n",
                cwmp_rpc_cpe_string(cwmp_resp->rpc_cpe),
                cwmp_resp->acs, cwmp_resp->cpe, cwmp_resp->request_id);
        break;
    case EPC_RPC_CHECK:
        printf("RPC check, '%s' to %s/%s, status %s\n",
                cwmp_rpc_cpe_string(cwmp_resp->rpc_cpe),
                cwmp_resp->acs, cwmp_resp->cpe,
                te_rc_err2str(status));
        if (0 == status)
            print_rpc_response(cwmp_resp);
        if (TE_CWMP_FAULT == TE_RC_GET_ERROR(status))
        {
#define FAULT_BUF_SIZE 0x8000
            char *fault_buf = malloc(FAULT_BUF_SIZE);
            snprint_cwmpFault(fault_buf, FAULT_BUF_SIZE,
                              cwmp_resp->from_cpe.fault);
            printf("%s\n", fault_buf);
            free(fault_buf);
        }
        break;
    case EPC_GET_INFORM:
        {
            _cwmp__Inform *inform = cwmp_resp->from_cpe.inform;
            printf("Get Inform from %s/%s, id %d\n",
                    cwmp_resp->acs, cwmp_resp->cpe, cwmp_resp->request_id);
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
    case EPC_HTTP_RESP:
        /* Nothing to do */
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
int   acse_fork = 0;

char *cli_logfile = NULL;
char *acse_logfile = NULL;

struct poptOption acse_cli_opts[] = 
{
    {"epc-socket", 'e', POPT_ARG_STRING, &epc_sock_name, 0,
            "filename for EPC socket", "EPC socket"},
    {"fork",       'f', POPT_ARG_NONE,   &acse_fork, 0,
            "whether to fork", "flag to fork"},
    {"script",     's', POPT_ARG_STRING, &script_name, 0, 
            "filename with list of commands to perform before operation",
            "script"},
#ifndef CLI_SINGLE
    {"daemon-logfile",'d', POPT_ARG_STRING, &acse_logfile, 0,
            "filename for ACSE daemon logfile", "CLI logfile"},
#endif
    {"cli-logfile",'c', POPT_ARG_STRING,   &cli_logfile, 0,
            "filename for CLI logfile", "CLI logfile"},
    {NULL, 0, 0, NULL, 0, NULL, NULL}
};

static int
dummy_init()
{
    acs_t *acs;
    cpe_t *cpe;
    
    db_add_acs(acs_def_name);
    db_add_cpe(acs_def_name, cpe_def_name);

    acs = db_find_acs(acs_def_name);
    cpe = db_find_cpe(acs, acs_def_name, cpe_def_name);

    acs->port = 8080;

#if 0
    /* login for konst home box */
    cpe->acs_auth.login =
        strdup("000261-Home Gateway-V601L622R1A0-1001742119");
    cpe->acs_auth.passwd = strdup("z7cD7CTDA1DrQKUb");
#else
    /* login for bolger box */
    cpe->acs_auth.login =
        strdup("000261-Home Gateway-V60200000000-0010501606");
    cpe->acs_auth.passwd = strdup("ry7TSp8ry7RAdyxRp9BkBXzSrh8DBSdH");
#endif
    cpe->cr_auth.login  = strdup(cpe->acs_auth.login);
    cpe->cr_auth.passwd = strdup(cpe->acs_auth.passwd);
            
    /* acse_enable_acs(acs); */
    return 0;
}

int 
main(int argc, const char **argv)
{
    te_errno rc;
    FILE *script_fd = NULL;
    int rpoll, rpopt;
    poptContext cont;
    pid_t acse_main_pid = 0;
    int acse_main_status;

    strcpy(acs_def_name, "A");
    strcpy(cpe_def_name, "box");

    cont = poptGetContext(NULL, argc, argv, acse_cli_opts, 0);
    
    rpopt = poptGetNextOpt(cont); /* this really parse all command line */

#ifndef CLI_SINGLE
    if (acse_fork)
    {
        acse_main_pid = fork();
        if (acse_main_pid == 0)
        {

            te_lgr_entity = "ACSE daemon";
            if (acse_logfile != NULL)
            {
                FILE   *log_fd = fopen(acse_logfile, "a");
                if (log_fd == NULL)
                {
                    perror("open ACSE logfile failed");
                    exit(1);
                }
                te_log_message_file_out = log_fd;
            }
            dummy_init();

            if ((rc = acse_epc_disp_init(NULL, NULL)) != 0)
            {
                ERROR("Fail create EPC dispatcher %r", rc);
                return 1;
            }

            acse_loop();
            exit(0);
        }
        if (acse_main_pid < 0)
        {
            perror("fork failed");
            exit(2);
        }
        /* Now we in CLI process, just continue.*/
    }
#endif /* CLI_SINGLE */
    if (cli_logfile != NULL)
    {
        FILE   *log_fd = fopen(cli_logfile, "a");
        if (log_fd == NULL)
        {
            perror("open CLI logfile failed");
            exit(1);
        }
        te_log_message_file_out = log_fd;
    }

    if ((rc = acse_epc_open(epc_sock_name, NULL, ACSE_EPC_CLIENT))
        != 0)
    {
        ERROR("open EPC failed %r", rc);
        return 1;
    }
    atexit(&cli_exit_handler);

    if (NULL != script_name)
    {
        if ((script_fd = fopen(script_name, "r")) == NULL)
            perror("open script failed: ");
    }

    printf("\n> "); fflush(stdout);

    /* TODO process command line script */
    /* main loop */
    while (1)
    {
        struct pollfd pfd[2];

        pfd[0].fd = 0; /* stdin */
        pfd[0].events = POLLIN;
        pfd[0].revents = 0;
        pfd[1].fd = acse_epc_socket();
        pfd[1].events = POLLIN;
        pfd[1].revents = 0;

        if (NULL == script_fd &&
            (rpoll = poll(pfd, 2, -1)) < 0)
        {
            perror("poll failed");
            break;
        }

        if (pfd[0].revents || script_fd != NULL)
        {
            char buf[BUF_SIZE];

            ssize_t r;
            if (script_fd != 0)
            {
                if (fgets(buf, BUF_SIZE, script_fd) == NULL)
                {
                    fclose(script_fd);
                    script_fd = NULL;
                    continue;
                }
                if (buf[0] == '#')
                    continue;
                printf("$ %s", buf);
            }
            else
            {
                r = read(pfd[0].fd, buf, BUF_SIZE);
                if (r < 0)
                {
                    perror("read fail");
                    break;
                }
                if (r == 0) /* The end of input */
                    break;
                buf[r] = '\0';
            }

            cli_perform_cmd(acse_cmd_list, buf);
            printf("> "); fflush(stdout);
        }
        /* Really, this should not happen, EPC messages are received and 
           processed while perform command above. */
        if (pfd[1].revents) 
        {
            acse_epc_msg_t msg_resp;
            rc = acse_epc_recv(&msg_resp);
            if (TE_RC_GET_ERROR(rc) == TE_ENOTCONN)
                break;
            else if (rc != 0)
                RING("EPC recv error %r", rc);
            switch (msg_resp.opcode)
            {
                case EPC_CONFIG_RESPONSE:
                    print_config_response(msg_resp.status,
                                          msg_resp.data.cfg);
                    break;
                case EPC_CWMP_RESPONSE:
                    print_cwmp_response(msg_resp.status,
                                        msg_resp.data.cwmp);
                    break;
                default:
                    ERROR("Unexpected opcode 0x%x from EPC",
                         msg_resp.opcode);
            }
            printf("> "); fflush(stdout);
        }
    }
    if ((rc = acse_epc_close()) != 0)
    {
        ERROR("CLI: EPC close failed %r", rc);
    }
#ifndef CLI_SINGLE
    acse_main_status = 0;
    waitpid(acse_main_pid, &acse_main_status, 0);
    if (acse_main_status)
    {
        WARN("ACSE finished with status %d", acse_main_status);
    }
#endif
    return 0;
}

