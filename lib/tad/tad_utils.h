/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 *
 * Declarations of types and functions, used in common and 
 * protocol-specific modules implemnting TAD.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_UTILS_H__
#define __TE_TAD_UTILS_H__ 

#ifndef PACKAGE_VERSION
#include "config.h"
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h" 
#include "tad_common.h"
#include "rcf_ch_api.h"

#ifdef __cplusplus
extern "C" {
#endif





/* ============= Types and structures definitions =============== */



/**
 * Template argument iteration enums and structures
 */ 


/** 
 * Type of iteration argument
 */
typedef enum {
    TAD_TMPL_ARG_INT, /**< Integer */
    TAD_TMPL_ARG_STR, /**< Character string */
    TAD_TMPL_ARG_OCT, /**< Octet array */
} tad_tmpl_arg_type_t;

/**
 * Template argument value plain C presentation
 */
typedef struct tad_tmpl_arg_t { 
    tad_tmpl_arg_type_t type;  /**< Type of argument */

    size_t       length;       /**< Length of argument data */
    union {
        int      arg_int;      /**< Integer value */
        char    *arg_str;      /**< Pointer to character sting value */
        uint8_t *arg_oct;      /**< Pointer to octet array value */
    };
} tad_tmpl_arg_t;

/**
 * Type of template iterator.
 */
typedef enum {
    TAD_TMPL_ITER_INT_SEQ,   /**< Explicit sequence of int values */
    TAD_TMPL_ITER_STR_SEQ,   /**< Explicit sequence of string values */
    TAD_TMPL_ITER_FOR,       /**< Simple for - arithmetical progression */
} tad_tmpl_iter_type_t;   


/** Default value of begin of 'simple-for' iterator */
#define TAD_ARG_SIMPLE_FOR_BEGIN_DEF  1
/** Default value of step of 'simple-for' iterator */
#define TAD_ARG_SIMPLE_FOR_STEP_DEF   1

/**
 * Template iterator plain C structure
 */
typedef struct {
    tad_tmpl_iter_type_t type; /**< Iterator type */
    union {
        struct {
            size_t length;     /**< Length of sequence */
            int    last_index; /**< Index of last value */ 
            int  **ints;       /**< Array with sequence */
        } int_seq;             /**< Explicit integer sequence */
        struct {
            size_t length;     /**< Length of sequence */
            int    last_index; /**< Index of last value */ 
            char **strings;    /**< Array with sequence */
        } str_seq;             /**< Explicit string sequence */
        struct {
            int begin;         /**< Begin of progression */
            int end;           /**< End of progression */
            int step;          /**< Step of progression */
        } simple_for;          /**< Arithmetical progression */
    };
} tad_tmpl_iter_spec_t;


          
/** 
 * Type of payload specification in traffic template.
 */
typedef enum {
    TAD_PLD_UNKNOWN,  /**< Undefined, used when there is no pld spec. */
    TAD_PLD_BYTES,    /**< Byte sequence */
    TAD_PLD_LENGTH,   /**< Only length specified, bytes are random */
    TAD_PLD_SCRIPT,   /**< Special script for generation */
    TAD_PLD_FUNCTION, /**< Name of function, which generate payload */
} tad_payload_type;



/**
 * Type of node in arithmetical expression presentation tree 
 */
typedef enum {
    TAD_EXPR_CONSTANT = 0, /**< Constant value */
    TAD_EXPR_ARG_LINK,     /**< Link to some arg value */
    TAD_EXPR_ADD,          /**< Binary addition node */
    TAD_EXPR_SUBSTR,       /**< Binary substraction node */
    TAD_EXPR_MULT,         /**< Binary multiplication node */
    TAD_EXPR_DIV,          /**< Binary division node */
    TAD_EXPR_U_MINUS,      /**< Unary minus node */
} tad_expr_node_type;

/**
 * Type for expression presentation struct
 */
typedef struct tad_int_expr_t tad_int_expr_t;

/**
 * Struct for arithmetic (and boolean?) expressions in traffic operations
 * Expression is constructed with for arithmetical operations from 
 * constants and "variables", which are references to iterated artuments.
 */
struct tad_int_expr_t { 
    tad_expr_node_type  n_type; /**< node type */
    size_t              d_len;  /**< length of data: 
                                     - for node with operation is length
                                       of array with operands. 
                                     - for constant node is 'sizeof' 
                                       integer variable, may be 4 or 8.  */
    union {
        int32_t val_i32;        /**< int 32 value */
        int64_t val_i64;        /**< int 64 value */
        int     arg_num;        /**< number of referenced argument */
        tad_int_expr_t *exprs;  /**< array with operands */
    };
};

/**
 * Struct for octet or character string handling.
 */
typedef struct tad_du_data_t {
    size_t len;             /**< length */
    union {
        uint8_t *oct_str;   /**< pointer to octet string */
        char    *char_str;  /**< pointer to character string */
    };
} tad_du_data_t;

/**
 * Types of data unit, which may occure in traffic generaing template.
 */
typedef enum {
    TAD_DU_UNDEF,  /**< leaf is undefined, should be taken from default */
    TAD_DU_I32,    /**< uxplicit 32-bit integer value */
    TAD_DU_I64,    /**< uxplicit 64-bit integer value */
    TAD_DU_STRING, /**< character string */
    TAD_DU_OCTS,   /**< octet string */
    TAD_DU_EXPR,   /**< arithmetic expression */
} tad_du_type_t;

/**
 * Handler of message field data unit
 */
typedef struct {
    tad_du_type_t       du_type;        /**< Type of data unit */
    union {
        int32_t         val_i32;        /**< 32-bit integer */
        int64_t         val_i64;        /**< 64-bit integer */
        tad_du_data_t   val_data;       /**< character or octet string */
        tad_int_expr_t *val_int_expr;   /**< arithmetic expression */
    };
} tad_data_unit_t;





/* ============= Function prototypes declarations =============== */

/**
 * Prepare binary data by NDS.
 *
 * @param csap_descr    CSAP description structure
 * @param nds           ASN value with traffic-template NDS, should be
 *                      preprocessed (all iteration and function calls
 *                      performed)
 * @param handle        handle of RCF connection
 * @param args          array with template iteration parameter values,
 *                      may be used to prepare binary data, 
 *                      references to interation parameter values may
 *                      be set in ASN traffic template PDUs
 * @param arg_num       length of array above
 * @param pld_type      type of payload in nds, passed to make function
 *                      more fast
 * @param pld_data      payload data read from original NDS
 * @param pkts          packets with generated binary data (OUT)
 *
 * @return zero on success, otherwise error code.  
 */
extern int tad_tr_send_prepare_bin(csap_p csap_descr, asn_value *nds, 
                                   struct rcf_comm_connection *handle, 
                                   const tad_tmpl_arg_t *args, 
                                   size_t arg_num, 
                                   tad_payload_type pld_type, 
                                   const void *pld_data, 
                                   csap_pkts_p pkts);


/**
 * Confirm traffic template or pattern PDUS set with CSAP settings and 
 * protocol defaults. 
 * This function changes passed ASN value, user have to ensure that changes
 * will be set in traffic template or pattern ASN value which will be used 
 * in next operation. This may be done by such ways:
 *
 * Pass pointer got by asn_get_subvalue method, or write modified value 
 * into original NDS. 
 *
 * @param csap_descr    CSAP descriptor.
 * @param pdus          ASN value with SEQUENCE OF Generic-PDU (IN/OUT).
 *
 * @return zero on success, otherwise error code.
 */
extern int tad_confirm_pdus(csap_p csap_descr, asn_value *pdus);

/**
 * Transform payload symbolic type label of ASN choice to enum.
 *
 * @param label         Char string with ASN choice label.
 *
 * @return tad_payload_type enum value.
 */
extern tad_payload_type tad_payload_asn_label_to_enum(const char *label);



/**
 * Parse textual presentation of expression. Syntax is Perl-like, references
 * to template arguments noted as $1, $2, etc.
 *
 * @param string        Text with expression. 
 * @param expr          Place where pointer to new expression will 
 *                      be put (OUT).
 * @param syms          Quantity of parsed symbols, if syntax error occured,
 *                      error position (OUT).  
 *
 * @return status code.
 */
extern int tad_int_expr_parse(const char *string, 
                              tad_int_expr_t **expr, int *syms);

/**
 * Initialize tad_int_expr_t structure with single constant value, storing
 * binary array up to 8 bytes length. Array is assumed as "network byte 
 * order" and converted to "host byte order" while saveing in 64-bit 
 * integer.
 *
 * @param arr   pointer to binary array.
 * @param len   length of array.
 *
 * @return  pointer to new tad_int_expr_r structure, or NULL if no memory 
 *          or too beg length passed.
 */
extern tad_int_expr_t *tad_int_expr_constant_arr(uint8_t *arr, size_t len);

/**
 * Initialize tad_int_expr_t structure with single constant value.
 *
 * @param n     Value.
 *
 * @return pointer to new tad_int_expr_r structure.
 */
extern tad_int_expr_t *tad_int_expr_constant(int64_t n);

/**
 * Free data allocated for expression. 
 */
extern void tad_int_expr_free(tad_int_expr_t *expr);

/**
 * Calculate value of expression as function of argument set
 *
 * @param expr          Expression structure.
 * @param args          Array with arguments.
 * @param result        Location for result (OUT).
 *
 * @return status code.
 */
extern int tad_int_expr_calculate(const tad_int_expr_t *expr, 
                                  const tad_tmpl_arg_t *args,
                                  int64_t *result);

/**
 * Convert 64-bit integer from network order to the host and vise versa. 
 *
 * @param n     Integer to be converted.
 *
 * @return converted integer
 */
extern uint64_t tad_ntohll(uint64_t n);


/**
 * Init argument iteration array template arguments specification.
 * Memory block for array assumed to be allocated.
 *
 * @param arg_specs     Array of template argument specifications.
 * @param arg_specs_num Length of array.
 * @param arg_iterated  Array of template arguments (OUT).
 *
 * @retval      - positive on success itertaion.
 * @retval      - zero if iteration finished.
 * @retval      - negative if invalid arguments passed.
 */
extern int tad_init_tmpl_args(tad_tmpl_iter_spec_t *arg_specs,
                              size_t arg_specs_num, 
                              tad_tmpl_arg_t *arg_iterated);


/**
 * Perform next iteration for passed template arguments.
 *
 * @param arg_specs     Array of template argument specifications.
 * @param arg_specs_num Length of array.
 * @param arg_iterated  Array of template arguments (OUT).
 *
 * @retval      - positive on success itertaion.
 * @retval      - zero if iteration finished.
 * @retval      - negative if invalid arguments passed.
 */
extern int tad_iterate_tmpl_args(tad_tmpl_iter_spec_t *arg_specs,
                                 size_t arg_specs_num, 
                                 tad_tmpl_arg_t *arg_iterated);

/**
 * Get argument set from template ASN value and put it into plain-C array
 *
 * @param arg_set       ASN value of type "SEQENCE OF Template-Parameter",
 *                      which is subvalue with label 'arg-sets' in
 *                      Traffic-Template.
 * @param arg_specs     Memory block for arg_spec array. should 
 *                      be allocated by user. 
 * @param arg_num       Length of arg_spec array, i.e. quantity of
 *                      Template-Arguments in template.
 *
 * @return zero on success, otherwise error code. 
 */
extern int tad_get_tmpl_arg_specs(const asn_value *arg_set, 
                                  tad_tmpl_iter_spec_t *arg_specs,
                                  size_t arg_num);
 
/**
 * Convert DATA-UNIT ASN field of PDU to plain C structure.
 * Memory need to store dynamic data, is allocated in this method and
 * should be freed by user. 
 *
 * @param pdu_val       ASN value with pdu, which DATA-UNIT field 
 *                      should be converted.
 * @param label         Label of DATA_UNIT field in PDU.
 * @param location      Location for converted structure (OUT). 
 *
 * @return zero on success or error code. 
 */ 
extern int tad_data_unit_convert(const asn_value *pdu_val, 
                                 const char *label,
                                 tad_data_unit_t *location);

/**
 * Constract data-unit structure from specified binary data for 
 * simple per-byte compare. 
 *
 * @param data          Binary data which should be compared.
 * @param d_len         Length of data.
 * @param location      Location of data-unit structure to be 
 *                      initialized (OUT).
 *
 * @return error status.
 */
extern int tad_data_unit_from_bin(const uint8_t *data, size_t d_len, 
                                  tad_data_unit_t *location);

/**
 * Clear data_unit structure, e.g. free data allocated for internal usage. 
 * Memory block used by data_unit itself is not freed!
 * 
 * @param du    Pointer to data_unit structure to be cleared. 
 */
extern void tad_data_unit_clear(tad_data_unit_t *du);

/**
 * Generic method to match data in incoming packet with DATA_UNIT pattern
 * field. If data matches, it will be written into respective field in 
 * pkt_pdu. 
 * Label of field should be provided by user. If pattern has "plain"
 * type, data will be simply compared. If it is integer, data will be
 * converted from "network byte order" to "host byte order" before matching.
 *
 * @param pattern       Pattern structure. 
 * @param pkt_pdu       ASN value with parsed packet PDU (OUT). 
 * @param data          Binary data to be matched.
 * @param d_len         Length of data packet to be matched, in bytes. 
 * @param label         Textual label of desired field.
 *
 * @return zero on success (that means "data matches to the pattern"),
 *              otherwise error code. 
 *
 * This function is depricated, and leaved here only for easy backward 
 * rollbacks. Use 'ndn_match_data_units' instead. 
 * This function will be removed just when 'ndn_match_data_units' will 
 * be completed and debugged.
 */
extern int tad_univ_match_field(const tad_data_unit_t *pattern,
                                asn_value *pkt_pdu, uint8_t *data, 
                                size_t d_len, const char *label);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_UTILS_H__ */
