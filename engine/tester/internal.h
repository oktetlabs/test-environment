/** @file
 * @brief Tester Subsystem
 *
 * Internal definitions
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_ENGINE_TESTER_INTERNAL_H__
#define __TE_ENGINE_TESTER_INTERNAL_H__

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif 

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ten.h"

#include "tester_flags.h"
#include "test_params.h"
#include "run_path.h"
#include "reqs.h"


/** Define it to enable support of timeouts in Tester */
#undef TESTER_TIMEOUT_SUPPORT

#define TESTER_TIMEOUT_DEF      60000

/** Format string for Valgrind output filename */
#define TESTER_VG_FILENAME_FMT  "vg.test.%d"

/** Format string for GDB init filename */
#define TESTER_GDB_FILENAME_FMT "gdb.%d"


/** Test ID */
typedef int test_id;


/** Element of the list of strings */
typedef struct tqe_string {
    TAILQ_ENTRY(tqe_string)     links;  /**< List links */
    char                       *v;      /**< Value */
} tqe_string;

/** Head of the list of strings */
typedef TAILQ_HEAD(tqh_strings, tqe_string) tqh_strings;


/** Information about person (maintainer or author) */
typedef struct person_info {
    TAILQ_ENTRY(person_info)    links;  /**< List links */

    char   *name;   /**< Name (optional) */
    char   *mailto; /**< E-mail addresses */
} person_info;

/** Head of the list with information about persons */
typedef TAILQ_HEAD(persons_info, person_info) persons_info;


/** Information about Test Suite */
typedef struct test_suite_info {
    TAILQ_ENTRY(test_suite_info)    links;  /**< List links */

    char   *name;   /**< Name of the Test Suite */
    char   *src;    /**< Path where Test Suite sources are located */
    char   *bin;    /**< Path where Test Suite executables are located */
} test_suite_info;

/** Head of the list with information about Test Suites */
typedef TAILQ_HEAD(test_suites_info, test_suite_info) test_suites_info;


/** Option from the Tester configuration file */
typedef struct test_option {
    TAILQ_ENTRY(test_option)    links;  /**< List links */

    char           *name;       /**< Option name */
    char           *value;      /**< Option value */
    tqh_strings     contexts;   /**< List of context where this option
                                     should be applied */
} test_option;

/** List of options */
typedef TAILQ_HEAD(test_options, test_option) test_options;


/** Descriptor of the variable or argument type */
typedef struct test_var_arg_type {
    char   *name;   /**< Type name */
} test_var_arg_type;


/** Value of the variable or argument */
typedef struct test_var_arg_value {
    TAILQ_ENTRY(test_var_arg_value) links;  /**< List links */

    char                       *id;
    struct test_var_arg_value  *ref;
    char                       *ext;
    test_requirements           reqs;
    char                       *value;
} test_var_arg_value;

/** List of value of the variable or argument */
typedef TAILQ_HEAD(test_var_arg_values,
                   test_var_arg_value) test_var_arg_values;

/** Test flags */
#define TEST_RANDOM_SPECIFIED   (1 << 0)


/** Common attributes of the variable or argument */
typedef struct test_var_arg_attrs {
    te_bool             random;     /**< Random or strict values usage */
    test_var_arg_type  *type;       /**< Pointer to type descriptor */
    char               *list;       /**< Name of the iteration list */
    test_var_arg_value *preferred;  /**< Preferred value for list iteration */
    unsigned int        flags;      /**< TEST_RANDOM_SPECIFIED */
} test_var_arg_attrs;


/** Attributes of any run item */
typedef struct run_item_attrs {
    struct timeval  timeout;
    te_bool         track_conf;
} run_item_attrs;


/** Test script */
typedef struct test_script {
    char               *name;
    char               *descr;
    char               *execute;
    test_requirements   reqs;
} test_script;


/** Test session variable */
typedef struct test_var_arg {
    TAILQ_ENTRY(test_var_arg)   links;

    char                   *name;       /**< Name */
    te_bool                 handdown;
    test_var_arg_values     values;     /**< Values */
    test_var_arg_attrs      attrs;
} test_var_arg;

/** List of test session variables */
typedef TAILQ_HEAD(test_vars_args, test_var_arg) test_vars_args;

/* Forwards */
struct run_item;
typedef struct run_item run_item;

/** List of run itens */
typedef TAILQ_HEAD(run_items, run_item) run_items;


/** Test session */
typedef struct test_session {
    char           *name;           /**< Name */
    test_vars_args  vars;           /**< List of variables */
    run_item       *exception;      /**< Exception handler */
    run_item       *keepalive;      /**< Keep-alive handler */
    run_item       *prologue;       /**< Prologue */
    run_item       *epilogue;       /**< Epilogue */
    run_items       run_items;      /**< List of run items */
    te_bool         simultaneous;   /**< Run all items simultaneously */
    te_bool         random;         /**< Run items in random order */
    unsigned int    flags;          /**< TEST_RANDOM_SPECIFIED */
} test_session;


/** Test package */
typedef struct test_package {
    TAILQ_ENTRY(test_package)   links;  /**< List links */

    char               *name;       /**< Name */
    char               *path;       /**< Path to the Test Package file */
    char               *descr;      /**< Description */
    persons_info        authors;    /**< List of authors */
    test_requirements   reqs;       /**< List of requirements */
    test_session        session;    /**< Provided session */
} test_package;

/** Head of the list of test packages */
typedef TAILQ_HEAD(test_packages, test_package) test_packages;


/** Types of run items */
typedef enum run_item_type {
    RUN_ITEM_NONE,
    RUN_ITEM_SCRIPT,
    RUN_ITEM_SESSION,
    RUN_ITEM_PACKAGE
} run_item_type;


#define TESTER_RUN_ITEM_FORCERANDOM     (1 << 0)

/** Unified run item */
struct run_item {
    TAILQ_ENTRY(run_item)   links;  /**< List links */

    char               *name;       /**< Optinal name of the run item */
    run_item_type       type;       /**< Type of the run item */
    union {
        test_script     script;
        test_session    session;
        test_package   *package;
    } u;
    test_vars_args      args;               /**< Arguments */
    run_item_attrs      attrs;              /**< Package run attributes */
    unsigned int        loglevel;
    te_bool             allow_configure;    
    te_bool             allow_keepalive;
    te_bool             forcerandom;
    unsigned int        flags;
};

/* Forward */
struct tester_ctx;

/** Tester configuration file */
typedef struct tester_cfg {
    TAILQ_ENTRY(tester_cfg) links;      /**< List links */

    const char         *filename;       /**< Name of the file with
                                             configuration */
    persons_info        maintainers;    /**< Configuration maintainers */
    char               *descr;          /**< Optional description */
    test_suites_info    suites;         /**< Information about test suites */
    test_requirements   reqs;           /**< List of target requirements */
    test_options        options;        /**< List of options */
    run_items           runs;           /**< List of items to run */

    test_packages       packages;       /**< List of mentioned packages */
    test_package       *cur_pkg;        /**< Pointer to the package
                                             which is being parsed now */
} tester_cfg;

/** Head of Tester configuration files list */
typedef TAILQ_HEAD(tester_cfgs, tester_cfg) tester_cfgs;


/** Tester global context */
typedef struct tester_ctx {
    unsigned int        id;         /**< ID of the Tester context */

    unsigned int        flags;      /**< Flags (enum tester_flags) */
    int                 timeout;    /**< Global execution timeout (sec) */
    test_suites_info    suites;     /**< Information about test suites */
    test_requirements   reqs;       /**< List of target requirements
                                         specified in command line */
    tester_run_path    *path;       /**< Path to run and/or path options */

} tester_ctx;


/**
 * Free Tester context.
 *
 * @param ctx       Tester context to be freed
 */
static inline void
tester_ctx_free(tester_ctx *ctx)
{
    test_requirements_free(&ctx->reqs);
    free(ctx);
}

/**
 * Get unique test ID.
 */
extern test_id tester_get_id(void);

extern int tester_parse_config(tester_cfg *cfg, const tester_ctx *ctx);
extern int tester_run_config(tester_ctx *ctx, tester_cfg *cfg);

/**
 * Build Test Suite.
 *
 * @param flags     Tester context flags
 * @param suite     Test Suites
 *
 * @return Status code.
 */
extern int tester_build_suite(unsigned int flags,
                              const test_suite_info *suite);

/**
 * Log test start to terminal.
 *
 * @param type      Type of run item
 * @param name      Name of the test
 * @param parent    Parent ID
 * @param self      Self ID of the test
 * @param flags     Tester context flags
 */
extern void tester_out_start(run_item_type type, const char *name,
                             test_id parent, test_id self,
                             unsigned int flags);

/**
 * Log test done to terminal.
 *
 * @param type      Type of run item
 * @param name      Name of the test
 * @param parent    Parent ID
 * @param self      Self ID of the test
 * @param flags     Tester context flags
 * @param result    Test result
 */
extern void tester_out_done(run_item_type type, const char *name,
                            test_id parent, test_id self,
                            unsigned int flags, int result);

#endif /* !__TE_ENGINE_TESTER_INTERNAL_H__ */
