/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Ethernet
 *
 * Traffic Application Domain Command Handler.
 * Ethernet CSAP, stack-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD Ethernet"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_IF_ETHER_H
#include <sys/sys_ether.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_alloc.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "ndn_eth.h"
#include "tad_eth_impl.h"



/* See description tad_eth_impl.h */
te_errno
tad_eth_prepare_send(csap_p csap)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_send_open(&spec_data->sap, 0 /* FIXME */);
}

/* See description tad_eth_impl.h */
te_errno
tad_eth_shutdown_send(csap_p csap)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_send_close(&spec_data->sap);
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_prepare_recv(csap_p csap)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_open(&spec_data->sap, spec_data->recv_mode);
}

/* See description tad_eth_impl.h */
te_errno
tad_eth_shutdown_recv(csap_p csap)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_close(&spec_data->sap);
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_read_cb(csap_p csap, unsigned int timeout,
                tad_pkt *pkt, size_t *pkt_len)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv(&spec_data->sap, timeout, pkt, pkt_len);
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_write_cb(csap_p csap, const tad_pkt *pkt)
{
    tad_eth_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_send(&spec_data->sap, pkt);
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_rw_init_cb(csap_p csap)
{
    te_errno            rc;
    unsigned int        layer = csap_get_rw_layer(csap);
    char                device_id[TAD_ETH_SAP_IFNAME_SIZE];
    size_t              val_len;
    tad_eth_rw_data    *spec_data;
    const asn_value    *eth_csap_spec;


    eth_csap_spec = csap->layers[layer].nds;

    val_len = sizeof(device_id);
    rc = asn_read_value_field(eth_csap_spec, device_id, &val_len,
                              "device-id");
    if (rc != 0)
    {
        ERROR("device-id for Ethernet not found: %r", rc);
        return TE_RC(TE_TAD_CSAP, rc);
    }

    spec_data = TE_ALLOC(sizeof(*spec_data));
    if (spec_data == NULL)
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);

    rc = tad_eth_sap_attach(device_id, &spec_data->sap);
    if (rc != 0)
    {
        ERROR("Failed to attach Ethernet read-write layer to media: %r",
              rc);
        return rc;
    }
    spec_data->sap.csap = csap;

    val_len = sizeof(spec_data->recv_mode);
    rc = asn_read_value_field(eth_csap_spec, &spec_data->recv_mode,
                              &val_len, "receive-mode");
    if (rc != 0)
    {
        spec_data->recv_mode = TAD_ETH_RECV_DEF;
    }

    csap_set_rw_data(csap, spec_data);

    return 0;
}


/* See description tad_eth_impl.h */
te_errno
tad_eth_rw_destroy_cb(csap_p csap)
{
    tad_eth_rw_data    *spec_data = csap_get_rw_data(csap);
    te_errno            rc;

    if (spec_data == NULL)
    {
        WARN("Not ethernet CSAP %d special data found!", csap->id);
        return 0;
    }
    csap_set_rw_data(csap, NULL);

    rc = tad_eth_sap_detach(&spec_data->sap);

    free(spec_data);

    return rc;
}
