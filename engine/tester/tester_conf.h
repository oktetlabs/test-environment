/** @file
 * @brief Tester Subsystem
 *
 * Internal representation of Tester configuration file and packages
 * description.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TESTER_CONF_H__
#define __TE_TESTER_CONF_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_queue.h"
#include "logger_api.h"
#include "logger_ten.h"

#include "tester_defs.h"
#include "tester_flags.h"
#include "tester_reqs.h"
#include "tester_build.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Default timeout is one day */
#define TESTER_TIMEOUT_DEF      86400

/** Test flags */
#define TEST_INHERITED_KEEPALIVE    (1 << 0)
#define TEST_INHERITED_EXCEPTION    (1 << 1)


/** Information about person (maintainer or author) */
typedef struct person_info {
    TAILQ_ENTRY(person_info)    links;  /**< List links */

    char   *name;   /**< Name (optional) */
    char   *mailto; /**< E-mail addresses */
} person_info;

/** Head of the list with information about persons */
typedef TAILQ_HEAD(persons_info, person_info) persons_info;



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


/* Forward declaration */
struct test_entity_value;
/** Value of the variable or argument */
typedef struct test_entity_value test_entity_value;

/** List of value of the variable or argument */
typedef struct test_entity_values {
    TAILQ_HEAD(, test_entity_value) head;   /**< Head of the list */
    unsigned int                    num;    /**< Number of values in
                                                 the list */
} test_entity_values;


/** Description of value's type */
typedef struct test_value_type {
    LIST_ENTRY(test_value_type)     links;  /**< List links */
    char                           *name;   /**< Type name */
    const struct test_value_type   *type;   /**< Parent type */
    test_entity_values              values; /**< Values */
} test_value_type;

/** List of value's types */
typedef LIST_HEAD(test_value_types, test_value_type) test_value_types;


/** Value of the variable or argument */
struct test_entity_value {
    TAILQ_ENTRY(test_entity_value) links;  /**< List links */

    char                       *name;   /**< Identifier */
    const test_value_type      *type;   /**< Type of the value */
    char                       *plain;  /**< Plain value */
    const test_entity_value    *ref;    /**< Reference to another value */
    char                       *ext;    /**< Reference to external value */
    test_requirements           reqs;   /**< Attached requirements */
};


/** Types of service executables handdown */
typedef enum tester_handdown {
    TESTER_HANDDOWN_NONE,
    TESTER_HANDDOWN_CHILDREN,
    TESTER_HANDDOWN_DESCENDANTS,
} tester_handdown;

/** Default value of the executable handdown attribute */
#define TESTER_HANDDOWN_DEF TESTER_HANDDOWN_NONE


/** Types of Tester configuration tracking */
typedef enum tester_track_conf {
    TESTER_TRACK_CONF_UNSPEC,
    TESTER_TRACK_CONF_YES,
    TESTER_TRACK_CONF_NO,
    TESTER_TRACK_CONF_SILENT,
} tester_track_conf;

/** Default value of the track_conf attribute */
#define TESTER_TRACK_CONF_DEF   TESTER_TRACK_CONF_YES


/** Attributes of any test (script, session) */
typedef struct test_attrs {
    struct timeval      timeout;        /**< Execution timeout */
    tester_track_conf   track_conf;     /**< Type of configurations
                                             changes tracking */
    tester_handdown     track_conf_hd;  /**< Inheritance of 'track_conf'
                                             attribute */
} test_attrs;


/** Test script */
typedef struct test_script {
    char               *name;       /**< Name of the script */
    char               *objective;  /**< Objective */
    char               *page;       /**< HTML page with documentation */
    char               *execute;    /**< Full path to executable */
    test_requirements   reqs;       /**< Set of requirements */
    test_attrs          attrs;      /**< Test attributes */
} test_script;


/** Test session variable */
typedef struct test_var_arg {
    TAILQ_ENTRY(test_var_arg)   links;  /**< List links */

    char                    *name;      /**< Name */
    const test_value_type   *type;      /**< Pointer to type descriptor */
    test_entity_values       values;    /**< Values */

    char                    *list;      /**< Name of the iteration list */
    const test_entity_value *preferred; /**< Preferred value for list 
                                             iteration */

    te_bool                  handdown;  /**< Handdown session variable
                                             to all children */
} test_var_arg;

/** List of test session variables */
typedef TAILQ_HEAD(test_vars_args, test_var_arg) test_vars_args;

/* Forwards */
struct run_item;
/** Unified run item */
typedef struct run_item run_item;

/** List of run itens */
typedef TAILQ_HEAD(run_items, run_item) run_items;


/** Test session */
typedef struct test_session {
    const struct test_session  *parent; /**< Parent test session */

    char               *name;           /**< Name or NULL */
    test_attrs          attrs;          /**< Test attributes */
    test_value_types    types;          /**< Types declared in session */
    test_vars_args      vars;           /**< List of variables */
    run_item           *exception;      /**< Exception handler */
    run_item           *keepalive;      /**< Keep-alive handler */
    run_item           *prologue;       /**< Prologue */
    run_item           *epilogue;       /**< Epilogue */
    run_items           run_items;      /**< List of run items */
    te_bool             simultaneous;   /**< Run all items simultaneously */
    unsigned int        flags;          /**< Flags */
} test_session;


/** Information about test script */
typedef struct test_info {
    TAILQ_ENTRY(test_info)  links;  /**< List links */

    char   *name;                   /**< Test name */
    char   *page;                   /**< HTML page with documentation */
    char   *objective;              /**< Objective of the test */
} test_info;

/** Head of the list with test info */
typedef TAILQ_HEAD(tests_info, test_info) tests_info;


/** Test package */
typedef struct test_package {
    TAILQ_ENTRY(test_package)   links;  /**< List links */

    char               *name;       /**< Name */
    char               *path;       /**< Path to the Test Package file */
    char               *objective;  /**< Description */
    persons_info        authors;    /**< List of authors */
    test_requirements   reqs;       /**< List of requirements */
    test_session        session;    /**< Provided session */

    tests_info         *ti;         /**< Information about scripts */
} test_package;

/** Head of the list of test packages */
typedef TAILQ_HEAD(test_packages, test_package) test_packages;

/** Information about run item variable/argument list. */
typedef struct test_var_arg_list {
    LIST_ENTRY(test_var_arg_list)   links;  /**< Linked list links */

    const char     *name;       /**< Name of the list */
    unsigned int    len;        /**< Length of the list */
    unsigned int    n_iters;    /**< Number of outer iterations of the
                                     list */
} test_var_arg_list;

/**
 * Head of the list with information about run item variable/argument
 * lists.
 */
typedef LIST_HEAD(test_var_arg_lists, test_var_arg_list) test_var_arg_lists;


/** Unified run item */
struct run_item {
    TAILQ_ENTRY(run_item)   links;  /**< List links */

    const test_session *context;    /**< Parent session */

    char               *name;       /**< Name or NULL */
    tester_handdown     handdown;   /**< Type of executable inheritance */
    run_item_type       type;       /**< Type of the run item */
    union {
        test_script     script;     /**< Test script */
        test_session    session;    /**< Test session */
        test_package   *package;    /**< Test package */
    } u;                            /**< Type specific data */
    test_vars_args      args;       /**< Arguments */
    test_var_arg_lists  lists;      /**< "Lists" of variables/arguments */
    unsigned int        iterate;    /**< Number of iterations */
    unsigned int        loglevel;   /**< Log level to be used by
                                         the item */

    unsigned int        n_args;     /**< Total Number of arguments
                                         including inherited */
    unsigned int        n_iters;    /**< Number of iterations */
    unsigned int        weight;     /**< Number of children iterations
                                         in single iteration */
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
    test_suites_info    suites;         /**< Information about test
                                             suites */
    logic_expr         *targets;        /**< Target requirements
                                             expression */
    test_options        options;        /**< List of options */
    run_items           runs;           /**< List of items to run */

    test_packages       packages;       /**< List of mentioned packages */
    test_package       *cur_pkg;        /**< Pointer to the package
                                             which is being parsed now */
    unsigned int        total_iters;    /**< Total number of iterations
                                             in the test configuration */
} tester_cfg;

/** Head of Tester configuration files list */
typedef TAILQ_HEAD(tester_cfgs, tester_cfg) tester_cfgs;



/**
 * Get attributes of the test executed by the run item.
 *
 * @param ri            Run item
 *
 * @return Pointer to the test attributes.
 */
static inline test_attrs *
test_get_attrs(run_item *ri)
{
    assert(ri != NULL);
    switch (ri->type)
    {
        case RUN_ITEM_SCRIPT:
            return &ri->u.script.attrs;

        case RUN_ITEM_SESSION:
            return &ri->u.session.attrs;

        case RUN_ITEM_PACKAGE:
            assert(ri->u.package != NULL);
            return &ri->u.package->session.attrs;

        default:
            assert(FALSE);
            return NULL;
    }
}

/**
 * Get name of the test executed by the run item.
 *
 * @param ri            Run item
 *
 * @return Pointer to the test attributes.
 */
static inline const char *
test_get_name(const run_item *ri)
{
    assert(ri != NULL);
    switch (ri->type)
    {
        case RUN_ITEM_SCRIPT:
            return ri->u.script.name;

        case RUN_ITEM_SESSION:
            return ri->u.session.name;

        case RUN_ITEM_PACKAGE:
            assert(ri->u.package != NULL);
            return ri->u.package->name;

        default:
            assert(FALSE);
            return NULL;
    }
}

/**
 * Get pointer to variable/argument values.
 *
 * @param va            Variable/argument
 */
static inline test_entity_values *
test_var_arg_values(const test_var_arg *va)
{
    if (va->values.head.tqh_first == NULL)
    {
        assert(va->type != NULL);
        return (test_entity_values *)&va->type->values;
    }
    else
    {
        return (test_entity_values *)&va->values;
    }
}


/**
 * Prototype of the function to be called for each argument of the run
 * item.
 *
 * @param va            Inherited variable or argument
 * @param opaque        Opaque data
 *
 * @return Status code.
 */
typedef te_errno (* test_var_arg_enum_cb)(const test_var_arg *va,
                                          void               *opaque);

/**
 * Enumerate run item arguments including inherited from session
 * variables.
 *
 * @param ri            Run item
 * @param callback      Function to be called for each argument
 * @param opaque        Data to be passed in callback function
 *
 * @return Status code.
 */
extern te_errno test_run_item_enum_args(const run_item       *ri,
                                        test_var_arg_enum_cb  callback,
                                        void                 *opaque);

/**
 * Find argument of the run item by name.
 * The function takes into account lists when calculate @a n_values and
 * @a outer_iters.
 *
 * @param ri            Run item
 * @param name          Name of the argument to find
 * @param n_values      Location for total number of values of the
 *                      argument or NULL
 * @param outer_iters   Location for total number of outer iterations
 *                      because of argument before this one or NULL
 *
 * @return Pointer to found argument or NULL.
 */
extern const test_var_arg *test_run_item_find_arg(
                               const run_item *ri,
                               const char     *name,
                               unsigned int   *n_values,
                               unsigned int   *outer_iters);


/**
 * Prototype of the function to be called for each singleton value
 * of the variable/argument.
 *
 * @param value         Singleton value (plain or external)
 * @param opaque        Opaque data
 *
 * @return Status code.
 */
typedef te_errno (* test_entity_value_enum_cb)(
                     const test_entity_value *value, void *opaque);

/**
 * Recovery callback to be used in the case of failure.
 *
 * @param status        Status code
 * @param opaque        Recovery opaque data
 *
 * @return Status code.
 */
typedef te_errno (* test_entity_value_enum_error_cb)(
                     const test_entity_value *value,
                     te_errno status, void *opaque);

/**
 * Enumerate values from the list in current variables context.
 *
 * @param vars          Variables context or NULL
 * @param values        List of values
 * @param callback      Function to be called for each singleton value
 * @param opaque        Data to be passed in callback function
 * @param enum_error_cb Function to be called on back path when
 *                      enumeration error occur (for example, when
 *                      value has been found)
 * @param ee_opaque     Opaque data for @a enum_error_cb function
 *
 * @return Status code.
 */
extern te_errno test_entity_values_enum(
                    const test_vars_args            *vars,
                    const test_entity_values        *values,
                    test_entity_value_enum_cb        callback,
                    void                            *opaque,
                    test_entity_value_enum_error_cb  enum_error_cb,
                    void                            *ee_opaque);

/**
 * Enumerate singleton values of the run item argument or session
 * variable.
 *
 * @param ri            Run item or NULL
 * @param va            Varialbe/argument
 * @param callback      Function to be called for each singleton value
 * @param opaque        Data to be passed in callback function
 * @param enum_error_cb Function to be called on back path when
 *                      enumeration error occur (for example, when
 *                      value has been found)
 * @param ee_opaque     Opaque data for @a enum_error_cb function
 *
 * @return Status code.
 */
extern te_errno test_var_arg_enum_values(
                    const run_item                  *ri,
                    const test_var_arg              *va,
                    test_entity_value_enum_cb        callback,
                    void                            *opaque,
                    test_entity_value_enum_error_cb  enum_error_cb,
                    void                            *ee_opaque);


/**
 * Get value of the run item argument by index.
 *
 * @param ri            Run item context
 * @param va            Argument
 * @param index         Index of the value to get (0,...)
 * @param enum_error_cb Function to be called on back path when
 *                      enumeration error occur (for example, when
 *                      value has been found)
 * @param ee_opaque     Opaque data for @a enum_error_cb function
 * @param value         Location for plain value pointer
 *
 * @return Status code.
 */
extern te_errno test_var_arg_get_value(
                    const run_item                     *ri,
                    const test_var_arg                 *va,
                    const unsigned int                  index,
                    test_entity_value_enum_error_cb     enum_error_cb,  
                    void                               *ee_opaque,
                    const test_entity_value           **value);



/**
 * Allocatate and initialize Tester configuration.
 *
 * @param filename      Name of the file with configuration
 *
 * @return Pointer to allocated and initialized Tester configuration or
 *         NULL.
 */
extern tester_cfg * tester_cfg_new(const char *filename);

/**
 * Parse Tester configuratin files.
 *
 * @param cfgs          Tester configurations with not parsed file
 * @param build         Build test suites
 * @param verbose       Be verbose in the case of build failure
 *
 * @return Status code.
 */
extern te_errno tester_parse_configs(tester_cfgs *cfgs,
                                     te_bool build, te_bool verbose);

/**
 * Prepare configuration to be passed by testing scenario generator.
 *
 * @param cfgs          List of Tester configurations
 * @param iters         Location of total number of iterations
 *
 * @return Status code.
 */
extern te_errno tester_prepare_configs(tester_cfgs  *cfgs,
                                       unsigned int *iters);

/**
 * Free Tester configurations.
 *
 * @param cfgs          List of Tester configurations to be freed
 */
extern void tester_cfgs_free(tester_cfgs *cfgs);


/**
 * Controls of Tester configuration traverse.
 */
typedef enum tester_cfg_walk_ctl {
    TESTER_CFG_WALK_CONT,   /**< Continue */
    TESTER_CFG_WALK_BACK,   /**< Continue in backward direction */
    TESTER_CFG_WALK_BREAK,  /**< Break repeation or iteration loop and
                                 continue */
    TESTER_CFG_WALK_SKIP,   /**< Skip this item and continue with 
                                 the rest */
    TESTER_CFG_WALK_EXC,    /**< Call session exception handler */
    TESTER_CFG_WALK_FIN,    /**< No necessity to walk new items, but
                                 call end callbacks of entered items */
    TESTER_CFG_WALK_STOP,   /**< Stop by user request */
    TESTER_CFG_WALK_INTR,   /**< Interrupt testing because of keep-alive
                                 validation or exception handler failure */
    TESTER_CFG_WALK_FAULT,  /**< Interrupt because of internal error */
} tester_cfg_walk_ctl;

/** Walk is in service routine */
#define TESTER_CFG_WALK_SERVICE             1
/** Force walk to enter exception handler of every session */
#define TESTER_CFG_WALK_FORCE_EXCEPTION     2

/**
 * Functions to be called when traversing Tester configuration.
 */
typedef struct tester_cfg_walk {
    tester_cfg_walk_ctl (* cfg_start)(tester_cfg *,
                                      unsigned int, void *);
    tester_cfg_walk_ctl (* cfg_end)(tester_cfg *,
                                    unsigned int, void *);
    tester_cfg_walk_ctl (* pkg_start)(run_item *, test_package *,
                                      unsigned int, void *);
    tester_cfg_walk_ctl (* pkg_end)(run_item *, test_package *,
                                    unsigned int, void *);
    tester_cfg_walk_ctl (* session_start)(run_item *, test_session *,
                                          unsigned int, void *);
    tester_cfg_walk_ctl (* session_end)(run_item *, test_session *,
                                        unsigned int, void *);
    tester_cfg_walk_ctl (* prologue_start)(run_item *,
                                           unsigned int, void *);
    tester_cfg_walk_ctl (* prologue_end)(run_item *,
                                         unsigned int, void *);
    tester_cfg_walk_ctl (* epilogue_start)(run_item *,
                                           unsigned int, void *);
    tester_cfg_walk_ctl (* epilogue_end)(run_item *,
                                         unsigned int, void *);
    tester_cfg_walk_ctl (* keepalive_start)(run_item *,
                                            unsigned int, void *);
    tester_cfg_walk_ctl (* keepalive_end)(run_item *,
                                          unsigned int, void *);
    tester_cfg_walk_ctl (* exception_start)(run_item *,
                                            unsigned int, void *);
    tester_cfg_walk_ctl (* exception_end)(run_item *,
                                          unsigned int, void *);
    tester_cfg_walk_ctl (* run_start)(run_item *, unsigned int,
                                      unsigned int, void *);
    tester_cfg_walk_ctl (* run_end)(run_item *, unsigned int,
                                    unsigned int, void *);
    tester_cfg_walk_ctl (* iter_start)(run_item *, unsigned int,
                                       unsigned int, unsigned int,
                                       void *);
    tester_cfg_walk_ctl (* iter_end)(run_item *, unsigned int,
                                     unsigned int, unsigned int,
                                     void *);
    tester_cfg_walk_ctl (* repeat_start)(run_item *, unsigned int,
                                         unsigned int, void *);
    tester_cfg_walk_ctl (* repeat_end)(run_item *, unsigned int,
                                       unsigned int, void *);
    tester_cfg_walk_ctl (* script)(run_item *, test_script *,
                                   unsigned int, void *);
} tester_cfg_walk;

/**
 * Traverse Tester configurations.
 *
 * @note The function does not modify any data under pointers specified
 *       as function parameters. However, @e const qualifier is
 *       discarded when opaque data and configuration element are passed
 *       in callbacks provided by user.
 *
 * @param cfgs          List of configurations
 * @param walk_cbs      Walk callbacks
 * @param walk_flags    Walk control flags
 * @param opaque        Opaque data to be passed in callbacks
 *
 * @return Walk control.
 */
extern tester_cfg_walk_ctl tester_configs_walk(
                               const tester_cfgs     *cfgs,
                               const tester_cfg_walk *walk_cbs,
                               const unsigned int     walk_flags,
                               const void            *opaque);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_CONF_H__ */
