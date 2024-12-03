/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief API to work with compound strings.
 *
 * @defgroup te_tools_te_compound Compound strings
 * @ingroup te_tools
 * @{
 *
 * Compound strings are an extension to ordinary TE dynamic
 * strings that can hold one-dimensional arrays of string
 * and key-value pairs.
 *
 * They employ special characters, normally not found in
 * regular strings to separate items from each other and keys from values.
 * So they cannot be used to store arbitrary binary data, but instead
 * they are fully compatible with regular TE strings and C strings
 * (i.e. no embedded zeroes).
 *
 * The main usecase for compound strings is to extend existing APIs
 * that were designed to handle simple strings only so that they could
 * pass around structured values without losing backward compatibility.
 *
 * Key-value pairs are stored sorted in compound strings, so that two
 * equivalent key-value sets would compare equal. This makes compound
 * strings slightly more efficient on read that plain key-value pairs API.
 */

#ifndef __TE_COMPOUND_H__
#define __TE_COMPOUND_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"
#include "te_vector.h"
#include "te_kvpair.h"
#include "te_json.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The kind of a compound string.
 *
 * This is mostly important for JSON serialization.
 */
typedef enum te_compound_kind {
    /** Empty compound string. */
    TE_COMPOUND_NULL,
    /** Simple non-empty string without internal structure. */
    TE_COMPOUND_PLAIN,
    /** A compound containing only unnamed items. */
    TE_COMPOUND_ARRAY,
    /**
     * A compound containing at least one named item.
     *
     * It may also have unnamed items as well, making it
     * a hybrid of an array and a dictionary, but such
     * compounds will still always be serialized as JSON
     * objects.
     */
    TE_COMPOUND_OBJECT,
} te_compound_kind;

/**
 * The mode of operation for duplicate keys.
 */
typedef enum te_compound_mod_op {
    /** Append a new value after an existing one. */
    TE_COMPOUND_MOD_OP_APPEND,
    /** Prepend a new value before an existing one. */
    TE_COMPOUND_MOD_OP_PREPEND,
    /** Replace all existing values with a new one. */
    TE_COMPOUND_MOD_OP_REPLACE,
} te_compound_mod_op;

/**
 * Item separator.
 *
 * A string that is part of a compound value cannot contain
 * this character.
 */
#define TE_COMPOUND_ITEM_SEP '\x1E'

/**
 * Key-value separator.
 *
 * A strinng that is part of a compound value should not
 * contain this character.
 */
#define TE_COMPOUND_KEY_SEP  '\x1F'

/**
 * Determine the kind of a compound string.
 *
 * @param comp  Compound string.
 *
 * @return The kind of the string.
 */
extern te_compound_kind te_compound_classify(const te_string *comp);

/**
 * Validate the compound string.
 *
 * Errors will be logged by this function.
 *
 * @param comp  Compound string.
 *
 * @return @c true if the compound string is well-formed.
 */
extern bool te_compound_validate(const te_string *comp);

/**
 * Like te_compound_validate(), but accepts a plain C string
 * as an input.
 *
 * The reason for this function existence is to avoid creating
 * an intermediate #te_string if a compound string was obtained
 * from some external source (e.g. read from a file).
 *
 * @param comp  Compound string representation.
 *
 * @return @c true if @p comp represents a valid compound string.
 */
extern bool te_compound_validate_str(const char *comp);

/**
 * Extract a value associated with a given key in the compound.
 *
 * If @p key is @c NULL, an @p idx'th unnamed item is extracted.
 *
 * A compound that has no structure is treated as single-item array.
 *
 * If there is no value associated with a given key or if there is less
 * values than @p idx, nothing is extracted and the function just returns
 * @c false.
 *
 * @param[out] dst   String which is appended the extracted value.
 * @param[in]  comp  Compound string.
 * @param[in]  key   Key of a value (may be @c NULL)
 * @param[in]  idx   Index of a value among equal keys.
 *
 * @return @c true if a value has been extracted.
 */
extern bool te_compound_extract(te_string *dst, const te_string *comp,
                                const char *key, size_t idx);

/**
 * Count the number of values associated with a given key or the number
 * of unnamed items if @p key is @c NULL.
 *
 * @param comp  Compound string.
 * @param key   Key (may be @c NULL).
 *
 * @return The number of values associated with @p key.
 */
extern size_t te_compound_count(const te_string *comp, const char *key);

/**
 * Set or unset a value inside a compound associated with a given key.
 *
 * The behaviour of this function when there is already a value associated
 * with the given key depends on @p mod_op:
 * - #TE_COMPOUND_MOD_OP_APPEND: a new value is appended after all
 *                               existing values;
 * - #TE_COMPOUND_MOD_OP_PREPEND: a new value is prepended before all
 *                                existing values;
 * - #TE_COMPOUND_MOD_OP_REPLACE: a new value is replacing all
 *                                existing values.
 *
 * If there is no existing value for a given key, all modification modes
 * are identical.
 *
 * If @p val_fmt is @c NULL, all existing values are deleted irrespective
 * of the value of @p mod_op.
 *
 * When @p key is @c NULL, the function operates on unnamed items.
 * Please note that, unlike #te_vec, it is not possible to replace
 * a single unnamed item at a given index: if @p mod_op is
 * #TE_COMPOUND_MOD_OP_REPLACE, all existing unnamed items will be replaced
 * with the new one.
 *
 * If a value is added to a plain string, the latter is converted to
 a single-item array.
 *
 * @param comp     Compound string.
 * @param key      Key (may be @c NULL).
 * @param mod_op   Modification mode.
 * @param val_fmt  Format string (may be @c NULL).
 * @param args     Variadic list to format.
 */
extern void te_compound_set_va(te_string *comp, const char *key,
                               te_compound_mod_op mod_op,
                               const char *val_fmt, va_list args)
                               TE_LIKE_VPRINTF(4);

/**
 * Like te_compound_set_va(), but accepts variable number of arguments.
 *
 * @param comp     Compound string.
 * @param key      Key (may be @c NULL).
 * @param mod_op   Modification mode.
 * @param val_fmt  Format string (may be @c NULL).
 * @param ...      Arguments to format.
 */
extern void te_compound_set(te_string *comp, const char *key,
                            te_compound_mod_op mod_op,
                            const char *val_fmt, ...) TE_LIKE_PRINTF(4, 5);

/**
 * Append a new value to the compound without any checks.
 *
 * Careless use of this function may result in an ill-formed compound
 * string, so it should be used in special cases.
 *
 * One use case for the function is to implement a filter for another
 * compound, when it is known for sure that items would come in the right
 * order.
 *
 * @param comp     Compound string.
 * @param key      Key to append (may be @c NULL).
 * @param value    Value to add (should not be @c NULL).
 */
static inline void
te_compound_append_fast(te_string *comp, const char *key, const char *value)
{
    if (key == NULL)
    {
        te_string_append(comp, "%s%c", value, TE_COMPOUND_ITEM_SEP);
    }
    else
    {
        te_string_append(comp, "%s%c%s%c", key, TE_COMPOUND_KEY_SEP,
                         value, TE_COMPOUND_ITEM_SEP);
    }
}

/**
 * Merge a compound string into another one.
 *
 * The behaviour for keys from @p src that are already
 * in @p dst depends on the value of @p mod_op:
 * - #TE_COMPOUND_MOD_OP_APPEND: all values from @p src for a given
 *                               key would come after all values
 *                               originally from @p dst for the same
 *                               key;
 * - #TE_COMPOUND_MOD_OP_PREPEND: all values from @p src for a given
 *                               key would come before all values
 *                               originally from @p dst for the same
 *                               key;
 * - #TE_COMPOUND_MOD_OP_REPLACE: values from @p src would replace
 *                                values from @p dst for a given key.
 *
 * The function behaves almost exactly as if @p src were iterated
 * and te_compound_set() were called for every item, however,
 * it is much more efficient.
 *
 * @param dst     Destination compound.
 * @param src     Source compound to merge from.
 * @param mod_op  Modification mode.
 */
extern void te_compound_merge(te_string *dst, const te_string *src,
                              te_compound_mod_op mod_op);

/**
 * Function type of callbacks for te_compound_iterate().
 *
 * The callback may freely modify the contents of @p key and
 * @p value, but it must never try to free it or to store pointers
 * to them across the calls, i.e. they should be treated as local
 * arrays.
 *
 * @param key       Current key (may be @c NULL).
 * @param idx       Index of the current value with a given key.
 * @param value     Current value (never @c NULL).
 * @param has_more  @p true if there are more items to process
 *                  (not necessary with the same key),
 * @param user      Callback user data.
 *
 * @return Status code.
 * @retval TE_EOK te_compound_iterate() will stop immediately and
 *                return success to the caller.
 */
typedef te_errno te_compound_iter_fn(char *key, size_t idx,
                                     char *value, bool has_more,
                                     void *user);

/**
 * Iterate over all items in the compound.
 *
 * All unnamed items will be processed before any named one.
 * Named items will be processed in the ascending order of their keys.
 *
 * If a compound is empty, the callback will not be called.
 * If a compound is a non-empty plain string, the callback will be
 * called exactly once, as if it were a single-item array.
 *
 * If a callback returns a non-zero status code, the processing
 * stops and the status code is returned
 * (#TE_EOK being replaced with zero).
 *
 * @param src       Compound string.
 * @param callback  Callback.
 * @param user      Callback data.
 *
 * @return Status code.
 * @retval TE_ENODATA  The compound is empty and the callback was not
 *                     called.
 */
extern te_errno te_compound_iterate(const te_string *src,
                                    te_compound_iter_fn *callback,
                                    void *user);


/**
 * Like te_compound_iterate(), but accepts a plain C string as
 * an input.
 *
 * The reason for this function existence is to avoid creating
 * an intermediate #te_string if a compound string was obtained
 * from some external source (e.g. read from a file).
 *
 * @param src       Compound string representation.
 * @param callback  Callback.
 * @param user      Callback data.
 *
 * @return Status code.
 * @retval TE_ENODATA  The compound is empty and the callback was not
 *                     called.
 *
 * @pre te_compound_validate_str() must return @c true for @p src.
 */
extern te_errno te_compound_iterate_str(const char *src,
                                        te_compound_iter_fn *callback,
                                        void *user);

/**
 * Append a vector of strings to the compound.
 *
 * All existing values in @p src are preserved.
 *
 * @param[out] dst   Destination compound.
 * @param[in]  vec   Vector of string pointers.
 */
extern void te_vec2compound(te_string *dst, const te_vec *vec);

/**
 * Append key-value pairs to the compound.
 *
 * All existing values in @p src are preserved.
 *
 * @param[out] dst   Destination compound.
 * @param[in]  kv    Key-value pairs.
 */
extern void te_kvpair2compound(te_string *dst, const te_kvpair_h *kv);

/**
 * Append all values from the compound to a vector.
 *
 * Values of named items will also be appended after unnamed items,
 * but their keys will be ignored.
 *
 * The values will be copied, so they need to be eventually freed
 * by a vector user.
 *
 * @param[out]   dst        Destination vector of string pointers.
 * @param[in]    compound   Source compound.
 */
extern void te_compound2vec(te_vec *dst, const te_string *compound);

/**
 * Append all values from the compound to a key-value list.
 *
 * Keys for unnamed items will be generated from their indices.
 *
 * @param[out]  dst         Destination key-value list.
 * @param[in]   compound    Source compound.
 *
 * @bug Due to limitations of #te_kvpair_h API, if there are several
 *      values associated with a given key, they will appear in @p dst
 *      in reverse order, so te_compound2kvpair() and te_kvpair2compound()
 *      are not exact inverse of each other.
 */
extern void te_compound2kvpair(te_kvpair_h *dst, const te_string *compound);

/**
 * Serialize a compound as a JSON entity.
 *
 * The result of the function depends on the result of
 * te_compound_classify():
 * - #TE_COMPOUND_NULL: @c null will be produced;
 * - #TE_COMPOUND_PLAIN: a JSON string will be produced;
 * - #TE_COMPOUND_ARRAY: a JSON array will be produced;
 * - #TE_COMPOUND_OBJECT: a JSON object will be produced.
 *
 * If @p compound contains both named and unnamed items, it will be
 * serialized as a JSON object, but unnamed items will have keys generated
 * from their indices.
 *
 * If @p compound contains multiple values associated with some keys,
 * all values for a given key but the first will have an index appended to
 * a key, so the generated JSON would never contain invalid duplicate keys.
 *
 * @param ctx        JSON context.
 * @param compound   Source compound.
 */
extern void te_json_add_compound(te_json_ctx_t *ctx, const te_string *compound);


/**
 * Updates a compound value associated with a key in key-value pair list.
 *
 * The behaviour of this function is essentially the same as
 * te_compound_set_va(), but if there was no value associated with
 * @p outer_key and @p inner_key is @c NULL, the added value will be
 * a plain string, not a one-level array compound.
 *
 * If the compound value is empty after an update, the binding will be
 * removed altogether.
 *
 * If there are multiple bindings for @p outer_key, the function always
 * operates on the most recent one.
 *
 * @param[in,out] dst         Target key-value list.
 * @param[in]     outer_key   The key for @p dst (may not be @c NULL).
 * @param[in]     inner_key   The key for the compound (may be @c NULL).
 * @param[in]     mod_op      Modification mode.
 * @param[in]     val_fmt     Format string (may be @c NULL).
 * @param[in]     args        Variadic list to format.
 *
 * @bug This function is not very efficient due to the way key-value pairs
 *      are implemented.
 */
extern void te_kvpair_set_compound_va(te_kvpair_h *dst, const char *outer_key,
                                      const char *inner_key,
                                      te_compound_mod_op mod_op,
                                      const char *val_fmt, va_list args)
                                      TE_LIKE_VPRINTF(5);

/**
 * Like te_kvpair_set_compound_va(), but accepts variable number of
 * arguments.
 *
 * @param[in,out] dst         Target key-value list.
 * @param[in]     outer_key   The key for @p dst (may not be @c NULL).
 * @param[in]     inner_key   The key for the compound (may be @c NULL).
 * @param[in]     mod_op      Modification mode.
 * @param[in]     val_fmt     Format string (may be @c NULL).
 * @param[in]     ...         Arguments to format.
 */
extern void te_kvpair_set_compound(te_kvpair_h *dst, const char *outer_key,
                                   const char *inner_key,
                                   te_compound_mod_op mod_op,
                                   const char *val_fmt, ...)
                                   TE_LIKE_PRINTF(5, 6);

/**
 * Construct a name that may reliably identify a compound item
 * within some named value.
 *
 * For example, if a parameter named @c param holds a compound
 * with two values for a field @c field, the second value
 * may be addresed as @c param_field1.
 *
 * @param[out]   dst   TE string to append the name.
 * @param[in]    stem  Name stem (i.e. the name of a parameter
 *                     holding a compound value, may not be
 *                     @c NULL)
 * @param[in]    key   Key to address (may be @c NULL).
 * @param[in]    idx   Index of a value in a multi-valued binding.
 */
extern void te_compound_build_name(te_string *dst, const char *stem,
                                   const char *key, size_t idx);

/**
 * Apply a function to a member of compound referenced by @p key.
 *
 * The @p key should have been eventually constructed with
 * te_compound_build_name() with @p stem as an argument, otherwise
 * #TE_ENODATA will be returned.
 *
 * The @p callback has the same signature as for te_compound_iterate(),
 * but at most one field is ever processed.
 *
 * @param compound   Compound string.
 * @param stem       Stem of @p key.
 * @param key        Name referencing a field in @p compound.
 * @param callback   Callback.
 * @param user       User data to callback.
 *
 * @return Status code (usuallaly the one returned by @p callback).
 * @retval TE_ENODATA  It is returned if @p callback has not been called:
 *                     if @p key does not start with @p stem, if there is
 *                     no field with a given name or if there are not
 *                     enough associated values for a given inded.
 */
extern te_errno te_compound_dereference(const te_string *compound,
                                        const char *stem, const char *key,
                                        te_compound_iter_fn *callback,
                                        void *user);

/**
 * Like te_compound_dereference(), but accepts a plain C string
 * as an input.
 *
 * @param compound   Compound string.
 * @param stem       Stem of @p key.
 * @param key        Name referencing a field in @p compound.
 * @param callback   Callback.
 * @param user       User data to callback.
 *
 * @return Status code (see te_compound_dereference() for details).
 */
extern te_errno te_compound_dereference_str(const char *compound,
                                            const char *stem, const char *key,
                                            te_compound_iter_fn *callback,
                                            void *user);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_COMPOUND_H__ */
/**@} <!-- END te_tools_te_compound --> */
