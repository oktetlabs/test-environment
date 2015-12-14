/** @file
 * @brief Common code for complex object in configuration tree
 *
 * The complex object is a node in the configuration tree, which includes
 * in its instance name a description of the object's fields.
 *
 * Example:
 *    /agent:Agt_A/rule:priority=32766,family=2,type=1,table=254 =
 * Instance name of this node is:
 *    priority=32766,family=2,type=1,table=254
 * It corresponds to 4 parameters: @b priority, @b family, @b type and
 * @b table.
 *
 * To use functions of this file you must create a declaration of
 * the struct's fields. It's an array of type @b te_conf_obj. To simplify
 * the creation the @b TE_CONF_OBJ_INIT should be use.
 *
 * There are several generic field types: @b uint8_t, @b uint32_t,
 * @b sockaddr and @b str.
 *
 * To create a custom type for structure @b te_conf_obj_func 3 functions
 * should be implement. @b te_conf_obj_{type}_to_str and
 * te_conf_obj_{type}_from_str are used to convert an object to string and
 * backward. @b te_conf_obj_{type}_compare is used to compare objects.
 * Then an instance of this structure should be created and used in the
 * @b TE_CONF_OBJ_INIT.
 *
 * When fields of a target structure are initialized, interaction with
 * the object may by done using functions:  @b te_conf_obj_to_str,
 * @b te_conf_obj_from_str and @b te_conf_obj_compare.
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LIB_CONF_OID_CONF_OBJECT_H_
#define __TE_LIB_CONF_OID_CONF_OBJECT_H_

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Constants to explain result of the comparison */
typedef enum te_conf_obj_compare_result {
    TE_CONF_OBJ_COMPARE_EQUAL,      /**< Values are equal */
    TE_CONF_OBJ_COMPARE_CONTAINS,   /**< First argument contains a second */
    TE_CONF_OBJ_COMPARE_DIFFERENT,  /**< Values are not equal */
    TE_CONF_OBJ_COMPARE_ERROR       /**< Failed comparison */
} te_conf_obj_compare_result;

/**
 * Transform a specific structure to a structure @b te_string
 *
 * @param [inout]str    String with data
 * @param [in]name      Field name
 * @param [in]arg       Pointer to a value of specific structure
 *
 * @return              Status code
 */
typedef te_errno (*te_conf_obj_func_to_str)(
        te_string *str, const char *name, const void *arg);

/**
 * Transform a string @b char * to a specific structure
 *
 * @param [in] value    Input value
 * @param [out]arg      Pointer to a value of specific structure
 * @param [in] options  User options (field @p opts from
 *                      structure @b te_conf_obj)
 *
 * @return              Status code
 */
typedef te_errno (*te_conf_obj_func_from_str)(
    const char *value, void *arg, const void *options);

/**
 * Compare two specific structures
 *
 * @param [in]a     First structure
 * @param [in]b     Second structure
 *
 * @return          Result of the comparison
 * @retval ==0      Values are not equal
 * @retval !=0      Values are equal
 */
typedef int (*te_conf_obj_func_compare)(const void *a, const void *b);

/** Methods for interaction with configurator objects of a specific type */
typedef struct te_conf_obj_methods {
    te_conf_obj_func_to_str     to_str;     /**< Transform to string */
    te_conf_obj_func_from_str   from_str;   /**< Transform from string */
    te_conf_obj_func_compare    compare;    /**< Compare two structures */
} te_conf_obj_methods;

/** Context for an interaction with a field of a complex object */
typedef struct te_conf_obj {
    const char             *name;       /**< Field name */
    const size_t            offset;     /**< Offset from a base address in
                                             structure */
    const uint32_t          flag;       /**< Flag corresponding to
                                             a specified field */
    const void             *options;    /**< User options */

    te_conf_obj_methods    *methods;    /**< Object methods */
} te_conf_obj;

/**
 * Initialize a structure fields
 *
 * @param _base_type        Type of basic structure
 * @param _methods_value    Object methods
 * @param _field            Field name containing a value
 * @param _flag_value       Flag corresponding to a specified @p _field
 * @param _options_value    User options
 */
#define TE_CONF_OBJ_INIT(_base_type, _methods_value, _field,    \
                         _flag_value, _options_value)           \
    {                                                           \
        .name     = #_field,                                    \
        .offset   = offsetof(_base_type, _field),               \
        .flag     = (_flag_value),                              \
        .options  = (_options_value),                           \
        .methods  = (_methods_value)                            \
    }

/** @name Declaration of common data types */
extern te_conf_obj_methods te_conf_obj_methods_uint8_t;
extern te_conf_obj_methods te_conf_obj_methods_uint32_t;
extern te_conf_obj_methods te_conf_obj_methods_sockaddr;
extern te_conf_obj_methods te_conf_obj_methods_str;
/** @} */

/**
 * Transform object to string
 *
 * @param [in]  fields          Specifies fields of an object
 * @param [in]  fields_number   Number of the object fields
 *                              (elements in @p fields)
 * @param [in]  base            Base address of an object
 * @param [in]  mask            Mask of filled fields in an object
 * @param [out] str             Pointer to result string
 *
 * @return                  Status code
 */
extern te_errno te_conf_obj_to_str(
        const te_conf_obj *fields, size_t fields_number,
        const void *base, uint32_t mask, char **str);

/**
 * Transform string to object
 *
 * @param [in]  fields          Specifies fields of an object
 * @param [in]  fields_number   Number of the object fields
 *                              (elements in @p fields)
 * @param [in]  str             Source string
 * @param [out] required        Finally required fields
 * @param [in]  base            Base address of an object
 * @param [in]  mask            Mask of filled fields in an object
 *
 * @return                  Status code
 */
extern te_errno te_conf_obj_from_str(
        const te_conf_obj *fields, size_t fields_number,
        const char *str, uint32_t *required, void *base, uint32_t *mask);

/**
 * Compare two objects
 *
 * @param [in] fields           Specifies fields of an object
 * @param [in] fields_number    Number of the object fields
 *                              (elements in @p fields)
 * @param [in] required         Mask required fields
 * @param [in] base_a           Base address of an object A
 * @param [in] mask_a           Mask of filled fields in an object A
 * @param [in] base_b           Base address of an object B
 * @param [in] mask_b           Mask of filled fields in an object B
 *
 * @return                      Result of comparison
 */
extern te_conf_obj_compare_result te_conf_obj_compare(
        const te_conf_obj *fields, size_t fields_number,
        uint32_t required,
        const void *base_a, uint32_t mask_a,
        const void *base_b, uint32_t mask_b);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_LIB_CONF_OID_CONF_OBJECT_H_ */
