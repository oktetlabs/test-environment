/** @file
 * @brief Dummy functions for Test Agent sniffers calls.
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 */

#include "te_config.h"
#include "rcf_pch_internal.h"
#include "te_errno.h"

/**
 * Dummy function for rcf_ch_get_sniffers call. Used when the agent does not
 * support the sniffer framework.
 *
 * @return (TE_RCF_PCH, TE_ENOPROTOOPT)     value means the protocol is
 *                                          not supported
 */
te_errno
rcf_ch_get_sniffers(struct rcf_comm_connection *handle, char *cbuf,
                    size_t buflen, size_t answer_plen,
                    const char *sniff_id_str)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(sniff_id_str);
    return TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT);
}

/**
 * Dummy function for rcf_ch_get_snif_dump call. Used when the agent does
 * not support the sniffer framework.
 *
 * @return (TE_RCF_PCH, TE_ENOPROTOOPT)     value means the protocol is
 *                                          not supported
 */
te_errno
rcf_ch_get_snif_dump(struct rcf_comm_connection *handle, char *cbuf,
                    size_t buflen, size_t answer_plen,
                    const char *sniff_id_str)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(sniff_id_str);
    return TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT);
}
