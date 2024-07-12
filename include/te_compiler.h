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
#define __has_attribute_oldgcc_constructor 1
#endif /* __GNUC__ */
#endif

#ifndef __has_builtin
/*
 * __has_attribute has been supported in GCC since 5.x
 * and in clang since 2.9, so we may assume all modern
 * GCC-compatible compilers support it, but since RHEL 7
 * still use 4.x by default, we'd better support it as well.
 */
#ifndef __GNUC__
#define __has_builtin(bi_) 0
#else
#define __has_builtin(bi_) __has_builtin_oldgcc_##bi_
#define __has_builtin_oldgcc___builtin_types_compatible_p 1
#define __has_builtin_oldgcc___builtin_choose_expr 1
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

#if __has_attribute(constructor)
/**
 * Mark a function as a constructor. It will be executed
 * before the main function.
 */
#define TE_CONSTRUCTOR_AVAILABLE 1
#define TE_CONSTRUCTOR_PRIORITY(n) __attribute__((constructor(n)))
#define TE_CONSTRUCTOR __attribute__((constructor))
#else
#define TE_CONSTRUCTOR_AVAILABLE 0
#define TE_CONSTRUCTOR_PRIORITY(n) CONSTRUCTOR ATTRIBUTE UNSUPPORTED BY COMPILER
#define TE_CONSTRUCTOR CONSTRUCTOR ATTRIBUTE UNSUPPORTED BY COMPILER
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

#if __STDC_VERSION__ >= 201112L
/*
 * Check that the type of @p obj_ is compatible with @p type_.
 *
 * Unlike a cast, both type and value of @p obj_ always remain
 * unchanged, but a compile-time error is raised if @p obj_ is
 * not compatible with @p type_.
 *
 * If generic type dispatching is not supported by the compiler,
 * the macro is a no-op.
 *
 * @param type_   type to check
 * @param obj_    some value
 *
 * @return @p obj_ unchanged.
 */
#define TE_TYPE_ASSERT(type_, obj_) _Generic((obj_), type_: (obj_))
#elif __has_builtin(__builtin_types_compatible_p) && \
    __has_builtin(__builtin_choose_expr)
#define TE_TYPE_ASSERT(type_, obj_) \
    __builtin_choose_expr(__builtin_types_compatible_p(TE_TYPEOF(obj_), \
                                                       type_),          \
                          (obj_), (void)0)
#else
#define TE_TYPE_ASSERT(type_, obj_) (obj_)
#endif

#if __STDC_VERSION__ >= 201112L
/**
 * Choose one of expressions depending on the type of @p obj_.
 *
 * If the type of @p obj_ is compatible with @p type1_, @p expr1_
 * is returned, otherwise if the type is compatible with @p type2_,
 * @p expr2_ is returned, otherwise a compilation error is raised.
 *
 * @p type1_ and @p type2_ must not be compatible with each other.
 *
 * @p obj_ is never evaluated, only its static type is considered
 * and @p expr2_ is not evaluated if @p expr1_ is chosen and vice versa.
 *
 * If the compiler does not support type dispatching, @p expr1_ is
 * tentatively returned.
 *
 * @param obj_    selector object
 * @param type1_  first type
 * @param expr1_  first alternative
 * @param type2_  second type
 * @param expr2_  second alternative
 *
 * @return @p expr1_ or @p expr2_ depending on the type of @p obj_.
 */
#define TE_TYPE_ALTERNATIVE(obj_, type1_, expr1_, type2_, expr2_)   \
    _Generic((obj_), type1_: (expr1_), type2_: (expr2_))
#elif __has_builtin(__builtin_types_compatible_p) &&    \
    __has_builtin(__builtin_choose_expr)
#define TE_TYPE_ALTERNATIVE(obj_, type1_, expr1_, type2_, expr2_) \
    __builtin_choose_expr(__builtin_types_compatible_p(TE_TYPEOF(obj_), \
                                                       type1_),         \
                          (expr1_),                                     \
                          __builtin_choose_expr(                        \
                              __builtin_types_compatible_p(             \
                                  TE_TYPEOF(obj_),                      \
                                  type2_),                              \
                              (expr2_),                                 \
                              ((struct undefined *)NULL)))
#else
#define TE_TYPE_ALTERNATIVE(obj_, type1_, expr1_, type2_, expr2_) (expr1_)
#endif

#endif /* __TE_COMPILER_H__ */
