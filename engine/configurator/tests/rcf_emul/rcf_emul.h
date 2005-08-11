/** @file
 * @brief Configurator Tester
 *
 * RCF Emulator definitions
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

#ifndef __RCF_EMUL_H__
#define __RCF_EMUL_H__

#include "te_config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_ten.h"

#include "ipc_server.h"
#include "rcf_common.h"
#include "rcf_internal.h"
#include "rcf_api.h"

#include "../db/db.h"

/**
 * Set to 1 if the TE Logger should be supported.
 * If it is set to 0 the native Configurator tester
 * logger is used.
 */
#define SUPPORT_TE_LOGGER 1

/**
 * The name of the only existing agent.
 */
#define AGENT_NAME_MAX_LENGTH 64

/** Default select timeout in seconds */
#define RCF_SELECT_TIMEOUT      1   

/**
 * Maximum number of configurations stored by the RCF
 * emulator.
 */
#define MAX_CONF_NUMBER    10

/**
 * Name of the file which is used to send binary attachments.
 */
#define BIN_ATTACH_FILE_NAME "/tmp/conf_get_data.tmp"

/**
 * Name of the file which is used to handle Get log requests from the
 * logger.
 */
#define TA_LOG_FILE "/tmp/ta_log_file.tmp"

/** 
 * Main function. It should be called from the test in the 
 * separate thread.
 *
 * @param param           Currently the name for the configuration 
 *                        file which is used to init the database.
 *
 * @return                NULL if everything is correct
 *                        not NULL otherwise
 */
/**TODO: return is not supported */
extern void *rcf_emulate(void *param);

/** 
 * TA List request handler.
 *
 * @param ta_list       Pointer to the current agent
 *                      list. (OUT)
 *
 * @return              0 - if the function ends correctly
 */
typedef int (* rcfrh_ta_list)(char **ta_list);

/** 
 * TA Check request handler.
 *
 * @param ta_name       Name of the agent to check.
 * @param result        Check result.
 *
 * @return              0 - if the function ends correctly
 */
typedef int (* rcfrh_ta_check)(char *ta_name, int *result);

/**
 * Reboot request handler.
 *
 * @param ta_name       Name of the agent to reboot.
 * @param result        Reboot result.
 *
 * @return              0 - if the function ends correctly.
 */
typedef int (* rcfrh_reboot)(char *ta_name, int *result);

/**
 * Conf get request handler.
 *
 * @param ta_name       Name of the agent on which this request should
 *                      be handled.
 * @param oid           Description of the information to get.
 * @param answer        Pointer to the request answer buffer. (OUT)
 * @param ans_len       Length of the request answer. (OUT)
 * 
 * @return              0 - if the function ends correctly.
 */
typedef int (* rcfrh_conf_get)(char *ta_name, char *oid, 
                               char **answer, int *ans_len);

/**
 * Conf add request handler.
 *
 * @param ta_name       Name of the agent on which this request should
 *                      be handled.
 * @param oid           ID of the object or object instance to add.
 * @param value         Value of the object instance (or NULL if the
 *                      object is added.
 *
 * @return              0 - id of the newly added entity.
 */
typedef int (* rcfrh_conf_add)(char *ta_name, char *oid, char *value);

/**
 * Conf set request handler.
 *
 * @param ta_name       Name of the agent on which this request should
 *                      be handled.
 * @param oid           ID of the object instance value of which is
 *                      changed.
 * @param value         New value of the object instance.
 *
 * @return              0 - if the function ends correctly.
 */
typedef int (* rcfrh_conf_set)(char *ta_name, char *oid, char *value);

/**
 * Conf del request handler.
 *
 * @param ta_name       Name of the agent on which this request should
 *                      be handled.
 * @param oid           ID of the object or object instance to delete.
 *
 * @return              0 - if the function ends correctly.
 */
typedef int (* rcfrh_conf_del)(char *ta_name, char *oid);

/**
 * Conf group start request handler.
 *
 * @param ta_name       Name of the agent on which this request should
 *                      be handled.
 * @param grp_name      Name of the command group to start.
 *
 * @return              0 - if the function ends correctly.
 * @retval              TE_EINVAL - there is already a command group
 */
typedef int (* rcfrh_conf_grp_start)(char *ta_name, char *grp_name);

/**
 * Conf group end request handler.
 *
 * @param ta_name       Name of the agent on which this request should
 *                      be handled.
 * @param grp_name      Name of the command group to end.
 *
 * @return              0 - if the function ends correctly.
 * @retval              TE_EINVAL - no group was started
 */
typedef int (* rcfrh_conf_grp_end)(char *ta_name, char *grp_name);

/**
 * Configuration structure. It describes configuration
 * of the request handlers on the RCF emulator.
 */
typedef struct reqest_handlers
{
    rcfrh_ta_list         ta_list;         /**< TA List request handler  */
    rcfrh_ta_check        ta_check;        /**< TA Check request handler */
    rcfrh_reboot          reboot;          /**< Reboot request handler   */
    rcfrh_conf_get        conf_get;        /**< Conf get request handler */
    rcfrh_conf_add        conf_add;        /**< Conf add request handler */
    rcfrh_conf_set        conf_set;        /**< Conf set request handler */
    rcfrh_conf_del        conf_del;        /**< Conf del request handler */
    rcfrh_conf_grp_start  conf_grp_start;  /**< Conf group start         */
    rcfrh_conf_grp_end    conf_grp_end;    /**< Conf group end           */
} request_handler;

/** Configuration of the RCF emulator request handlers */
typedef request_handler *handler_configuration;

/** 
 * Obtain request_handler structure by the configuration identifier.
 *
 * @param id        Configuration identifier
 *
 * @return structure pointer or NULL
 */
extern request_handler *rcf_get_cfg_by_id(int id);


/**
 * TA List default request handler.
 *
 * @param ta_list   List of all test agents. By default there 
 *                  is only one agent with name Agt_T.
 *
 * @return          0 - if the function ends correctly
 * @retval          TE_ENOMEM - failed to malloc ta_list
 */
extern int rcfrh_ta_list_default(char **ta_list);

/**
 * TA Check default request handler.
 *
 * @param ta_name   Name of the agent to check. By default
 *                  there is only one agent with name Agt_T.
 * @param result    Check result.
 *
 * @return          0 - if the function ends correctly.
 * @retval          TE_EINVAL - ta_name is not Agt_T
 */
extern int rcfrh_ta_check_default(char *ta_name, int *result);

/**
 * Reboot default request handler.
 *
 * @param ta_name   Name of the agent to reboot. By default there
 *                  is only one agent with name Agt_T.
 * @param result    Reboot result. By default is zero.
 *
 * @return          0 - if the function ends correctly.
 * @retval          TE_EINVAL - ta_name is not Agt_T
 */
extern int rcfrh_reboot_default(char *ta_name, int *result);

/**
 * Conf get default request handler.
 * 
 * @param ta_name   Name of the agent on which the request
 *                  should be handled. By default there
 *                  is only one agent with name Agt_T.
 * @param oid       Description of the information to be retreived
 *                  from the database.
 * @param answer    Answer to the request. If the @p oid is not
 *                  wildcard the answer is sent to the configurator
 *                  with the rcf_msg. Otherwise it is sent as binary
 *                  attachment.
 * @param ans_len   Length of the answer.
 *
 * @return          0 - if the function ends correctly
 * @retval          TE_EINVAL - ta_name is not Agt_T
 * @retval          TE_ENOMEM - failed to allocate some buffer 
 * @retval          TE_EINVAL - wrong oid format
 */
extern int rcfrh_conf_get_default(char *ta_name, char *oid, 
                                  char **answer, int *ans_len);

/**
 * Conf add default request handler.
 * 
 * @param ta_name   Name of the agent on which the request
 *                  should be handled. By default there
 *                  is only one agent with name Agt_T.
 * @param oid       ID of the entity to add (no wildcard is allowed)
 * @param value     Value of the object instance if the object 
 *                  instance is added or NULL.
 *
 * @return          ID of the newly added entity if the function 
 *                  ends correctly
 * @retval          -TE_EINVAL - ta_name is not Agt_T
 * @retval          -TE_ENOMEM - failed to allocate some buffer or to
 *                            many entities in the database
 */
extern int rcfrh_conf_add_default(char *ta_name, char *oid, char *value);

/**
 * Conf set default request handler.
 *
 * @param ta_name   Name of the agent on which the request
 *                  should be handled. By default there
 *                  is only one agent with name Agt_T.
 * @param oid       ID of the object instance to change the value
 *                  of.
 * @param value     New value of the object instance.
 *
 * @return          0 - if the function ends correctly
 * @retval          TE_EINVAL - ta_name is not Agt_T
 * @retval          TE_EINVAL - no instance with such name
 */
extern int rcfrh_conf_set_default(char *ta_name, char *oid, char *value);

/**
 * Conf del default request handler.
 *
 * @param ta_name   Name of the agent on which the request
 *                  should be handled. By default there is
 *                  only one agent with name Agt_T.
 * @param oid       ID of the object or object instance to delete.
 *
 * @return          0 - if the object was deleted.
 * @retval          TE_EINVAL - ta_name is not Agt_T
 * @retval          TE_EINVAL - no object or object instance with
 *                           such name
 */
extern int rcfrh_conf_del_default(char *ta_name, char *oid);

/**
 * Conf group start default request handler.
 *
 * @param ta_name   Name of the agent on which the command
 *                  group shold be started. By default there is
 *                  only one agent with name Agt_T.
 * @param grp_name  Name of the group to start.
 *
 * @return          0 - if the functon ends correctly
 * @retval          TE_EINVAL - ta_name is not Agt_T
 */
extern int rcfrh_conf_grp_start_default(char *ta_name, char *grp_name);

/**
 * Conf group end default request handler.
 *
 * @param ta_name   Name of the agent on which the command
 *                  group shold be ended. By default there is
 *                  only one agent with name Agt_T.
 * @param grp_name  Name of the group to end.
 *
 * @return          0 - if the functon ends correctly
 * @retval          TE_EINVAL - ta_name is not Agt_T
 */
extern int rcfrh_conf_grp_end_default(char *ta_name, char *grp_name);

/**
 * Function create request handlers configuration.
 *
 * @return          Configuration ID if the configuration
 *                  was successfuly created.
 * @retval          -TE_ENOMEM - no more configurations is
 *                            allowed or failed to allocate
 *                            configuration structure
 */
extern int rcfrh_configuration_create(void);

/**
 * Function deletes request handlers configuration.
 * If the deleted configuration was set as current
 * handler configuration than current handler configuration
 * is not defined anymore.
 *
 * @param conf_id   ID of the configuration to delete.
 *
 * @return          0 - if the configuration was deleted
 *                      successfuly
 * @retval          TE_EINVAL - if there is no configuration
 *                           with such ID
 */
extern int rcfrh_configuration_delete(int conf_id);

/**
 * Changes current handler configuration.
 *
 * @param conf_id   ID of the configuration that should be
 *                  marked as current.
 *
 * @return          0 - if the current configuration was
 *                      changed correctly
 * @retval          TE_EINVAL - no configuration with such ID
 */
extern int rcfrh_configuration_set_current(int conf_id);

/**
 * Sets the handler in the current configuration for 
 * given type of request.
 *
 * @param conf_     Configuration to be updated
 * @param opcode_   Request type
 * @param handler_  Hander so set for that type of request.
 *
 */
#define RCFRH_SET_CONF_HANDLER_CONF(conf_, opcode_, handler_) \
    do {                                                          \
        switch (opcode_)                                          \
        {                                                         \
            case RCFOP_TALIST:                                    \
                conf_->ta_list = &handler_;                       \
                break;                                            \
            case RCFOP_TACHECK:                                   \
                conf_->ta_check = &handler_;                      \
                break;                                            \
            case RCFOP_REBOOT:                                    \
                conf_->reboot = &handler_;                        \
                break;                                            \
            case RCFOP_CONFGET:                                   \
                conf_->conf_get = &handler_;                      \
                break;                                            \
            case RCFOP_CONFSET:                                   \
                conf_->conf_set = &handler_;                      \
                break;                                            \
            case RCFOP_CONFADD:                                   \
                conf_->conf_add = &handler_;                      \
                break;                                            \
            case RCFOP_CONFDEL:                                   \
                conf_->conf_del = &handler_;                      \
                break;                                            \
            case RCFOP_CONFGRP_START:                             \
                conf_->conf_grp_start = &handler_;                \
                break;                                            \
            case RCFOP_CONFGRP_END:                               \
                conf_->conf_grp_start = &handler_;                \
                break;                                            \
            default:                                              \
                VERB("Wrong opcode passed");                      \
        }                                                         \
    } while (0)

/**
 * Sets the handler in the current configuration for the 
 * given type of request.
 *
 * @param conf_id_  Identifier of the configuration to be updated
 * @param opcode_   Request type
 * @param handler_  Hander so set for that type of request.
 *
 */
#define RCFRH_SET_CONF_HANDLER(conf_id_, opcode_, handler_) \
    do {                                                          \
        request_handler *req_ = rcf_get_cfg_by_id(conf_id_);      \
                                                                  \
        if (req_ != NULL)                                         \
            RCFRH_SET_CONF_HANDLER_CONF(req_, opcode_, handler_); \
    } while (0)

/**
 * Sets the handler in the current configuration for 
 * given type of request.
 *
 * @param opcode_   Request type
 * @param handler_  Hander so set for that type of request.
 *
 */
#define RCFRH_SET_HANDLER(opcode_, handler_) \
        RCFRH_SET_CONF_HANDLER_CONF(current_handler_conf, opcode_, handler_)

/**
 * Function registers default handler for all types of requests
 * in the configuration with ID conf_id.
 *
 * @param conf_id    ID of the configuration.
 *
 * @return           0 - if the handlers were registered
 * @retval           TE_EINVAL - no configuration with such ID
 */
extern int rcfrh_set_default_handlers(int conf_id);

/**
 * Function registers default handler in the given configuration
 * for the given type of request.
 *
 * @param opcode     Request type.
 * @param conf       Configuration to change.
 *
 * @return           0 - handler was registered correctly
 * @retval           TE_EINVAL - no such request type
 */
extern int rcfrh_set_default_handler(rcf_op_t opcode, request_handler *conf);

/**
 * Marco creates an empty configuration and checks if
 * there were some errors.
 *
 * @param conf_id_        ID of the newly created configuration.
 */
#define RCFRH_CONFIGURATION_CREATE(conf_id_) \
    do {                                                 \
        conf_id_ = rcfrh_configuration_create();         \
        if (conf_id_ < 0)                                \
        {                                                \
            ERROR("Failed to create configuration\n");   \
            goto cleanup;                                \
                                                         \
        }                                                \
    } while (0)

/**
 * Sets all handlers in the given configuration to
 * default.
 *
 * @param conf_id_       ID of the configuration
 */
#define RCFRH_SET_DEFAULT_HANDLERS(conf_id_) \
    do {                                                                    \
        if (rcfrh_set_default_handlers(conf_id_) != 0)                      \
        {                                                                   \
            ERROR("Failed to set all handlers to default in configuration " \
                  "with ID=%d", conf_id_);                                  \
            goto cleanup;                                                   \
        }                                                                   \
    } while (0)

/**
 * Changes configuration in current use.
 *
 * @param conf_id_       ID of the newly used configuration.
 */
#define RCFRH_CONFIGURATION_SET_CURRENT(conf_id_) \
    do {                                                            \
        if (rcfrh_configuration_set_current(conf_id_) != 0)         \
        {                                                           \
            ERROR("Failed to change configuration in current use "  \
                  "to configuration with ID=%d", conf_id_);         \
            goto cleanup;                                           \
        }                                                           \
    } while (0)

/**
 * Returns pointer to the current handler for the
 * given event type.
 *
 * @param opcode    Event type.
 *
 * @return          Pointer to the handler or NULL (if no
 *                  handler is registered for that type of
 *                  event.
 */
void *get_handler(rcf_op_t opcode);


/*****************************************************************
 *                          Agents.                              *
 *****************************************************************/

/**
 * Maximum number of the agents.
 */
#define MAX_AGENTS_NUMBER 10 

/**
 * Types of the agents.
 */
typedef enum {
    LINUX,              /**< Linux agent. */
    WINDOWS,            /**< Windows agent. */
} agent_type;

/**
 * Agent structure.
 */
typedef struct agent_s
{
    char       name[AGENT_NAME_MAX_LENGTH];  /**< Name of the agent */
 
    agent_type type;                         /**< Type of the agent */
} agent_t;

/** Pointerr to the agents structure. */
typedef agent_t *agent;

/**
 * Adds the agent with the given name of the specified tyep.
 *
 * @param agents_name  Name of the agent to add.
 * @param type         Type of the agent to add.
 *
 * @return             0 or errno
 */
int rcfrh_agent_add(char *agents_name, agent_type type);

/**
 * Deletes the agent with the given name from
 * the agents list.
 *
 * @param agents_name   Name of the agent to be deleted.
 *
 * @return             0 of errno.
 */
int rcfrh_agent_del(char *agents_name);

/**
 * Lists all agents from the agents list. Maximum number of
 * the agent names which can fit into the given buffer
 * is placed there.
 *
 * @param agents       Buffer, where the list of all agents should
 *                     be placed.
 * @param list_size    Size of the given buffer.
 *
 * @return             length of the filled part of the given buffer
 */
int rcfrh_agents_list(char *agents, int list_size);

/**
 * Gets type of the agent with the given name.
 *
 * @param agents_name   Name of the agent to get type of.
 *
 * @return              Type of the agent with the given name in string
 *                      form.
 */
char *rcfrh_agent_get_type(char *agents_name);

/**
 * Checks, if the agents with the given name was created.
 *
 * @param agents_name  Name of the agents the existence of which should
 *                     be checked.
 *
 * @return             1 - if the agents exists
 *                     0 - otherwise
 */
int rcfrh_is_agent(char *agents_name);

#endif /* !__RCF_EMUL_H__ */
