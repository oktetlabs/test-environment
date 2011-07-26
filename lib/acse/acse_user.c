/** @file
 * @brief ACSE 
 *
 * ACSE user utilities library
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "ACSE user utils"

#include "te_config.h"


#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <poll.h>

#include "acse_epc.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"

#include "acse_epc.h"
#include "acse_user.h"


static acse_epc_cwmp_data_t cwmp_msg;
static acse_epc_config_data_t cfg_data;


/**
 * Initializes acse params substructure with the supplied parameters.
 *
 * @param gid           Group identifier
 * @param oid           Object identifier
 * @param acs           Name of the acs instance
 * @param cpe           Name of the cpe instance
 *
 * @return              Status code
 */
static te_errno
prepare_params(acse_epc_config_data_t *config_params,
               char const *oid,
               char const *acs, char const *cpe)
{
    if (oid != NULL)
    {
        const char *last_label = rindex(oid, '/');
        unsigned i;

        if (NULL == last_label)
            last_label = oid;
        else 
            last_label++; /* shift to the label begin */
        if (strlen(last_label) >= sizeof(config_params->oid))
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        for (i = 0; last_label[i] != '\0' && last_label[i] != ':'; i++)
            config_params->oid[i] = last_label[i];
        config_params->oid[i] = '\0';
    }
    else
        config_params->oid[0] = '\0';

    if (acs != NULL)
    {
        if (strlen(acs) >= sizeof(config_params->acs))
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        strcpy(config_params->acs, acs);
    }
    else
        config_params->acs[0] = '\0';

    if (cpe != NULL)
    {
        if (strlen(cpe) >= sizeof(config_params->cpe))
            return TE_RC(TE_TA_UNIX, TE_EINVAL);

        strcpy(config_params->cpe, cpe);
    }
    else
        config_params->cpe[0] = '\0';

    return 0;
}

/* See description in acse_user.h */
te_errno
acse_conf_prepare(acse_cfg_op_t fun, acse_epc_config_data_t **user_c_data)
{
    memset(&cfg_data, 0, sizeof(cfg_data));

    if (NULL != user_c_data)
        *user_c_data = &cfg_data;

    cfg_data.op.magic = EPC_CONFIG_MAGIC;
    cfg_data.op.fun = fun;

    return 0;
}


/* See description in acse_user.h */
te_errno
acse_conf_call(acse_epc_config_data_t **user_cfg_result)
{
    te_errno rc;
    int             epc_socket = acse_epc_socket();
    acse_epc_config_data_t *cfg_res = malloc(sizeof(*cfg_res));
#if 0
    struct timespec epc_ts = {0, 300000000}; /* 300 ms */
#else
    struct timespec epc_ts = {2, 0}; /* 2 sec */
#endif
    struct pollfd   pfd = {0, POLLIN, 0};
    int             pollrc;

    rc = acse_epc_conf_send(&cfg_data);

    if (rc != 0)
    {
        ERROR("EPC send rc %r", rc);
        return rc;
    }

    pfd.fd = epc_socket;
    pollrc = ppoll(&pfd, 1, &epc_ts, NULL);
    if (pollrc < 0)
    {
        int saved_errno = errno;
        ERROR("poll on EPC socket failed, sys errno: %s",
                strerror(saved_errno));
        return TE_OS_RC(TE_TA_UNIX, saved_errno);
    }
    if (pollrc == 0)
    {
        ERROR("config EPC operation timed out");
        return TE_RC(TE_TA_UNIX, TE_ETIMEDOUT);
    }

    rc = acse_epc_conf_recv(cfg_res);
    if (rc != 0)
    {
        ERROR("ACSE config: EPC recv failed %r", rc);
        return TE_RC(TE_TA_ACSE, rc);
    }

    if (NULL != user_cfg_result)
        *user_cfg_result = cfg_res;

    if ((rc = cfg_res->status) != 0)
        WARN("%s(): status of EPC operation %r", __FUNCTION__, rc);

    return TE_RC(TE_ACSE, rc);
}

/* See description in acse_user.h */
te_errno
acse_conf_op(char const *oid, char const *acs, char const *cpe,
             const char *value, acse_cfg_op_t fun,
             acse_epc_config_data_t **cfg_result)
{
    te_errno    rc; 
    acse_cfg_level_t        level;
    acse_epc_config_data_t *cfg_request;

    if (acse_epc_socket() < 0)
        return TE_ENOTCONN;

    if (NULL == cfg_result)
        return TE_EINVAL;

    if (EPC_CFG_MODIFY == fun && NULL == value)
        return TE_EINVAL;

    if (EPC_CFG_LIST == fun)
    {
        if (acs != NULL && acs[0]) /* check is there ACS label */
            level = EPC_CFG_CPE; /* ACS specified, get list of its CPE */
        else
            level = EPC_CFG_ACS; /* ACS not specified, get list of ACS */
    }
    else
    {
        if (cpe != NULL && cpe[0]) /* check is there CPE label */
            level = EPC_CFG_CPE; 
        else
            level = EPC_CFG_ACS;
    }

    acse_conf_prepare(fun, &cfg_request);
    cfg_request->op.level = level;

    rc = prepare_params(cfg_request, oid, acs, cpe);
    if (rc != 0)
    {
        ERROR("wrong labels passed to ACSE configurator subtree");
        return rc;
    }

    if (EPC_CFG_MODIFY == fun)
        strncpy(cfg_request->value, value, sizeof(cfg_request->value));
    else
        cfg_request->value[0] = '\0';

    rc = acse_conf_call(cfg_result);

    return rc;
}


/* See description in acse_user.h */
te_errno
acse_cwmp_prepare(const char *acs, const char *cpe,
                  acse_epc_cwmp_op_t fun,
                  acse_epc_cwmp_data_t **cwmp_data)
{
    memset(&cwmp_msg, 0, sizeof(cwmp_msg));

    cwmp_msg.op = fun;
        
    if (acs)
        strcpy(cwmp_msg.acs, acs);
    if (cpe) 
        strcpy(cwmp_msg.cpe, cpe);

    if (NULL != cwmp_data)
        *cwmp_data = &cwmp_msg;

    return 0;
}

/* See description in acse_user.h */
te_errno
acse_cwmp_call(size_t *data_len, acse_epc_cwmp_data_t **cwmp_data)
{
    te_errno rc;
    struct timespec epc_ts = {1, 0}; /* 1 sec */
    struct pollfd   pfd = {0, POLLIN, 0};
    int             pollrc;

    rc = acse_epc_cwmp_send(&cwmp_msg);
    if (rc != 0)
    {
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);
        return rc;
    }

    pfd.fd = epc_from_acse_pipe[0];
    pollrc = ppoll(&pfd, 1, &epc_ts, NULL);

    if (pollrc < 0)
    {
        int saved_errno = errno;
        ERROR("call ACSE CWMP, recv answer;"
              " poll on EPC socket failed, sys errno: %s",
                strerror(saved_errno));
        return TE_OS_RC(TE_TA_UNIX, saved_errno);
    }
    if (pollrc == 0)
    {
        ERROR("call ACSE CWMP, recv answer EPC timed out");
        return TE_RC(TE_TA_UNIX, TE_ETIMEDOUT);
    }

    rc = acse_epc_cwmp_recv(cwmp_data, data_len);
    if (rc != 0)
        ERROR("%s(): EPC recv failed %r", __FUNCTION__, rc);

    return TE_RC(TE_TA_ACSE, rc);
}


/* See description in acse_user.h */
te_errno
acse_cwmp_connreq(const char *acs, const char *cpe,
                  acse_epc_cwmp_data_t **cwmp_data)
{
    te_errno rc;

    rc = acse_cwmp_prepare(acs, cpe, EPC_CONN_REQ, NULL);
    if (0 == rc)
        rc = acse_cwmp_call(NULL, cwmp_data);
    else 
        WARN("acse_cwmp_connreq(): rc of acse_cwmp_prepare() -> %r", rc);
    if (0 != rc)
    {
        WARN("acse_cwmp_connreq(): rc of acse_cwmp_call() -> %r", rc);
        return TE_RC(TE_TA_ACSE, rc);
    }
    return 0;
}


te_errno
acse_cwmp_rpc_call(const char *acs, const char *cpe,
                   acse_request_id_t *request_id,
                   te_cwmp_rpc_cpe_t  rpc_cpe,
                   cwmp_data_to_cpe_t to_cpe)
{
    acse_epc_cwmp_data_t *cwmp_data = NULL;
    te_errno rc;

    rc = acse_cwmp_prepare(acs, cpe, EPC_RPC_CALL, &cwmp_data);
    if (rc != 0)
        return TE_RC(TE_TA_ACSE, rc);
    cwmp_data->to_cpe.p = to_cpe.p;
    cwmp_data->rpc_cpe = rpc_cpe;
    rc = acse_cwmp_call(NULL, &cwmp_data);
    if (0 != rc)
        return TE_RC(TE_TA_ACSE, rc);
    if (0 != cwmp_data->status)
        return TE_RC(TE_ACSE, cwmp_data->status);
    if (request_id != NULL)
        *request_id = cwmp_data->request_id;
    return 0;
}


te_errno
acse_http_code(const char *acs, const char *cpe,
               int http_code, const char *location)
{
    acse_epc_cwmp_data_t *cwmp_data = calloc(1, sizeof(*cwmp_data));
    size_t loc_len = 1; /* last zero even for empty 'location' */
    te_errno rc = 0;

    if (NULL == acs || NULL == cpe)
        return TE_EINVAL;
    if (NULL != location)
        loc_len = strlen(location) + 1;

    cwmp_data->op = EPC_HTTP_RESP;
        
    strcpy(cwmp_data->acs, acs);
    strcpy(cwmp_data->cpe, cpe);

    cwmp_data->to_cpe.http_code = http_code;
    if (NULL != location)
        strcpy((char *)cwmp_data->enc_start, location);

    VERB("%s() send msg, http code %d, loc '%s'", __FUNCTION__, 
         cwmp_data->to_cpe.http_code, (char *)cwmp_data->enc_start);


    rc = acse_epc_cwmp_send(cwmp_data);

    free(cwmp_data);

    if (rc != 0)
    {
        ERROR("%s(): EPC send failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = acse_epc_cwmp_recv(&cwmp_data, NULL);
    if (rc != 0)
    {
        ERROR("%s(): EPC recv failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TA_ACSE, rc);
    }

    return TE_RC(TE_ACSE, cwmp_data->status);
}

