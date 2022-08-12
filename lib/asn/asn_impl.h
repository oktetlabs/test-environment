/** @file
 * @brief ASN.1 library internal interface
 *
 * Definitions of structures for internal ASN.1 value presentation.
 * Declarations of API for processing ASN.1 values.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 */

#ifndef __TE_ASN_IMPL_H__
#define __TE_ASN_IMPL_H__

#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 boolean values of false and true are defined as: FALSE is encoded as
 * zero (0), TRUE is encoded as a nonzero value. And according to
 * https://msdn.microsoft.com/ru-ru/bb648639 TRUE is 0xff
 */
#define ASN_FALSE   0
#define ASN_TRUE    0xff

/**
 * ASN.1 tagging type
 */
typedef enum {
    AUTOMATIC, /**< Tags are assigned automatically */
    IMPLICIT,  /**< Tag is not inserted for this value */
    EXPLICIT   /**< Tag is explicitely specified */
} asn_tagging_type;

/**
 * Compare two ASN.1 tags.
 *
 * @param l  first argument;
 * @param r  second argument;
 *
 * @return truth value of tag equality
 * @retval 1 if tags are equal;
 * @retval 0 if tags are differ;
 */
extern int asn_tag_equal(asn_tag_t l, asn_tag_t r);

/**
 * Element of array, specifying named subvalue in complex ASN.1 value.
 */
typedef struct asn_named_entry_t
{
    char           *const name; /**< Text label of subvalue */
    const asn_type *const type; /**< ASN.1 type of subvalue */
    asn_tag_t             tag;  /**< Tag of subvalue */
} asn_named_entry_t;

/**
 * Element of array, specifying named integer value in enumerated type.
 */
typedef struct asn_enum_entry_t
{
    char * const name;  /**< Text label of value */
    int          value; /**< Value itself */
} asn_enum_entry_t;

/**
 * ASN.1 type internal presentation
 */
struct asn_type {
    const char  *name;   /**< ASN.1 name of type, if any assigned. */

    asn_tag_t    tag;    /**< tag value of type. */
    asn_syntax   syntax; /**< syntax of type, that is "type" of value
                              itself. */

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
 * ASN.1 Value internal presentation
 */
struct asn_value
{
    const asn_type *asn_type; /**< ASN.1 type of value. */
    asn_tag_t       tag;      /**< ASN.1 tag of value. */
    asn_syntax      syntax;   /**< ASN.1 syntax of value. */

    char           *name;     /**< Name of value itself or field label,
                                   may be NULL or empty string. */

    size_t          len; /**<
                    length of value. semantic is depended on syntax:
                    - primitive syntax:
                        -# INTEGER --
                            zero for usual native 'int' or number of bits
                            used.
                        -# LONG_INT, CHAR_STRING, OCT_STRING, REAL --
                            number of used octets;
                        -# OBJECT IDENTIFIER --
                            number of sub-ids, which sub-id has usual for
                            current architecture size of 'int';
                        -# BIT_STRING -- number of bits;
                    - compound syntax:
                          number of sub-values,
                          This field should be one or zero (for non-complete
                          values) for CHOICE and TAGGED syntaxes.
                        */

    union {
        int          integer;   /**< for INTEGER-based syntaxes */
        asn_value  **array;     /**< for COMPOUND syntaxes */
        void        *other;     /**< Other syntaxes: octet and character
                                     strings, long ints, etc. Pointer
                                     is casted explicitely in internal
                                     sources. */
    } data;        /**< Syntax-specific data */

    int mark;
    int txt_len;   /**< Length of textual presentation of value,
                        may be unknown, this is denoted by -1.
                        Zero value means incomplete value. */
    int c_indent;
    int c_lines;
    char *path;    /**< Path to this value from root of some container.
                        It is valid ONLY inside asn_walk_depth function.
                        Root container is a container which was passed to
                        asn_walk_depth. Use asn_get_value_path from
                        walk_func to obtain this path */
};

/* See description in 'asn_usr.h' */
struct asn_child_desc {
    asn_value    *value;
    unsigned int  index;
};


/**
 * Find one-depth sub-type for passed ASN.1 type tree by its label.
 * This function is applicable only for ASN.1 types with COMPOUND syntax.
 *
 * @param type       pointer to ASN.1 value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of 'type',
 *                   for syntaxes "*_OF" and "TAGGED" this parameter
 *                   is ignored.
 * @param found_type pointer to found ASN.1 type (OUT).
 *
 * @return zero on success, otherwise error code.
 */
extern te_errno asn_impl_find_subtype(const asn_type * type,
                                      const char *label,
                                      const asn_type ** found_type);

/**
 * Find one-depth subvalue in ASN.1 value tree by its label.
 * This method is applicable only to values with COMPOUND syntax.
 *
 * @param container  pointer to ASN.1 value which leaf field is interested;
 * @param label      textual field label, specifying subvalue of
 *                   'container'.
 *                   Label for 'SEQUENCE OF' and 'SET OF' subvalues
 *                   is decimal notation of its integer index in array.
 * @param found_val  pointer to found subvalue (OUT).
 *
 * @return zero on success, otherwise error code.
 */
extern te_errno asn_impl_find_subvalue(const asn_value *container,
                                       const char *label,
                                       asn_value const **found_val);

/**
 * Find numeric index of subvalue in ASN.1 type specification by
 * symbolic label.
 *
 * If type syntax in CHOICE, 'labels' may start from
 * CHOICE field label with leading '#'.
 * For CHOICE  got index is offset of child specification in ASN.1 type
 * definition, but not in ASN.1 value instance.
 *
 * @param type          ASN.1 type.
 * @param labels        Labels string.
 * @param index         Location for found index (OUT).
 * @param rest_labels   Location for pointer to rest labels (OUT).
 *
 * @return status code
 */
extern te_errno asn_child_named_index(const asn_type *type,
                                      const char *labels, int *index,
                                      const char **rest_labels);

/**
 * Determine numeric index of field in structure presenting ASN.1 type
 * by tag of subvalue.
 * This method is applicable only to values with COMPOUND syntax with
 * named components: 'SEQUENCE', 'SET' and 'CHOICE'.
 *
 * @param type       ASN.1 type which subvalue is interested.
 * @param tag_class  class of ASN.1 tag
 * @param tag_val    value of ASN.1 tag
 * @param index      found index, unchanged if error occurred (OUT).
 *
 * @return zero on success, otherwise error code.
 */
extern te_errno asn_child_tag_index(const asn_type *type,
                                    asn_tag_class tag_class,
                                    uint16_t tag_val,
                                    int *index);


/**
 * Internal method for insert child by its index in container
 * type named-array. For CHOICE syntax index used for check
 * that new_value has respective type as specified in ASN.1 type.
 *
 * This method does not check that incoming pointers are not NULL,
 * so be carefull, when call it directly.
 *
 * @param container     ASN.1 value which child should be updated,
 *                      have to be of syntax SEQUENCE, SET, or CHOICE
 * @param child         New ASN.1 value for child, may be NULL.
 * @param index         Index of child.
 *
 * @return zero on success, otherwise error code.
 */
extern te_errno asn_put_child_by_index(asn_value *container,
                                       asn_value *child,
                                       int index);

/**
 * Internal method for get child by its index in container
 * type named-array. For CHOICE syntax index used for check
 * that really contained subvalue has respective type and choice name
 * as specified in ASN.1 type.
 *
 * This method does not check that incoming pointers are not NULL,
 * so be carefull, when call it directly.
 *
 * @param container     ASN.1 value which child should be updated,
 *                      have to be of syntax SEQUENCE, SET, or CHOICE
 * @param child         Location for pointer to the child (OUT).
 * @param index         Index of child.
 *
 *
 * @return zero on success, otherwise error code.
 */
extern te_errno asn_get_child_by_index(const asn_value *container,
                                       asn_value **child,
                                       int index);

extern te_bool asn_clean_count(asn_value *value);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __TE_ASN_IMPL_H__ */
