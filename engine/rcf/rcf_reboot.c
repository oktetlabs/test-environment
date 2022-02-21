/** @file
 * @brief RCF routines for TA reboot
 *
 *
 * Copyright (C) 2021 OKTET Labs. All rights reserved.
 *
 * @author Artemii Morozov <Artemii.Morozov@oktetlabs.ru>
 */
#include "te_config.h"

#include "rcf.h"
#include "te_str.h"

#include "logger_api.h"
#include "logger_ten.h"

/** Timeout between attempts to initialize the agent in seconds */
#define RCF_RESTART_TA_ATTEMPT_TIMEOUT 5

static te_bool
is_timed_out(time_t timestart, double timeout)
{
    time_t t = time(NULL);

    return difftime(t, timestart) > timeout;
}

void
rcf_ta_reboot_init_ctx(ta *agent)
{
    agent->reboot_ctx.state = TA_REBOOT_STATE_IDLE;
    agent->reboot_ctx.is_agent_reboot_msg_sent = FALSE;
    agent->reboot_ctx.is_answer_recv = FALSE;
    agent->reboot_ctx.restart_attempt = 0;
}

static const char *
ta_reboot_type2str(ta_reboot_type type)
{
    switch (type)
    {
        case TA_REBOOT_TYPE_AGENT:
            return "restart the TA process";

        case TA_REBOOT_TYPE_HOST:
            return "reboot the host";

        case TA_REBOOT_TYPE_COLD:
            return "cold reboot the host";

        default:
            return "<unknown>";
    }
}

static void
log_reboot_state(ta *agent, ta_reboot_state state)
{
    switch (state)
    {
        case TA_REBOOT_STATE_IDLE:
            RING("Agent '%s' in normal state", agent->name);
            break;

        case TA_REBOOT_STATE_LOG_FLUSH:
            RING("%s: agent '%s' is waiting for the logs to be flushed",
                 ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
            break;

        case TA_REBOOT_STATE_WAITING:
            RING("%s: sending a message requesting a reboot to TA '%s'",
                 ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
            break;

        case TA_REBOOT_STATE_WAITING_ACK:
            RING("%s: waiting for a response to a message about restarting ",
                 ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
            break;

        case TA_REBOOT_STATE_REBOOTING:
            RING("%s: waiting for a reboot TA '%s'",
                 ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
            break;
    }
}

/* See description in rcf.h */
void
rcf_set_ta_reboot_state(ta *agent, ta_reboot_state state)
{
    log_reboot_state(agent, state);

    agent->reboot_ctx.state = state;
    agent->reboot_ctx.reboot_timestamp = time(NULL);
}

te_bool
rcf_ta_reboot_before_req(ta *agent, usrreq *req)
{
    /*
     * In the LOG_FLUSH state an agent can accept only one GET_LOG command
     */
    if (agent->reboot_ctx.state == TA_REBOOT_STATE_LOG_FLUSH &&
        req->message->opcode != RCFOP_GET_LOG)
    {
        WARN("The agent is waiting for reboot");
        return FALSE;
    }

    /*
     * If the agent in the REBOOTING state, it cannot accept requests.
     * If the agent in the WAITING state, then it can only
     * accept a reboot request.
     */
    if (agent->reboot_ctx.state == TA_REBOOT_STATE_REBOOTING ||
        (agent->reboot_ctx.state == TA_REBOOT_STATE_WAITING &&
         req->message->opcode != RCFOP_REBOOT))
    {
        if (!agent->reboot_ctx.is_agent_reboot_msg_sent)
        {
            ERROR("Agent `%s` in the reboot state", agent->name);
            agent->reboot_ctx.is_agent_reboot_msg_sent = TRUE;
        }

        return FALSE;
    }

    return TRUE;
}

te_bool
rcf_ta_reboot_on_req_reply(ta *agent, rcf_op_t opcode)
{
    if (agent->reboot_ctx.state == TA_REBOOT_STATE_LOG_FLUSH &&
        opcode == RCFOP_GET_LOG)
    {
        /*
         * If RCF is waiting for logs from an agent to reboot that agent,
         * there is no need to push the next waiting request and so on.
         * This will make the reboot state machine.
         */
        agent->reboot_ctx.is_answer_recv = TRUE;
        return TRUE;
    }

    return FALSE;
}

te_errno
rcf_ta_reboot_is_reboot_answer(ta *agent, usrreq *req)
{
    if (req->message->opcode == RCFOP_REBOOT)
        agent->reboot_ctx.is_answer_recv = TRUE;

    return 0;
}

te_errno
rcf_ta_reboot_is_cold_reboot_answer(ta *agent, usrreq *req)
{
    if (req->message->opcode == RCFOP_EXECUTE &&
        strcmp(req->message->id, "cold_reboot") == 0)
    {
        agent->reboot_ctx.is_answer_recv = TRUE;
    }

    return 0;
}

/**
 * @c TA_REBOOT_STATE_LOG_FLUSH handler.
 *
 * If time to waiting the log flush is expired then force set the next
 * reboot state for the @p agent.
 * If the logs are flushed, respond to all pending and sending requests
 * and set the next reboot state for the @p agent.
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
log_flush_state_handler(ta *agent)
{
    if (agent->reboot_ctx.is_answer_recv)
    {
        usrreq *req;

        rcf_answer_all_requests(&(agent->waiting), TE_ETAREBOOTING);
        rcf_answer_all_requests(&(agent->pending), TE_ETAREBOOTING);

        /*
         * If the request with id 0 was sent earlier
         * then we need to try to wait for a response.
         */
        req = rcf_find_user_request(&(agent->sent), 0);
        if (req != NULL)
            return 0;

        agent->conn_locked = FALSE;

        if (agent->reboot_ctx.current_type == TA_REBOOT_TYPE_AGENT)
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_REBOOTING);
        else
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_WAITING);

        agent->reboot_ctx.is_answer_recv = FALSE;
        return 0;
    }

    if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                     RCF_LOG_FLUSHED_TIMEOUT))
    {
        rcf_answer_all_requests(&(agent->waiting), TE_ETAREBOOTING);
        rcf_answer_all_requests(&(agent->pending), TE_ETAREBOOTING);
        rcf_answer_all_requests(&(agent->sent), TE_ETAREBOOTING);

        if (agent->reboot_ctx.current_type == TA_REBOOT_TYPE_AGENT)
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_REBOOTING);
        else
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_WAITING);

        agent->reboot_ctx.is_answer_recv = FALSE;
    }

    return 0;
}

/**
 * @c TA_REBOOT_STATE_WAITING handler for @c TA_REBOOT_TYPE_HOST
 *
 * Make and send a reboot request to the @p agent and set the next
 * reboot state for the @p agent.
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
waiting_state_host_reboot_handler(ta *agent)
{
    usrreq         *req = NULL;

    req = rcf_alloc_usrreq();
    if (req == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return TE_ENOMEM;
    }

    req->user = NULL;
    req->timeout = RCF_CMD_TIMEOUT;
    strcpy(req->message->ta, agent->name);
    req->message->sid = 0;
    req->message->opcode = RCFOP_REBOOT;
    req->message->data_len = 0;
    req->cb = rcf_ta_reboot_is_reboot_answer;

    if (rcf_send_cmd(agent, req) != 0)
    {
        ERROR("Failed to send message");
        agent->reboot_ctx.error = TE_ECOMM;
        return 0;
    }

    rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_WAITING_ACK);

    return 0;
}

/**
 * @c TA_REBOOT_STATE_WAITING handler for @c TA_REBOOT_TYPE_COLD
 *
 * Make and send a cold reboot request to the power control TA associated
 * with @p agent and set the next reboot state for the @p agent.
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
waiting_state_cold_reboot_handler(ta *agent)
{
    ta     *power_ta;
    rcf_msg *tmp;
    usrreq *req;
    size_t  param_len;

    if (agent->cold_reboot_ta == NULL)
    {
        RING("Cold rebooting is not supported for '%s'", agent->name);
        agent->reboot_ctx.error = TE_EINVAL;
        return 0;
    }

    RING("Cold rebooting TA '%s' using '%s', '%s'", agent->name,
         agent->cold_reboot_ta, agent->cold_reboot_param);

    power_ta = rcf_find_ta_by_name(agent->cold_reboot_ta);
    if (power_ta == NULL)
    {
        ERROR("Non-existant TA '%s' is specified for cold_reboot of '%s'",
              agent->cold_reboot_ta, agent->name);
        return TE_EINVAL;
    }

    if ((power_ta->flags & TA_DEAD) != 0)
    {
        ERROR("Power agent '%s' for TA '%s' is dead!",
              power_ta->name, agent->name);
        return TE_ETADEAD;
    }

    req = rcf_alloc_usrreq();
    if (req == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return TE_ENOMEM;
    }

    param_len = strlen(agent->cold_reboot_param) + 1;
    if ((tmp = realloc(req->message, sizeof(rcf_msg) + param_len)) == NULL)
    {
        ERROR("%s(): failed to re-allocate memory", __FUNCTION__);
        free(req->message);
        free(req);
        return TE_ENOMEM;
    }

    req->message = (rcf_msg *)tmp;
    req->user = NULL;
    req->timeout = RCF_CMD_TIMEOUT;
    strcpy(req->message->ta, power_ta->name);
    req->message->sid = 0;
    req->message->opcode = RCFOP_EXECUTE;
    req->message->intparm = RCF_FUNC;
    strcpy(req->message->id, "cold_reboot");
    req->message->num = 1;
    req->message->flags |= PARAMETERS_ARGV;
    strcpy(req->message->data, agent->cold_reboot_param);
    req->message->data_len = param_len;
    req->cb = rcf_ta_reboot_is_cold_reboot_answer;

    if (rcf_send_cmd(power_ta, req) != 0)
    {
        ERROR("Failed to send message to '%s'", power_ta->name);
        return TE_ECOMM;
    }

    rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_WAITING_ACK);
    return 0;
}


/**
 * @c TA_REBOOT_STATE_WAITING handler.
 *
 * @note When restarting the TA process @c TA_REBOOT_STATE_WAITING in
 *       this case is empty.
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
waiting_state_handler(ta *agent)
{
    te_errno rc = 0;

    switch (agent->reboot_ctx.current_type)
    {
        case TA_REBOOT_TYPE_AGENT:
            break;

        case TA_REBOOT_TYPE_HOST:
            rc = waiting_state_host_reboot_handler(agent);
            break;

        case TA_REBOOT_TYPE_COLD:
            rc = waiting_state_cold_reboot_handler(agent);
            break;

        default:
            ERROR("Unsupported reboot type %d",
                  agent->reboot_ctx.current_type);
            return TE_EINVAL;
    }

    return rc;
}

/**
 * @c TA_REBOOT_STATE_WAITING_ACK handeler for @c TA_REBOOT_TYPE_HOST
 *
 * Check that response from TA is received and set the next reboot state
 * for the agent.
 *
 * @return Status code
 */
static te_errno
waiting_ack_state_host_reboot_handler(ta *agent)
{
    if (agent->reboot_ctx.is_answer_recv)
    {
        rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_REBOOTING);
        agent->reboot_ctx.is_answer_recv = FALSE;
    }

    if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                     RCF_ACK_HOST_REBOOT_TIMEOUT))
    {
        WARN("Agent '%s' doesn't respond to the reboot request",
             agent->name);
        agent->reboot_ctx.error = TE_ETIMEDOUT;
    }

    return 0;
}

/**
 * @c TA_REBOOT_STATE_WAITING_ACK handeler for @c TA_REBOOT_TYPE_COLD
 *
 * Check that response from power TA is received and set the next reboot state
 * for the agent.
 *
 * @return Status code
 */
static te_errno
waiting_ack_state_cold_reboot_handler(ta *agent)
{
    ta *power_ta;

    power_ta = rcf_find_ta_by_name(agent->cold_reboot_ta);
    if (power_ta == NULL)
    {
        ERROR("Non-existant TA is specified for cold_reboot of '%s'",
              agent->name);
        return TE_EINVAL;
    }

    if (power_ta->reboot_ctx.is_answer_recv)
    {
        rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_REBOOTING);
        power_ta->reboot_ctx.is_answer_recv = FALSE;
    }

    if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                     RCF_ACK_HOST_REBOOT_TIMEOUT))
    {
        WARN("Agent '%s' doesn't respond to the reboot request",
             power_ta->name);
        agent->reboot_ctx.error = TE_ETIMEDOUT;
    }

    return 0;
}

/**
 * @c TA_REBOOT_STATE_WAITING_ACK handeler.
 *
 * For @c TA_REBOOT_TYPE_HOST:
 *     check that response from TA is received and set the next reboot state
 *     for the agent.
 *
 * For @c TA_REBOOT_TYPE_COLD:
 *     check that response from power controll TA associated with @p agent
 *     is received and set the next reboot state for the @p agent.
 *
 * @note When restarting the TA process @c TA_REBOOT_STATE_WAITING_ACK in
 *       this case is empty.
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
waiting_ack_state_handler(ta *agent)
{
    te_errno rc = 0;

    switch (agent->reboot_ctx.current_type)
    {
        case TA_REBOOT_TYPE_AGENT:
            break;

        case TA_REBOOT_TYPE_HOST:
            rc = waiting_ack_state_host_reboot_handler(agent);
            break;

        case TA_REBOOT_TYPE_COLD:
            rc = waiting_ack_state_cold_reboot_handler(agent);
            break;

        default:
            ERROR("Unsupported reboot type %d",
                  agent->reboot_ctx.current_type);
            return TE_EINVAL;
    }

    return rc;
}

/**
 * Try to soft shutdown the agent
 *
 * @param agent Test Agent
 */
static void
try_soft_shutdown(ta *agent)
{
    char cmd[RCF_MAX_LEN];
    te_errno rc;
    time_t t = time(NULL);

    if (agent->flags & TA_DEAD)
    {
        WARN("Agent is '%s' is dead. Soft shutdown failed", agent->name);
        return;
    }

    TE_SPRINTF(cmd, "SID %d %s", ++agent->sid, "shutdown");
    rc = (agent->m.transmit)(agent->handle,
                             cmd, strlen(cmd) + 1);
    if (rc != 0)
    {
        WARN("Soft shutdown of TA '%s' failed", agent->name);
        agent->flags |= TA_DEAD;
        return;
    }

    /* TODO: This should be moved to a separate function */
    while (!is_timed_out(t, RCF_SHUTDOWN_TIMEOUT))
    {
        struct timeval tv  = tv0;
        fd_set         set = set0;

        select(FD_SETSIZE, &set, NULL, NULL, &tv);

        if ((agent->m.is_ready)(agent->handle))
        {
            char    answer[16];
            char   *ba;
            size_t  len = sizeof(cmd);

            if ((agent->m.receive)(agent->handle, cmd,
                                &len, &ba) != 0)
            {
                continue;
            }

            TE_SPRINTF(answer, "SID %d 0", agent->sid);

            if (strcmp(cmd, answer) != 0)
                continue;

            INFO("Test Agent '%s' is down", agent->name);
            agent->flags |= TA_DOWN;
            (agent->m.close)(agent->handle, &set0);
            break;
        }
    }

    if (~agent->flags & TA_DOWN)
    {
        WARN("Soft shutdown of TA '%s' failed", agent->name);
        agent->flags |= TA_DEAD;
    }
}

/**
 * @c TA_REBOOT_STATE_REBOOTING for @c TA_REBOOT_TYPE_AGENT
 *
 * Restart TA process.
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
rebooting_state_agent_reboot_handler(ta *agent)
{
    te_errno rc;

    try_soft_shutdown(agent);

    rc = (agent->m.finish)(agent->handle, NULL);
    if (rc != 0)
    {
        WARN("Cannot reboot TA '%s': finish failed %r", agent->name, rc);
        agent->reboot_ctx.error = rc;
        return 0;
    }

    agent->handle = NULL;

    RING("Test Agent '%s' is stopped", agent->name);
    rc = rcf_init_agent(agent);
    if (rc != 0)
    {
        ERROR("Cannot reboot TA '%s'", agent->name);
        agent->reboot_ctx.error = rc;
        return 0;
    }

    rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_IDLE);
    return 0;
}

/**
 * @c TA_REBOOT_STATE_REBOOTING for @c TA_REBOOT_TYPE_HOST
 *
 * Try to init agent every @c RCF_RESTART_TA_ATTEMPT_TIMEOUT second
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
rebooting_state_host_reboot_handler(ta *agent)
{
    te_errno rc = 0;

    if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                     RCF_HOST_REBOOT_TIMEOUT))
    {
        WARN("Cannot start the agent after %s timeout",
             RCF_HOST_REBOOT_TIMEOUT);
        agent->reboot_ctx.error = TE_ETIMEDOUT;
        return 0;
    }

    if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                     RCF_RESTART_TA_ATTEMPT_TIMEOUT *
                     (agent->reboot_ctx.restart_attempt + 1)))
    {
        agent->reboot_ctx.restart_attempt++;
        rc = rcf_init_agent(agent);
        if (rc == 0)
        {
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_IDLE);
        }
        else
        {
            agent->handle = NULL;
            /*
             * after a failed start of the agent, it is marked as DEAD,
             * but in case of a failed reboot attempt, we should not do this
             */
            agent->flags &= ~TA_DEAD;
        }
    }

    return 0;
}

/**
 * @c TA_REBOOT_STATE_REBOOTING for @c TA_REBOOT_TYPE_COLD
 *
 * Try to init agent every @c RCF_RESTART_TA_ATTEMPT_TIMEOUT second
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
rebooting_state_cold_reboot_handler(ta *agent)
{
    te_errno rc = 0;

    if (!agent->reboot_ctx.is_cold_reboot_time_expired)
    {
        if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                         agent->cold_reboot_timeout))
        {
            agent->reboot_ctx.reboot_timestamp = time(NULL);
            agent->reboot_ctx.is_cold_reboot_time_expired = TRUE;
        }

        return 0;
    }

    if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                     agent->cold_reboot_timeout))
    {
        WARN("Cannot start the agent after %s timeout",
             agent->cold_reboot_timeout);
        agent->reboot_ctx.error = TE_ETIMEDOUT;
        return 0;
    }

    if (is_timed_out(agent->reboot_ctx.reboot_timestamp,
                     RCF_RESTART_TA_ATTEMPT_TIMEOUT *
                     (agent->reboot_ctx.restart_attempt + 1)))
    {
        agent->reboot_ctx.restart_attempt++;
        rc = rcf_init_agent(agent);
        if (rc == 0)
        {
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_IDLE);
        }
        else
        {
            agent->handle = NULL;
            /*
             * after a failed start of the agent, it is marked as DEAD,
             * but in case of a failed reboot attempt, we should not do this
             */
            agent->flags &= ~TA_DEAD;
        }
    }

    return 0;
}

/**
 * @c TA_REBOOT_STATE_REBOOTING handler.
 *
 * @param agent Test Agent structure
 *
 * @return Status code
 */
static te_errno
rebooting_state_handler(ta *agent)
{
    te_errno rc = 0;

    switch (agent->reboot_ctx.current_type)
    {
        case TA_REBOOT_TYPE_AGENT:
            rc = rebooting_state_agent_reboot_handler(agent);
            break;

        case TA_REBOOT_TYPE_HOST:
            rc = rebooting_state_host_reboot_handler(agent);
            break;

        case TA_REBOOT_TYPE_COLD:
            rc = rebooting_state_cold_reboot_handler(agent);
            break;

        default:
            ERROR("Unsupported reboot type %d",
                  agent->reboot_ctx.current_type);
            return TE_EINVAL;
    }

    return rc;
}

/* See description in rcf.h */
void
rcf_ta_reboot_state_handler(ta *agent)
{
    te_errno error = 0;

    switch (agent->reboot_ctx.state)
    {
        case TA_REBOOT_STATE_IDLE:
            /* Do nothing if the agent in the normal state */
            return;

        case TA_REBOOT_STATE_LOG_FLUSH:
            error = log_flush_state_handler(agent);
            break;

        case TA_REBOOT_STATE_WAITING:
            error = waiting_state_handler(agent);
            break;

        case TA_REBOOT_STATE_WAITING_ACK:
            error = waiting_ack_state_handler(agent);
            break;

        case TA_REBOOT_STATE_REBOOTING:
            error = rebooting_state_handler(agent);
            break;

        default:
            ERROR("Unsupported reboot type %d",
                  agent->reboot_ctx.current_type);
            error = TE_EINVAL;
    }

    if (error != 0)
    {
        ERROR("%s for '%s' is failed",
              ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
        agent->reboot_ctx.req->message->error = TE_RC(TE_RCF, TE_EFAIL);
        rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_IDLE);
        rcf_answer_user_request(agent->reboot_ctx.req);
        ta_checker.req = NULL;
        rcf_set_ta_unrecoverable(agent);
        agent->handle = NULL;
        return;
    }

    if (agent->reboot_ctx.error != 0)
    {
        if (agent->reboot_ctx.requested_type > agent->reboot_ctx.current_type)
        {
            WARN("%s for '%s' is failed",
                 ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
            RING("Use %s instead of %s for '%s'",
                 ta_reboot_type2str(agent->reboot_ctx.current_type + 1),
                 ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
            agent->reboot_ctx.current_type++;
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_WAITING);
            agent->reboot_ctx.error = 0;
            agent->reboot_ctx.is_agent_reboot_msg_sent = FALSE;
        }
        else
        {
            ERROR("%s for '%s' is failed",
              ta_reboot_type2str(agent->reboot_ctx.current_type), agent->name);
            agent->reboot_ctx.req->message->error = TE_RC(TE_RCF, TE_EFAIL);
            rcf_set_ta_reboot_state(agent, TA_REBOOT_STATE_IDLE);
            rcf_answer_user_request(agent->reboot_ctx.req);
            rcf_set_ta_unrecoverable(agent);
            agent->handle = NULL;
            ta_checker.req = NULL;
        }
    }
    else if (agent->reboot_ctx.state == TA_REBOOT_STATE_IDLE)
    {
        RING("TA '%s' has successfully rebooted", agent->name);
        rcf_answer_user_request(agent->reboot_ctx.req);
        ta_checker.req = NULL;
    }

    return;
}
