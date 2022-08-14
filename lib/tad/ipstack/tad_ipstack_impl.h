/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IP Stack CSAPs implementaion internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_IPSTACK_IMPL_H__
#define __TE_TAD_IPSTACK_IMPL_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>


#include "te_defs.h"
#include "te_errno.h"
#include "te_ipstack.h"

#include "asn_usr.h"
#include "ndn_ipstack.h"

#include "logger_api.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"


#ifdef __cplusplus
extern "C" {
#endif

/** Size of IPv6 address in bytes */
#define IP6_ADDR_LEN 16

/** The offset to the total length field in IPv4 header, bytes */
#define IP4_HDR_TOTAL_LEN_OFFSET 2

/**
 * The length of IPv6 Pseudo Header used in calculation of
 * Upper-Layer checksums (see RFC 2460, section 8.1 for details)
 */
#define IP6_PSEUDO_HDR_LEN (IP6_ADDR_LEN * 2 + 8)

/** The index of IPv4 header DU corresponding to 'h-checksum' PDU field */
#define IP4_HDR_H_CKSUM_DU_INDEX 11

/** The index of UDP header DU corresponding to 'length' PDU field */
#define UDP_HDR_P_LEN_DU_INDEX 2

/** The index of UDP header DU corresponding to 'checksum' PDU field */
#define UDP_HDR_CKSUM_DU_INDEX 3

/** The length of IP header field 'version' is 4 bits */
#define IP_HDR_VERSION_LEN 4

/** The shift of IP header field 'version' is 4 bits */
#define IP_HDR_VERSION_SHIFT 4

/** IPv4 version number */
#define IP4_VERSION 4

/** IPv6 version number */
#define IP6_VERSION 6

/** The offset (the number of 32-bit words) to the IPv4 source address */
#define IP4_HDR_SRC_OFFSET 3

/** The offset (the number of 32-bit words) to the IPv4 destination address */
#define IP4_HDR_DST_OFFSET 4

/** The offset (the number of 32-bit words) to the IPv6 source address */
#define IP6_HDR_SRC_OFFSET 2

/** The offset (the number of 32-bit words) to the IPv6 destination address */
#define IP6_HDR_DST_OFFSET 6

/** Length of IPv6 address (the number of 32-bit words) */
#define IP6_HDR_SIN6_ADDR_LEN 4

/** The index of TCP header DU corresponding to 'checksum' PDU field */
#define TCP_HDR_CKSUM_DU_INDEX 7


/*
 * TCP CSAP specific data
 */
typedef struct tcp_csap_specific_data
{
    unsigned short   local_port;    /**< Local TCP port, in HOST order*/
    unsigned short   remote_port;   /**< Remote TCP port, in HOST order*/

    unsigned short   src_port;    /**< Source TCP port for current packet*/
    unsigned short   dst_port;    /**< Dest.  TCP port for current packet*/

    uint16_t         data_tag;
    size_t           opt_bin_len;
    const asn_value *options;

    tad_data_unit_t  du_src_port;
    tad_data_unit_t  du_dst_port;
    tad_data_unit_t  du_seqn;
    tad_data_unit_t  du_ackn;
    tad_data_unit_t  du_hlen;
    tad_data_unit_t  du_flags;
    tad_data_unit_t  du_win_size;
    tad_data_unit_t  du_checksum;
    tad_data_unit_t  du_urg_p;

} tcp_csap_specific_data_t;




/*
 * IP callbacks
 */

/**
 * Callback to init IPv4 layer as read/write layer of the CSAP.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */
extern te_errno tad_ip4_rw_init_cb(csap_p csap);


/**
 * Callback to destroy IPv4 layer as read/write layer of the CSAP.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */
extern te_errno tad_ip4_rw_destroy_cb(csap_p csap);

/**
 * Callback for read data from media of IPv4 CSAP.
 *
 * The function complies with csap_read_cb_t prototype.
 */
extern te_errno tad_ip4_read_cb(csap_p csap, unsigned int timeout,
                                tad_pkt *pkt, size_t *pkt_len);

/**
 * Callback for write data to media of IPv4 CSAP.
 *
 * The function complies with csap_write_cb_t prototype.
 */
extern te_errno tad_ip4_write_cb(csap_p csap, const tad_pkt *pkt);


/**
 * Callback for init 'ip4' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_ip4_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'ip4' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_ip4_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with IPv4 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_ip4_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback for confirm pattern PDU with IPv4 CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_ip4_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_ip4_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);


extern te_errno tad_ip4_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_ip4_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_ip4_match_do_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);


/**
 * Callback to release data prepared by confirm callback or packet match.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_ip4_release_pdu_cb(csap_p csap, unsigned int layer,
                                   void *opaque);


/*
 * ICMPv4 callbacks
 */

/**
 * Callback for init 'icmp4' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_icmp4_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'icmp4' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_icmp4_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with ICMPv4 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_icmp4_confirm_tmpl_cb(csap_p         csap,
                                          unsigned int   layer,
                                          asn_value     *layer_pdu,
                                          void         **p_opaque);

/**
 * Callback for confirm pattern PDU with ICMPv4 CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_icmp4_confirm_ptrn_cb(csap_p         csap,
                                          unsigned int   layer,
                                          asn_value     *layer_pdu,
                                          void         **p_opaque);


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_icmp4_gen_bin_cb(csap_p                csap,
                                     unsigned int          layer,
                                     const asn_value      *tmpl_pdu,
                                     void                 *opaque,
                                     const tad_tmpl_arg_t *args,
                                     size_t                arg_num,
                                     tad_pkts             *sdus,
                                     tad_pkts             *pdus);


extern te_errno tad_icmp4_match_pre_cb(csap_p              csap,
                                       unsigned int        layer,
                                       tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_icmp4_match_post_cb(csap_p              csap,
                                        unsigned int        layer,
                                        tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_icmp4_match_do_cb(csap_p           csap,
                                      unsigned int     layer,
                                      const asn_value *ptrn_pdu,
                                      void            *ptrn_opaque,
                                      tad_recv_pkt    *meta_pkt,
                                      tad_pkt         *pdu,
                                      tad_pkt         *sdu);


/**
 * Callback to release data prepared by confirm callback or packet match.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_icmp4_release_pdu_cb(csap_p csap, unsigned int layer,
                                     void *opaque);


/*
 * UDP callbacks
 */

/**
 * Callback for init 'udp' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_udp_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'udp' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_udp_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with UDP CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_udp_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback for confirm pattern PDU with UDP CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_udp_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_udp_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);


extern te_errno tad_udp_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_udp_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_udp_match_do_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);


/**
 * Callback to release data prepared by confirm callback or packet match.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_udp_release_pdu_cb(csap_p csap, unsigned int layer,
                                   void *opaque);



/*
 * TCP callbacks
 */

/**
 * Callback for init 'tcp' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_tcp_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'tcp' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_tcp_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for read parameter value of TCP CSAP.
 *
 * The function complies with csap_layer_get_param_cb_t prototype.
 */
extern char *tad_tcp_get_param_cb(csap_p csap, unsigned int layer,
                                  const char *param);

/**
 * Callback for confirm template TCP PDU with TCP CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_tcp_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *tmpl_pdu,
                                        void         **p_opaque);

/**
 * Callback for confirm pattern TCP PDU with TCP CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_tcp_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *tmpl_pdu,
                                        void         **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_tcp_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_tcp_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);

/**
 * Callback to release data prepared by confirm callback or packet match
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype
 */
extern void tad_tcp_release_opaque_cb(csap_p          csap,
                                      unsigned int    layer,
                                      void           *opaque);

/**
 * Callback for init 'ip6' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_ip6_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'ip6' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_ip6_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for confirm template PDU with IPv6 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_ip6_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_ip6_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);

/**
 * Callback to release data prepared by confirm callback or packet match.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_ip6_release_pdu_cb(csap_p csap, unsigned int layer,
                                   void *opaque);

/**
 * Callback for confirm pattern PDU with IPv6 CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_ip6_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

extern te_errno tad_ip6_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_ip6_match_do_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);

extern te_errno tad_ip6_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for init 'icmp6' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_icmp6_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'icmp6' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_icmp6_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback to release data prepared by confirm callback or packet match.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_icmp6_release_pdu_cb(csap_p csap, unsigned int layer,
                                     void *opaque);

/**
 * Callback for confirm template PDU with ICMPv6 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_icmp6_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                                          asn_value *layer_pdu,
                                          void **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_icmp6_gen_bin_cb(csap_p csap, unsigned int layer,
                                     const asn_value *tmpl_pdu,
                                     void *opaque,
                                     const tad_tmpl_arg_t *args,
                                     size_t arg_num,
                                     tad_pkts *sdus, tad_pkts *pdus);

/**
 * Callback for confirm pattern PDU with ICMPv6 CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_icmp6_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                                          asn_value *layer_pdu,
                                          void **p_opaque);

extern te_errno tad_icmp6_match_pre_cb(csap_p csap, unsigned int layer,
                                       tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_icmp6_match_post_cb(csap_p csap, unsigned int layer,
                                        tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_icmp6_match_do_cb(csap_p csap, unsigned int layer,
                                      const asn_value *ptrn_pdu,
                                      void *ptrn_opaque,
                                      tad_recv_pkt *meta_pkt, tad_pkt *pdu,
                                      tad_pkt *sdu);

/**
 * A tool to interpret a result of an advanced checksum matching
 *
 * @param   csap                CSAP instance
 * @param   cksum_str_code      The 'script' string code of 'checksum' DU
 * @param   cksum               The calculated checksum value
 * @param   layer               The layer number
 *
 * @return  Status code
 */
te_errno
tad_does_cksum_match(csap_p              csap,
                     tad_cksum_str_code  cksum_str_code,
                     uint16_t            cksum,
                     unsigned int        layer);

/**
 * An auxiliary tool to match L4 checksum (advanced mode)
 *
 * @param   csap                CSAP instance
 * @param   pdu                 L4 header
 * @param   meta_pkt            TAD meta packet
 * @param   layer               The layer number
 * @param   l4_proto            L4 protocol ID
 * @param   cksum_str_code      The 'script' string code of 'checksum' DU
 *
 * @return  Status code
 */
te_errno tad_l4_match_cksum_advanced(csap_p              csap,
                                     tad_pkt            *pdu,
                                     tad_recv_pkt       *meta_pkt,
                                     unsigned int        layer,
                                     uint8_t             l4_proto,
                                     tad_cksum_str_code  cksum_str_code);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_IPSTACK_IMPL_H__ */
