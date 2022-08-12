/** @file
 * @brief TAPI TAD NDN
 *
 * @defgroup tapi_ndn Network Data Notation (NDN)
 * @ingroup tapi_tad_main
 * @{
 *
 * Declarations of API for TAPI NDN.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAPI_NDN_H__
#define __TE_TAPI_NDN_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"
#include "te_kvpair.h"
#include "tapi_test.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get ndn_traffic_template ASN.1 type test parameter.
 *
 * @param _var_name  Variable whose name is the same as the name of
 *                   parameter we get the value
 */
#define TEST_GET_NDN_TRAFFIC_TEMPLATE(_var_name) \
    do {                                                        \
        const char *_str_val;                                   \
        int _parsed;                                            \
                                                                \
        _str_val = test_get_param(argc, argv, #_var_name);      \
        if (_str_val == NULL)                                   \
            TEST_STOP;                                          \
                                                                \
        CHECK_RC(asn_parse_value_text(_str_val,                 \
                                      ndn_traffic_template,     \
                                      &(_var_name),             \
                                      &_parsed));               \
        if (_str_val[_parsed] != '\0')                          \
            TEST_FAIL("Trailing symbols after traffic "         \
                      "template '%s'", &_str_val[_parsed]);     \
    } while (0)

/** Flags used to designate transformations which take place in hardware */
#define SEND_COND_HW_OFFL_IP_CKSUM       (1U << 0)
#define SEND_COND_HW_OFFL_OUTER_IP_CKSUM (1U << 1)
#define SEND_COND_HW_OFFL_L4_CKSUM       (1U << 2)
#define SEND_COND_HW_OFFL_TSO            (1U << 3)
#define SEND_COND_HW_OFFL_VLAN           (1U << 4)

typedef struct send_transform {
    unsigned int                    hw_flags;

    uint16_t                        tso_segsz;
    uint16_t                        vlan_tci;
} send_transform;

#define RX_XFRM_HW_OFFL_VLAN_STRIP (1U << 0)
#define RX_XFRM_HW_OFFL_QINQ_STRIP (1U << 1)

#define RX_XFRM_EFFECT_VLAN_TCI       (1U << 0)
#define RX_XFRM_EFFECT_OUTER_VLAN_TCI (1U << 1)

typedef struct receive_transform {
    unsigned int                    hw_flags;
    unsigned int                    effects;

    /* host byte order */
    uint16_t                        vlan_tci;
    uint16_t                        outer_vlan_tci;
} receive_transform;


/** Segmentation fine-tuning configuration */
struct tapi_ndn_gso_conf {
        size_t payload_barrier; /**< Barrier inside super-frame payload
                                     (i.e. without any headers) at which segment
                                     should end and next full segment should
                                     start. */
};

/**
 * Check ASN.1 value pointer. If it is NULL, initialize a new value of
 * specified type. All errors are logged inside the function. There is
 * no necessity to log them after call.
 *
 * @param value     Location of ASN.1 value pointer
 * @param type      ASN.1 type to which value should belong
 *
 * @return Status code.
 *
 * @todo Check that ASN.1 value belongs to @a type
 */
extern te_errno tapi_tad_init_asn_value(asn_value      **value,
                                        const asn_type  *type);

/**
 * Add a new CSAP specification layer.
 *
 * @param csap_spec     Location of ASN.1 value with CSAP specification
 *                      (if NULL pointer is stored in location, a new
 *                      CSAP specification is initialized)
 * @param layer_type    ASN.1 type of a new layer
 * @param layer_choice  String name of a new layer as ASN.1 choice
 *                      (including '#', e.g. "#eth")
 * @param layer_spec    Location for a new ASN.1 value with layer
 *                      specification (may be NULL)
 *
 * @return Status code.
 */
extern te_errno tapi_tad_csap_add_layer(asn_value       **csap_spec,
                                        const asn_type   *layer_type,
                                        const char       *layer_choice,
                                        asn_value       **layer_spec);

/**
 * Add a new layer specification in traffic template/pattern.
 *
 * @param obj_spec      Location of ASN.1 value with Template of pattern spec
 *                      (if NULL pointer is stored in location, a new
 *                      CSAP specification is initialized)
 * @param is_pattern    Flag wheather required NDN is traffic pattern
 * @param pdu_type      ASN.1 type of a new PDU
 * @param pdu_choice    String name of a new PDU as ASN.1 choice
 *                      (including '#', e.g. "#eth")
 * @param pdu_spec      Location for a new ASN.1 value with PDU
 *                      specification (may be NULL)
 *
 * @return Status code.
 */
extern te_errno tapi_tad_tmpl_ptrn_add_layer(asn_value       **obj_spec,
                                             te_bool           is_pattern,
                                             const asn_type   *pdu_type,
                                             const char       *pdu_choice,
                                             asn_value       **pdu_spec);

/**
 * Add a new unit in the traffic pattern specification.
 *
 * @param obj_spec      Location of ASN.1 value with Template of pattern spec
 *                      (if NULL pointer is stored in location, a new
 *                      CSAP specification is initialized)
 * @param unit_spec     Location for a pointer to a new pattern unit or
 *                      NULL
 *
 * @return Status code.
 */
extern te_errno tapi_tad_new_ptrn_unit(asn_value **obj_spec,
                                       asn_value **unit_spec);

/**
 * Set payload of the last unit in the traffic template or pattern
 * specification.
 *
 * @param obj_spec      Location of ASN.1 value with Template of Pattern
 *                      specification (if NULL pointer is stored in
 *                      location, a new one is initialized)
 * @param is_pattern    Flag whether required NDN is traffic pattern or
 *                      template
 * @param payload       Pointer to payload data
 * @param length        Payload length
 *
 * @note If @a payload is @c NULL and @a length is not @c 0, random
 *       payload contents is generated on sending and any payload of
 *       specified length is matched.
 *
 * @return Status code.
 */
extern te_errno tapi_tad_tmpl_ptrn_set_payload_plain(
                    asn_value  **obj_spec,
                    te_bool      is_pattern,
                    const void  *payload,
                    size_t       length);

/**
 * Free all the PDU fields of a choice denoted by a given DU tag
 * (eg. NDN_DU_SCRIPT) from all the PDUs in a given PDU sequence
 *
 * @param pdus          ASN.1 value contining a PDU sequence
 * @param du_tag        DU tag value to designate a DU choice
 *
 * @return Status code
 */
extern te_errno tapi_pdus_free_fields_by_du_tag(asn_value      *pdus,
                                                asn_tag_value   du_tag);

/**
 * Find outer and inner PDUs in a given PDU sequence ASN.1 value
 * and provide them back to the caller in the form of two arrays.
 * This function does not copy the PDU ASN.1 values it discovers.
 *
 * @param pdu_seq       PDU sequence ASN.1 value to process
 * @param nb_pdus_o_out Location for the number of outer PDU ASN.1 values
 * @param pdus_o_out    Location for outer PDU ASN.1 values
 * @param nb_pdus_i_out Location for the number of inner PDU ASN.1 values
 * @param pdus_i_out    Location for inner PDU ASN.1 values
 *
 * @return Status code
 */
extern te_errno tapi_tad_pdus_relist_outer_inner(asn_value      *pdu_seq,
                                                 unsigned int   *nb_pdus_o_out,
                                                 asn_value    ***pdus_o_out,
                                                 unsigned int   *nb_pdus_i_out,
                                                 asn_value    ***pdus_i_out);

/**
 * Convenient @c tapi_tad_pdus_relist_outer_inner() wrapper
 * which assumes template ASN.1 value as its first argument.
 *
 * @param tmpl          Traffic template
 * @param nb_pdus_o_out Location for the number of outer PDU ASN.1 values
 * @param pdus_o_out    Location for outer PDU ASN.1 values
 * @param nb_pdus_i_out Location for the number of inner PDU ASN.1 values
 * @param pdus_i_out    Location for inner PDU ASN.1 values
 */
extern te_errno tapi_tad_tmpl_relist_outer_inner_pdus(asn_value      *tmpl,
                                                      unsigned int   *nb_pdus_o_out,
                                                      asn_value    ***pdus_o_out,
                                                      unsigned int   *nb_pdus_i_out,
                                                      asn_value    ***pdus_i_out);

/**
 * Convert an ASN.1 template to a pattern containing
 * the same set of PDUs as ones in the template
 *
 * @param template      ASN.1 template to be converted into a pattern
 *
 * @return ASN.1 value containing a pattern or @c NULL
 */
extern asn_value *tapi_tad_mk_pattern_from_template(asn_value *template);

/**
 * Convert an array of ASN.1 'ndn_raw_packet'-s to a unified
 * ASN.1 'ndn_traffic_pattern' carrying an exact sequence of
 * 'ndn_traffic_pattern_unit'-s applicable to match definite
 * packets which are to be received by the peer side in case
 * if the initial 'ndn_raw_packet'-s are sent and (possibly)
 * undergo some transformations (eg, HW offloads are active)
 *
 * @deprecated This API is not well-thought, and the
 *             implementation is mind-boggling.
 *
 *             Please consider using better, granular helpers
 *             to do varios editing, namely:
 *
 *             - @c tapi_ndn_pkt_demand_correct_ip_cksum()
 *             - @c tapi_ndn_pkt_demand_correct_udp_cksum()
 *             - @c tapi_ndn_pkt_demand_correct_tcp_cksum()
 *             - @c tapi_ndn_superframe_gso()
 *             - @c tapi_ndn_tso_pkts_edit()
 *             - @c tapi_ndn_gso_pkts_ip_len_edit()
 *             - @c tapi_ndn_gso_pkts_ip_id_edit()
 *             - @c tapi_ndn_gso_pkts_udp_len_edit() .
 *
 *             Please use @c tapi_ndn_pkts_to_ptrn()
 *             for the actual conversion.
 *
 *             Consider removing this API and all connected helpers.
 *
 * @note The given set of possible transformations is only
 *       considered with respect to all 'ndn_raw_packet'-s
 *       in the array, i.e., some individual peculiarities
 *       of the items cannot be taken into account, hence,
 *       if one needs to process any of individual packets
 *       independently, separate calls should be performed
 *       (eg, if SEND_COND_HW_OFFL_TSO flag is present, it
 *       means that TSO shall be done for all the packets)
 *
 * @param packets      An ASN.1 'ndn_raw_packet'-s to be processed
 * @param n_packets    The number of items available within @p packets
 * @param transform    A set of parameters describing some trasformations
 *                     which are expected to affect the outgoing packets
 * @param pattern_out  Location for the pattern which is to be produced
 *
 * @return Status code
 */
extern te_errno tapi_tad_packets_to_pattern(asn_value         **packets,
                                            unsigned int        n_packets,
                                            send_transform     *transform,
                                            asn_value         **pattern_out);

/**
 * Concatenate two traffic patterns
 *
 * @param dst           The first pattern to which @p src is to be appended
 * @param src           The second pattern which is to be appended to @p dst
 *
 * @note @p src will be freed internally after successful concatenation only
 *
 * @return Status code
 */
extern te_errno tapi_tad_concat_patterns(asn_value  *dst,
                                         asn_value  *src);

/**
 * Aggregate the copies of pattern units from all the patterns within a given
 * array to make a single pattern suitable for matching heterogeneous packets
 *
 * @param patterns      An array containing initial patterns to be aggregated
 * @param nb_patterns   The number of patterns available in @p patterns array
 * @param pattern_out   Location for the new pattern which is to be produced
 *
 * @note The function doesn't change or free the initial patterns;
 *       @c tapi_tad_concat_patterns() is fed by copies internally
 *
 * @return Status code
 */
extern te_errno tapi_tad_aggregate_patterns(asn_value     **patterns,
                                            unsigned int    nb_patterns,
                                            asn_value     **pattern_out);


/* Forward to avoid inclusiong of tapi_env.h which may be missing */
struct tapi_env;

/**
 * The function iterates through ASN.1 value,
 * find all data units with "#env" value choice
 * and substitute it in accordance with @p params
 * and @p env
 *
 * @param value     ASN value, which content should be substituted
 * @param params    Key-value pairs with test parameters or @c NULL
 * @param env       Environment or @c NULL
 *
 * @return Status code
 */
extern te_errno tapi_ndn_subst_env(asn_value *value, te_kvpair_h *params,
                                   struct tapi_env *env);

typedef enum tapi_ndn_level_e {
    TAPI_NDN_OUTER_L3 = 0,
    TAPI_NDN_OUTER_L4,
    TAPI_NDN_TUNNEL,
    TAPI_NDN_INNER_L3,
    TAPI_NDN_INNER_L4,
    TAPI_NDN_NLEVELS,
} tapi_ndn_level_t;

/**
 * Given a traffic template, inspect its PDUs and fill
 * out the output array of header types to provide the
 * caller with details about protocols in use.
 *
 * @note The API will indicate missing PDUs by @c TE_PROTO_INVALID .
 *
 * @param tmpl The traffic template
 * @param hdrs Array of size @c TAPI_NDN_NLEVELS to store the results in
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_tmpl_classify(
                            const asn_value    *tmpl,
                            te_tad_protocols_t  hdrs[TAPI_NDN_NLEVELS]);

/**
 * Given a traffic template, set its IPv4 checksum plain value.
 *
 * @param tmpl  The traffic template
 * @param cksum The checksum value to be set (host byte order)
 * @param level @c TAPI_NDN_OUTER_L3 or @c TAPI_NDN_INNER_L3
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_tmpl_set_ip_cksum(asn_value        *tmpl,
                                           uint16_t          cksum,
                                           tapi_ndn_level_t  level);

/**
 * Given a traffic template, set its UDP checksum plain value.
 *
 * @param tmpl  The traffic template
 * @param cksum The checksum value to be set (host byte order)
 * @param level @c TAPI_NDN_OUTER_L4 or @c TAPI_NDN_INNER_L4
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_tmpl_set_udp_cksum(asn_value        *tmpl,
                                            uint16_t          cksum,
                                            tapi_ndn_level_t  level);

/**
 * Given a traffic template, set its TCP checksum plain value.
 *
 * @param tmpl  The traffic template
 * @param cksum The checksum value to be set (host byte order)
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_tmpl_set_tcp_cksum(asn_value *tmpl,
                                            uint16_t   cksum);

/**
 * Given a traffic template, set its TCP flags plain value.
 *
 * @param tmpl  The traffic template
 * @param flags The flags value to be set
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_tmpl_set_tcp_flags(asn_value *tmpl,
                                            uint8_t    flags);

/**
 * Given a traffic template, set its payload length.
 * This function doesn't set the payload data since
 * random payload will be provided later by Tx CSAP.
 *
 * @param tmpl        The traffic template
 * @param payload_len Payload length value
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_tmpl_set_payload_len(asn_value    *tmpl,
                                              unsigned int  payload_len);

/**
 * Given an ASN.1 raw packet and VLAN TCI, inject a VLAN tag to
 * the outer Ethernet PDU thus simulating a VLAN tag HW offload.
 *
 * @param pkt      The ASN.1 raw packet to be edited
 * @param vlan_tci VLAN TCI (host byte order) to be injected
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_pkt_inject_vlan_tag(asn_value *pkt,
                                             uint16_t   vlan_tci);

/**
 * Inject VLAN tags to outer Ethernet PDU in a PDU sequence.
 *
 * @param[in,out] pdus          PDU sequence
 * @param[in]     vid           VLAN IDs
 * @param[in]     prio          VLAN priority fields
 * @param[in]     cfi           VLAN CFI fields
 * @param[in]     n_tags        Number of tags to be pushed, specifies
 *                              size of @p vid, @p prio, @p cfi
 *
 * @note    The API works only with exactly specified or not set
 *          vlan headers. Ranged fields are not supported.
 *
 * @note    @c UINT16_MAX for @p vid, @p prio or @p cfi means that this value
 *          is not specified in the PDU
 *
 * @return Status code
 */
extern te_errno tapi_ndn_pdus_inject_vlan_tags(asn_value *pdus,
                                               const uint16_t *vid,
                                               const uint16_t *prio,
                                               const uint16_t *cfi,
                                               size_t n_tags);

/**
 * Remove VLAN tags from outer Ethernet PDU in a PDU sequence.
 *
 * @param[in,out] pdus          PDU sequence
 * @param[in]     n_tags        Number of tags to remove
 *
 * @note    The API works only with exactly specified or not set
 *          VLAN headers. Ranged fields are not supported.
 *
 * @return Status code
 */
extern te_errno tapi_ndn_pdus_remove_vlan_tags(asn_value *pdus,
                                               size_t n_tags);

/**
 * Read TCI value of VLAN tags of a Ethernet PDU.
 *
 * @param[in]     eth           ASN.1 value of ethernet PDU
 * @param[in,out] n_tags        Size of the @p vid, @p prio and @p cfi arrays,
 *                              on success contains the number of tags that
 *                              were read (may be @c 0 if @p eth does not have
 *                              any VLAN tags)
 * @param[out]    vid           VLAN IDs
 * @param[out]    prio          VLAN priority fields
 * @param[out]    cfi           VLAN CFI fields
 *
 * @note    The API works only with exactly specified or not set
 *          vlan headers. Ranged fields are not supported.
 *
 * @note    @c UINT16_MAX for @p vid, @p prio or @p cfi means that this value
 *          is not specified in the PDU
 *
 * @return Status code
 */
extern te_errno tapi_ndn_eth_read_vlan_tci(const asn_value *eth,
                                           size_t *n_tags, uint16_t *vid,
                                           uint16_t *prio, uint16_t *cfi);
/**
 * Take an ASN.1 raw packet which is going to be transformed to
 * a pattern and override IPv4 checksum field to require correct
 * checksum in the packet which is about to be received on peer.
 *
 * @param pkt   The ASN.1 raw packet to be edited
 * @param level @c TAPI_NDN_OUTER_L3 or @c TAPI_NDN_INNER_L3
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_pkt_demand_correct_ip_cksum(asn_value        *pkt,
                                                     tapi_ndn_level_t  level);

/**
 * Take an ASN.1 raw packet which is going to be transformed to
 * a pattern and override UDP checksum field to require correct
 * checksum in the packet which is about to be received on peer.
 *
 * @param pkt         The ASN.1 raw packet to be edited
 * @param can_be_zero If @c TRUE, allow for zero checksum value
 * @param level       @c TAPI_NDN_OUTER_L4 or @c TAPI_NDN_INNER_L4
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_pkt_demand_correct_udp_cksum(
                                                asn_value        *pkt,
                                                te_bool           can_be_zero,
                                                tapi_ndn_level_t  level);

/**
 * Take an ASN.1 raw packet which is going to be transformed to
 * a pattern and override TCP checksum field to require correct
 * checksum in the packet which is about to be received on peer.
 *
 * @param pkt The ASN.1 raw packet to be edited
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_pkt_demand_correct_tcp_cksum(asn_value *pkt);

/**
 * Take an ASN.1 raw packet containing a superframe and conduct
 * GSO slicing to produce a burst of ASN.1 raw packets each one
 * containing unchanged PDUs from the original superframe and a
 * properly sized chunk of payload read out from the same frame.
 *
 * @param superframe      The superframe to undergo GSO slicing
 * @param seg_payload_len The desired length of a payload chunk
 * @param gso_conf        Segmentation fine-tuning configuration
 * @param pkts_out        Location for the resulting packets
 * @param nb_pkts_out     Location for the number of the packets
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_superframe_gso(
                            asn_value                         *superframe,
                            size_t                             seg_payload_len,
                            const struct tapi_ndn_gso_conf    *gso_conf,
                            asn_value                       ***pkts_out,
                            unsigned int                      *nb_pkts_out);

/**
 * Given a bunch of ASN.1 raw packets originating from some GSO
 * transaction, conduct a very minimal TSO edit across TCP PDUs.
 * Line up TCP sequence numbers as per GSO segment payload size
 * and amend TCP flags so that all the packets but the last one
 * have FIN and PSH bits cleared whilst CWR bit (if present) is
 * retained intact solely in the very first packet of the array.
 *
 * @param pkts    The array of ASN.1 raw packets
 * @param nb_pkts The number of ASN.1 raw packets
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_tso_pkts_edit(asn_value    **pkts,
                                       unsigned int   nb_pkts);

/**
 * Given a bunch of ASN.1 raw packets originating from some GSO
 * transaction, fix IP inner / outer length field in each frame.
 *
 * @param pkts        The array of ASN.1 raw packets
 * @param nb_pkts     The number of ASN.1 raw packets
 * @param ip_te_proto @c TE_PROTO_IP4 or @c TE_PROTO_IP6
 * @param level       @c TAPI_NDN_OUTER_L3 or @c TAPI_NDN_INNER_L3
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_gso_pkts_ip_len_edit(asn_value          **pkts,
                                              unsigned int         nb_pkts,
                                              te_tad_protocols_t   ip_te_proto,
                                              tapi_ndn_level_t     level);

/**
 * Given a bunch of ASN.1 raw packets originating from some GSO
 * transaction, line up IPv4 ID field values across the packets.
 *
 * @param pkts    The array of ASN.1 raw packets
 * @param nb_pkts The number of ASN.1 raw packets
 * @param level   @c TAPI_NDN_OUTER_L3 or @c TAPI_NDN_INNER_L3
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_gso_pkts_ip_id_edit(asn_value        **pkts,
                                             unsigned int       nb_pkts,
                                             tapi_ndn_level_t   level);

/**
 * Given a bunch of ASN.1 raw packets originating from some GSO
 * transaction, fix UDP inner / outer length field in each frame.
 *
 * @param pkts    The array of ASN.1 raw packets
 * @param nb_pkts The number of ASN.1 raw packets
 * @param level   @c TAPI_NDN_OUTER_L4 or @c TAPI_NDN_INNER_L4
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_gso_pkts_udp_len_edit(asn_value        **pkts,
                                               unsigned int       nb_pkts,
                                               tapi_ndn_level_t   level);

/**
 * Convert ASN.1 raw packets to a multi-unit traffic pattern.
 *
 * @param pkts     The array of ASN.1 raw packets
 * @param nb_pkts  The number of ASN.1 raw packets
 * @param ptrn_out Location for the resulting pattern
 *
 * @return Status code.
 */
extern te_errno tapi_ndn_pkts_to_ptrn(asn_value    **pkts,
                                      unsigned int   nb_pkts,
                                      asn_value    **ptrn_out);

/**
 * Transform a pattern of coming packets in accordance with receive effects
 *
 * @param rx_transform    A set of parameters describing some trasformations
 *                        which are expected to affect the packets on receive
 * @param ptrn            Location of a pattern to be transformed
 *
 * @return Status code.
 */
extern te_errno tapi_eth_transform_ptrn_on_rx(receive_transform *rx_transform,
                                              asn_value **ptrn);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_NDN_H__ */

/**@} <!-- END tapi_ndn --> */
