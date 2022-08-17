/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * Helper functions to make regular expression substitutions
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TRC_RE_SUBST_H__
#define __TE_TRC_RE_SUBST_H__

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_REGEX_H
#include <regex.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRC_RE_KEY_URL        "URL"
#define TRC_RE_KEY_TABLE_HREF "TABLE"
#define TRC_RE_KEY_SCRIPT     "SCRIPT"
#define TRC_RE_KEY_TAGS       "TAGS"

/** Regular expression match substitution */
typedef struct trc_re_match_subst {
    TAILQ_ENTRY(trc_re_match_subst) links;  /**< List links */

    te_bool             match;  /**< Match or string */
    union {
        char           *str;    /**< String to insert */
        unsigned int    match;  /**< Match index to insert */
    } u;
} trc_re_match_subst;

/** Regular expression match substitutions list */
typedef TAILQ_HEAD(trc_re_match_substs, trc_re_match_subst)
    trc_re_match_substs;

/** Regular expression substitution */
typedef struct trc_re_subst {
    TAILQ_ENTRY(trc_re_subst)   links;  /**< List links */

    regex_t                 re;         /**< Compiled regular expression */
    unsigned int            max_match;  /**< Number of used matches
                                             in substitution */
    char                   *str;        /**< Substitution string */
    trc_re_match_substs     with;       /**< Substitute with */

    regmatch_t             *matches;    /**< Match data */
} trc_re_subst;

/** Regular expression substitutions list */
typedef TAILQ_HEAD(trc_re_substs, trc_re_subst) trc_re_substs;

/** Regular expression substitution */
typedef struct trc_re_namespace {
    TAILQ_ENTRY(trc_re_namespace) links;  /**< List links */

    char          *name;   /**< Namespace name */
    trc_re_substs  substs; /**< Regexp substitutions * list */
} trc_re_namespace;

/** List of substitutions namespaces */
typedef TAILQ_HEAD(trc_re_namespaces, trc_re_namespace)
        trc_re_namespaces;


/** Key substitutions */
extern trc_re_substs key_substs;

/** Key namespaces */
extern trc_re_namespaces key_namespaces;


/**
 * Free resourses allocated for the list of regular expression
 * substitutions.
 */
extern void trc_re_substs_free(trc_re_substs *substs);

/**
 * Read substitutions from file.
 *
 * @param file          File name
 * @param substs        List to add substitutions to
 *
 * @return Status code.
 */
extern te_errno trc_re_substs_read(const char *file, trc_re_substs *substs);

/**
 * Read substitutions from file.
 *
 * @param file          File name
 * @param substs        List to add substitutions to
 *
 * @return              Status code.
 */
extern te_errno trc_re_namespaces_read(const char *file,
                                       trc_re_namespaces *namespaces);

/**
 * Execute substitutions.
 *
 * @param substs        List of substitutions to do
 * @param str           String to substitute in
 * @param f             File to write to
 *
 * @return Status code.
 */
extern void trc_re_substs_exec_start(const trc_re_substs *substs,
                                     const char *str, FILE *f);

/**
 * Do regular expression key substitutions.
 *
 * @param key           Keys string
 * @param f             File to output to
 */
extern void trc_re_key_substs(const char *name, const char *key, FILE *f);

/**
 * Execute substitutions.
 *
 * @param substs        List of substitutions to do
 * @param str           String to substitute in
 * @param buf           Buffer to put substituted string
 * @param buf_size      Buffer size
 *
 * @return              Status code.
 */
extern ssize_t trc_re_substs_exec_buf_start(const trc_re_substs *substs,
                                            const char          *str,
                                            char                *buf,
                                            ssize_t              buf_size);

/**
 * Do regular expression key substitutions.
 *
 * @param name          Regular expression namespace name
 * @param key           Keys string
 *
 * @return              Substituted string or NULL.
 */
extern char *trc_re_key_substs_buf(const char *name, const char *key);

extern te_errno trc_key_substs_read(const char *file);

extern void trc_key_substs_free(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_RE_SUBST_H__ */
