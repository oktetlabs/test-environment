/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD PCAP
 *
 * Traffic Application Domain Command Handler.
 * Ethernet-PCAP CSAP stack-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD Ethernet-PCAP"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_errno.h"
#include "te_alloc.h"
#include "tad_common.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tad_eth_sap.h"
#include "tad_csap_inst.h"
#include "tad_pcap_impl.h"


/** Ethernet layer read/write specific data */
typedef struct tad_pcap_rw_data {
    tad_eth_sap     sap;        /**< Ethernet service access point */
    unsigned int    recv_mode;  /**< Default receive mode */
} tad_pcap_rw_data;



/* See description tad_pcap_impl.h */
te_errno
tad_pcap_prepare_recv(csap_p csap)
{
    tad_pcap_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_open(&spec_data->sap, spec_data->recv_mode);
}

/* See description tad_pcap_impl.h */
te_errno
tad_pcap_shutdown_recv(csap_p csap)
{
    tad_pcap_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv_close(&spec_data->sap);
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_read_cb(csap_p csap, unsigned int timeout,
                tad_pkt *pkt, size_t *pkt_len)
{
    tad_pcap_rw_data *spec_data = csap_get_rw_data(csap);

    assert(spec_data != NULL);

    return tad_eth_sap_recv(&spec_data->sap, timeout, pkt, pkt_len);
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_rw_init_cb(csap_p csap)
{
    te_errno            rc;
    unsigned int        layer = csap_get_rw_layer(csap);
    char                device_id[TAD_ETH_SAP_IFNAME_SIZE];
    size_t              val_len;
    tad_pcap_rw_data   *spec_data;
    const asn_value    *pcap_csap_spec;


    pcap_csap_spec = csap->layers[layer].nds;

    val_len = sizeof(device_id);
    rc = asn_read_value_field(pcap_csap_spec, device_id, &val_len,
                              "ifname");
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
    rc = asn_read_value_field(pcap_csap_spec, &spec_data->recv_mode,
                              &val_len, "receive-mode");
    if (rc != 0)
    {
        spec_data->recv_mode = TAD_ETH_RECV_DEF;
    }

    csap_set_rw_data(csap, spec_data);

    return 0;
}


/* See description tad_pcap_impl.h */
te_errno
tad_pcap_rw_destroy_cb(csap_p csap)
{
    tad_pcap_rw_data    *spec_data = csap_get_rw_data(csap);
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
