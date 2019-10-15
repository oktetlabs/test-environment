/** @file
 *
 * @brief generic tool options TAPI
 *
 * @defgroup tapi_job_opt helper functions for handling options
 * @ingroup te_ts_tapi
 * @{
 *
 * TAPI to handle tool options.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
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
 * @param[out] arg          Vector to put formatted argument to. Functions are
 *                          allowed to put multiple string in the vector. In
 *                          that case multiple arguments will be created.
 *
 * @return                  Status code
 * @retval  TE_ENOENT       The argument must be skipped (along with
 *                          prefix/suffix);
 */
typedef te_errno (*tapi_job_opt_arg_format)(const void *opt, te_vec *arg);

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
} tapi_job_opt_bind;

/**
 * Array of arguments.
 *
 * @note    Only array-type field members are supported, pointers
 *          to an array are not.
 */
typedef struct tapi_job_opt_array {
    /** Size of an array */
    size_t array_length;
    /** Size of an element in the array */
    size_t element_size;
    /**
     * Option bind for every element in the array.
     *
     * @note    the bind's offset is the offset of the array member.
     */
    tapi_job_opt_bind bind;
} tapi_job_opt_array;

/** A convenience vector constructor to define option binds */
#define TAPI_JOB_OPT_SET(...)                                              \
    {__VA_ARGS__, {NULL, NULL, NULL, FALSE, 0}}

/**
 * Create a vector with command line arguments by processing option binds.
 *
 * @param[in]  path         Tool path (program location).
 * @param[in]  binds        Binds between @p opt and produced arguments.
 * @param[in]  opt          Tool's custom option struct which is forwarded
 *                          to argument formatting callback.
 * @param[out] tool_args    Vector with command line arguments.
 *
 * @return Status code.
 */
extern te_errno tapi_job_opt_build_args(const char *path,
                                         const tapi_job_opt_bind *binds,
                                         const void *opt, te_vec *tool_args);

/**
 * Release a vector with command line arguments.
 *
 * @param args      Vector with command line arguments
 */
extern void tapi_job_opt_args_free(te_vec *args);

/**
 * @defgroup tapi_job_opt_formatting functions for argument formatting
 * @{
 *
 * @param[in]     value     Pointer to an argument.
 * @param[inout]  args      Argument vector to which formatted argument
 *                          is appended.
 */

/** value type: `unsigned int` */
te_errno tapi_job_opt_create_uint(const void *value, te_vec *args);

/** value type: `char *` */
te_errno tapi_job_opt_create_string(const void *value, te_vec *args);

/** value type: `te_bool` */
te_errno tapi_job_opt_create_bool(const void *value, te_vec *args);

/** value type: `tapi_job_opt_array` */
te_errno tapi_job_opt_create_array(const void *value, te_vec *args);

/**@} <!-- END tapi_job_opt_formatting --> */

/**
 * @defgroup tapi_job_opt_bind_constructors convenience macros for option
 *                                          description
 * @{
 */

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
    { tapi_job_opt_create_uint, _prefix, _concat_prefix, _suffix, \
      offsetof(_struct, _field) }

/**
 * Bind `te_bool` argument.
 *
 * @param[in]     _prefix   Argument prefix.
 * @param[in]     _struct   Option struct.
 * @param[in]     _field    Field name of the bool in option struct.
 */
#define TAPI_JOB_OPT_BOOL(_prefix, _struct, _field) \
    { tapi_job_opt_create_bool, _prefix, FALSE, NULL, \
      offsetof(_struct, _field) }

/**
 * Bind `char *` argument.
 *
 * @param[in]     _prefix           Argument prefix.
 * @param[in]     _concat_prefix    Concatenate prefix with argument if @c TRUE
 * @param[in]     _struct           Option struct.
 * @param[in]     _field            Field name of the string in option struct.
 */
#define TAPI_JOB_OPT_STRING(_prefix, _concat_prefix, _struct, _field) \
    { tapi_job_opt_create_string, _prefix, _concat_prefix, NULL, \
      offsetof(_struct, _field) }

/**
 * Bind `tapi_job_opt_array` argument.
 *
 * @param[in]     _struct           Option struct.
 * @param[in]     _field            Field name of the array in option struct.
 */
#define TAPI_JOB_OPT_ARRAY(_struct, _field) \
    { tapi_job_opt_create_array, NULL, FALSE, NULL, offsetof(_struct, _field) }

/**@} <!-- END tapi_job_opt_bind_constructors --> */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_JOB_OPT_H__ */

/**@} <!-- END tapi_job_opt --> */
