/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief JSON-handling routines.
 *
 * @defgroup te_tools_te_json JSON routines.
 * @ingroup te_tools
 * @{
 *
 * These functions provide an easy but robust way to convert
 * native data to JSON format.
 *
 * Note, however, that it is not a full-fledged JSON serializer,
 * so it is still possible to generate invalid JSON if the API is
 * used carelessly.
 *
 * Since they are intended to be used with C data of known layout,
 * they do not report errors: if anything goes wrong, it's considered
 * API contract violation and an assertion fires.
 */

#ifndef __TE_JSON_H__
#define __TE_JSON_H__

#include <inttypes.h>
#include <math.h>

#include "te_errno.h"
#include "te_defs.h"
#include "te_string.h"
#include "te_kvpair.h"
#include "te_enum.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The kind of JSON compound. */
typedef enum te_json_compound {
    TE_JSON_COMPOUND_TOPLEVEL,     /**< toplevel */
    TE_JSON_COMPOUND_ARRAY,        /**< array */
    TE_JSON_COMPOUND_OBJECT,       /**< object (dictionary) */
    TE_JSON_COMPOUND_OBJECT_VALUE, /**< object value */
    TE_JSON_COMPOUND_STRING,       /**< string value */
} te_json_compound;

/** Maximum nesting level for JSON serialization */
#define TE_JSON_MAX_NEST 16

/** One level of JSON value nesting */
typedef struct te_json_level_t {
    /** the kind of compound at this level  */
    te_json_compound kind;
    /** number of items already added at this level */
    size_t n_items;
} te_json_level_t;

/**
 * The context for JSON serialization.
 *
 * @note While the structure is declared public to make on-stack variables
 *       possible, it shall be treated as an opaque and only initialized
 *       with TE_JSON_INIT_STR() and then passed to the API from this group.
 */
typedef struct te_json_ctx_t {
    /** JSON */
    te_string *dest;

    /** stack of nested JSON compounds */
    te_json_level_t nesting[TE_JSON_MAX_NEST];

    /** current nesting depth */
    size_t current_level;
} te_json_ctx_t;

/**
 * Initializer for a JSON context.
 *
 * @param str_   destination string
 */
#define TE_JSON_INIT_STR(str_) \
    {                                                               \
        .dest = (str_),                                             \
        .nesting = {[0] = {                                         \
                .kind = TE_JSON_COMPOUND_TOPLEVEL,                  \
                .n_items = 0}},                                     \
        .current_level = 0                                          \
    }

/**
 * Serialize a simple JSON value formatted according to @p fmt.
 *
 * @attention
 * This function does no escaping, so is not intended for general usage.
 * Normally one should use one of type-specific functions.
 *
 * @param ctx JSON context
 * @param fmt format string
 * @param ... arguments
 *
 * @sa te_json_add_null, te_json_add_integer,
 *     te_json_add_float, te_json_add_string
 */
extern void te_json_add_simple(te_json_ctx_t *ctx,
                               const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/**
 * Serialize JSON null.
 *
 * @param ctx JSON context
 */
static inline void
te_json_add_null(te_json_ctx_t *ctx)
{
    te_json_add_simple(ctx, "null");
}

/**
 * Serialize a JSON boolean.
 *
 * @param ctx JSON context
 * @param val boolean value
 */
static inline void
te_json_add_bool(te_json_ctx_t *ctx, te_bool val)
{
    te_json_add_simple(ctx, val ? "true" : "false");
}


/**
 * Serialize a JSON integer value.
 *
 * @param ctx JSON context
 * @param val integral value
 */
static inline void
te_json_add_integer(te_json_ctx_t *ctx, intmax_t val)
{
    te_json_add_simple(ctx, "%jd", val);
}

/**
 * Serialize a JSON floating value.
 *
 * If @p val is not finite (infinity or NaN), @c null is serialized.
 *
 * @param ctx       JSON context
 * @param val       Value
 * @param precision Floating-point precision
 */
static inline void
te_json_add_float(te_json_ctx_t *ctx, double val, unsigned int precision)
{
    if (!isfinite(val))
        te_json_add_null(ctx);
    else
        te_json_add_simple(ctx, "%.*g", precision, val);
}

/**
 * Serialize a string value escaping it if necessary.
 *
 * Namely, double quotes, backslashes and control characters are
 * properly escaped. No special provision is made for Unicode non-ASCII
 * characters, though. Embedded zeroes are not supported.
 *
 * Double quotes are added around the value.
 *
 * @param ctx      JSON context
 * @param fmt      format string
 * @param ...      arguments
 */
extern void te_json_add_string(te_json_ctx_t *ctx,
                               const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));;

/**
 * Start serializing a JSON string.
 *
 * te_json_append_string() should be used to append bytes to
 * the serialized string. At the end, te_json_end() must be called
 * to finalize JSON string. Double quotes around the composed string
 * value are added automatically.
 * Unlike te_json_add_string(), this allows to construct string
 * value incrementally instead of specifying it all at once.
 *
 * @param ctx JSON context
 */
extern void te_json_start_string(te_json_ctx_t *ctx);

/**
 * Append formatted string to a string value which was started with
 * te_json_start_string(). The same escaping is done as with
 * te_json_add_string().
 *
 * @param ctx      JSON context
 * @param fmt      format string
 * @param ...      arguments
 *
 * @sa te_json_append_string_va()
 */
extern void te_json_append_string(te_json_ctx_t *ctx,
                                  const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/**
 * Same as te_json_append_string() but accepts va_list instead of
 * a list of arguments.
 *
 * @param ctx      JSON context
 * @param fmt      format string
 * @param args     arguments
 */
extern void te_json_append_string_va(te_json_ctx_t *ctx, const char *fmt,
                                     va_list args);

/**
 * Start serializing a JSON array.
 *
 * te_json_end() shall be called after all items are serialized.
 *
 * @param ctx JSON context
 */
extern void te_json_start_array(te_json_ctx_t *ctx);

/**
 * Start serializing a JSON object.
 *
 * te_json_end() shall be called after all items are serialized.
 *
 * @param ctx JSON context
 */
extern void te_json_start_object(te_json_ctx_t *ctx);

/**
 * Finalize the current JSON value nesting.
 *
 * This function must be called after te_json_start_object()
 * or te_json_start_array(). It may be called at the toplevel
 * and has no effect in this case.
 *
 * @param ctx JSON context
 */
extern void te_json_end(te_json_ctx_t *ctx);

/**
 * Mark the beginning of a new key in an object.
 *
 * @param ctx  JSON context
 * @param key  key (it is assumed that it does not need escaping);
 *             @c NULL value is treated as an empty string
 */
extern void te_json_add_key(te_json_ctx_t *ctx, const char *key);

/**
 * Output a new key of an object with a given string value.
 *
 * If @p val is @c NULL, nothing is added, not a null value.
 *
 * @param ctx  JSON context
 * @param key  key
 * @param val  string value
 */
static inline void
te_json_add_key_str(te_json_ctx_t *ctx, const char *key, const char *val)
{
    if (val != NULL)
    {
        te_json_add_key(ctx, key);
        te_json_add_string(ctx, "%s", val);
    }
}

/**
 * Output a new key with an enumeration-like value.
 *
 * The string representation of a value is produced by applying @p map
 * to @p val. If @p val is not found in @p map or if it maps to @c NULL
 * explicitly, the key is omitted.
 *
 * @param ctx      JSON context
 * @param map      enum-to-string mapping
 * @param key      key
 * @param val      enum value
 */
static inline void
te_json_add_key_enum(te_json_ctx_t *ctx, const te_enum_map *map,
                     const char *key, int val)
{
    te_json_add_key_str(ctx, key, te_enum_map_from_any_value(map, val, NULL));
}

/**
 * Serialize an array of strings.
 *
 * If @p skip_null is @c TRUE, @c NULL values in @p strs
 * will be skipped, otherwise serialized as JSON @c null.
 *
 * @param ctx       JSON context
 * @param skip_null null handling mode
 * @param n_strs    number of elements in @p strs
 * @param strs      array of strings
 */
extern void te_json_add_array_str(te_json_ctx_t *ctx, te_bool skip_null,
                                  size_t n_strs, const char *strs[n_strs]);

/**
 * Serialize a list of kv_pairs as a JSON object.
 *
 * @param ctx  JSON context
 * @param head Key-value list
 */
extern void te_json_add_kvpair(te_json_ctx_t *ctx, const te_kvpair_h *head);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_JSON_H__ */
/**@} <!-- END te_tools_te_json --> */
