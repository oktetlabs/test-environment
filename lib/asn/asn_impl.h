/** @file 
 * @brief ASN.1 library internal interface
 *
 * Definitions of structures for internal ASN.1 value presentation.
 * Declarations of API for processing ASN.1 values. 
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_ASN_IMPL_H__
#define __TE_ASN_IMPL_H__

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUTOMATIC,
    IMPLICIT,
    EXPLICIT
} asn_tagging_type;

/**
 * Compare two ASN tags.
 *
 * @param l  first argument;
 * @param r  second argument; 
 *
 * @return truth value of tag equality 
 * @retval 1 if tags are equal;
 * @retval 0 if tags are differ; 
 */
extern int asn_tag_equal(asn_tag_t, asn_tag_t);

typedef struct asn_named_entry_t
{
    char           *const name;
    const asn_type *const type;
    asn_tag_t             tag;
} asn_named_entry_t;

typedef struct asn_enum_entry_t
{
    char * const name;
    int          value;
} asn_enum_entry_t;

/**
 * ASN type internal presentation
 */
struct asn_type {
    const char  *name;   /**< ASN.1 name of type, if any assigned. */

    asn_tag_t    tag;    /**< tag value of type. */
    asn_syntax   syntax; /**< syntax of type, that is "type" of value itself. */

    size_t       len; /**< Size of value, if any specified as SIZE clause
                           in ASN.1 type specification.. 
                           Zero if not specified.
                           Whereas clause SIZE may not be used with
                           constructions with named fields, for such types
                           this structure member used for quantity of 
                           named fields. 
                           For INTEGER -- zero for usual native 'int' 
                           or number of bits used.
                           For ENUMERATED -- number of named values.
                        */ 
    union 
    {
        const asn_named_entry_t 
                       *named_entries; /**< for syntaxies SEQUENCE, SET 
                                                    and CHOICE */
        const asn_type *subtype;       /**< for syntaxies *_OF and TAGGED */

        const asn_enum_entry_t  
                       *enum_entries;  /**< for syntax ENUMERATED */
    } sp; /* syntax specific info */
};



/**
 * ASN Value internal presentation
 */
struct asn_value 
{
    const asn_type * asn_type; /**< ASN.1 type of value. */
    asn_tag_t        tag;      /**< ASN.1 tag of value. */
    asn_syntax       syntax;   /**< ASN.1 syntax of value. */

    char        * name;     /**< name of value itself or field label, 
                                 may be NULL or empty string. */

    size_t          len; /**< 
                    length of value. semantic is depended on syntax: 
                    - primitive syntax:
                        -# INTEGER -- 
                            zero for usual native 'int' or number of bits used. 
                        -# LONG_INT, CHAR_STRING, OCT_STRING, REAL --
                            number of used octets;
                        -# OBJECT IDENTIFIER -- 
                            number of sub-ids, which sub-id has usual for
                            current architecture size of 'int';
                        -# BIT_STRING -- number of bits;    
                    - constraint syntax:       
                          number of sub-values, 
                          This field should be one or zero (for non-complete 
                          values) for CHOICE and TAGGED syntaxes.  
                        */ 

    union {
        int          integer;
        void        *other;
        asn_value_p *array;
    } data;

    int txt_len;   /**< Length of textual presentation of value, 
                        may be unknown, this is denoted by -1. 
                        Zero value means incomplete value. */
};


/**
 * Find one-depth sub-type for passed ASN type tree by its label.
 * This function is applicable only for ASN types with CONSTRAINT syntax.
 *
 * @param type       pointer to ASN value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of 'type',
 *                   for syntaxes "*_OF" and "TAGGED" this parameter is ignored. 
 * @param found_type pointer to found ASN type (OUT).
 *
 * @return zero on success, otherwise error code. 
 */ 
extern int asn_impl_find_subtype(const asn_type * type, const char *label,
                                  const asn_type ** found_type);

/**
 * Find one-depth subvalue in ASN value tree by its label.
 * This method is applicable only to values with CONSTRAINT syntax. 
 *
 * @param container  pointer to ASN value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of 'container'. 
 *                   Label for 'SEQUENCE OF' and 'SET OF' subvalues 
 *                   is decimal notation of its integer index in array.
 * @param found_val  pointer to found subvalue (OUT).
 *
 * @return zero on success, otherwise error code. 
 */ 
extern int asn_impl_find_subvalue(const asn_value *container, 
                                  const char *label, 
                                  asn_value const **found_val);

/**
 * Determine numeric index of field label in structure presenting ASN.1 type.
 * This method is applicable only to values with CONSTRAINT syntax with named
 * components: 'SEQUENCE', 'SET' and 'CHOICE'. 
 *
 * @param type       ASN type which subvalue is interested. 
 * @param label      textual field label, specifying subvalue in type. 
 * @param index      found index, unchanged if error occurred (OUT).
 *
 * @return zero on success, otherwise error code. 
 */
extern int asn_impl_named_subvalue_index(const asn_type *type,
                                         const char *label, int *index);

/**
 * Determine numeric index of field in structure presenting ASN.1 type
 * by tag of subvalue.
 * This method is applicable only to values with CONSTRAINT syntax with
 * named components: 'SEQUENCE', 'SET' and 'CHOICE'. 
 *
 * @param type       ASN type which subvalue is interested. 
 * @param tag_class  class of ASN tag
 * @param tag_val    value of ASN tag
 * @param index      found index, unchanged if error occurred (OUT).
 *
 * @return zero on success, otherwise error code. 
 */
extern int asn_child_tag_index(const asn_type *type,
                               asn_tag_class tag_class, uint16_t tag_val,
                               int *index);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __TE_ASN_IMPL_H__ */
