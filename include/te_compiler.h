/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Compiler-dependent definitions.
 *
 * Thunks for compiler-specific stuff such as GCC attributes.
 */

#ifndef __TE_COMPILER_H__
#define __TE_COMPILER_H__

#ifndef __has_attribute
/*
 * __has_attribute has been supported in GCC since 5.x
 * and in clang since 2.9, so we may assume all modern
 * GCC-compatible compilers support it, but since RHEL 7
 * still use 4.x by default, we'd better support it as well.
 */
#ifndef __GNUC__
#define __has_attribute(attr_) 0
#else
#define __has_attribute(attr_) __has_attribute_oldgcc_##attr_
#define __has_attribute_oldgcc_format 1
#define __has_attribute_oldgcc_deprecated 1
#define __has_attribute_oldgcc_sentinel 1
#endif /* __GNUC__ */
#endif

#if __has_attribute(format)
/**
 * Declare that a function is similiar to @c printf, taking
 * a printf-style format string in its argument @p fmtarg_, and
 * variable arguments starting at @p va_.
 *
 * @param fmtarg_  the number of printf-style format string argument
 * @param va_      the number of the first variadic argument
 */
#define TE_LIKE_PRINTF(fmtarg_, va_) \
    __attribute__((format (printf, fmtarg_, va_)))

/**
 * Declare that a function is similiar to @c vprintf, taking
 * a printf-style format string in its argument @p fmtarg_.
 *
 * @param fmtarg_  the number of printf-style format string argument
 */
#define TE_LIKE_VPRINTF(fmtarg_) __attribute__((format (printf, fmtarg_, 0)))

#else

#define TE_LIKE_PRINTF(fmtarg_, va_)
#define TE_LIKE_VPRINTF(fmtarg_, va_)

#endif

#if __has_attribute(sentinel)
/**
 * Mandate that variadic arguments be terminated with
 * a @c NULL pointer.
 *
 * See https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-sentinel-function-attribute
 * for details.
 */
#define TE_REQUIRE_SENTINEL __attribute__((sentinel))
#else
#define TE_REQUIRE_SENTINEL
#endif

#if __has_attribute(deprecated)
/**
 * Mark an entity as deprecated. A reference to such
 * entity would trigger a warning.
 *
 * @note In most cases deprecated entities should instead
 *       have @c @deprecated in their documentation.
 *       See https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=771c035372a036f83353eef46dbb829780330234
 *       for some rationale.
 */
#define TE_DEPRECATED __attribute__((deprecated))
#else
#define TE_DEPRECATED
#endif

/*
 * typeof is supported by gcc and clang and clang defines __GNUC__,
 * so a single check is sufficient.
 */
#ifdef __GNUC__
/**
 * A feature macro indicating whether @c typeof is supported.
 *
 * @todo Add a check for C23's @c typeof when it is standardized.
 */
#define TE_HAS_RELIABLE_TYPEOF 1
#else
#define TE_HAS_RELIABLE_TYPEOF 0
#endif

#if TE_HAS_RELIABLE_TYPEOF
/**
 * Get a type of @p obj_.
 *
 * If @c typeof is not supported on a given platform,
 * it falls back to @c void which will certainly break
 * all non-pointer cases, but at least `TE_TYPEOF(obj) *`
 * would remain compilable.
 *
 * For better portability TE_CAST_TYPEOF() should be
 * used instead of `(TE_TYPEOF(obj))(val)`.
 */
#define TE_TYPEOF(obj_) __typeof__(obj_)

/**
 * Cast @p src_ to the type of @p obj_.
 *
 * If @c typeof is not supported, it falls back to no-op.
 *
 * @param obj_   Object of target type.
 * @param src_   Source object.
 */
#define TE_CAST_TYPEOF(obj_, src_) ((TE_TYPEOF(obj_))(src_))
#else
#define TE_TYPEOF(obj_) void
#define TE_CAST_TYPEOF(obj_) (obj_)
#endif

/**
 * Cast @p src_ to the type of pointer to @p obj_.
 *
 * If @c typeof is not supported, it falls back to
 * a generic `void *`.
 *
 * @param obj_   Object of target type.
 * @param src_   Source pointer.
 */
#define TE_CAST_TYPEOF_PTR(obj_, src_) ((TE_TYPEOF(obj_) *)(src_))


#endif /* __TE_COMPILER_H__ */
