/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2019-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 *
 * @brief generic tool options TAPI
 *
 * @defgroup tapi_job_opt Helper functions for handling options
 * @ingroup tapi_job
 * @{
 *
 * TAPI to handle tool options.
 */

#ifndef __TE_TAPI_JOB_OPT_H__
#define __TE_TAPI_JOB_OPT_H__

#include "te_defs.h"
#include "te_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Formatting function pointer: put formatted command-line argument in a vector.
 *
 * @param[in]  opt          Pointer to an argument, with specific type for
 *                          every formatting function.
 * @param[in]  priv         Pointer to private data that may be needed for
 *                          a formatting function.
 * @param[out] arg          Vector to put formatted argument to. Functions are
 *                          allowed to put multiple string in the vector. In
 *                          that case multiple arguments will be created.
 *
 * @return                  Status code
 * @retval  TE_ENOENT       The argument must be skipped (along with
 *                          prefix/suffix);
 */
typedef te_errno (*tapi_job_opt_arg_format)(const void *opt, const void *priv,
                                            te_vec *arg);

/**
 * Bind between a tool's option struct field and command line arguments.
 *
 * Field here is a member of a struct. The struct is the tool's custom
 * struct with command line options.
 */
typedef struct tapi_job_opt_bind {
    /** Formatting function */
    tapi_job_opt_arg_format fmt_func;
    /** Argument prefix */
    const char *prefix;
    /** Concatenate prefix with the argument */
    te_bool concatenate_prefix;
    /** Argument suffix (always concatenated with the argument) */
    const char *suffix;
    /** Offset of a field */
    size_t opt_offset;
    /** Private data for value conversion */
    const void *priv;
} tapi_job_opt_bind;

/**
 * The descriptor for an array-typed field.
 * This structure shall *never* be used directly,
 * see TAPI_JOB_OPT_ARRAY()
 */
typedef struct tapi_job_opt_array {
    /** The offset of the data field relative to the length */
    size_t array_offset;
    /** The data field contains a pointer to an array. */
    te_bool is_ptr;
    /** Size of an element in the array */
    size_t element_size;
    /** Separator between array elements */
    const char *sep;
    /** Option bind for every element in the array */
    tapi_job_opt_bind bind;
} tapi_job_opt_array;

/**
 * The descriptor for an struct-typed field.
 * This structure shall *never* be used directly,
 * see TAPI_JOB_OPT_STRUCT()
 */
typedef struct tapi_job_opt_struct {
    /** Separator between array elements */
    const char *sep;
    /** Options bind for all elements in the structure */
    tapi_job_opt_bind *binds;
} tapi_job_opt_struct;

/** A convenience vector constructor to define option binds */
#define TAPI_JOB_OPT_SET(...)                                              \
    {__VA_ARGS__, {NULL, NULL, NULL, FALSE, 0, NULL}}

/**
 * Create a vector with command line arguments by processing option binds.
 *
 * @param[in]  path         Tool path (program location).
 * @param[in]  binds        Binds between @p opt and produced arguments,
 *                          or @c NULL.
 * @param[in]  opt          Tool's custom option struct which is forwarded
 *                          to argument formatting callback, or @c NULL
 *                          if the previous argument is @c NULL.
 * @param[out] tool_args    Vector with command line arguments.
 *
 * @return Status code.
 *
 * @note @p tool_args contents is overwritten by this function, so
 *       it should have no elements prior to the call, or a memory
 *       leak would ensue.
 *
 * @sa tapi_job_opt_append_strings()
 * @sa tapi_job_opt_append_args()
 */
extern te_errno tapi_job_opt_build_args(const char *path,
                                        const tapi_job_opt_bind *binds,
                                        const void *opt, te_vec *tool_args);

/**
 * Append a list of strings to an already built argument vector.
 *
 * The terminating @c NULL is moved accordingly.
 *
 * @param[in]      items      @c NULL-terminated array of strings
 *                            (they are strdup'ed in the vector).
 * @param[in,out]  tool_args  Argument vector (must be initialized).
 *
 * @return Status code
 *
 * @sa tapi_job_opt_build_args()
 */
extern te_errno tapi_job_opt_append_strings(const char **items,
                                            te_vec *tool_args);

/**
 * Append options to an already built argument vector.
 *
 * The terminating @c NULL is moved accordingly.
 *
 *
 * @param[in]     binds        Binds between @p opt and produced arguments,
 *                             or @c NULL.
 * @param[in]     opt          Tool's custom option struct which is forwarded
 *                             to argument formatting callback, or @c NULL
 *                             if the previous argument is @c NULL.
 * @param[in,out] tool_args    Argument vector (must be initialized).
 *
 * @return Status code.
 *
 * @sa tapi_job_opt_build_args()
 */
extern te_errno tapi_job_opt_append_args(const tapi_job_opt_bind *binds,
                                         const void *opt, te_vec *tool_args);


/** Unsigned integer which can be left undefined.
 *
 * This is guaranteed to be assignment-compatible with te_optional_uint_t.
 */
typedef te_optional_uint_t tapi_job_opt_uint_t;

/**
 * Defined value for tapi_job_opt_uint_t.
 *
 * @param _x        Value to set.
 */
#define TAPI_JOB_OPT_UINT_VAL(_x) TE_OPTIONAL_UINT_VAL(_x)

/** Undefined value for tapi_job_opt_uint_t. */
#define TAPI_JOB_OPT_UINT_UNDEF TE_OPTIONAL_UINT_UNDEF

/** Unsigned long integer which can be left undefined.
 *
 * This is guaranteed to be assignment-compatible with te_optional_uintmax_t.
 */
typedef te_optional_uintmax_t tapi_job_opt_uintmax_t;

/**
 * Defined value for tapi_job_opt_uintmax_t.
 *
 * @param _x        Value to set.
 */
#define TAPI_JOB_OPT_UINTMAX_VAL(_x) TE_OPTIONAL_UINTMAX_VAL(_x)

/** Undefined value for tapi_job_opt_uintmax_t. */
#define TAPI_JOB_OPT_UINTMAX_UNDEF TE_OPTIONAL_UINTMAX_UNDEF

/** Double which can be left undefined.
 *
 * This is guaranteed to be assignment-compatible with te_optional_double_t.
 */
typedef te_optional_double_t tapi_job_opt_double_t;

/**
 * Defined value for tapi_job_opt_double_t.
 *
 * @param _x        Value to set.
 */
#define TAPI_JOB_OPT_DOUBLE_VAL(_x) TE_OPTIONAL_DOUBLE_VAL(_x)

/** Undefined value for tapi_job_opt_double_t. */
#define TAPI_JOB_OPT_DOUBLE_UNDEF TE_OPTIONAL_DOUBLE_UNDEF

/**
 * @defgroup tapi_job_opt_formatting functions for argument formatting
 * @{
 *
 * @param[in]     value     Pointer to an argument.
 * @param[in,out] args      Argument vector to which formatted argument
 *                          is appended.
 */

/** value type: `tapi_job_opt_uint_t` */
te_errno tapi_job_opt_create_uint_t(const void *value, const void *priv,
                                    te_vec *args);

/** value type: `tapi_job_opt_uint_t` */
te_errno tapi_job_opt_create_uint_t_hex(const void *value, const void *priv,
                                        te_vec *args);

/** value type: `tapi_job_opt_uint_t` */
te_errno tapi_job_opt_create_uint_t_octal(const void *value, const void *priv,
                                          te_vec *args);

/** value type: `unsigned int` */
te_errno tapi_job_opt_create_uint(const void *value, const void *priv,
                                  te_vec *args);

/** value type: `tapi_job_opt_uintmax_t` */
te_errno tapi_job_opt_create_uintmax_t(const void *value, const void *priv,
                                       te_vec *args);

/** value type: `unsigned int`, may be omitted */
te_errno tapi_job_opt_create_uint_omittable(const void *value, const void *priv,
                                            te_vec *args);

/** value type: `tapi_job_opt_double_t` */
te_errno tapi_job_opt_create_double_t(const void *value, const void *priv,
                                      te_vec *args);

/** value type: `char *` */
te_errno tapi_job_opt_create_string(const void *value, const void *priv,
                                    te_vec *args);

/** value type: `te_bool` */
te_errno tapi_job_opt_create_bool(const void *value, const void *priv,
                                  te_vec *args);

/** value type: an inline array */
te_errno tapi_job_opt_create_array(const void *value, const void *priv,
                                   te_vec *args);

/** Value type: an inline array */
te_errno tapi_job_opt_create_embed_array(const void *value, const void *priv,
                                         te_vec *args);

/** Value type: an inline struct */
te_errno tapi_job_opt_create_struct(const void *value, const void *priv,
                                    te_vec *args);

/** value type: none */
te_errno tapi_job_opt_create_dummy(const void *value, const void *priv,
                                   te_vec *args);

/** value type: 'struct sockaddr *' */
te_errno tapi_job_opt_create_sockaddr_ptr(const void *value, const void *priv,
                                          te_vec *args);

/** value type: 'struct sockaddr *' */
te_errno tapi_job_opt_create_addr_port_ptr(const void *value, const void *priv,
                                           te_vec *args);

/** value type: 'struct sockaddr *' */
te_errno tapi_job_opt_create_sockport_ptr(const void *value, const void *priv,
                                          te_vec *args);

/** Value type: te_sockaddr_subnet */
te_errno tapi_job_opt_create_sockaddr_subnet(const void *value,
                                             const void *priv,
                                             te_vec *args);

/** value type: any enum */
te_errno tapi_job_opt_create_enum(const void *value, const void *priv,
                                  te_vec *args);

/** value type: te_bool */
te_errno tapi_job_opt_create_enum_bool(const void *value, const void *priv,
                                       te_vec *args);

/** value type: te_bool3 */
extern te_errno tapi_job_opt_create_enum_bool3(const void *value,
                                               const void *priv,
                                               te_vec *args);

/**@} <!-- END tapi_job_opt_formatting --> */

/**
 * @defgroup tapi_job_opt_bind_constructors convenience macros for option
 *                                          description
 * @{
 */


/**
 * An auxiliary wrapper that provides an offset of the @p _field
 * in the @p _struct statically checking that its size is the same
 * as the size of @p _exptype
 */
#define TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, _exptype)    \
    (TE_COMPILE_TIME_ASSERT_EXPR(TE_SIZEOF_FIELD(_struct, _field) == \
                                 sizeof(_exptype)) ?                 \
     offsetof(_struct, _field) : 0)

/** Value type used in TAPI_JOB_OPT_UINT_T(). */
#define TAPI_JOB_OPT_UINT_T_TYPE tapi_job_opt_uint_t

/**
 * Bind `tapi_job_opt_uint_t` argument.
 *
 * @param[in] _prefix         Argument prefix.
 * @param[in] _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in] _suffix         Argument suffix.
 * @param[in] _struct         Option struct.
 * @param[in] _field          Field name in option struct.
 */
#define TAPI_JOB_OPT_UINT_T(_prefix, _concat_prefix, _suffix, \
                            _struct, _field)                           \
    { tapi_job_opt_create_uint_t, _prefix, _concat_prefix, _suffix,    \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                  \
                                     TAPI_JOB_OPT_UINT_T_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_UINT_T_HEX(). */
#define TAPI_JOB_OPT_UINT_T_HEX_TYPE tapi_job_opt_uint_t

/**
 * Bind `tapi_job_opt_uint_t` argument, specifying it in hexadecimal
 * format in command line.
 *
 * @param[in] _prefix         Argument prefix.
 * @param[in] _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in] _suffix         Argument suffix.
 * @param[in] _struct         Option struct.
 * @param[in] _field          Field name in option struct.
 */
#define TAPI_JOB_OPT_UINT_T_HEX(_prefix, _concat_prefix, _suffix, \
                                _struct, _field)                           \
    { tapi_job_opt_create_uint_t_hex, _prefix, _concat_prefix, _suffix,    \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                      \
                                     TAPI_JOB_OPT_UINT_T_HEX_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_UINT_T_OCTAL(). */
#define TAPI_JOB_OPT_UINT_T_OCTAL_TYPE tapi_job_opt_uint_t

/**
 * Bind `tapi_job_opt_uint_t` argument, specifying it in octal
 * format in command line.
 *
 * @param[in] _prefix         Argument prefix.
 * @param[in] _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in] _suffix         Argument suffix.
 * @param[in] _struct         Option struct.
 * @param[in] _field          Field name in option struct.
 */
#define TAPI_JOB_OPT_UINT_T_OCTAL(_prefix, _concat_prefix, _suffix, \
                                  _struct, _field)                           \
    { tapi_job_opt_create_uint_t_octal, _prefix, _concat_prefix, _suffix,    \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                        \
                                     TAPI_JOB_OPT_UINT_T_OCTAL_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_UINTMAX_T(). */
#define TAPI_JOB_OPT_UINTMAX_T_TYPE tapi_job_opt_uintmax_t

/**
 * Bind `tapi_job_opt_uintmax_t` argument.
 *
 * @param[in] _prefix         Argument prefix.
 * @param[in] _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in] _suffix         Argument suffix.
 * @param[in] _struct         Option struct.
 * @param[in] _field          Field name in option struct.
 */
#define TAPI_JOB_OPT_UINTMAX_T(_prefix, _concat_prefix, _suffix, \
                               _struct, _field)                           \
    { tapi_job_opt_create_uintmax_t, _prefix, _concat_prefix, _suffix,    \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                     \
                                     TAPI_JOB_OPT_UINTMAX_T_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_UINT(). */
#define TAPI_JOB_OPT_UINT_TYPE unsigned int

/**
 * Bind `unsigned int` argument.
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate prefix with argument if @c TRUE
 * @param[in]     _suffix           Argument suffix.
 * @param[in]     _struct           Option struct.
 * @param[in]     _field            Field name of the uint in option struct.
 */
#define TAPI_JOB_OPT_UINT(_prefix, _concat_prefix, _suffix, _struct, _field) \
    { tapi_job_opt_create_uint, _prefix, _concat_prefix, _suffix,            \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                        \
                                     TAPI_JOB_OPT_UINT_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_UINT_OMITTABLE(). */
#define TAPI_JOB_OPT_UINT_OMITTABLE_TYPE unsigned int

/**
 * Similar to @ref TAPI_JOB_OPT_UINT, but the argument will not be included in
 * command line options if argument's value is equal to
 * @ref TAPI_JOB_OPT_OMIT_UINT. This implies that the real argument's value
 * cannot be equal to the value of @ref TAPI_JOB_OPT_OMIT_UINT.
 *
 * @note This macro is deprecated, use tapi_job_uint_t with
 *       TAPI_JOB_OPT_UINT_T() instead.
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate prefix with argument if @c TRUE
 * @param[in]     _suffix           Argument suffix.
 * @param[in]     _struct           Option struct.
 * @param[in]     _field            Field name of the uint in option struct.
 */
#define TAPI_JOB_OPT_UINT_OMITTABLE(_prefix, _concat_prefix, _suffix, \
                                    _struct, _field)                           \
    { tapi_job_opt_create_uint_omittable, _prefix, _concat_prefix, _suffix,    \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                          \
                                     TAPI_JOB_OPT_UINT_OMITTABLE_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_DOUBLE(). */
#define TAPI_JOB_OPT_DOUBLE_TYPE tapi_job_opt_double_t

/**
 * Bind `tapi_job_opt_double_t` argument.
 *
 * @param[in] _prefix         Argument prefix.
 * @param[in] _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in] _suffix         Argument suffix.
 * @param[in] _struct         Option struct.
 * @param[in] _field          Field name in option struct.
 */
#define TAPI_JOB_OPT_DOUBLE(_prefix, _concat_prefix, _suffix, \
                            _struct, _field)                           \
    { tapi_job_opt_create_double_t, _prefix, _concat_prefix, _suffix,  \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                  \
                                     TAPI_JOB_OPT_DOUBLE_TYPE), NULL }

/**
 * The value is used to omit uint argument when it is bound
 * with @ref TAPI_JOB_OPT_UINT_OMITTABLE.
 */
#define TAPI_JOB_OPT_OMIT_UINT 0xdeadbeef

/** Value type used in TAPI_JOB_OPT_BOOL(). */
#define TAPI_JOB_OPT_BOOL_TYPE te_bool

/**
 * Bind `te_bool` argument.
 *
 * @param[in]     _prefix   Argument prefix.
 * @param[in]     _struct   Option struct.
 * @param[in]     _field    Field name of the bool in option struct.
 */
#define TAPI_JOB_OPT_BOOL(_prefix, _struct, _field) \
    { tapi_job_opt_create_bool, _prefix, FALSE, NULL,                \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                \
                                     TAPI_JOB_OPT_BOOL_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_STRING(). */
#define TAPI_JOB_OPT_STRING_TYPE char *

/**
 * Bind `char *` argument.
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate prefix with argument if @c TRUE
 * @param[in]     _struct           Option struct.
 * @param[in]     _field            Field name of the string in option struct.
 */
#define TAPI_JOB_OPT_STRING(_prefix, _concat_prefix, _struct, _field) \
    { tapi_job_opt_create_string, _prefix, _concat_prefix, NULL,       \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                  \
                                     TAPI_JOB_OPT_STRING_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_QUOTED_STRING(). */
#define TAPI_JOB_OPT_QUOTED_STRING_TYPE char *

/**
 * Bind @ref char * argument (formatted as
 * @c <quotation_mark>string<quotation_mark>).
 *
 * @param[in] prefix_            Argument prefix.
 * @param[in] quotation_mark_    Quote symbol.
 * @param[in] struct_            Option struct.
 * @param[in] field_             Field name of the string in option struct.
 */
#define TAPI_JOB_OPT_QUOTED_STRING(prefix_, quotation_mark_, struct_, field_) \
    { tapi_job_opt_create_string,                                             \
      prefix_ quotation_mark_, TRUE, quotation_mark_,                         \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(struct_, field_,                         \
                                     TAPI_JOB_OPT_QUOTED_STRING_TYPE), NULL }

/**
 * An adaptor for indirect struct fields.
 *
 * Macros for option formatting would not work with indirect non-struct values,
 * since they calculate offset of a field from the start of a struct.
 *
 * This adaptor creates an anonymous structure with a field of a correct type.
 * For example, there is the following structure with a field:
 * @code
 *     typedef struct binds {
 *         size_t argc;
 *         const char **argv;
 *     } binds;
 * @endcode
 * Where @c argv is an pointer to an array of strings of @c argc length.
 * This field can be processed as follows:
 * @code
 *     TAPI_JOB_OPT_ARRAY_PTR(binds, argc, argv,
 *         TAPI_JOB_OPT_CONTENT(TAPI_JOB_OPT_STRING(NULL, FALSE))
 * @endcode
 *
 * @note New formatting macros shall define a `*_TYPE` macro for this adaptor to
 *       work (for example, @ref TAPI_JOB_OPT_STRING_TYPE()).
 *
 * @param optname_    Name of the macro.
 * @param ...         Arguments required for the macro.
 *                    Except for the last two: type and field.
 */
#define TAPI_JOB_OPT_CONTENT(optname_, ...) \
    optname_(__VA_ARGS__, struct { optname_##_TYPE __item; }, __item) \

/**
 * Generic macro to bind tapi_job_opt_array argument.
 *
 * @note This macros shall *never* be used directly.
 *
 * @sa TAPI_JOB_OPT_ARRAY(), TAPI_JOB_OPT_EMBED_ARRAY(),
 *     TAPI_JOB_OPT_ARRAY_PTR(), TAPI_JOB_OPT_EMBED_ARRAY_PTR(),
 */
#define TAPI_JOB_OPT_ARRAY_GEN(func_, prefix_, concat_prefix_, sep_, suffix_, \
                               struct_, lenfield_, arrfield_, is_ptr_, ...)   \
    { func_, prefix_, concat_prefix_, suffix_,                                \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(struct_, lenfield_, size_t),             \
      &(tapi_job_opt_array){                                                  \
          .array_offset =                                                     \
              TE_COMPILE_TIME_ASSERT_EXPR(offsetof(struct_, arrfield_) >      \
                                          offsetof(struct_, lenfield_)) ?     \
              offsetof(struct_, arrfield_) - offsetof(struct_, lenfield_) :   \
              0,                                                              \
          .element_size = TE_SIZEOF_FIELD(struct_, arrfield_[0]),             \
          .is_ptr = (is_ptr_),                                                \
          .sep = (sep_),                                                      \
          .bind = __VA_ARGS__ } }

/**
 * Bind `tapi_job_opt_array` argument.
 *
 * @param[in]     _struct           Option struct.
 * @param[in]     _lenfield         Field name of the array length in
 *                                  the option struct (`size_t`).
 * @param[in]     _arrfield         Field name of the array data field.
 *                                  It must be the genuine inline array,
 *                                  not a pointer, and it must come after
 *                                  the @p _lenfield
 * @param         ...               Binding for a element
 *                                  (must be a valid initializer list for
 *                                  tapi_job_opt_bind, e.g as provided
 *                                  by convenience macros; the argument is
 *                                  a vararg because members of the list
 *                                  are treated by the preprocessor as
 *                                  individual macro arguments).
 *                                  The offset of the binding shall be that
 *                                  of `_arrfield[0]` (mind the index) for
 *                                  field size checks to work correctly.
 */
#define TAPI_JOB_OPT_ARRAY(_struct, _lenfield, _arrfield, ...) \
    TAPI_JOB_OPT_ARRAY_GEN(tapi_job_opt_create_array,                         \
                           NULL, FALSE, NULL, NULL,                           \
                           _struct, _lenfield, _arrfield, FALSE, __VA_ARGS__)


/**
 * Bind `tapi_job_opt_array` argument.
 *
 * Unlike TAPI_JOB_OPT_ARRAY(), all array elements will be packed into
 * a single argument, separated by @p _sep. This combined argument may
 * have a @p _prefix and a @p _suffix, like e.g. TAPI_JOB_OPT_UINT()
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate the prefix with an argument
 *                                  if @c TRUE.
 * @param[in]     _sep              Array element separator.
 * @param[in]     _suffix           Argument suffix.
 * @param[in]     _struct           Option struct.
 * @param[in]     _lenfield         Field name of the array length in
 *                                  the option struct (`size_t`).
 * @param[in]     _arrfield         Field name of the array data field.
 *                                  It must be the genuine inline array,
 *                                  not a pointer, and it must come after
 *                                  the @p _lenfield.
 * @param         ...               Binding for a element
 *                                  (must be a valid initializer list for
 *                                  tapi_job_opt_bind, e.g as provided
 *                                  by convenience macros; the argument is
 *                                  a vararg because members of the list
 *                                  are treated by the preprocessor as
 *                                  individual macro arguments).
 *                                  The offset of the binding shall be that
 *                                  of `_arrfield[0]` (mind the index) for
 *                                  field size checks to work correctly.
 */
#define TAPI_JOB_OPT_EMBED_ARRAY(_prefix, _concat_prefix, _sep, _suffix, \
                                 _struct, _lenfield, _arrfield, ...)          \
    TAPI_JOB_OPT_ARRAY_GEN(tapi_job_opt_create_embed_array,                   \
                           _prefix, _concat_prefix, _sep, _suffix,            \
                           _struct, _lenfield, _arrfield, FALSE, __VA_ARGS__)

/**
 * Bind tapi_job_opt_array argument with pointer to array.
 *
 * @param[in] struct_      Option struct.
 * @param[in] lenfield_    Field name of the array length in
 *                         the option struct (@ref size_t).
 * @param[in] arrfield_    Field name of the pointer to array data field.
 *                         It must be the pointer to array,
 *                         and it must come after the @p lenfield_.
 * @param     ...          Binding for a element (must be a valid initializer
 *                         list for tapi_job_opt_bind, e.g as provided
 *                         by convenience macros; the argument is a vararg
 *                         because members of the list are treated by the
 *                         preprocessor as individual macro arguments).
 *                         To set the correct offset, use TAPI_JOB_OPT_CONTENT()
 *                         wrapper.
 */
#define TAPI_JOB_OPT_ARRAY_PTR(struct_, lenfield_, arrfield_, ...) \
    TAPI_JOB_OPT_ARRAY_GEN(tapi_job_opt_create_array,                        \
                           NULL, FALSE, NULL, NULL,                          \
                           struct_, lenfield_, arrfield_, TRUE, __VA_ARGS__)

/**
 * Bind tapi_job_opt_array argument with pointer to array.
 *
 * Unlike TAPI_JOB_OPT_ARRAY_PTR(), all array elements will be packed into
 * a single argument, separated by @p sep_. This combined argument may
 * have a @p prefix_ and a @p suffix_, like e.g. TAPI_JOB_OPT_UINT().
 *
 * @param[in] prefix_           Argument prefix.
 * @param[in] concat_prefix_    Concatenate the prefix with an argument
 *                              if @c TRUE.
 * @param[in] sep_              Array element separator.
 * @param[in] suffix_           Argument suffix.
 * @param[in] struct_           Option struct.
 * @param[in] lenfield_         Field name of the array length in
 *                              the option struct (@ref size_t).
 * @param[in] arrfield_         Field name of the pointer to array data field.
 *                              It must be the pointer to array,
 *                              and it must come after the @p lenfield_.
 * @param     ...               Binding for a element
 *                              (must be a valid initializer list for
 *                              tapi_job_opt_bind, e.g as provided
 *                              by convenience macros; the argument is
 *                              a vararg because members of the list
 *                              are treated by the preprocessor as
 *                              individual macro arguments).
 *                              To set the correct offset, use
 *                              TAPI_JOB_OPT_CONTENT() wrapper.
 */
#define TAPI_JOB_OPT_EMBED_ARRAY_PTR(prefix_, concat_prefix_, sep_, suffix_, \
                                     struct_, lenfield_, arrfield_, ...)     \
    TAPI_JOB_OPT_ARRAY_GEN(tapi_job_opt_create_embed_array,                  \
                           prefix_, concat_prefix_, sep_, suffix_,           \
                           struct_, lenfield_, arrfield_, TRUE, __VA_ARGS__)

/**
 * Bind `tapi_job_opt_struct` argument.
 *
 * Unlike TAPI_JOB_OPT_ARRAY() and TAPI_JOB_OPT_EMBED_ARRAY(),
 * it allows to create structures with different types of variables
 * and output them using a separator @p _sep. This combined argument
 * may have a @p _prefix and a @p _suffix, like e.g. TAPI_JOB_OPT_UINT().
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate the prefix with an argument
 *                                  if @c TRUE.
 * @param[in]     _sep              Array element separator.
 * @param[in]     _suffix           Argument suffix.
 * @param         ...               Bindings for elements
 *                                  (must be a valid initializer list for
 *                                  tapi_job_opt_bind, e.g as provided
 *                                  by convenience macros; the argument is
 *                                  a vararg because members of the list
 *                                  are treated by the preprocessor as
 *                                  individual macro arguments).
 */
#define TAPI_JOB_OPT_STRUCT(_prefix, _concat_prefix, _sep, _suffix, ...) \
    { tapi_job_opt_create_struct, _prefix, _concat_prefix, _suffix, 0,   \
      &(tapi_job_opt_struct){                                            \
          .sep = (_sep),                                                 \
          .binds = (tapi_job_opt_bind[])TAPI_JOB_OPT_SET(__VA_ARGS__) } }

/**
 * Bind none argument.
 *
 * @param[in]     _prefix           Option name.
 */
#define TAPI_JOB_OPT_DUMMY(_prefix) \
    { tapi_job_opt_create_dummy, _prefix, FALSE, NULL, 0, NULL }

/** Value type used in TAPI_JOB_OPT_SOCKADDR_PTR(). */
#define TAPI_JOB_OPT_SOCKADDR_PTR_TYPE struct sockaddr *

/**
 * Bind `struct sockaddr *` argument.
 * The argument won't be included to command line if the field is @c NULL.
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate prefix with argument if @c TRUE
 * @param[in]     _struct           Option struct.
 * @param[in]     _field            Field name of the string in option struct.
 */
#define TAPI_JOB_OPT_SOCKADDR_PTR(_prefix, _concat_prefix, _struct, _field) \
    { tapi_job_opt_create_sockaddr_ptr, _prefix, _concat_prefix, NULL,       \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                        \
                                     TAPI_JOB_OPT_SOCKADDR_PTR_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_ADDR_PORT_PTR(). */
#define TAPI_JOB_OPT_ADDR_PORT_PTR_TYPE struct sockaddr *

/**
 * Bind `struct sockaddr *` argument (formatted as "address:port").
 * The argument won't be included in command line if the field is @c NULL.
 *
 * @param[in]   _prefix         Argument prefix.
 * @param[in]   _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in]   _struct         Option struct.
 * @param[in]   _field          Field name of the address in option struct.
 */
#define TAPI_JOB_OPT_ADDR_PORT_PTR(_prefix, _concat_prefix, _struct, _field) \
    { tapi_job_opt_create_addr_port_ptr, _prefix, _concat_prefix, NULL,       \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                         \
                                     TAPI_JOB_OPT_ADDR_PORT_PTR_TYPE), NULL }

/** Value type used in TAPI_JOB_OPT_SOCKPORT_PTR(). */
#define TAPI_JOB_OPT_SOCKPORT_PTR_TYPE struct sockaddr *

/**
 * Bind `struct sockaddr *` argument (formatted as "port").
 * The argument won't be included in command line if the field is @c NULL.
 *
 * @param[in]   _prefix         Argument prefix.
 * @param[in]   _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in]   _struct         Option struct.
 * @param[in]   _field          Field name of the address in option struct.
 */
#define TAPI_JOB_OPT_SOCKPORT_PTR(_prefix, _concat_prefix, _struct, _field) \
    { tapi_job_opt_create_sockport_ptr, _prefix, _concat_prefix, NULL,       \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                        \
                                     TAPI_JOB_OPT_SOCKPORT_PTR_TYPE), NULL }

/**
 * Value type used in TAPI_JOB_OPT_SOCKADDR_SUBNET().
 *
 * @ingroup tapi_job_opt_bind_constructors_wrappers
 */
#define TAPI_JOB_OPT_SOCKADDR_SUBNET_TYPE te_sockaddr_subnet

/**
 * Bind te_sockaddr_subnet argument (formatted as @c addr/prefix_len).
 *
 * @note The argument won't be included in command line if the field is @c NULL.
 *
 * @param[in] prefix_           Argument prefix.
 * @param[in] concat_prefix_    Concatenate prefix with argument if @c TRUE.
 * @param[in] struct_           Option struct.
 * @param[in] field_            Field name of the subnet in option struct.
 */
#define TAPI_JOB_OPT_SOCKADDR_SUBNET(prefix_, concat_prefix_, struct_, field_) \
    { tapi_job_opt_create_sockaddr_subnet, prefix_, concat_prefix_, NULL,      \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(struct_, field_,                          \
                                     TAPI_JOB_OPT_SOCKADDR_SUBNET_TYPE), NULL }

/** Undefined value for enum option */
#define TAPI_JOB_OPT_ENUM_UNDEF INT_MIN

/**
 * Bind an enumeration argument using a custom mapping @p _map from values
 * to strings.
 *
 * @param[in]   _prefix         Argument prefix.
 * @param[in]   _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in]   _struct         Option struct.
 * @param[in]   _field          Field name of the enumeration in option struct.
 *                              The field must have the same size as int.
 *                              In most (if not all) standard ABIs this
 *                              means `enum`-typed fields may be freely used.
 *                              There is a compile-check for that constraint.
 * @param[in]   _map            Enumeration mapping (te_enum_map array)
 */
#define TAPI_JOB_OPT_ENUM(_prefix, _concat_prefix, _struct, _field, _map) \
    { tapi_job_opt_create_enum, _prefix, _concat_prefix, NULL,            \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, int), (_map) }

/**
 * Bind a boolean argument using a custom mapping @p _map from values
 * to strings.
 *
 * @note Constrast with TAPI_JOB_OPT_BOOL which adds or omits
 *       the parameterless option depending on the booleanb value.
 *
 * @param[in]   _prefix         Argument prefix.
 * @param[in]   _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in]   _struct         Option struct.
 * @param[in]   _field          Field name of the boolean in option struct.
 *                              The field must have the same size as int.
 *                              In most (if not all) standard ABIs this
 *                              means `enum`-typed fields may be freely used.
 *                              There is a compile-check for that constraint.
 * @param[in]   _map            Enumeration mapping (te_enum_map array).
 *                              The map shall contain two elements for
 *                              `TRUE` and `FALSE`
 *
 */
#define TAPI_JOB_OPT_ENUM_BOOL(_prefix, _concat_prefix, _struct, _field, _map) \
    { tapi_job_opt_create_enum_bool, _prefix, _concat_prefix, NULL,            \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, te_bool), (_map) }

/**
 * Bind a ternary boolean argument using a custom mapping @p _map from values
 * to strings.
 *
 * @param[in]   _prefix         Argument prefix.
 * @param[in]   _concat_prefix  Concatenate prefix with argument if @c TRUE.
 * @param[in]   _struct         Option struct.
 * @param[in]   _field          Field name of the boolean in option struct.
 *                              The field must have the same size as int.
 *                              In most (if not all) standard ABIs this
 *                              means `enum`-typed fields may be freely used.
 *                              There is a compile-check for that constraint.
 * @param[in]   _map            Enumeration mapping (te_enum_map array).
 *                              The map shall contain two elements for
 *                              `TE_BOOL3_TRUE` and `TE_BOOL3_FALSE`.
 *
 */
#define TAPI_JOB_OPT_ENUM_BOOL3(_prefix, _concat_prefix, _struct, _field, _map)\
    { tapi_job_opt_create_enum_bool3, _prefix, _concat_prefix, NULL,           \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, te_bool3), (_map) }

/**@} <!-- END tapi_job_opt_bind_constructors --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_JOB_OPT_H__ */

/**@} <!-- END tapi_job_opt --> */
