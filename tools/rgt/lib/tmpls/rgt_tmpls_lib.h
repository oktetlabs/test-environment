/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment: Generic interface for working with
 * output templates.
 *
 * The module provides functions for parsing and output templates
 *
 * There is a set of templates each for a particular log piece(s) such as
 * document start/end, log start/end and so on.
 * The templates can be constant or parametrized strings, which contain
 * places that should be filled in with appropriate values gathered
 * from the parsing context. For example parametrized template is needed
 * for start log message parts, which should be expanded with log level,
 * entity and user name of a log message.
 * The places where context-specific values should be output is marked
 * with a pair of "@@" signs (doubled "at" sign) and the name of a context
 * variable between them, whose value should be expanded there.
 *
 * For example:
 *
 * This an example of how a parametrized template might look:
 * <p>
 * Here @@user@@ should be inserted a value of context-specific
 * variable "user"
 * </p>
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RGT_TMPLS_LIB_H__
#define __TE_RGT_TMPLS_LIB_H__

#include <stdio.h>

#include "te_defs.h"
#include "te_stdint.h"
#include "te_string.h"

/** Delimiter to be used for determination variables in templates */
#define RGT_TMPLS_VAR_DELIMETER "@@"

/**
 * Each template before using is parsed onto blocks each block can be
 * whether a constant sting or a variable value.
 * For example the following single line template:
 *  The value of A is @@%s:A@@, the value of B is @@%s:B@@.
 * Will be split into five blocks:
 *  1. const string: "The value of A is "
 *  2. a value of variable: $A (output via %s format string)
 *  3. const string: ", the value of B is "
 *  4. a value of variable: $B (output via %s format string)
 *  5. const string: "."
 *
 */

/** The list of possible attribyte types */
typedef enum rgt_attr_type {
    RGT_ATTR_TYPE_STR, /**< attribute of type string */
    RGT_ATTR_TYPE_UINT32, /**< attribute of type uint32_t */
    RGT_ATTR_TYPE_UNKNOWN, /**< Unknown attribute type */
} rgt_attr_type_t;

/** Structure to keep attribute value */
typedef struct rgt_attrs {
    rgt_attr_type_t  type; /**< Attribute type */
    const char      *name; /**< Attribute name */
    union {
        const char *str_val; /**< The value for string attribute */
        uint32_t    uint32_val; /**< The value for uint32_t attribute */
    };
} rgt_attrs_t;

/** Specifies the type of block */
typedef enum rgt_blk_type {
    RGT_BLK_TYPE_CSTR, /**< A block contains a constant string */
    RGT_BLK_TYPE_VAR, /**< A block contains a value of a variable */
} rgt_blk_type_t;

/** The structure represents single variable in template */
typedef struct rgt_var_def {
    const char *name; /**< Name of a variable whose value should be
                           output in this block */
    const char *fmt_str; /**< Format string for output this variable */
} rgt_var_def_t;


/** The structure represents one block of a template */
typedef struct rgt_blk {
    rgt_blk_type_t type; /**< Defines the type of a block */

    /** The union field to access depends on the value of blk_type field */
    union {
        const char    *start_data; /**< Pointer to the beginning of
                                        a constant string, which should
                                        be output in this block */
        rgt_var_def_t  var; /**< Variable data structure */
    };

} rgt_blk_t;

/** Structure to keep a single template */
typedef struct rgt_tmpl {
    char         *fname; /**< Themplate file name */
    char         *raw_ptr; /**< Pointer to the beginning of the memory
                                allocated under a log template */
    rgt_blk_t    *blocks; /**< Pointer to an array of blocks */
    int           n_blocks; /**< Total number of blocks */
} rgt_tmpl_t;

/**
 * Create rgt attribute list based on libxml attribute format
 *
 * @param xml_attrs  XML attributes in libxml native format
 *                   (array of pairs: attribute name, attribute value)
 *
 * @return Pointer to the list of attributes in rgt format
 *
 * @note Currently you can only use one attribute list at the same time,
 * so that after using the list free it with rgt_tmpls_attrs_free().
 */
extern rgt_attrs_t *rgt_tmpls_attrs_new(const char **xml_attrs);

/**
 * Free the list of attributes.
 *
 * @param attrs  Rgt attributes
 */
extern void rgt_tmpls_attrs_free(rgt_attrs_t *attrs);

/**
 * Save a copy of rgt attribute list
 *
 * @param attrs  Rgt attributes to save
 *
 * @return Pointer to the saved list
 *
 * @note Similar to rgt_tmpls_attrs_new() above, there can be only one copy
 * of saved attributes.
 *
 * @sa rgt_tmpls_attrs_saved_free
 */
extern rgt_attrs_t * rgt_tmpls_attrs_save(rgt_attrs_t *attrs);

/**
 * Free the saved copy of rgt attribute list
 *
 * @param attrs  Saved copy of rgt attributes
 *
 * @sa rgt_tmpls_attrs_save
 */
extern void rgt_tmpls_attrs_saved_free(rgt_attrs_t *attrs);

/**
 * Add a new string attribute into the list of rgt attributes.
 *
 * @param attrs    Rgt attributes list
 * @param name     A new attribute name
 * @param fmt_str  Format string for the attribute value followed by
 *                 the list of arguments
 */
extern void rgt_tmpls_attrs_add_fstr(rgt_attrs_t *attrs, const char *name,
                                     const char *fmt_str, ...);

/**
 * Update the value of string attribute in the list of rgt attributes.
 * If not exist it adds this attribute.
 *
 * @param attrs    Rgt attributes list
 * @param name     A new attribute name
 * @param fmt_str  Format string for the attribute value followed by
 *                 the list of arguments
 */
extern void rgt_tmpls_attrs_set_fstr(rgt_attrs_t *attrs, const char *name,
                                     const char *fmt_str, ...);

/**
 * Add a new uint32_t attribute into the list of rgt attributes.
 *
 * @param attrs  Rgt attributes list
 * @param name   A new attribute name
 * @param val    Attribute value
 */
extern void rgt_tmpls_attrs_add_uint32(rgt_attrs_t *attrs, const char *name,
                                       uint32_t val);

/**
 * Update the value of uint32_t attribute in the list of rgt attributes.
 * If not exist it adds this attribute.
 *
 * @param attrs  Rgt attributes list
 * @param name   A new attribute name
 * @param val    Attribute value
 */
extern void rgt_tmpls_attrs_set_uint32(rgt_attrs_t *attrs, const char *name,
                                       uint32_t val);

/**
 * Get the value of attribute from the list of attributes in
 * libxml native format.
 *
 * @param attrs  XML attributes in libxml native format
 *               (array of pairs: attribute name, attribute value)
 * @param name   Attribute name, whose value we want to get
 *
 * @return Attribute value.
 */
extern const char *rgt_tmpls_xml_attrs_get(const char **attrs,
                                           const char *name);

/**
 * Get the prefix to resource files of running utility (tmpls, misc, etc.)
 *
 * @note First call intializes prefix. If function succeeds, following calls
 *       return the same value. @p util_path and @p argv0 will be ignored in
 *       following calls
 *
 * @warning Assumes that the executable is in installdir/bindir
 *          and datadir is "share"
 *
 * @param[in] util_path  path to the utility resource directory from datadir
 * @param[in] argv0      the name of called program (pass argv[0] from main)
 * @param[in] size       size of the buffer pointed by prefix
 * @param[out] prefix    pointer to the prefix to be written
 *
 * @return  Status code
 *
 * @retval 0    On success
 * @retval 1    On failure
 */

extern int rgt_resource_files_prefix_get(const char *util_path,
                                         const char *argv0,
                                         size_t size,
                                         char *prefix);

/**
 * Parses themplate files and fills in an appropriate data structure.
 *
 * @param files     An array of template files to be parsed
 * @param prefix    A path that must be prefixed before every files entry
 * @param tmpls     An array of internal representation of the templates
 * @param tmpl_num  Number of templates in the arrays (the same as the
 *                  number of files passed)
 *
 * @return  Status of the operation
 *
 * @retval 0  On success
 * @retval 1  On failure
 *
 * @se If an error occures the function output error message into stderr
 */
extern int rgt_tmpls_parse(const char **files, const char *prefix,
                           rgt_tmpl_t *tmpls, size_t tmpl_num);

/**
 * Frees internal representation of templates
 *
 * @param  tmpls     Pointer to an array of templates
 * @param  tmpl_num  Number of templates in the array
 */
extern void rgt_tmpls_free(rgt_tmpl_t *tmpls, size_t tmpl_num);

/**
 * Outputs a template block by block into specified file.
 *
 * @param out_fd    Output file descriptor
 * @param tmpl      Pointer to a template to be output
 * @param attrs     Pointer to an array of attributes for that template
 *
 * @return  Status of the operation
 *
 * @retval 0  On success
 * @retval 1  On failure
 *
 * @se If an error occures the function output error message into stderr
 */
extern int rgt_tmpls_output(FILE *out_fd, rgt_tmpl_t *tmpl,
                            const rgt_attrs_t *attrs);

/**
 * Outputs a template block by block into specified TE string.
 *
 * @param str       TE string to which to append data
 * @param tmpl      Pointer to a template to be output
 * @param attrs     Pointer to an array of attributes for that template
 *
 * @return  Status of the operation
 *
 * @retval 0  On success
 * @retval 1  On failure
 *
 * @se If an error occures the function outputs error message into stderr
 */
extern int rgt_tmpls_output_str(te_string *str, rgt_tmpl_t *tmpl,
                                const rgt_attrs_t *attrs);

extern void rgt_attr_settings_init(const char *sep, int length);

#endif /* __TE_RGT_TMPLS_LIB_H__ */
