/** @file
 * @brief Unix Test Agent serial console parser support.
 *
 * Defenition of unix TA serial console parse configuring support.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_SERIAL_PARSER_H__
#define __TE_SERIAL_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"

/* Max length of the parser name */
#define TE_SERIAL_MAX_NAME  63
/* Max pattern length */
#define TE_SERIAL_MAX_PATT  255
/* Base size of the buffer for lists */
#define PARSER_LIST_SIZE    512
/* Default value of polling interval */
#define TE_SERIAL_INTERVAL  100
/* Default conserver port */
#define TE_SERIAL_PORT      3109
/* Default user name */
#define TE_SERIAL_USER      "tester"
/* Default log level */
#define TE_SERIAL_LLEVEL    "WARN"

#define TE_SERIAL_MALLOC(ptr, size)       \
    if ((ptr = malloc(size)) == NULL)   \
        assert(0);

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
 * Entry point to the parser thread
 * 
 * @param parser    Pointer to the parser config structure
 */
extern int te_serial_parser(serial_parser_t *parser);

/**
 * Log host serial output via Logger component
 *
 * @param ready       Parameter release semaphore
 * @param argc        Number of string arguments
 * @param argv        String arguments:
 *                    - log user
 *                    - log level
 *                    - message interval
 *                    - tty name
 *                    (if it does not start with "/", it is interpreted
 *                     as a conserver connection designator)
 *                    - sharing mode (opt)
 */
extern int serial_console_log(void *ready, int argc, char *argv[]);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_SERIAL_PARSER_H__ */
