/** @file
 * @brief TAD Types
 *
 * Traffic Application Domain library types.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: tad_pkt.h 22602 2006-01-09 13:12:42Z arybchik $
 */

#ifndef __TE_LIB_TAD_TYPES_H__
#define __TE_LIB_TAD_TYPES_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_stdint.h"
#include "asn_usr.h"
#include "tad_common.h"


#undef assert
#define assert(x) \
    ( { do {                    \
        if (!(x))           \
        {                   \
            *(int *)0 = 0;  \
        }                   \
    } while (0); } )


/**
 * Constants for last unprocessed traffic command to the CSAP
 */
typedef enum {
    TAD_OP_IDLE,        /**< no traffic operation, waiting for command */
    TAD_OP_SEND,        /**< trsend_start */
    TAD_OP_SEND_RECV,   /**< trsend_recv */
    TAD_OP_RECV,        /**< trrecv_start */
    TAD_OP_GET,         /**< trrecv_get */
    TAD_OP_WAIT,        /**< trrecv_wait */
    TAD_OP_STOP,        /**< tr{send|recv}_stop */
    TAD_OP_DESTROY,     /**< csap_destroy */
    TAD_OP_SEND_DONE,   /**< internal command to nofity that send
                             processing has been finished */
    TAD_OP_RECV_DONE,   /**< internal command to nofity that receive
                             processing has been finished */
} tad_traffic_op_t;


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
    TAD_EXPR_MOD,          /**< Binary modulo */
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
    TAD_DU_UNDEF = 0,   /**< leaf is undefined */
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


/*
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
struct tad_tmpl_arg_t { 
    tad_tmpl_arg_type_t type;  /**< Type of argument */

    size_t       length;       /**< Length of argument data */
    union {
        int      arg_int;      /**< Integer value */
        char    *arg_str;      /**< Pointer to character sting value */
        uint8_t *arg_oct;      /**< Pointer to octet array value */
    };
};

/**
 * Type of template iterator.
 */
typedef enum {
    TAD_TMPL_ITER_INT_SEQ,   /**< explicit sequence of int values */
    TAD_TMPL_ITER_INT_ASSOC, /**< explicit sequence of int values, 
                                    not iterated separately, but associated
                                    with previour iterator */
    TAD_TMPL_ITER_STR_SEQ,   /**< explicit sequence of string values */
    TAD_TMPL_ITER_FOR,       /**< simple for - arithmetical progression */
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
            int    last_index; /**< Index of last value, -1 means none */ 
            int   *ints;       /**< Array with sequence */
        } int_seq;             /**< Explicit integer sequence */
        struct {
            size_t length;     /**< Length of sequence */
            int    last_index; /**< Index of last value, -1 means none */ 
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
    TAD_PLD_UNKNOWN,  /**< unknown type of payload */
    TAD_PLD_UNSPEC,   /**< undefined, used when there is no pld spec. */
    TAD_PLD_BYTES,    /**< byte sequence */
    TAD_PLD_LENGTH,   /**< only length specified, bytes are random */
    TAD_PLD_FUNCTION, /**< name of function, which generate payload */
    TAD_PLD_STREAM,   /**< parameters for data stream generating */
    TAD_PLD_MASK,     /**< pattern/mask specification */
} tad_payload_type;


/**
 * Type for reference to user function for generating data to be sent.
 *
 * @param csap_id       Identifier of CSAP
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl          ASN value with template. 
 *                      function should replace that field (which it should
 *                      generate) with #plain (in headers) or #bytes 
 *                      (in payload) choice (IN/OUT)
 *
 * @return zero on success or error code.
 */
typedef int (*tad_user_generate_method)(int csap_id, int layer,
                                        asn_value *tmpl);


/**
 * Preprocessed payload specification, ready for iteration 
 * and binary generating. 
 */
typedef struct {
    tad_payload_type type;      /**< Type of payload spec */

    union {
        struct {
            size_t   length;    /**< Payload length */
            uint8_t *data;      /**< Byte array */
        } plain;                

        struct {
            size_t   length;    /**< Value/mask length */
            uint8_t *value;     /**< Expected value when mask applied */
            uint8_t *mask;      /**< The mask */
            te_bool  exact_len; /**< Whether length of matched data
                                     should be exactly the same? */
        } mask;

        struct {
            tad_data_unit_t     offset;
            tad_data_unit_t     length;
            tad_stream_callback func;
        } stream; 

        tad_user_generate_method func;
    };
} tad_payload_spec_t;


/**
 * Pointer type to CSAP instance
 */
typedef struct csap_instance *csap_p;


#endif /* !__TE_LIB_TAD_TYPES_H__ */
