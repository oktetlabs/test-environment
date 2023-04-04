/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Parameters expansion API
 *
 * Definitions of the C API that allows to expand parameters in a string
 *
 * Copyright (C) 2018-2023 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TOOLS_EXPAND_H__
#define __TE_TOOLS_EXPAND_H__

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"

#ifdef TE_EXPAND_XML
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#endif

#include "te_string.h"
#include "te_kvpair.h"

/** Maximum number of positional arguments */
#define TE_EXPAND_MAX_POS_ARGS 10

/**
 * Type for parameter expanding functions for te_string_expand_parameters().
 *
 * The function is expected to append a value associated
 * with @p name to @p dest, or leave @p dest if @p name is
 * undefined.
 *
 * @param[in]  name   parameter name
 * @param[in]  ctx    parameter expansion context
 * @param[out] dest   destination string
 *
 * @return @c TRUE if @p dest has been appended to
 *
 * @note The function is allowed to return @c TRUE without
 *       actually modifying @p dest meaning that @p name is
 *       associated with an "explicitly empty" value.
 */
typedef te_bool (*te_expand_param_func)(const char *name, const void *ctx,
                                        te_string *dest);

/**
 * Expands parameters in a string.
 *
 * Parameter names are mapped to values with @p expand_param callback.
 * Everything else is appended verbatim to @p dest string.
 *
 * The parameter names must be enclosed in `${` and `}`.
 *
 * Conditional expansion is supported:
 * - `${NAME:-VALUE}` is expanded into @c VALUE if @c NAME variable is not set,
 *   otherwise to its value
 * - `${NAME:+VALUE}` is expanded into @c VALUE if @c NAME variable is set,
 *   otherwise to an empty string.
 *
 * @param[in]  src               source string
 * @param[in]  expand_param      parameter expansion function
 * @param[in]  ctx               parameter expansion context
 * @param[out] dest              destination string
 *
 * @return status code
 * @retval TE_EINVAL  Unmatched `${` found
 *
 * @note If @p src has a valid syntax, the function and its derivatives
 *       always return success.
 */
extern te_errno te_string_expand_parameters(const char *src,
                                            te_expand_param_func expand_param,
                                            const void *ctx, te_string *dest);

/**
 * Expand environment variables in a string.
 *
 * See te_string_expand_parameters() for the expansion syntax.
 *
 * @param[in]  src               source string
 * @param[in]  posargs           positional arguments
 *                               (accesible through `${[0-9]}`, may be @c NULL)
 * @param[out] dest              destination string
 *
 * @return status code
 */
extern te_errno te_string_expand_env_vars(const char *src,
                                          const char *posargs
                                          [static TE_EXPAND_MAX_POS_ARGS],
                                          te_string *dest);


/**
 * Expand key references in a string.
 *
 * See te_string_expand_parameters() for the expansion syntax.
 *
 * @param[in]  src               source string
 * @param[in]  posargs           positional arguments
 *                               (accesible through `${[0-9]}`, may be @c NULL)
 * @param[in]  kvpairs           key-value pairs
 * @param[out] dest              destination string
 *
 * @return status code
 */
extern te_errno te_string_expand_kvpairs(const char *src,
                                         const char *posargs
                                         [static TE_EXPAND_MAX_POS_ARGS],
                                         const te_kvpair_h *kvpairs,
                                         te_string *dest);

/**
 * A function type for getting a value by name from given context.
 *
 * @param name   parameter name
 * @param ctx    parameter context
 *
 * @return a value associated with @p name or @c NULL
 *
 * @deprecated This type is only used by deprecated old
 *             te_expand_parameters(). See te_expand_param_func().
 */
typedef const char *(*te_param_value_getter)(const char *name, const void *ctx);

/**
 * Expands parameters in a string.
 *
 * See te_string_expand_parameters() for the expansion syntax.
 *
 * @param[in]  src              source string
 * @param[in]  posargs          positional arguments
 *                              (accessible through `${[0-9]}`, may be @c NULL)
 * @param[in]  get_param_value  function to get param value by name
 * @param[in]  params_ctx       context for parameters
 * @param[out] retval           resulting string
 *                              (must be free()'d by the caller)
 *
 * @return status code
 * @retval TE_EINVAL  Unmatched `${` found
 *
 * @deprecated te_string_expand_parameters() should be used instead.
 */
extern te_errno te_expand_parameters(const char *src, const char **posargs,
                                     te_param_value_getter get_param_value,
                                     const void *params_ctx, char **retval);

/**
 * Expands environment variables in a string.
 *
 * See te_string_expand_parameters() for the expansion syntax.
 *
 * @param[in]  src     source string
 * @param[in]  posargs positional arguments
 *                     (accessible through `${[0-9]}`, may be @c NULL)
 * @param[out] retval  resulting string
 *                     (must be free()'d by the caller)
 *
 * @return status code
 *
 * @deprecated te_string_expand_env_vars() should be used instead.
 */
static inline te_errno
te_expand_env_vars(const char *src, const char **posargs, char **retval)
{
    te_string tmp = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_expand_env_vars(src, posargs, &tmp);
    if (rc != 0)
        te_string_free(&tmp);
    else
        te_string_move(retval, &tmp);

    return rc;
}

/**
 * Expands key-value pairs in a string.
 *
 * See te_string_expand_parameters() for the expansion syntax.
 *
 * @param[in]  src     source string
 * @param[in]  posargs positional arguments
 *                     (accessible through `${[0-9]}`, may be @c NULL)
 * @param[in]  kvpairs key-value pairs
 * @param[out] retval  resulting string
 *                     (must be free()'d by the caller)
 *
 * @return status code
 *
 * @deprecated te_string_expand_kvpairs() should be used instead.
 */
static inline te_errno
te_expand_kvpairs(const char *src, const char **posargs,
                  const te_kvpair_h *kvpairs, char **retval)
{
    te_string tmp = TE_STRING_INIT;
    te_errno rc;

    rc = te_string_expand_kvpairs(src, posargs, kvpairs, &tmp);
    if (rc != 0)
        te_string_free(&tmp);
    else
        te_string_move(retval, &tmp);

    return rc;
}

#ifdef TE_EXPAND_XML
/**
 * A wrapper around xmlGetProp that expands custom parameters from
 * list of key-value pairs if given. Otherwise it expands environment variable
 * references.
 *
 * @param node          XML node
 * @param name          XML attribute name
 * @param expand_vars   List of key-value pairs for expansion in file,
 *                      @c NULL if environment variables are used for
 *                      substitutions
 *
 * @return The expanded attribute value or NULL if no attribute
 * or an error occurred while expanding.
 *
 * @sa cfg_expand_env_vars
 */
static inline char *
xmlGetProp_exp_vars_or_env(xmlNodePtr node, const xmlChar *name,
                           const te_kvpair_h *kvpairs)
{
    xmlChar *value = xmlGetProp(node, name);

    if (value)
    {
        char *result = NULL;
        te_errno rc;

        if (kvpairs == NULL)
            rc = te_expand_env_vars((const char *)value, NULL, &result);
        else
            rc = te_expand_kvpairs((const char *)value, NULL, kvpairs, &result);

        if (rc == 0)
        {
            xmlFree(value);
            value = (xmlChar *)result;
        }
        else
        {
            ERROR("Error substituting variables in %s '%s': %r",
                  name, value, rc);
            xmlFree(value);
            value = NULL;
        }
    }
    return (char *)value;
}

/**
 * Case of xmlGetProp_exp_vars_or_env that expands only environment
 * variables reference.
 *
 * @param node    XML node
 * @param name    XML attribute name
 *
 * @return The expanded attribute value or NULL if no attribute
 * or an error occurred while expanding.
 *
 * @sa cfg_expand_env_vars
 */
static inline char *
xmlGetProp_exp(xmlNodePtr node, const xmlChar *name)
{
    return xmlGetProp_exp_vars_or_env(node, name, NULL);
}
#endif /* TE_EXPAND_XML */



#endif /* !__TE_TOOLS_EXPAND_H__ */
