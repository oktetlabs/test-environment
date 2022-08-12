/** @file
 *
 * @brief generic tool options TAPI
 *
 * @defgroup tapi_job_opt Helper functions for handling options
 * @ingroup tapi_job
 * @{
 *
 * TAPI to handle tool options.
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
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
    /** Size of an element in the array */
    size_t element_size;
    /** Separator between array elements */
    const char *sep;
    /** Option bind for every element in the array */
    tapi_job_opt_bind bind;
} tapi_job_opt_array;

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
 */
extern te_errno tapi_job_opt_build_args(const char *path,
                                        const tapi_job_opt_bind *binds,
                                        const void *opt, te_vec *tool_args);

/** Unsigned integer which can be left undefined */
typedef struct tapi_job_opt_uint_t {
    unsigned int value; /**< Value */
    te_bool defined;    /**< If @c TRUE, value is defined */
} tapi_job_opt_uint_t;

/**
 * Defined value for tapi_job_opt_uint_t.
 *
 * @param _x        Value to set.
 */
#define TAPI_JOB_OPT_UINT_VAL(_x) \
    (tapi_job_opt_uint_t){ .value = (_x), .defined = TRUE }

/** Undefined value for tapi_job_opt_uint_t. */
#define TAPI_JOB_OPT_UINT_UNDEF \
    (tapi_job_opt_uint_t){ .defined = FALSE }

/** Double which can be left undefined */
typedef struct tapi_job_opt_double_t {
    double value; /**< Value */
    te_bool defined; /**< If @c TRUE, value is defined */
} tapi_job_opt_double_t;

/**
 * Defined value for tapi_job_opt_double_t.
 *
 * @param _x        Value to set.
 */
#define TAPI_JOB_OPT_DOUBLE_VAL(_x) \
    (tapi_job_opt_double_t){ .value = (_x), .defined = TRUE }

/** Undefined value for tapi_job_opt_double_t. */
#define TAPI_JOB_OPT_DOUBLE_UNDEF \
    (tapi_job_opt_double_t){ .defined = FALSE }

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

/** value type: any enum */
te_errno tapi_job_opt_create_enum(const void *value, const void *priv,
                                  te_vec *args);

/** value type: te_bool */
te_errno tapi_job_opt_create_enum_bool(const void *value, const void *priv,
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
                            _struct, _field)                          \
    { tapi_job_opt_create_uint_t, _prefix, _concat_prefix, _suffix,   \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                 \
                                     tapi_job_opt_uint_t), NULL }

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
                                _struct, _field)                        \
    { tapi_job_opt_create_uint_t_hex, _prefix, _concat_prefix, _suffix, \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                   \
                                     tapi_job_opt_uint_t), NULL }

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
                                  _struct, _field)                        \
    { tapi_job_opt_create_uint_t_octal, _prefix, _concat_prefix, _suffix, \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                     \
                                     tapi_job_opt_uint_t), NULL }

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
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, unsigned int), NULL }

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
                                    _struct, _field)                        \
    { tapi_job_opt_create_uint_omittable, _prefix, _concat_prefix, _suffix, \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, unsigned int), NULL }

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
                            _struct, _field)                            \
    { tapi_job_opt_create_double_t, _prefix, _concat_prefix, _suffix,   \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                   \
                                     tapi_job_opt_double_t), NULL }

/**
 * The value is used to omit uint argument when it is bound
 * with @ref TAPI_JOB_OPT_UINT_OMITTABLE.
 */
#define TAPI_JOB_OPT_OMIT_UINT 0xdeadbeef

/**
 * Bind `te_bool` argument.
 *
 * @param[in]     _prefix   Argument prefix.
 * @param[in]     _struct   Option struct.
 * @param[in]     _field    Field name of the bool in option struct.
 */
#define TAPI_JOB_OPT_BOOL(_prefix, _struct, _field) \
    { tapi_job_opt_create_bool, _prefix, FALSE, NULL,                   \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, te_bool), NULL }

/**
 * Bind `char *` argument.
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate prefix with argument if @c TRUE
 * @param[in]     _struct           Option struct.
 * @param[in]     _field            Field name of the string in option struct.
 */
#define TAPI_JOB_OPT_STRING(_prefix, _concat_prefix, _struct, _field) \
    { tapi_job_opt_create_string, _prefix, _concat_prefix, NULL,      \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field, char *), NULL }

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
    { tapi_job_opt_create_array, NULL, FALSE, NULL,                         \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _lenfield, size_t),           \
      &(tapi_job_opt_array){                                                \
          .array_offset =                                                   \
              TE_COMPILE_TIME_ASSERT_EXPR(offsetof(_struct, _arrfield) >    \
                                          offsetof(_struct, _lenfield)) ?   \
              offsetof(_struct, _arrfield) - offsetof(_struct, _lenfield) : \
              0,                                                            \
          .element_size = TE_SIZEOF_FIELD(_struct, _arrfield[0]),           \
          .bind = __VA_ARGS__ } }


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
                                 _struct, _lenfield, _arrfield, ...)        \
    { tapi_job_opt_create_embed_array, _prefix, _concat_prefix, _suffix,    \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _lenfield, size_t),           \
      &(tapi_job_opt_array){                                                \
          .array_offset =                                                   \
              TE_COMPILE_TIME_ASSERT_EXPR(offsetof(_struct, _arrfield) >    \
                                          offsetof(_struct, _lenfield)) ?   \
              offsetof(_struct, _arrfield) - offsetof(_struct, _lenfield) : \
              0,                                                            \
          .element_size = TE_SIZEOF_FIELD(_struct, _arrfield[0]),           \
          .sep = (_sep),                                                    \
          .bind = __VA_ARGS__ } }

/**
 * Bind none argument.
 *
 * @param[in]     _prefix           Option name.
 */
#define TAPI_JOB_OPT_DUMMY(_prefix) \
    { tapi_job_opt_create_dummy, _prefix, FALSE, NULL, 0, NULL }

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
    { tapi_job_opt_create_sockaddr_ptr, _prefix, _concat_prefix, NULL,      \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                       \
                                     struct sockaddr *), NULL }

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
    { tapi_job_opt_create_addr_port_ptr, _prefix, _concat_prefix, NULL,      \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                        \
                                     struct sockaddr *), NULL }

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
    { tapi_job_opt_create_sockport_ptr, _prefix, _concat_prefix, NULL,      \
      TAPI_JOB_OPT_OFFSETOF_CHK_SIZE(_struct, _field,                       \
                                     struct sockaddr *), NULL }

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


/**@} <!-- END tapi_job_opt_bind_constructors --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_JOB_OPT_H__ */

/**@} <!-- END tapi_job_opt --> */
