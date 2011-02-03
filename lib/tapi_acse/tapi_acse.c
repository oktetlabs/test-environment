/** @file
 * @brief Test API for ACSE usage
 *
 * Implementation of Test API to ACSE.
 *
 * Copyright (C) 2009 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI ACSE"

#include "te_config.h"

#include "logger_api.h"
#include "rcf_common.h"
#include "tapi_acse.h"
#include "tapi_cfg_base.h"

#include "rcf_rpc.h"
// #include "tapi_rpc_tr069.h"

#include "acse_epc.h"
#include "cwmp_data.h"

#include "te_sockaddr.h"


/**
 * Copy internal gSOAP CWMP array of strings to string_array_t. 
 * Parameter src_ is assumed to have fields 'char ** __ptrstring' and
 * 'int __size'.
 * Parameter 'arr_' is local variable for pointer to new string_array_t.
 */
#define COPY_STRING_ARRAY(src_, arr_) \
    do { \
        int i; \
        (arr_) = (string_array_t *)malloc(sizeof(string_array_t)); \
        (arr_)->size = (src_)->__size; \
        (arr_)->items = calloc((arr_)->size, sizeof(char*)); \
        for (i = 0; i < (src_)->__size; i++) \
            (arr_)->items[i] = strdup((src_)->__ptrstring[i]); \
    } while (0)



#define CHECK_RC(expr_) \
    do { \
        if ((rc = (expr_)) != 0)                                       \
        {                                                              \
            WARN("%s():%d, util fails %r", __FUNCTION__, __LINE__, rc);\
            return TE_RC(TE_TAPI, rc);                                 \
        }                                                              \
    } while (0)

/* see description in tapi_acse.h */
te_errno
tapi_acse_start(const char *ta)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 1),
                                "/agent:%s/acse:", ta);
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_stop(const char *ta)
{
    te_errno rc;
    char     buf[256];

    TE_SPRINTF(buf, "/agent:%s/acse:", ta);

    if ((rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, 0), "%s", buf)) == 0)
        return cfg_synchronize(buf, TRUE);

    return rc;
}

/**
 * Local method to detect whether ACSE Config parameter is integer-type. 
 * Should be modified when ACSE CS subtree updated.
 */
static inline int
acse_is_int_var(const char *name)
{
    return
        ((0 == strcmp(name, "port"))     || 
         (0 == strcmp(name, "ssl"))      ||
         (0 == strcmp(name, "enabled"))  ||
         (0 == strcmp(name, "cr_state")) ||
         (0 == strcmp(name, "sync_mode")) ||
         (0 == strcmp(name, "hold_requests")) ||
         (0 == strcmp(name, "cwmp_state"))  );
}

te_errno
tapi_acse_ta_cs_init(tapi_acse_context_t *ctx)
{
    char          buf[CFG_OID_MAX];
    int           i_val;
    cfg_val_type  type;
    cfg_handle    acs_handle = CFG_HANDLE_INVALID,
                  cpe_handle = CFG_HANDLE_INVALID;
    te_errno      rc;

    /* Check, if there running ACSE itself */
    type = CVT_INTEGER;
    rc = cfg_get_instance_fmt(&type, &i_val, "/agent:%s/acse:", ctx->ta);
    if (rc != 0) return TE_RC(TE_TAPI, rc);
    if (i_val != 1)
    {
        WARN("ACSE is not running, val %d", i_val);
        return TE_RC(TE_TAPI, TE_ESRCH);
    }

#define C_ACS 1
#define C_CPE 2

#define CHECK_CREATE_ACS_CPE(_lv) \
    do {                                                                \
        snprintf(buf, sizeof(buf), "/agent:%s/acse:/acs:%s%s%s",       \
                 ctx->ta, ctx->acs_name,                                \
                 (_lv == C_CPE) ? "/cpe:" : "",                         \
                 (_lv == C_CPE) ? ctx->cpe_name : "" );                 \
        if ((rc = cfg_find_str(buf, (_lv == C_CPE) ?                    \
                                    &cpe_handle : &acs_handle)) != 0)   \
        {                                                               \
            if (TE_ENOENT == TE_RC_GET_ERROR(rc))                       \
                rc = cfg_add_instance_str(buf, NULL, CVT_NONE);         \
            else                                                        \
                ERROR("%s(): find '%s', %r", __FUNCTION__, buf, rc);    \
        }                                                               \
        if (0 != rc)                                                    \
            return TE_RC(TE_TAPI, rc);                                  \
    } while (0)

#define COPY_ACS_CPE_PARAM(_lv, _type, _par_name) \
    do { \
        char *str = NULL; int num = 0;                                  \
        type = _type;                                                   \
        if ((rc = cfg_get_instance_fmt(&type,                           \
                    (CVT_INTEGER == _type) ? (void*)&num : (void*)&str, \
                    "/local:/acse:/%s:%s/%s:",                          \
                    (_lv == C_CPE) ? "cpe" : "acs",                     \
                    (_lv == C_CPE) ? ctx->cpe_name : ctx->acs_name,     \
                        _par_name)                               ) != 0 \
            || (rc = cfg_set_instance_fmt(_type,                        \
                            (CVT_INTEGER == _type) ? (void*)num : str,  \
                            "/agent:%s/acse:/acs:%s%s%s/%s:",           \
                            ctx->ta, ctx->acs_name,                     \
                            (_lv == C_CPE) ? "/cpe:" : "",              \
                            (_lv == C_CPE) ? ctx->cpe_name : "",        \
                            _par_name)                          ) != 0) \
        {                                                               \
            ERROR("copy '%s' %s param from local to TA tailed, %r",     \
                 _par_name, (_lv == C_CPE) ? "cpe" : "acs", rc);        \
            return TE_RC(TE_TAPI, rc);                                  \
        }                                                               \
    } while (0)

    /* Check, if there wanted ACS on the running ACSE */
    CHECK_CREATE_ACS_CPE(C_ACS);
    /* Check, if there wanted CPE on the running ACSE */
    CHECK_CREATE_ACS_CPE(C_CPE);

    if (CFG_HANDLE_INVALID == acs_handle) /* ACS on ACSE was created */
    { 
        COPY_ACS_CPE_PARAM(C_ACS, CVT_INTEGER, "port");
        COPY_ACS_CPE_PARAM(C_ACS, CVT_STRING, "http_root");
        COPY_ACS_CPE_PARAM(C_ACS, CVT_STRING, "auth_mode");
        COPY_ACS_CPE_PARAM(C_ACS, CVT_STRING, "url");
    }

    if (CFG_HANDLE_INVALID == cpe_handle) /* CPE on ACSE was created */
    { 
        COPY_ACS_CPE_PARAM(C_CPE, CVT_STRING, "login");
        COPY_ACS_CPE_PARAM(C_CPE, CVT_STRING, "passwd");
    }

#undef COPY_ACS_PARAM
#undef C_ACS
#undef C_CPE
    return 0;
}

/* see description in tapi_acse.h */
tapi_acse_context_t *
tapi_acse_ctx_init(const char *ta)
{
    const char  *box_name = getenv("CPE_NAME");
    tapi_acse_context_t *ctx = calloc(1, sizeof(*ctx));
    te_errno rc = 0;

    if (NULL == box_name)
    {
#if 0
        WARN("init TAPI ACSE context, no CPE_NAME, find first CPE");
#else
        ERROR("no CPE_NAME specified.");
        return NULL;
#endif
    }
    else
        RING("init ACSE context, CPE_NAME='%s', let's find this CPE...", 
             box_name);

    ctx->ta = strdup(ta);

    do {
        unsigned num = 0;
        cfg_handle *handles = NULL;
        char *name;

        if ((rc = rcf_rpc_server_get(ta, "acse_ctl", NULL, FALSE, TRUE,
                                     FALSE, &(ctx->rpc_srv)) ) != 0)
        {
            ERROR("Init RPC server on TA '%s' failed %r", ta, rc);
            break;
        }

        ctx->timeout = 20; /* Experimentally discovered optimal value. */
        ctx->req_id = 0;

        if ((rc = cfg_find_pattern_fmt(&num, &handles,
                                       "/local:/acse:/acs:*")) != 0 ||
            0 == num)
        {
            ERROR("Cannot find ACS in local db: rc %r, num found %d",
                  rc, num);
            break;
        }
        cfg_get_inst_name(handles[0], &name);
        ctx->acs_name = name;
        free(handles); handles = NULL; num = 0;

        if (NULL != box_name)
        {
            ctx->cpe_name = box_name;
            RING("init ctx: %s/%s", ctx->acs_name, ctx->cpe_name);
        }
        else
        {
            if ((rc = cfg_find_pattern_fmt(&num, &handles,
                                           "/local:/acse:/cpe:*")) != 0 ||
                0 == num)
            {
                ERROR("Cannot find CPE in local db: rc %r, num found %d",
                      rc, num);
                break;
            }
            cfg_get_inst_name(handles[0], &name);
            ctx->cpe_name = name;
        }

        if (0 != (rc = tapi_acse_ta_cs_init(ctx)))
            break;

        return ctx;
    } while (0);
    free(ctx);
    return NULL;
}

/** generic internal method for ACSE manage operations */
static inline te_errno
tapi_acse_manage_vlist(const char *ta, const char *acs_name,
                     const char *cpe_name,
                     acse_op_t opcode, va_list  ap)
{
    te_errno gen_rc = 0;

    char cpe_name_buf[RCF_MAX_PATH] = "";

    if (cpe_name != NULL)
        snprintf(cpe_name_buf, RCF_MAX_PATH, "/cpe:%s", cpe_name);

    if (ACSE_OP_ADD == opcode)
    {
        gen_rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, 0),
                          "/agent:%s/acse:/acs:%s%s", ta, acs_name,
                          cpe_name_buf);
    }
    if (ACSE_OP_DEL == opcode)
    {
        gen_rc = cfg_del_instance_fmt(FALSE,
                                      "/agent:%s/acse:/acs:%s%s",
                                      ta, acs_name, cpe_name_buf);
        return gen_rc;
    }

    while (1)
    {
        char *name = va_arg(ap, char *);
        char buf[RCF_MAX_PATH];
        te_errno rc = 0;

        snprintf(buf, RCF_MAX_PATH, "/agent:%s/acse:/acs:%s%s/%s:",
                 ta, acs_name, cpe_name_buf, name);

        if (VA_END_LIST == name)
            break;

        if (ACSE_OP_OBTAIN == opcode)
        { 
            cfg_val_type type;

            if (acse_is_int_var(name))
            {
                /* integer parameters */
                int *p_val = va_arg(ap, int *);
                type = CVT_INTEGER;
                rc = cfg_get_instance_fmt(&type, p_val, "%s", buf);
            }
            else /* string parameters */
            {
                char **p_val = va_arg(ap, char **);
                type = CVT_STRING;
                rc = cfg_get_instance_fmt(&type, p_val, "%s", buf);
            }
        }
        else
        {
            if (acse_is_int_var(name))
            {/* integer parameters */
                int val = va_arg(ap, int);
                rc = cfg_set_instance_fmt(CFG_VAL(INTEGER, val), "%s", buf);
            }
            else /* string parameters */
            {
                char *val = va_arg(ap, char *);
                rc = cfg_set_instance_fmt(CFG_VAL(STRING, val), "%s", buf);
            }
        }
        if (0 == gen_rc) /* store in 'gen_rc' first TE errno */
            gen_rc = rc;
    } 

    return gen_rc;
}



te_errno
tapi_acse_manage_cpe(tapi_acse_context_t *ctx,
                     acse_op_t opcode, ...)
{
    va_list  ap;
    te_errno rc;
    va_start(ap, opcode);
    rc = tapi_acse_manage_vlist(ctx->ta, ctx->acs_name, ctx->cpe_name,
                                opcode, ap);
    va_end(ap);
    return rc;
}



/* see description in tapi_acse.h */
te_errno
tapi_acse_manage_acs(tapi_acse_context_t *ctx,
                     acse_op_t opcode, ...)
{
    va_list  ap;
    te_errno rc;
    va_start(ap, opcode);
    rc = tapi_acse_manage_vlist(ctx->ta, ctx->acs_name, NULL, opcode, ap);
    va_end(ap);
    return rc;
}


/*
 * ==================== Useful config ACSE methods =====================
 */

/* see description in tapi_acse.h */
te_errno
tapi_acse_clear_acs(tapi_acse_context_t *ctx)
{
    te_errno rc;
    rc = tapi_acse_manage_acs(ctx, ACSE_OP_MODIFY,
                              "enabled", FALSE, VA_END_LIST);
    if (0 == rc)
        rc = tapi_acse_manage_acs(ctx, ACSE_OP_MODIFY,
                                  "enabled", TRUE, VA_END_LIST);
    return rc;
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_clear_cpe(tapi_acse_context_t *ctx)
{
    te_errno rc;
    rc = tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                              "enabled", FALSE, VA_END_LIST);
    if (0 == rc)
        rc = tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "enabled", TRUE, VA_END_LIST);
    return rc;
}



/* see description in tapi_acse.h */
te_errno
tapi_acse_wait_cwmp_state(tapi_acse_context_t *ctx,
                          cwmp_sess_state_t want_state)
{
    cwmp_sess_state_t   cur_state = 0;
    te_errno            rc;
    int                 timeout = ctx->timeout;

    do {
        rc = tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                  "cwmp_state", &cur_state, VA_END_LIST);
        if (rc != 0)
            return rc;

    } while ((timeout < 0 || (timeout--) > 0) &&
             (want_state != cur_state) &&
             (sleep(1) == 0));

    if (0 == timeout && want_state != cur_state)
        return TE_ETIMEDOUT;

    return 0;
}



/* see description in tapi_acse.h */
te_errno
tapi_acse_wait_cr_state(tapi_acse_context_t *ctx,
                        acse_cr_state_t want_state)
{
    acse_cr_state_t     cur_state = 0;
    te_errno            rc;
    int                 timeout = ctx->timeout;

    do {
        rc = tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                  "cr_state", &cur_state, VA_END_LIST);
        if (rc != 0)
            return rc;

        if (CR_ERROR == cur_state)
        {
            ERROR("ConnectionRequest status is ERROR");
            return TE_EFAIL;
        }
    } while ((timeout < 0 || (timeout--) > 0) &&
             (want_state != cur_state) &&
             (sleep(1) == 0));

    if (0 == timeout && want_state != cur_state)
        return TE_ETIMEDOUT;

    return 0;
}



/*
 * ================== local wrappers for RCF RPC =================
 */


static inline te_errno
rpc_cwmp_op_call(rcf_rpc_server *rpcs,
                 const char *acs_name, const char *cpe_name,
                 te_cwmp_rpc_cpe_t cwmp_rpc,
                 uint8_t *buf, size_t buflen, 
                 acse_request_id_t *request_id) 
{
    tarpc_cwmp_op_call_in  in;
    tarpc_cwmp_op_call_out out;

    if (NULL == rpcs)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    if (NULL == acs_name || NULL == cpe_name)
    {
        ERROR("%s(): Invalid ACS/CPE handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.acs_name = strdup(acs_name);
    in.cpe_name = strdup(cpe_name);
    in.cwmp_rpc = cwmp_rpc;

    if (buf != NULL && buflen != 0)
    {
        in.buf.buf_len = buflen;
        in.buf.buf_val = buf;
    }
    else
    {
        in.buf.buf_len = 0;
        in.buf.buf_val = NULL;
    }

    rcf_rpc_call(rpcs, "cwmp_op_call", &in, &out);

    RING("TE RPC(%s,%s): cwmp_op_call(%s/%s, %s) -> %r",
                 rpcs->ta, rpcs->name,
                 acs_name, cpe_name,
                 cwmp_rpc_cpe_string(cwmp_rpc),
                 (te_errno)out.status);

    if (NULL != request_id)
        *request_id = out.request_id;

    return out.status;
}


static inline te_errno
rpc_cwmp_op_check(rcf_rpc_server *rpcs,
                  const char *acs_name, const char *cpe_name,
                  acse_request_id_t request_id,
                  te_cwmp_rpc_acs_t cwmp_rpc_acs,
                  te_cwmp_rpc_cpe_t *cwmp_rpc,
                  uint8_t **buf, size_t *buflen)
{
    tarpc_cwmp_op_check_in  in;
    tarpc_cwmp_op_check_out out;

    if (NULL == rpcs)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return TE_EINVAL;
    }
    if (NULL == acs_name || NULL == cpe_name)
    {
        ERROR("%s(): Invalid ACS/CPE handle", __FUNCTION__);
        return TE_EINVAL;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.acs_name = strdup(acs_name);
    in.cpe_name = strdup(cpe_name);
    in.request_id = request_id;
    in.cwmp_rpc = cwmp_rpc_acs;

    rcf_rpc_call(rpcs, "cwmp_op_check", &in, &out);

    if (buf != NULL && buflen != NULL && out.buf.buf_val != NULL)
    {
        *buflen = out.buf.buf_len;
        *buf = malloc(out.buf.buf_len);
        memcpy(*buf, out.buf.buf_val, out.buf.buf_len);
    }

    if (NULL != cwmp_rpc)
        *cwmp_rpc = out.cwmp_rpc;

    RING("RPC (%s,%s): cwmp_op_check(%s/%s, for %s) -> %r",
                 rpcs->ta, rpcs->name,
                 acs_name, cpe_name, 
                 request_id == 0 ? 
                     cwmp_rpc_acs_string(cwmp_rpc_acs) : 
                     cwmp_rpc_cpe_string(*cwmp_rpc),
                 (te_errno)out.status);

    return out.status;
}


static inline te_errno
rpc_cwmp_conn_req(rcf_rpc_server *rpcs,
                  const char *acs_name, const char *cpe_name)
{
    tarpc_cwmp_conn_req_in  in;
    tarpc_cwmp_conn_req_out out;

    RING("%s() called, srv %s, to %s/%s",
         __FUNCTION__, rpcs->name, acs_name, cpe_name);
    if (NULL == rpcs)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        return -1;
    }
    if (NULL == acs_name || NULL == cpe_name)
    {
        ERROR("%s(): Invalid ACS/CPE handle", __FUNCTION__);
        return -1;
    }

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    rpcs->op = RCF_RPC_CALL_WAIT;

    in.acs_name = strdup(acs_name);
    in.cpe_name = strdup(cpe_name);

    rcf_rpc_call(rpcs, "cwmp_conn_req", &in, &out);

    return out.status;
}



/*
 * =============== Generic methods for CWMP RPC ====================
 */

#define ACSE_BUF_SIZE 65536

te_errno
tapi_acse_cpe_rpc_call(tapi_acse_context_t *ctx,
                       te_cwmp_rpc_cpe_t cpe_rpc_code,
                       cwmp_data_to_cpe_t to_cpe)
{
    uint8_t *buf = malloc(ACSE_BUF_SIZE);
    ssize_t pack_s;

    if (NULL != to_cpe.p)
    {
        pack_s = cwmp_pack_call_data(to_cpe, cpe_rpc_code,
                                     buf, ACSE_BUF_SIZE);
        if (pack_s < 0)
        {
            ERROR("%s(): pack fail", __FUNCTION__);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }
    }
    else 
        pack_s = 0;

    return rpc_cwmp_op_call(ctx->rpc_srv, ctx->acs_name, ctx->cpe_name,
                            cpe_rpc_code, buf, pack_s, &(ctx->req_id));
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_cpe_rpc_response(tapi_acse_context_t *ctx,
                           te_cwmp_rpc_cpe_t *cpe_rpc_code,
                           cwmp_data_from_cpe_t *from_cpe)
{
    te_errno    rc;
    uint8_t    *cwmp_buf = NULL;
    size_t      buflen = 0;
    int         timeout = ctx->timeout;

    te_cwmp_rpc_cpe_t cwmp_rpc_loc = CWMP_RPC_NONE;

    do {
        rc = rpc_cwmp_op_check(ctx->rpc_srv, ctx->acs_name, ctx->cpe_name,
                               ctx->req_id, 0,
                               &cwmp_rpc_loc, &cwmp_buf, &buflen);
    } while ((timeout < 0 || (timeout--) > 0) &&
             (TE_EPENDING == TE_RC_GET_ERROR(rc)) &&
             (sleep(1) == 0));
    VERB("%s(): rc %r, cwmp rpc %s", __FUNCTION__, rc,
         cwmp_rpc_cpe_string(cwmp_rpc_loc));

    if ((0 == rc || TE_CWMP_FAULT == TE_RC_GET_ERROR(rc)) &&
        NULL != from_cpe)
    {
        ssize_t unp_rc;
        if (NULL == cwmp_buf || 0 == buflen)
        {
            WARN("op_check return success, but buffer is NULL.");
            return 0;
        }
        unp_rc = cwmp_unpack_response_data(cwmp_buf, buflen, cwmp_rpc_loc);

        if (0 == unp_rc)
            from_cpe->p = cwmp_buf;
        else
        {
            from_cpe->p = NULL;
            ERROR("%s(): unpack error, rc %r", __FUNCTION__, unp_rc);
            return TE_RC(TE_TAPI, unp_rc);
        }
        if (NULL != cpe_rpc_code)
            *cpe_rpc_code = cwmp_rpc_loc;

        if (TE_CWMP_FAULT == TE_RC_GET_ERROR(rc))
            tapi_acse_log_fault(from_cpe->fault);

    }
    return rc;
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_get_rpc_acs(tapi_acse_context_t *ctx,
                      te_cwmp_rpc_acs_t rpc_acs,
                      cwmp_data_from_cpe_t *from_cpe)
{
    te_errno    rc;
    int         timeout = ctx->timeout;
    uint8_t    *cwmp_buf = NULL;
    size_t      buflen = 0;

    do {
        rc = rpc_cwmp_op_check(ctx->rpc_srv, ctx->acs_name, ctx->cpe_name,
                               0, rpc_acs, NULL, &cwmp_buf, &buflen);
    } while ((timeout < 0 || (timeout--) > 0) &&
             (TE_ENOENT == TE_RC_GET_ERROR(rc)) &&
             (sleep(1) == 0));
    VERB("%s(): rc %r", __FUNCTION__, rc);

    if (0 == rc && NULL != from_cpe)
    {
        ssize_t unp_rc;
        if (NULL == cwmp_buf || 0 == buflen)
        {
            WARN("op_check return success, but buffer is NULL.");
            return 0;
        }
        unp_rc = cwmp_unpack_acs_rpc_data(cwmp_buf, buflen, rpc_acs);

        if (0 == unp_rc)
            from_cpe->p = cwmp_buf;
        else
        {
            from_cpe->p = NULL;
            ERROR("%s(): unpack error, rc %r", __FUNCTION__, unp_rc);
            return TE_RC(TE_TAPI, unp_rc);
        }
    }
    return rc;
}

/*
 * ==================== CWMP RPC methods =========================
 */



/* see description in tapi_acse.h */
te_errno
tapi_acse_get_rpc_methods(tapi_acse_context_t *ctx)
{
    return rpc_cwmp_op_call(ctx->rpc_srv, ctx->acs_name, ctx->cpe_name,
                            CWMP_RPC_get_rpc_methods,
                            NULL, 0, &(ctx->req_id));
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_get_rpc_methods_resp(tapi_acse_context_t *ctx,
                               string_array_t **resp)
{
    cwmp_get_rpc_methods_response_t *from_cpe_r = NULL;

    te_errno rc = tapi_acse_cpe_rpc_response(ctx, NULL,
                PTR_FROM_CPE(from_cpe_r));

    if (0 == rc && NULL != resp && NULL != from_cpe_r)
        COPY_STRING_ARRAY(from_cpe_r->MethodList_, *resp);
    return rc;
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_download(tapi_acse_context_t *ctx,
                   cwmp_download_t *req)
{
    cwmp_data_to_cpe_t to_cpe_loc;
    to_cpe_loc.download = req;

    return tapi_acse_cpe_rpc_call(ctx, CWMP_RPC_download, to_cpe_loc);
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_download_resp(tapi_acse_context_t *ctx,
                        cwmp_download_response_t **resp)
{
    cwmp_data_from_cpe_t from_cpe_loc = {.p = NULL};
    te_errno rc = tapi_acse_cpe_rpc_response(ctx, NULL, &from_cpe_loc);
    if (NULL != resp && NULL != from_cpe_loc.p)
        *resp = from_cpe_loc.download_r;
    return rc;
}



/* see description in tapi_acse.h */
te_errno
tapi_acse_get_parameter_values(tapi_acse_context_t *ctx,
                               string_array_t *names)
{
    cwmp_data_to_cpe_t to_cpe_loc;
    ParameterNames             par_list;
    _cwmp__GetParameterValues  req;

    if (NULL == ctx || NULL == names)
        return TE_EINVAL;
    cwmp_str_array_log(TE_LL_RING, "Issue GetParameterValues", names);

    req.ParameterNames_ = &par_list;
    to_cpe_loc.get_parameter_values = &req;

    par_list.__ptrstring = names->items;
    par_list.__size      = names->size;

    return tapi_acse_cpe_rpc_call(ctx, CWMP_RPC_get_parameter_values,
                                  to_cpe_loc);
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_get_parameter_values_resp(tapi_acse_context_t *ctx,
                                    cwmp_values_array_t **resp)
{
    cwmp_data_from_cpe_t from_cpe_loc = {.p = NULL};

    te_errno rc = tapi_acse_cpe_rpc_response(ctx, NULL, &from_cpe_loc);

    /* Copy good response if it is necessary for user */
    if (0 == rc && NULL != resp && NULL != from_cpe_loc.p)
    {
        *resp = cwmp_copy_par_value_list(
            from_cpe_loc.get_parameter_values_r->ParameterList);
        cwmp_val_array_log(TE_LL_RING,
                           "Got GetParameterValuesResponse", *resp);
    }
    else
        RING("Got GetParameterValuesResponse, rc %r, from_cpe %p",
             rc, from_cpe_loc.p);
    if (NULL != from_cpe_loc.p)
        free(from_cpe_loc.p);
    return rc;
}

te_errno
tapi_acse_get_pvalues_sync(tapi_acse_context_t *ctx,
                           string_array_t *names,
                           cwmp_values_array_t **resp)
{
    te_errno rc;
    int sync_mode;
    cwmp_sess_state_t cwmp_state;
    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                  "sync_mode", &sync_mode, VA_END_LIST));
    CHECK_RC(tapi_acse_get_cwmp_state(ctx, &cwmp_state));
    if (sync_mode != 1 || cwmp_state != CWMP_PENDING)
    {
        ERROR("Call %s in wrong state, sync_mode is %d, cwmp state is %d",
             __FUNCTION__, sync_mode, cwmp_state);
        return TE_RC(TE_TAPI, TE_EDEADLK);
    }
    CHECK_RC(tapi_acse_get_parameter_values(ctx, names));
    CHECK_RC(tapi_acse_get_parameter_values_resp(ctx, resp));
    return 0;
}


te_errno
tapi_acse_get_pvalue_sync(tapi_acse_context_t *ctx,
                          const char *name,
                          cwmp_values_array_t **resp)
{
    te_errno rc;
    int sync_mode;
    cwmp_sess_state_t cwmp_state;
    string_array_t *names = cwmp_str_array_alloc(name, "", VA_END_LIST);

    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                                  "sync_mode", &sync_mode, VA_END_LIST));
    CHECK_RC(tapi_acse_get_cwmp_state(ctx, &cwmp_state));
    if (sync_mode != 1 || cwmp_state != CWMP_PENDING)
    {
        ERROR("Call %s in wrong state, sync_mode is %d, cwmp state is %d",
             __FUNCTION__, sync_mode, cwmp_state);
        return TE_RC(TE_TAPI, TE_EDEADLK);
    }
    CHECK_RC(tapi_acse_get_parameter_values(ctx, names));
    CHECK_RC(tapi_acse_get_parameter_values_resp(ctx, resp));
    cwmp_str_array_free(names);
    return 0;
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_get_parameter_names(tapi_acse_context_t *ctx,
                                  te_bool next_level,
                                  const char *fmt, ...)
{
    char name[256];
    char *name_ptr = name;
    va_list  ap;
    cwmp_get_parameter_names_t req;
    cwmp_data_to_cpe_t to_cpe_loc;

    va_start(ap, fmt);
    vsnprintf(name, sizeof(name), fmt, ap);
    va_end(ap);
    to_cpe_loc.get_parameter_names = &req;
    req.ParameterPath = &name_ptr;
    req.NextLevel = next_level;

    RING("Issue GetParameterNames for <%s>, next_level %d.", 
         name, (int)next_level);

    return tapi_acse_cpe_rpc_call(ctx, CWMP_RPC_get_parameter_names,
                                  to_cpe_loc);
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_get_parameter_names_resp(tapi_acse_context_t *ctx,
                                   string_array_t **resp)
{
    cwmp_data_from_cpe_t from_cpe_loc = {.p = NULL};
    te_errno rc = tapi_acse_cpe_rpc_response(ctx, NULL, &from_cpe_loc);
    if (0 == rc && NULL != resp && NULL != from_cpe_loc.p)
    { 
        int i;
        ParameterInfoList *name_list =
            from_cpe_loc.get_parameter_names_r->ParameterList;
        (*resp) = (string_array_t *)malloc(sizeof(string_array_t));
        (*resp)->size = name_list->__size;
        (*resp)->items = calloc(name_list->__size, sizeof(char*));
        for (i = 0; i < name_list->__size; i++)
            (*resp)->items[i] =
                strdup((name_list->__ptrParameterInfoStruct[i])->Name);
        cwmp_str_array_log(TE_LL_RING, "Got GetParameterNamesResponse",
                           *resp);
    }
    return rc;
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_set_parameter_values(tapi_acse_context_t *ctx,
                               const char *par_key,
                               cwmp_values_array_t *val_arr)
{
    cwmp_data_to_cpe_t to_cpe_loc;
    cwmp_set_parameter_values_t req;
    ParameterValueList pv_list;

    if (NULL == ctx || NULL == par_key || NULL == val_arr)
        return TE_EINVAL;

    cwmp_val_array_log(TE_LL_RING, "Issue SetParameterValues", val_arr);

    pv_list.__ptrParameterValueStruct = val_arr->items;
    pv_list.__size = val_arr->size;

    req.ParameterList = &pv_list;
    req.ParameterKey = strdup(par_key);
    to_cpe_loc.set_parameter_values = &req;

    return tapi_acse_cpe_rpc_call(ctx, CWMP_RPC_set_parameter_values,
                             to_cpe_loc);
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_set_parameter_values_resp(tapi_acse_context_t *ctx, int *status)
{
    cwmp_data_from_cpe_t from_cpe_loc = {.p = NULL};
    te_errno rc = tapi_acse_cpe_rpc_response(ctx, NULL, &from_cpe_loc);
    if (0 == rc && NULL != status && NULL != from_cpe_loc.p)
        *status = from_cpe_loc.set_parameter_values_r->Status;
    RING("Got SetParameterValuesResponse, rc %r, status %d",
         rc, from_cpe_loc.set_parameter_values_r->Status);
    return rc;
}

te_errno
tapi_acse_get_parameter_attributes(tapi_acse_context_t *ctx,
                                   string_array_t *names)
{
    cwmp_get_parameter_attributes_t req;
    cwmp_parameter_names_t          par_names;

    req.ParameterNames_ = &par_names;
    if (NULL == names)
    {
        par_names.__ptrstring = NULL;
        par_names.__size = 0;
    }
    else
    {
        par_names.__ptrstring = names->items;
        par_names.__size      = names->size;
    }

    return tapi_acse_cpe_rpc_call(ctx, CWMP_RPC_get_parameter_attributes,
                                  PTR_TO_CPE(&req));
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_cpe_connect(tapi_acse_context_t *ctx)
{
    te_errno rc;

    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "sync_mode", TRUE, VA_END_LIST));

    CHECK_RC(tapi_acse_cpe_conn_request(ctx));

    rc = tapi_acse_wait_cr_state(ctx, CR_DONE);
    if (0 != rc)
    {
        cwmp_sess_state_t   cur_sess_state = 0;
        acse_cr_state_t     cur_cr_state = 0;
        sleep(3);
        CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_OBTAIN,
                  "cwmp_state", &cur_sess_state,
                  "cr_state", &cur_cr_state, VA_END_LIST));
        if (CWMP_NOP == cur_sess_state && CR_NONE == cur_cr_state)
        {
            CHECK_RC(tapi_acse_cpe_connect(ctx));
            CHECK_RC(tapi_acse_wait_cr_state(ctx, CR_DONE));
        }
    }
    CHECK_RC(tapi_acse_wait_cwmp_state(ctx, CWMP_PENDING));
    return 0;
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_cpe_conn_request(tapi_acse_context_t *ctx)
{
    return rpc_cwmp_conn_req(ctx->rpc_srv, ctx->acs_name, ctx->cpe_name);
}


/* see description in tapi_acse.h */
te_errno
tapi_acse_cpe_disconnect(tapi_acse_context_t *ctx)
{
    te_errno rc;
    /* TODO : this util simple activates sending empty response, 
     * this is do not automatically leads to terminate CWMP session.
     * Investigate standard and real behaviour of clients,
     * maybe add here some more actions and/or checks state... 
     * 
     * Usually this will terminate session. Single exclusion,
     * it seems, is true HoldRequests status.
     */

    CHECK_RC(rpc_cwmp_op_call(ctx->rpc_srv, ctx->acs_name, ctx->cpe_name,
                            CWMP_RPC_NONE, NULL, 0, NULL));
    CHECK_RC(tapi_acse_manage_cpe(ctx, ACSE_OP_MODIFY,
                                  "sync_mode", FALSE, VA_END_LIST));
    return 0;
}




/* see description in tapi_acse.h */
te_errno
tapi_acse_add_object(tapi_acse_context_t *ctx,
                     const char *obj_name, const char *param_key)
{
    char                obj_name_buf[256];
    char                param_key_buf[256];
    cwmp_add_object_t   add_object = {obj_name_buf, param_key_buf};
    cwmp_data_to_cpe_t  to_cpe_loc;

    to_cpe_loc.add_object = &add_object;

    strncpy(obj_name_buf, obj_name, sizeof(obj_name_buf));
    strncpy(param_key_buf, param_key, sizeof(param_key_buf));

    return tapi_acse_cpe_rpc_call(ctx, CWMP_RPC_add_object,
                                  to_cpe_loc);
}



/* see description in tapi_acse.h */
te_errno
tapi_acse_add_object_resp(tapi_acse_context_t *ctx,
                          int *obj_index, int *add_status)
{
    cwmp_data_from_cpe_t from_cpe_loc = {.p = NULL};
    te_errno             rc;

    from_cpe_loc.p = NULL;
    rc = tapi_acse_cpe_rpc_response(ctx, NULL, &from_cpe_loc);

    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(rc))
        tapi_acse_log_fault(from_cpe_loc.fault);
    else if (rc == 0)
    {
        if (NULL != obj_index)
            *obj_index = from_cpe_loc.add_object_r->InstanceNumber;
        if (NULL != add_status)
            *add_status = from_cpe_loc.add_object_r->Status;
    }
    free(from_cpe_loc.p);
    return rc;
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_delete_object(tapi_acse_context_t *ctx,
                        const char *obj_name, const char *param_key)
{
    char  obj_name_buf[256];
    char  param_key_buf[256];

    cwmp_delete_object_t  del_object = {obj_name_buf, param_key_buf};
    cwmp_data_to_cpe_t    to_cpe_loc;

    to_cpe_loc.delete_object = &del_object;

    strncpy(obj_name_buf, obj_name, sizeof(obj_name_buf));
    strncpy(param_key_buf, param_key, sizeof(param_key_buf));

    return tapi_acse_cpe_rpc_call(ctx, CWMP_RPC_delete_object,
                                  to_cpe_loc);
}

/* see description in tapi_acse.h */
te_errno
tapi_acse_delete_object_resp(tapi_acse_context_t *ctx, int *del_status)
{
    cwmp_data_from_cpe_t from_cpe_loc = {.p = NULL};
    te_cwmp_rpc_cpe_t    resp_code;
    te_errno             rc;

    from_cpe_loc.p = NULL;
    rc = tapi_acse_cpe_rpc_response(ctx, &resp_code, &from_cpe_loc);

    if (TE_CWMP_FAULT == TE_RC_GET_ERROR(rc))
        tapi_acse_log_fault(from_cpe_loc.fault);
    else if (rc == 0)
    {
        if (CWMP_RPC_delete_object == resp_code)
        {
            if (NULL != del_status)
                *del_status = from_cpe_loc.delete_object_r->Status;
        }
        else
        {
            WARN("%s(): received unexpected response, RPC %s",
                 __FUNCTION__, cwmp_rpc_cpe_string(resp_code));
            rc = TE_EFAIL;
        }
    }
    free(from_cpe_loc.p);
    return rc;
}




/*
 * ============= Useful routines for prepare CWMP RPC params =============
 */

#if 0
/* see description in tapi_acse.h */
cwmp_get_parameter_names *
cwmp_get_names_alloc(const char *name, te_bool next_level)
{
    cwmp_get_parameter_names *ret = calloc(1, sizeof(*ret));
    ret->NextLevel = next_level;
    ret->ParameterPath = malloc(sizeof(char*));
    if (NULL == name)
        ret->ParameterPath[0] = NULL;
    else    
        ret->ParameterPath[0] = strdup(name);
    return ret;
}

/* see description in tapi_acse.h */
void
cwmp_get_names_free(_cwmp__GetParameterNames *arg)
{
    if (NULL == arg)
        return;
    do {
        if (NULL == arg->ParameterPath)
            break;
        free(arg->ParameterPath[0]);
        free(arg->ParameterPath);
    } while (0);
    free(arg);
    return;
}


#endif

/* see description in tapi_acse.h */
void
cwmp_get_names_resp_free(cwmp_get_parameter_names_response_t *resp)
{
    if (NULL == resp)
        return;
    /* response is assumed to be obtained from this TAPI, with pointers,
        which are filled by cwmp_data_unpack_* methods, so there is only
        one block of allocated memory */
    free(resp);
    return;
}


/* 
 * =========== misc =============================
 */

te_errno
tapi_acse_get_full_url(tapi_acse_context_t *ctx,
                       struct sockaddr *addr,
                       char *str, size_t buflen)
{
    char  acs_addr_buf[200];
    int   acs_port;
    char *acs_url;
    int   acs_ssl;

    te_errno rc;

    inet_ntop(addr->sa_family, te_sockaddr_get_netaddr(addr),
              acs_addr_buf, sizeof(acs_addr_buf));

    rc = tapi_acse_manage_acs(ctx, ACSE_OP_OBTAIN,
                                  "port", &acs_port,
                                  "ssl", &acs_ssl,
                                  "url", &acs_url,
                                  VA_END_LIST);
    if (rc != 0)
        return rc;

    snprintf(str, buflen, "http%s://%s:%u%s",
             acs_ssl ? "s" : "", 
             acs_addr_buf, acs_port, acs_url);
    RING("prepared ACS url: '%s'", str);

    return 0;
}

