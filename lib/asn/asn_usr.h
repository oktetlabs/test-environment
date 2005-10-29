/** @file
 * @brief ASN.1 library user interface
 *
 * Declarations of user API for processing ASN.1 values. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 
#ifndef __TE_ASN_USR_H__
#define __TE_ASN_USR_H__

#include <sys/types.h>

#include "te_stdint.h"
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


/** 
 * Type: ASN_type
 * Only pointer to this structure is defined here, structure is only declared. 
 * 
 * Instances of this type shouldn't be dynamically created, they may be only
 * statically defined in special modules. Such modules may be either written 
 * manually or generated by a special tool.
 *
 */
struct asn_type;
typedef struct asn_type asn_type;
typedef struct asn_type *asn_type_p;

/** 
 * Type: asn_value
 */ 
struct asn_value;
typedef struct asn_value asn_value;
typedef struct asn_value *asn_value_p;

/**
 * Enumerated type with ASN syntax codes. All syntax codes are devided into
 * the following groups:
 *  - primitive syntacies, internal presentaion of which does
 *      not require memory allocation. 
 *  - primitive syntacies, internal presentaion of which 
 *      requires memory allocation, because number of octets occupied 
 *      depends on the value.  
 *  - constractive syntacies.
 *      Codes of types, which specification contain array of namedValues 
 *      (with types), have lower bit zero. 
 */
typedef enum { 
    SYNTAX_UNDEFINED = 0,  /**< Undefined syntax, used as error mark. */
    BOOL = 1,       /**< Boolean syntax */
    INTEGER,        /**< Integer syntax */
    PR_ASN_NULL,    /**< ASN_NULL syntax */
    ENUMERATED,     /**< Enum syntax */


    PRIMITIVE_VAR_LEN = 0x10, /**< value to be bit-anded with tag to determine
                                  primitive syntax with variable length */
    LONG_INT, /**< this syntax differs from "long int" in C! length of 
                   its data in octets is specified by asn_type field 'size'. */
    BIT_STRING,
    OCT_STRING,
    CHAR_STRING,
    REAL,
    OID, 
    
    CONSTRAINT = 0x20, /**< flag of constraint syntax */

    SEQUENCE = CONSTRAINT, 
    SEQUENCE_OF,
    SET,
    SET_OF,
    CHOICE,
    TAGGED
} asn_syntax;


/**
 * Enumerated type with ASN tag class codes. 
 */
typedef enum {
    UNIVERSAL,
    APPLICATION,
    CONTEXT_SPECIFIC,
    PRIVATE
} asn_tag_class;



typedef struct asn_tag_t {
    asn_tag_class  cl;
    unsigned short val;
} asn_tag_t;



/**
 * Init empty ASN value of specified type.
 *
 * @param type       ASN type to which value should belong
 *
 * @return pointer to new asn_value instance or NULL if error occurred. 
 */
extern asn_value_p asn_init_value(const asn_type *type);

/**
 * Init empty ASN value of specified type with a certain ASN tag.
 *
 * @param type       ASN type to which value should belong
 * @param tc         ASN tag class, see enum definition and ASN.1 standard
 * @param tag        ASN tag value, may be an arbitrary non-negative integer
 *
 * @return pointer to new asn_value instance or NULL if error occurred. 
 */
extern asn_value_p asn_init_value_tagged(const asn_type *type, 
                                         asn_tag_class tc, uint16_t tag);

/**
 * Make a copy of ASN value instance.
 *
 * @param value       ASN value to be copied
 *
 * @return pointer to new asn_value instance or NULL if error occurred. 
 */
extern asn_value *asn_copy_value(const asn_value *value);

/**
 * Free memory allocalted by ASN value instance.
 *
 * @param value       ASN value to be destroyed
 *
 * @return nothing
 */
extern void asn_free_value(asn_value_p value);

/**
 * Free subvalue of constraint ASN value instance.
 *
 * @param container   ASN value which subvalue should be destroyed
 * @param labels      string with dot-separated sequence of textual field
 *                    labels, specifying subvalue in ASN value tree with 
 *                    'container' as a root. Label for 'SEQUENCE OF' and 
 *                    'SET OF' subvalues is decimal notation of its integer 
 *                    index in array. Choice labels shouls be prepended by
 *                    symbol '#'
 *
 * @return zero on success, otherwise error code.
 */
extern int asn_free_subvalue(asn_value_p value, const char *labels);

/**
 * Free one-level subvalue of constraint ASN value instance by tag.
 * For CHOICE syntax value tag is ignored.
 *
 * @param value       ASN value which subvalue should be destroyed
 * @param tag_class   class of ASN tag
 * @param tag_val     value of ASN tag
 *
 * @return zero on success, otherwise error code.
 */
extern int asn_free_child_value(asn_value_p value, 
                               asn_tag_class tag_class, uint16_t tag_val);


/**
 * Obtain ASN type to which specified value belongs. 
 *
 * @param value       ASN value which type is interested
 *
 * @return pointer to asn_type instance or NULL if error occurred. 
 */
extern const asn_type *asn_get_type(const asn_value *value);

/**
 * Obtain textual label of ASN type. 
 *
 * @param type       ASN type which name is interested
 *
 * @return plain string with type name or NULL if error occurred. 
 */
extern const char *asn_get_type_name(const asn_type *type);



/**
 * Parse textual presentation of single ASN.1 value of specified type and
 * create a new instance of asn_value type with its internal presentation.
 * @note Text should correspoind to the "Value" production label in ASN.1 
 * specification.
 *
 * @param string        text to be parsed
 * @param type          expected type of value
 * @param parsed_val    parsed value (OUT)
 * @param syms_parsed   number of parsed symbols (OUT)
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_parse_value_text(const char *string, const asn_type *type, 
                                asn_value_p *parsed_val, int *parsed_syms); 

/**
 * Parse ASN.1 text with "Value assignment" (see ASN.1 specification) 
 * and create a new ASN value instance with internal presentation of this value.
 * If type of ASN value in 'string' is not known for module, string will not 
 * parsed and NULL will be returned. 
 * Name of value is stored in field 'name' of asn_value structure. 
 *
 * This function is not implemented. 
 *
 * @param string        text to be parsed
 * @param value         location for pointer to new parsed value
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_parse_value_assign_text(const char *string, 
                                               asn_value **value);

/**
 * Read ASN.1 text file, parse value assignments in it while they refer to 
 * ASN types which are known to the module, and add all parsed values 
 * to internal Value hash. 
 * Name of value is stored in field 'name' of asn_value structure. 
 *
 * This function is not implemented. 
 *
 * @param filename      name of file to be parsed
 * @param found_names   names of parsed ASN values (OUT)
 * @param found_len     length of array found_names[] (IN/OUT)
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_parse_file(const char *filename, char **found_names, int *found_len); 

/**
 * Read ASN.1 text file, parse DefinedValue of specified ASN type
 *
 * @param filename      name of file to be parsed
 * @param type          expected type of value
 * @param parsed_value  parsed value (OUT)
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT)
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_parse_dvalue_in_file(const char *filename, const asn_type *type, 
                                asn_value_p *parsed_value, int *syms_parsed); 

/**
 * Prepare textual ASN.1 presentation of passed value and put it into specified
 * buffer. 
 *
 * @param value         ASN value to be printed
 * @param buffer        buffer for ASN text
 * @param buf_len       length of buffer
 * @param indent        current indent, usually zero
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
extern int asn_sprint_value(const asn_value *value, char *buffer, size_t buf_len, 
                            unsigned int indent);

/**
 * Prepare textual ASN.1 presentation of passed value and save this string to
 * file with specified name. If file already exists, it will be overwritten.
 *
 * @param value         ASN value to be stored 
 * @param filename      name of the file 
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_save_to_file(const asn_value *value, const char *filename);


/**
 * BER encoding of passed ASN value.
 *
 * @param buf           pointer to buffer to be filled by coded data
 * @param buf_len       length of accessible buffer, function puts here 
 *                      length of encoded data (IN/OUT)
 * @param value         asn value to be encoded
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_encode(void *buf, size_t *buf_len, asn_value_p value); 

/**
 * BER decoding of passed data.
 *
 * @param data          pointer to data to be decoded
 *
 * @return pointer to new asn_value instance or NULL if error occurred. 
 */ 
extern asn_value_p asn_decode(const void *data);



/**
 * Put ASN value to the named one-depth leaf to the ASN value.
 * Free old leaf subvalue, if there was one. 
 * Subvalue is not copied, but inserted into ASN tree of 'container' as is.
 *
 * @param container     ASN value which child should be updated, 
 *                      have to be of syntax SEQUENCE, SET, or CHOICE
 * @param subvalue      new ASN value for child
 * @param tag_class     class of ASN tag
 * @param tag_val       value of ASN tag
 *
 * @return zero on success, otherwise error code.
 */
extern int asn_put_child_value(asn_value *container, asn_value *subvalue, 
                               asn_tag_class tag_class, uint16_t tag_val);

/**
 * The same as 'asn_put_child_value', but take as child specificator
 * its character label instead of tag. 
 *
 * @param container     ASN value which child should be updated, 
 *                      have to be of syntax SEQUENCE, SET, or CHOICE
 * @param subvalue      new ASN value for child
 * @param label         character label of child
 *
 * @return zero on success, otherwise error code.
 */
extern int asn_put_child_value_by_label(asn_value *container,
                                        asn_value *subvalue, 
                                        const char *label);

/**
 * Write data into primitive syntax leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param data          data to be written, should be in nature C format for
 *                      data type respective to leaf syntax
 * @param d_len         length of the data
 *                      Measured in octets for all types except OID and
 *                      BIT_STRING; for OID measured in sizeof(int)
 *                      for BIT_STRING measured in bits 
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_write_value_field(asn_value_p container, 
                                 const void *data, size_t d_len, 
                                 const char *labels);

/**
 * Read data from primitive syntax leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param data          pointer to buffer for read data (OUT)
 * @param d_len         length of available buffer / read data (IN/OUT).
 *                      Measured in octets for all types except OID and
 *                      BIT_STRING; for OID measured in sizeof(int),
 *                      for BIT_STRING measured in bits 
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_read_value_field(const asn_value *container,  
                                void *data, size_t *d_len,
                                const char *labels);


/**
 * Write 32-bit integer into leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param value         integer value to be written
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_write_int32(asn_value *container, 
                           int32_t value, const char *labels);

/**
 * Read 32-bit integer from leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param value         place for integer value to be read (OUT)
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_read_int32(const asn_value *container, 
                          int32_t *value, const char *labels);

/**
 * Write boolean into leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param value         boolean value to be written
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_write_bool(asn_value *container, 
                          te_bool value, const char *labels);

/**
 * Read boolean from leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param value         place for boolean value to be read (OUT)
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_read_bool(const asn_value *container, 
                         te_bool *value, const char *labels);

/**
 * Write character string into leaf in specified ASN value.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param value         string to be written
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_write_string(asn_value *container, 
                            const char *value, const char *labels);

/**
 * Read character string from leaf in specified ASN value.
 * User have to free() got pointer to string.
 *
 * @param container     pointer to ASN value which leaf field is interested
 * @param value         place for pointer to read string (OUT)
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_read_string(const asn_value *container, 
                           char **value, const char *labels);


/**
 * Write component of CONSTRAINT subvalue in ASN value tree. 
 *
 * @param container     root of ASN value tree which subvalue to be changed
 * @param elem_value    ASN value to be placed into the tree at place,
 *                      specified by labels
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_write_component_value(asn_value *container, 
                                     const asn_value *elem_value,
                                     const char *labels);

/**
 * Read component of CONSTRAINT subvalue in ASN value tree. 
 *
 * @param container     root of ASN value tree which subvalue is interested
 * @param elem_value    read ASN value, copy of subtree specified by
 *                      labels argument (OUT)
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_read_component_value(const asn_value *container, 
                                    asn_value_p *elem_value,
                                    const char *labels);


/**
 * Replace array element in indexed ('SEQUENCE OF' or 'SET OF') subvalue 
 * of root ASN value container. 
 *
 * @param container     root of ASN value tree which subvalue is interested
 * @param elem_value    ASN value to be placed into the array, specified 
 *                      by labels at place, specified by index
 * @param index         array index of element to be replaced
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_write_indexed(asn_value_p container,
                             const asn_value_p elem_value, 
                             int index, const char *labels);

/**
 * Read array element in indexed ('SEQUENCE OF' or 'SET OF') subvalue 
 * of root ASN value 'container'. 
 *
 * @param container     root of ASN value tree which subvalue is interested
 * @param index         array index of element to be read 
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return pointer to new asn_value instance or NULL if error occurred. 
 */ 
extern asn_value_p asn_read_indexed(const asn_value *container, 
                                    int index, const char *labels);

/**
 * Insert array element in indexed syntax (i.e. 'SEQUENCE OF' or 'SET OF') 
 * subvalue of root ASN value container. 
 *
 * @param container     root of ASN value tree which subvalue is interested
 * @param elem_value    ASN value to be placed into the array, specified 
 *                      by labels at place, specified by index
 * @param index         array index of place to which element should be
 *                      inserted
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_insert_indexed(asn_value_p container,
                              const asn_value_p elem_value, 
                              int index, const char *labels );

/**
 * Remove array element from indexed syntax (i.e. 'SEQUENCE OF' or 'SET OF')
 * subvalue of root ASN value container. 
 *
 * @param container     root of ASN value tree which subvalue is interested
 * @param index         array index of element to be removed
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success, otherwise error code.
 */ 
extern int asn_remove_indexed(asn_value_p container, 
                              int index, const char *labels );

/**
 * Get length of subvalue of root ASN value container. 
 * Semantic of length value depends on the ASN syntax.
 *    primitive syntax:
 *        INTEGER -- 
 *            zero for usual native 'int' or number of bits used. 
 *        LONG_INT, CHAR_STRING, OCT_STRING, REAL --
 *            number of octets;
 *        OBJECT IDENTIFIER -- 
 *            number of sub-ids, which sub-id has usual for
 *            current architecture size of 'int';
 *        BIT_STRING -- number of bits;    
 *    constraint syntax:       
 *          number of sub-values; should be one or zero (for non-complete 
 *          values) for CHOICE and TAGGED syntacies.  
 *
 * @param container     root of ASN value tree which subvalue is interested
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return length subvalue, -1 if error occurred. 
 */ 
extern int asn_get_length(const asn_value *container, const char *labels );

/**
 * Obtain ASN syntax of specified field in value. 
 *
 * @param value          ASN value which leaf syntax is interested
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return syntax of specified leaf in value.
 */
extern asn_syntax asn_get_syntax(const asn_value *value, const char *labels);


/**
 * Obtain ASN syntax type;
 *
 * @param type          ASN value which leaf syntax is interested
 *
 * @return syntax of specified leaf in value.
 */
extern asn_syntax asn_get_syntax_of_type(const asn_type *type);


/**
 * Get choice in subvalue of root ASN value container.  
 *
 * @param container     root of ASN value tree which subvalue is interested
 *
 * @return pointer to label or NULL on error. 
 */ 
extern const char *asn_get_choice_ptr(const asn_value *container);

/**
 * Get choice in subvalue of root ASN value container. 
 *
 * @param container     root of ASN value tree which subvalue is interested
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *                      Subvalue should have ASN syntax CHOICE
 * @param choice_label  string with label of choice in ASN value (OUT)
 * @param ch_lb_len     length of available buffer in choice_label
 *
 * @return zero or error code.
 */ 
extern int asn_get_choice(const asn_value *container, const char *labels,
                          char *choice_label, size_t ch_lb_len);


/**
 * Get name of value;
 *
 * @param container     value which name is interested
 *
 * @return value's name or NULL.
 */ 
extern const char *asn_get_name(const asn_value *container);


/**
 * Get constant pointer to subtype of some ASN type.
 * 
 * @param type          ASN type
 * @param subtype       location for pointer to ASN sub-type (OUT)
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 * @param labels        string with dot-separated sequence of textual field
 *                      labels, specifying interested sub-type
 *
 * @return zero or error code.
 */ 
extern int asn_get_subtype(const asn_type *type, 
                           const asn_type ** subtype, const char *labels);

/**
 * Get constant pointer to subvalue of some ASN value with CONSTRAINT syntax.
 * User may to try discard 'const' qualifier of obtained subvalue only 
 * if he (she) knows very well what he doing with ASN value. 
 * In particular, got subvalue should NOT be freed!
 *
 * This method is much faster then "asn_read_component_value' because it does
 * not make external copy of subvalue. 
 * 
 * @param container     root of ASN value tree which subvalue is interested
 * @param subval        location for pointer to ASN sub-value (OUT)
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success or error code.
 */ 
extern int asn_get_subvalue(const asn_value *container, 
                            const asn_value ** subval, 
                            const char *labels);


/**
 * Get constant pointer to subvalue of ASN value with indexed 
 * ('SEQUENCE OF' or 'SET OF') syntax. 
 * User may try to discard 'const' qualifier of obtained subvalue only 
 * if he (she) knows very well what he doing with ASN value. 
 * In particular, got subvalue should NOT be freed!
 *
 * This method is much faster then "asn_read_component_value' because it does
 * not make external copy of subvalue. 
 * 
 * @param container     root of ASN value tree which subvalue is interested
 * @param subval        location for pointer to ASN sub-value (OUT)
 * @param index         index of subvalue
 *
 * @return zero on success or error code.
 */ 
extern int asn_get_indexed(const asn_value *container, 
                           const asn_value **subval, 
                           int index);

/**
 * Get ASN type of on-level child of constaint ASN type by child tag.
 * 
 * @param type      root of ASN value tree which subvalue is interested
 * @param subtype   location for pointer to ASN sub-value (OUT)
 * @param tag_class class of ASN tag
 * @param tag_val   value of ASN tag
 *
 * @return zero on success or error code.
 */ 
extern int asn_get_child_type(const asn_type *type,
                              const asn_type **subtype,
                              asn_tag_class tag_class, 
                              uint16_t tag_val);

/**
 * Get constant pointer to direct subvalue of ASN value with named syntax 
 * ('SEQUENCE' or 'SET') by its tag. If there are more then one 
 * child with specified tag, method return first found. 
 *
 * User may try to discard 'const' qualifier of obtained subvalue only 
 * if he (she) knows very well what he doing with ASN value. 
 * In particular, got subvalue should NOT be freed!
 *
 * This method is much faster then "asn_read_component_value' because
 * it does not make external copy of subvalue. 
 * 
 * @param container     root of ASN value tree which subvalue is interested
 * @param subval        location for pointer to ASN sub-value (OUT)
 * @param tag_class     class of ASN tag
 * @param tag_val       value of ASN tag
 *
 * @return zero on success or error code.
 */ 
extern int asn_get_child_value(const asn_value *container,
                               const asn_value **subval,
                               asn_tag_class tag_class, 
                               uint16_t tag_val);


/**
 * Get constant pointer to direct subvalue of ASN value with CHOICE syntax.
 *
 * User may try to discard 'const' qualifier of obtained subvalue only 
 * if he (she) knows very well what he doing with ASN value. 
 * In particular, got subvalue should NOT be freed!
 *
 * This method is much faster then "asn_read_component_value' because
 * it does not make external copy of subvalue. 
 * 
 * @param container     root of ASN value tree which subvalue is interested
 * @param subval        location for pointer to ASN sub-value (OUT)
 * @param tag_class     class of ASN tag of subvalue (OUT)
 * @param tag_val       value of ASN tag of subvalue (OUT)
 *
 * @return zero on success or error code.
 */ 
extern int asn_get_choice_value(const asn_value *container,
                                const asn_value **subval,
                                asn_tag_class *tag_class, 
                                uint16_t *tag_val);

/**
 * Get constant pointer to data related to leaf plain-syntax sub-value 
 * of ASN value.
 *
 * Got pointer should NOT be freed!
 *
 * Write to got memory location is acceptable in case of simple data syntax
 * (e.g. PrintableString or INTEGER) and if new data has length not greater
 * then set in ASN value leaf; user has no way to change data length except
 * via 'asn_write_value_field' method. 
 *
 * This method is much faster then "asn_read_value_field' because it does
 * not copy data to user location, with large octet strings or OIDs
 * it may be significant.
 * 
 * @param container     root of ASN value tree which subvalue is interested
 * @param data_ptr      pointer to location for plain data pointer; 
 *                      usually it should be something like '&str', where
 *                      'srt' has type 'const char *' (OUT)
 * @param labels        textual ASN labels of subvalue; see 
 *                      asn_free_subvalue method for more description
 *
 * @return zero on success or error code.
 */ 
extern int asn_get_field_data(const asn_value *container, 
                              void *data_ptr, const char *labels);



/**
 * Get tag of value;
 *
 * @param container     value which name is intereseted
 *
 * @return value's name or NULL.
 */ 
extern unsigned short asn_get_tag(const asn_value *container);


/**
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value
 * @param indent        current indent, usually should be zero
 *
 * @return length the number of bytes required for textual 
 *         presentation of specified value.
 */ 
extern size_t asn_count_txt_len(const asn_value *value, unsigned int indent);



/**
 * Find ASN.1 tag value by textual label.
 *
 * @param type          ASN type descriptor, must have SEQUENCE, SET
 *                      or CHOICE syntax 
 * @param label         textual label of desired field
 * @param tag           location for ASN.1 tag (OUT)
 *
 * @return status code
 */
extern int asn_label_to_tag(const asn_type *type, const char *label, 
                            asn_tag_t *tag);

/**
 * declaration of structures which describes basic ASN types. 
 */

extern const asn_type * const asn_base_boolean;
extern const asn_type * const asn_base_integer;
extern const asn_type * const asn_base_enum;
extern const asn_type * const asn_base_charstring;
extern const asn_type * const asn_base_octstring;
extern const asn_type * const asn_base_bitstring;
extern const asn_type * const asn_base_real;
extern const asn_type * const asn_base_null;
extern const asn_type * const asn_base_objid;

extern const asn_type * const asn_base_int4;
extern const asn_type * const asn_base_int8;
extern const asn_type * const asn_base_int16;

extern const asn_type  asn_base_boolean_s;
extern const asn_type  asn_base_integer_s;
extern const asn_type  asn_base_enum_s;
extern const asn_type  asn_base_charstring_s;
extern const asn_type  asn_base_octstring_s;
extern const asn_type  asn_base_bitstring_s;
extern const asn_type  asn_base_real_s;
extern const asn_type  asn_base_null_s;
extern const asn_type  asn_base_objid_s;

extern const asn_type  asn_base_int4_s;
extern const asn_type  asn_base_int8_s;
extern const asn_type  asn_base_int16_s;


#ifdef __cplusplus
} /* for 'extern "C" {' */
#endif

#endif /* __TE_ASN_USR_H__ */
