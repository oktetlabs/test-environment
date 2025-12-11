/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to allocate L4 port
 */

#define TE_LGR_USER     "Conf L4 port TAPI"

#include "te_config.h"
#include "te_errno.h"
#include "conf_api.h"

te_errno
tapi_cfg_l4_port_alloc(const char *ta, int family, int type, uint16_t *port)
{
    int32_t next;
    te_errno rc;

    if (ta == NULL || port == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, family),
                              "/agent:%s/l4_port:/alloc:/next:/family:", ta);
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(INT32, type),
                              "/agent:%s/l4_port:/alloc:/next:/type:", ta);
    if (rc != 0)
        return rc;

    rc = cfg_get_int32(&next, "/agent:%s/l4_port:/alloc:/next:", ta);
    if (rc != 0)
        return rc;

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(NONE, NULL),
                              "/agent:%s/l4_port:/alloc:/allocated:%u",
                              ta, next);
    if (rc != 0)
        return rc;

    *port = next;
    return 0;
}
