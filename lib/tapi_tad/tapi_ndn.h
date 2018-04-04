/** @file
 * @brief TAPI TAD NDN
 *
 * @defgroup tapi_ndn Network Data Notation (NDN)
 * @ingroup tapi_tad_main
 * @{
 *
 * Declarations of API for TAPI NDN.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
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
 * Split outer PDUs from innner PDUs (if any)
 *
 * @param pdus_orig  PDU sequence to process
 * @param pdus_o_out Location for outer PDUs
 * @param pdus_i_out Location for inner PDUs
 *
 * @return Status code
 */
extern te_errno tapi_tad_pdus_relist_outer_inner(asn_value  *pdus_orig,
                                                 asn_value **pdus_o_out,
                                                 asn_value **pdus_i_out);

/**
 * Make new PDU sequence instances for outer PDUs and for
 * inner PDUs (if any) and relist the corresponding PDUs
 *
 * @param tmpl       Traffic template
 * @param pdus_o_out Location for outer PDUs
 * @param pdus_i_out Location for inner PDUs
 */
extern te_errno tapi_tad_tmpl_relist_outer_inner_pdus(asn_value  *tmpl,
                                                      asn_value **pdus_o_out,
                                                      asn_value **pdus_i_out);

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAPI_NDN_H__ */

/**@} <!-- END tapi_ndn --> */
