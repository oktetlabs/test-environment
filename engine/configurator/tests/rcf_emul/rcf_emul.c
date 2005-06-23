/** @file
 * @brief Configurator Tester
 *
 * RCF Emulator implementation
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 *
 * @author  Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 */

#define LOG_LEVEL 0xff
#define TE_LOG_LEVEL 0xff
#include "rcf_emul.h"

/** One request from the user */
typedef struct usrreq {
    rcf_msg                  *message; /**< Request message */
    struct ipc_server_client *user;    /**< RCF client IPC server */
} usrreq;

/** IPC server used by the RCF Emulator for ipc communications */
static struct ipc_server *server = NULL;
/** IPC server descriptor */
static int    server_fd;

/**
 * If handlers_state[REQUEST_TYPE] !=0 than there is a handler
 * registered for REQUEST_TYPE type of the request.
 */
static request_handler *current_handler_conf;

/**
 * Configurations set.
 */
static handler_configuration  handler_conf[MAX_CONF_NUMBER];

/**
 * Group start/end controller.
 * If strlen(current_group) == 0 then no group is started.
 */
char current_group[RCF_MAX_NAME];

/**
 * Agents list.
 */
static agent agents_list[MAX_AGENTS_NUMBER];

/**
 * Retreives configuration with the given ID from the
 * configurations database.
 */
request_handler *
rcf_get_cfg_by_id(int id)
{
    if (id < 0 || id > MAX_CONF_NUMBER)
        return NULL;

    return handler_conf[id];
}

/**
 * Sends the answer to the user request
 *
 * @param req        Request to answer, req->message field should
 *                   contain the answer.
 */
static void
answer_user_request(usrreq *req)
{
    if (req->user != NULL)
    {

        int rc = ipc_send_answer(server, req->user, (char *)req->message,
                                 sizeof(rcf_msg) + req->message->data_len);
        if (rc != 0)
        {
            ERROR("Cannot send an answer to user: errno %d", rc);
            RING("Failed msg has: opcode %d; TA %s; SID %d; file %s;",
                 req->message->opcode,
                 req->message->ta,
                 req->message->sid,
                 req->message->file);
        }
        free(req->message);
        free(req);
    }
}

/** 
 * Returns pointer to the current handler for the
 * given event type.
 */
void *
get_handler(rcf_op_t opcode)
{
    switch (opcode)
    {
        case RCFOP_TALIST:
            return current_handler_conf->ta_list;
        case RCFOP_TACHECK:
            return current_handler_conf->ta_check;
        case RCFOP_REBOOT:
            return current_handler_conf->reboot;
        case RCFOP_CONFGET:
            return current_handler_conf->conf_get;
        case RCFOP_CONFSET:
            return current_handler_conf->conf_set;
        case RCFOP_CONFADD:
            return current_handler_conf->conf_add;
        case RCFOP_CONFDEL:
            return current_handler_conf->conf_del;
        case RCFOP_CONFGRP_START:
            return current_handler_conf->conf_grp_start;
        case RCFOP_CONFGRP_END:
            return current_handler_conf->conf_grp_end;
        default:
            return NULL;
    }
}

/** 
 * Writes buffer to the file with name msg->file.
 *
 * @param msg        Message the data of which should be written to
 *                   the file.
 * @param buf        Buffer to write to the file.
 * @param buf_len    Length of the buffer to be written.
 *
 * @return           0 - if everything is correct
 * @retval           ENOENT - if failed to open file with name
 *                   msg->file for writing
 * @retval           ENOSPC - failed to write buffer to the file
 */
static int
write_binary_attachment(rcf_msg *msg,  char *buf, uint32_t buf_len)
{
    int file;
    int rc;

    if ((file = open(msg->file, O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRWXU | S_IRWXG | S_IRWXO)) < 0)
    {
        ERROR("Cannot open file %s for writing - skipping\n",
              msg->file);
        return ENOENT;
    }

    rc = write(file, buf, buf_len + sizeof('\0'));
    if (rc < 0)
    {
        ERROR("Failed to write buffer to the file");
        return ENOSPC;
    }

    msg->flags |= BINARY_ATTACHMENT;
    close(file);
    return 0;
}

/**
 * Process a request from the user: send the command to the Test Agent or
 * put the request to the pending queue.
 *
 *
 * @param req           user request structure
 *
 * @return              Error returned by the request handler or:
 * @retval ETEIO        no handler is registered for 
 *                      the req->message->opcode type of request
 * @retval EINVAL       if wrong opcode is given
 */

static int
process_user_request(usrreq *req)
{
    rcf_msg *msg = req->message;
    void    *handler;
    int      rc = -1;
    char    *data;
    int      data_len;

    msg->error = 0;
    VERB("Request %s is received, msg->id = %s", 
         rcf_op_to_string(msg->opcode),
         msg->id);

#if SUPPORT_TE_LOGGER
    /**
     * TODO: dirty hack, it is required to handle logger's 
     * ta_list request, which is received before any of the
     * request handlers is registered.
     */
    if (msg->opcode == RCFOP_TALIST)
    {
        char buf[MAX_AGENTS_NUMBER * AGENT_NAME_MAX_LENGTH + 1];

        rcfrh_agents_list(buf, MAX_AGENTS_NUMBER * AGENT_NAME_MAX_LENGTH + 1);
        strcpy(msg->data, buf);
        msg->data_len = strlen(msg->data) + 1;
        goto answer;        
    }
    else if (msg->opcode == RCFOP_TATYPE)
    {
        if (rcfrh_is_agent(msg->ta) > 0)
            strcpy(msg->id, rcfrh_agent_get_type(msg->ta));
        else
            msg->error = ETEIO;
        goto answer;
    }
    else if (msg->opcode == RCFOP_GET_LOG)
    {
        strcpy(msg->file, TA_LOG_FILE);
        msg->data_len = 0;
        rc = write_binary_attachment(msg, "", -sizeof('\0'));
        if (rc < 0)
        {
            ERROR("Failed to write agents log to the file, "
                  "rc = %d", rc);
            msg->error = rc;
        }
        goto answer;
    }
#endif

    handler = get_handler(msg->opcode);
    if (handler == NULL)
    {
        ERROR("No handler is set for %s type of request", 
              rcf_op_to_string(msg->opcode));
        msg->error = ETEIO;
        goto answer;
    }

    switch (msg->opcode)
    {
#if 0
        /**
         * TODO: currently this case is unused, because TE Logger is
         * not able to work correct without RCFOP_TALIST handler
         * registered.
         */
        case RCFOP_TALIST:
            ((rcfrh_ta_list) handler)(&data);
            strcpy(msg->data, data);
            msg->data_len = strlen(msg->data);
            free(data);
            break;
#endif
        case RCFOP_TACHECK:
            ((rcfrh_ta_check) handler)(msg->ta, &rc);
            if (rc < 0)
                msg->error = rc;
            break;
        case RCFOP_REBOOT:
            ((rcfrh_reboot) handler)(msg->ta, &rc);
            if (rc < 0)
                msg->error = rc;
            break;
        case RCFOP_CONFGET:
            rc = ((rcfrh_conf_get) handler)(msg->ta, 
                                            msg->id, 
                                            &data, 
                                            &data_len);
            if (rc < 0)
            {
                ERROR("Failed to handle conf get request with "
                      "msg->id = %s", msg->id);
                msg->error = rc;
                break;
            }
            if (strchr(msg->id, '*') != NULL || strstr(msg->id, "...") != NULL)
            {
                strcpy(msg->file, BIN_ATTACH_FILE_NAME);
                msg->data_len = 0;
                rc = write_binary_attachment(msg, data, data_len);
                if (rc < 0)
                {
                    ERROR("Failed to write binary attachment to the file, "
                          "rc = %d", rc);
                    msg->error = rc;
                    break;
                }
            }
            else
            {
                strcpy(msg->value, data);
                msg->data_len = 0;
            }
            break;
        case RCFOP_CONFSET:
            rc = ((rcfrh_conf_set) handler)(msg->ta,
                                            msg->id,
                                            msg->value);
            if (rc < 0)
            {
                ERROR("Failed to set instance value for "
                      "instance %s", msg->id);
                msg->error = rc;
            }
            break;
        case RCFOP_CONFADD:
            rc = ((rcfrh_conf_add) handler)(msg->ta,
                                            msg->id,
                                            msg->value);
            if (rc < 0)
            {
                ERROR("Failed to add entity with id = %s and "
                      "val = %s", msg->id, msg->value);
                msg->error = -rc;
            }
            break;
        case RCFOP_CONFDEL:
            rc = ((rcfrh_conf_del) handler)(msg->ta,
                                            msg->id);
            if (rc != 0)
            {
                ERROR("Failed to delete entity with id : %s", 
                      msg->id);
                msg->error = rc;
            }
            break;
        case RCFOP_CONFGRP_START:
            rc = ((rcfrh_conf_grp_start) handler)(msg->ta,
                                                  msg->value);
            if (rc < 0)
            {
                ERROR("Failed to start configurator command "
                      "group with name %s", msg->value);
                msg->error = rc;
            }
            break;
        case RCFOP_CONFGRP_END:
            rc = ((rcfrh_conf_grp_end) handler)(msg->ta,
                                                msg->value);
            if (rc < 0)
            {
                ERROR("Failed to end  configurator command "
                      "group with name %s", msg->value);
                msg->error = rc;
            }
            break;
        default:
            ERROR("ERROR, opcode is not supported");
            msg->error = EINVAL;
    }
answer:
    answer_user_request(req);
    return msg->error;
}

/** 
 * Inits RCF Emulator and related structures (database).
 *
 * @return     error, returned by the db_init() call
 */
static inline int
rcfrh_init(char *data_base_file_name)
{
    memset(&handler_conf, 0, sizeof(request_handler) * MAX_CONF_NUMBER);
    memset(agents_list, 0, sizeof(agents_list));
    rcfrh_agent_add("Agt_T", LINUX);
    return db_init(data_base_file_name);
}

/**
 * Shuts down the RCF Emulator.
 *
 * @return     error, returned by the db_free() call;
 */
static inline int
rcfrh_shutdown(void)
{
    int i;
    for (i = 0; i < MAX_CONF_NUMBER; i++)
    {
        if (handler_conf[i] != NULL)
            free(handler_conf[i]);
    }
    return db_free();
}

/**
 * Sets default handler in the given configuration for the
 * given type of request.
 */
int
rcfrh_set_default_handler(rcf_op_t opcode, request_handler *conf)
{
    switch (opcode)
    {
        case RCFOP_TALIST:
            conf->ta_list = &rcfrh_ta_list_default;
            break;
        case RCFOP_TACHECK:
            conf->ta_check = &rcfrh_ta_check_default;
            break;
        case RCFOP_REBOOT:
            conf->reboot = &rcfrh_reboot_default;
            break;
        case RCFOP_CONFGET:
            conf->conf_get = &rcfrh_conf_get_default;
            break;
        case RCFOP_CONFSET:
            conf->conf_set = &rcfrh_conf_set_default;
            break;
        case RCFOP_CONFADD:
            conf->conf_add = &rcfrh_conf_add_default;
            break;
        case RCFOP_CONFDEL:
            conf->conf_del = &rcfrh_conf_del_default;
            break;
        case RCFOP_CONFGRP_START:
            conf->conf_grp_start = &rcfrh_conf_grp_start_default;
            break;
        case RCFOP_CONFGRP_END:
            conf->conf_grp_end = &rcfrh_conf_grp_end_default;
            break;
        default:
            VERB("Wrong opcode passed to the "
                 "rcfrh_set_default_handler() function");
            return EINVAL;
    }
    return 0;
}

/**
 * Sets default handlers for all request in the configuration 
 * with the given ID.
 */
int
rcfrh_set_default_handlers(int conf_id)
{
    if (conf_id < 0 || conf_id > MAX_CONF_NUMBER || 
        handler_conf[conf_id] == NULL)
        return EINVAL;

    rcfrh_set_default_handler(RCFOP_TALIST, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_TACHECK, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_REBOOT, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_CONFGET, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_CONFSET, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_CONFADD, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_CONFDEL, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_CONFGRP_START, handler_conf[conf_id]);
    rcfrh_set_default_handler(RCFOP_CONFGRP_END, handler_conf[conf_id]);
    return 0;
}

/**
 * Main function of the RCF Emulator. Must be called in the separate
 * thread.
 */
void*
rcf_emulate(void* param)
{
    usrreq               *req = NULL;
    static                fd_set set0;
    static struct timeval tv0;   int     rc = -1;

    VERB("Starting RCF Emulator");
    rc = rcfrh_init((char *)param);
    if (rc != 0)
    {
        ERROR("Failed to initalize database with the configuration file %s, "
              "rc = %d", (char *)param, rc);
        goto error;
    }

    /* Registering ipc server */
    ipc_init();
    if (ipc_register_server(RCF_SERVER, &server) != 0)
    {
        ERROR("Failed to register IPC RCF_SERVER\n");
        goto error;
    }

    FD_ZERO(&set0);
    server_fd = ipc_get_server_fd(server);
    FD_SET(server_fd, &set0);
    tv0.tv_sec = RCF_SELECT_TIMEOUT;
    tv0.tv_usec = 0;

    VERB("Initialization is finished\n");
    while (1)
    {
        struct timeval  tv = tv0;
        fd_set          set = set0;
        size_t          len;
        rcf_msg        *msg;
        
        req = NULL;
        rc = -1;

        select(FD_SETSIZE, &set, NULL, NULL, &tv);

        if (FD_ISSET(server_fd, &set))
        {
            if ((req = (usrreq *)calloc(sizeof(usrreq), 1)) == NULL)
                goto error;

            if ((msg = (rcf_msg *)malloc(RCF_MAX_LEN)) == NULL)
            {
                free(req);
                goto error;
            }

            req->message = msg;
            len = RCF_MAX_LEN;
            if ((rc = ipc_receive_message(server, (char *)req->message,
                                          &len, &(req->user))) != 0)
            {
                ERROR("Failed to receive user request: errno=%d", rc);
                free(req->message);
                free(req);
                continue;
            }
            else
            {
                if (req->message->opcode == RCFOP_SHUTDOWN)
                {
                    VERB("Shutdown command is recieved");
                    break;
                }
                process_user_request(req);
            }
        }
    }

    if (req != NULL && req->message->opcode == RCFOP_SHUTDOWN)
        answer_user_request(req);

error:
    if (server != NULL)
        ipc_close_server(server);

    if (rc != 0)
        ERROR("Error exit");
    else
        RING("Exit");
    
    rcfrh_shutdown();
    return NULL;
}

/**
 * TA List default request handler.
 */
int
rcfrh_ta_list_default(char **ta_list)
{
    char *agents = calloc(MAX_AGENTS_NUMBER * AGENT_NAME_MAX_LENGTH + 1, 1);

    if (agents == NULL)
        return ENOMEM;
    rcfrh_agents_list(agents, MAX_AGENTS_NUMBER * AGENT_NAME_MAX_LENGTH + 1);
    
    *ta_list = agents;
    return 0;
}

/**
 * TA Check default request handler.
 */
int 
rcfrh_ta_check_default(char *ta_name, int *result)
{
    if (rcfrh_is_agent(ta_name) > 0)
    {
        *result = 0;
        return 0;
    }
    else
    {
        *result = EINVAL;
        return EINVAL;
    }
}

/**
 * Reboot default request handler.
 */
int
rcfrh_reboot_default(char *ta_name, int *result)
{
    if (rcfrh_is_agent(ta_name) > 0)
    {
        db_clear_agents_data(ta_name);

        *result = 0;
        return 0;
    }
    else
    {
        *result = EINVAL;
        return EINVAL;
    }
}

/**
 * Conf get default request handler.
 */
int
rcfrh_conf_get_default(char *ta_name, char *oid, char **answer, int *ans_len)
{
    int rc;

    if (rcfrh_is_agent(ta_name) > 0)
    {
        rc = db_get(oid, answer, ans_len);
            return rc;
    }
    else
        return EINVAL;
}

/**
 * Conf set default request handler.
 */
int
rcfrh_conf_set_default(char *ta_name, char *oid, char *value)
{
    int rc;

    if (rcfrh_is_agent(ta_name) > 0)
    {
        rc = db_set_inst(oid, value);
        if (rc != 0)
        {
            ERROR("Failed to set instance value for "
                  "instance %s", oid);
        }
    }
    else
        return EINVAL;

    return rc;
}

/**
 * Conf add default request handler.
 */
int
rcfrh_conf_add_default(char *ta_name, char *oid, char *value)
{
    int rc;

    if (rcfrh_is_agent(ta_name) > 0)
    {
        if (strchr(oid, ':') == NULL)
        {

            rc = db_add_object(oid);
            if (rc < 0)
            {
                ERROR("Failed to create object");
                return -rc;
            }
        }
        else
        {
            rc = db_add_instance(oid, value);
            if (rc < 0)
            {
                ERROR("Failed to create instance");
                return -rc;
            }
        }

        return rc;
    }

    ERROR("Wrong TA name %s", ta_name);
    return -EINVAL;
}

/**
 * Conf del default request handler.
 */
int 
rcfrh_conf_del_default(char *ta_name, char *oid)
{
    int rc;

    if (rcfrh_is_agent(ta_name) > 0)
    {
        rc = db_del(oid);
        if (rc != 0)
        {
            ERROR("Failed to delete entity with id : %s", oid);
            return rc;
        }
    }
    else 
        return EINVAL;

    return 0;
}

/**
 * Conf group start default request handler.
 */
int
rcfrh_conf_grp_start_default(char *ta_name, char *grp_name)
{
    if (rcfrh_is_agent(ta_name) > 0 && strlen(current_group) == 0)
    {
        strcpy(current_group, grp_name);
        return 0;
    }
    else
        return EINVAL;
}

/**
 * Conf group end default request handler.
 */
int
rcfrh_conf_grp_end_default(char *ta_name, char *grp_name)
{
    if (rcfrh_is_agent(ta_name) > 0 &&
        strcmp(current_group, grp_name) == 0)
    {
        current_group[0] = '\0';
        return 0;
    }
    else
        return EINVAL;
}

/**
 * Creates the configuration.
 */
int
rcfrh_configuration_create(void)
{
    int i = 0;

    while(i < MAX_CONF_NUMBER && handler_conf[i] != NULL)
        i++;

    if (i == MAX_CONF_NUMBER)
        return -ENOMEM;

    handler_conf[i] = (request_handler *)calloc(sizeof(request_handler), 1);
    if (handler_conf[i] == NULL)
        return -ENOMEM;

    return i;
}

/**
 * Deletes the configuration.
 */
int
rcfrh_configuration_delete(int conf_id)
{
    if (conf_id < 0 || conf_id > MAX_CONF_NUMBER ||
        handler_conf[conf_id] == NULL)
        return EINVAL;

    if (current_handler_conf == handler_conf[conf_id])
        current_handler_conf = NULL;

    free(handler_conf[conf_id]);
    handler_conf[conf_id] = NULL;
    return 0;
}

/**
 * Changes the crrent configuration.
 */
int 
rcfrh_configuration_set_current(int conf_id)
{    
    if (conf_id < 0 || conf_id > MAX_CONF_NUMBER ||
        handler_conf[conf_id] == NULL)
        return EINVAL;

    current_handler_conf = handler_conf[conf_id];
    return 0;
}

/**
 * Converts the type of the agent to the string form.
 */
static char *
rcfrh_agent_type2str(agent_type type)
{
    switch (type)
    {
        case LINUX:
            return "linux";
        case WINDOWS:
            return "windows";
        default:
            return "not supported yet";
    }
}

/**
 * Adds an agent with the given name to the agents list
 */
int
rcfrh_agent_add(char *agents_name, agent_type type)
{
    int i = 0;
    agent_t *agt;
    
    while (agents_list[i] != NULL)
        i++;

    if (i == MAX_AGENTS_NUMBER)
        return ENOBUFS;

    agt = calloc(sizeof(agent_t), 1);
    if (agt == NULL)
        return ENOMEM;

    strcpy(agt->name, agents_name);
    agt->type = type;

    agents_list[i] = agt;

    VERB("Agent with name %s and type %s is added, index = %d",
         agents_name, rcfrh_agent_type2str(type), i);
    return 0;
}

/**
 * Deletes agent with the given name from the agents list.
 */
int
rcfrh_agent_del(char *agents_name)
{
    int i = 0;

    while (agents_list[i] != NULL &&
           strcmp(agents_list[i]->name, agents_name) != 0)
        i++;

    if (i == MAX_AGENTS_NUMBER)
        return EINVAL;

    free(agents_list[i]);
    agents_list[i] = NULL;

    VERB("Agent with name %s is deleted",
         agents_name);
    return 0;
}

/**
 * Gets the agent list.
 */
int
rcfrh_agents_list(char *agents, int list_size)
{
    char buf[MAX_AGENTS_NUMBER * AGENT_NAME_MAX_LENGTH + 1];
    int  length = 1;
    int  i;

    memset(buf, 0, sizeof(buf));
    for (i = 0; i < MAX_AGENTS_NUMBER; i++)
    {
        if (agents_list[i] != NULL)
        {
            length += strlen(agents_list[i]->name);

            if (length > list_size)
            {
                length -= strlen(agents_list[i]->name);
                break;
            }
            strcat(buf, agents_list[i]->name);
        }
    }

    memset(agents, 0, list_size);
    strcpy(agents, buf);
    return length;
}

/**
 * Gets type of the agent with the given name.
 */
char *
rcfrh_agent_get_type(char *agents_name)
{
    int i = 0;

    while (agents_list[i] != NULL &&
           strcmp(agents_list[i]->name, agents_name) != 0)
        i++;
    if (i == MAX_AGENTS_NUMBER)
        return 0;
    
    return rcfrh_agent_type2str(agents_list[i]->type);
}

int
rcfrh_is_agent(char *agents_name)
{
    int i;

    for (i = 0; i < MAX_AGENTS_NUMBER; i++)
    {
        if (agents_list[i] != NULL &&
            strcmp(agents_list[i]->name, agents_name) == 0)
            return 1;
    }

    return 0;
}
