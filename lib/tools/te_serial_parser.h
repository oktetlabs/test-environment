/** @file
 * @brief Unix Test Agent serial console parser support.
 *
 * @defgroup te_tools_te_serial_parser Serial console parser
 * @ingroup te_tools
 * @{
 *
 * Defenition of unix TA serial console parse configuring support.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_SERIAL_PARSER_H__
#define __TE_SERIAL_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_serial_common.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"

/* Base size of the buffer for lists */
#define PARSER_LIST_SIZE    512
/* Default value of polling interval */
#define TE_SERIAL_INTERVAL  100

/** List of the parser events */
typedef struct serial_pattern_t {
    char    name[TE_SERIAL_MAX_NAME + 1];   /**< Name of the pattern */
    char    v[TE_SERIAL_MAX_PATT + 1];      /**< The pattern */

    SLIST_ENTRY(serial_pattern_t)  ent_pat_l; /**< Elements connector */
} serial_pattern_t;
SLIST_HEAD(serial_pattern_h_t, serial_pattern_t);
typedef struct serial_pattern_h_t serial_pattern_h_t;

/** List of the parser events */
typedef struct serial_event_t {
    char    name[TE_SERIAL_MAX_NAME + 1];   /**< Name of the event */
    char    t_name[TE_SERIAL_MAX_NAME + 1]; /**< Tester name of the event */
    int     count;                          /**< Event counter */
    te_bool status;                         /**< Status of the event */

    serial_pattern_h_t           patterns;  /**< Head of pattern list */
    SLIST_ENTRY(serial_event_t)  ent_ev_l;  /**< Elements connector */
} serial_event_t;
SLIST_HEAD(serial_event_h_t, serial_event_t);
typedef struct serial_event_h_t serial_event_h_t;

/** List of the serial console parsers settings */
typedef struct serial_parser_t {
    char    name[TE_SERIAL_MAX_NAME + 1];   /**< Name of the parser */
    char    c_name[TE_SERIAL_MAX_NAME + 1]; /**< The serial console name */
    char    user[TE_SERIAL_MAX_NAME + 1];   /**< User name */
    char    mode[TE_SERIAL_MAX_NAME + 1];   /**< Share mode */

    te_bool enable;                     /**< The parser thread state */
    int     port;                       /**< The serial console port */
    int     interval;                   /**< Intreval of polling console */
    te_bool logging;                    /**< Enable logging from console
                                             to the main log */
    int     level;                      /**< Message level for logging */
    char    log_user[TE_SERIAL_MAX_NAME + 1];   /**< Logger user name */
    te_bool rcf;                        /**< Launched via RCF */

    pthread_t                     thread;     /**< Thread identifier */
    pthread_mutex_t               mutex;      /**< Provides access to this
                                                   struct */
    serial_event_h_t              events;     /**< List of the events */
    SLIST_ENTRY(serial_parser_t)  ent_pars_l; /**< Elements connector */
} serial_parser_t;

/**
 * Initialize serial console parser configuration
 *
 * @return Status code
 */
extern te_errno ta_unix_serial_parser_init(void);

/**
 * Cleanup serial console parser
 *
 * @return Status code
 */
extern te_errno ta_unix_serial_parser_cleanup(void);

/**
 * Read data from parser thread
 *
 * @param parser    Pointer to the parser config structure
 */
extern int te_serial_parser(serial_parser_t *parser);

/**
 * Entry point to the parser thread
 *
 * @param parser    Pointer to the parser config structure
 */
extern int te_serial_parser_connect(serial_parser_t *parser);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_SERIAL_PARSER_H__ */
/**@} <!-- END te_tools_te_serial_parser --> */
