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


/** Test real parameter with specified value. */
typedef struct test_param {
    TAILQ_ENTRY(test_param) links;  /**< List links */

    char       *name;   /**< Parameter name */
    char       *value;  /**< Current parameter value */
    te_bool     clone;  /**< Clone this entry or not */
} test_param;

/** Head of the test real parameters list */
typedef TAILQ_HEAD(test_params, test_param) test_params;


/** Information about Test Suite */
typedef struct test_suite_info {
    TAILQ_ENTRY(test_suite_info)    links;  /**< List links */

    char   *name;   /**< Name of the Test Suite */
    char   *path;   /**< Path where Test Suite root directory is located */
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

    char   *id;
    char   *refvalue;
    char   *ext;
    char   *value;
} test_var_arg_value;

/** List of value of the variable or argument */
typedef TAILQ_HEAD(test_var_arg_values,
                   test_var_arg_value) test_var_arg_values;

/** Test flags */
#define TEST_RANDOM_SPECIFIED   (1 << 0)


/** Common attributes of the variable or argument */
typedef struct test_var_arg_attrs {
    te_bool             random;
    test_var_arg_type  *type;       /**< Pointer to type descriptor */
    te_bool             foreach;
    test_var_arg_value *preferred;
    unsigned int        flags;      /**< TEST_RANDOM_SPECIFIED */
} test_var_arg_attrs;

/** Common attributes of referred variable or referred argument */
typedef struct test_ref_var_arg_attrs {
    char               *refer;
} test_ref_var_arg_attrs;


/** Referred variable */
typedef struct test_ref_var {
    test_ref_var_arg_attrs  attrs;
} test_ref_var;

/** Simple variable */
typedef struct test_simple_var {
    test_var_arg_values     values;
    test_var_arg_attrs      attrs;
} test_simple_var;


/** Referred variable */
typedef struct test_ref_arg {
    test_ref_var_arg_attrs  attrs;
} test_ref_arg;

/** Simple argument */
typedef struct test_simple_arg {
    test_var_arg_values     values;
    test_var_arg_attrs      attrs;
} test_simple_arg;


/** Types of arguments */
typedef enum test_arg_type {
    TEST_ARG_SIMPLE,
    TEST_ARG_REFERRED
} test_arg_type;


/** Unified argument */
typedef struct test_arg {
    TAILQ_ENTRY(test_arg) links;

    char               *name;       /**< Name */
    test_arg_type       type;
    union {
        test_simple_arg arg;
        test_ref_arg    ref;
    } u;
} test_arg;

/** Head of the list of arguments */
typedef TAILQ_HEAD(test_args, test_arg) test_args;


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


/** Types of test session variables */
typedef enum test_session_var_type {
    TEST_SESSION_VAR_SIMPLE,
    TEST_SESSION_VAR_REFERRED
} test_session_var_type;

/** Test session variable */
typedef struct test_session_var {
    TAILQ_ENTRY(test_session_var)   links;

    char                   *name;       /**< Name */
    test_session_var_type   type;
    te_bool                 handdown;
    union {
        test_simple_var     var;
        test_ref_var        ref;
    } u;
} test_session_var;

/** List of test session variables */
typedef TAILQ_HEAD(test_session_vars, test_session_var) test_session_vars;

/* Forwards */
struct run_item;
typedef struct run_item run_item;

/** List of run itens */
typedef TAILQ_HEAD(run_items, run_item) run_items;


/** Test session */
typedef struct test_session {
    char               *name;           /**< Name */
    test_session_vars   vars;           /**< List of variables */
    run_item           *exception;      /**< Exception handler */
    run_item           *keepalive;      /**< Keep-alive handler */
    run_item           *prologue;       /**< Prologue */
    run_item           *epilogue;       /**< Epilogue */
    run_items           run_items;      /**< List of run items */
    te_bool             simultaneous;   /**< Run all items simultaneously */
    te_bool             random;         /**< Run items in random order */
    unsigned int        flags;          /**< TEST_RANDOM_SPECIFIED */
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
    test_args           args;               /**< Arguments */
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


/** Flags of the Tester global context */
enum tester_ctx_flags {
    TESTER_CTX_NOLOGUES = 0x01,     /**< Prologues/epilogues are disabled */
    TESTER_CTX_NORANDOM = 0x02,     /**< Disable randomness */
    TESTER_CTX_NOSIMULT = 0x04,     /**< Disable simultaneousness */
    TESTER_CTX_FAKE     = 0x08,     /**< Fake run */
    TESTER_CTX_VERBOSE  = 0x10,     /**< The first level of verbosity:
                                         output of run path to stdout */
    TESTER_CTX_VVERB    = 0x20,     /**< The second level of verbosity:
                                         additional output of test IDs
                                         on run paths */
    TESTER_CTX_VVVERB   = 0x40,     /**< The third level of verbosity */
    TESTER_CTX_INLOGUE  = 0x80,     /**< Is in prologue/epilogue */
    TESTER_CTX_VALGRIND = 0x100,    /**< Run scripts under valgrind */
    TESTER_CTX_GDB      = 0x200     /**< Run scripts under GDB in
                                         interactive mode */
};


/** Tester global context */
typedef struct tester_ctx {
    unsigned int        id;         /**< ID of the Tester context */

    unsigned int        flags;      /**< Flags (enum tester_ctx_flags) */
    int                 timeout;    /**< Global execution timeout (sec) */
    test_suites_info    suites;     /**< Information about test suites */
    test_requirements   reqs;       /**< List of target requirements
                                         specified in command line */
    tester_run_paths    paths;      /**< List of paths to run */

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
    tester_run_paths_free(&ctx->paths);
    free(ctx);
}

/**
 * Get unique test ID.
 */
extern test_id tester_get_id(void);

extern int tester_parse_config(tester_cfg *cfg);
extern int tester_run_config(tester_ctx *ctx, tester_cfg *cfg);

/**
 * Build list of Test Suites.
 *
 * @param suites    List of Test Suites
 *
 * @return Status code.
 */
extern int tester_build_suites(test_suites_info *suites);

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
