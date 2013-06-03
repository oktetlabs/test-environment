/** @file
 * @brief Test API of serial console parsers
 *
 * Definition of API to configure serial console parsers.
 * See design document OKTL-0000469-SERIAL.txt.
 * User manual:
 *   https://oktetlabs.ru/dokuwiki/doku.php/te/serial_console_parser
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

#ifndef __TE_TAPI_SERIAL_PARSE_H__
#define __TE_TAPI_SERIAL_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Max length of the path to file */
#define TE_SERIAL_MAX_PATH  256
/* Max test agent name length */
#define TE_SERIAL_MAX_NAME  64
/* Defult name of log user */
#define TE_SERIAL_PARSER_USER "tester"
/* Defult port for connection to conserver */
#define TE_SERIAL_PARSER_PORT 3109

/** 
 * Structure describes the parser identificator
 */
typedef struct tapi_parser_id {
    char *ta;       /**< Test agent name */
    char *name;     /**< The parser name */
    char *c_name;   /**< Serial console name */
    char *user;     /**< A user name for the conserver or NULL. In case if
                         NULL is used, default value 'tester' will be
                         assigned. */
    int   port;     /**< Port of the conserver */
    int   interval; /**< Interval to poll data from the conserver.
                         Use -1 for default value. */
} tapi_parser_id;

/**
 * Create a new empty Tester event
 * 
 * @param name          Event name
 * 
 * @return Status code
 */
extern te_errno tapi_serial_tester_event_add(const char *name);

/**
 * Remove a Tester event
 * 
 * @param name          Event name
 */
extern void tapi_serial_tester_event_del(const char *name);

/**
 * Add a new external handler for event
 * 
 * @param ev_name       Event name
 * @param h_name        Handler name
 * @param priority      Handler priority
 * @param path          Path to the handler
 * 
 * @return Status code
 */
extern te_errno tapi_serial_handler_ext_add(const char *ev_name,
                                            const char *h_name,
                                            int priority,
                                            const char *path);

/**
 * Add a new internal handler for event
 * 
 * @param ev_name       Event name
 * @param h_name        Handler name
 * @param priority      Handler priority
 * @param signo         Signal number
 * 
 * @return Status code
 */
extern te_errno tapi_serial_handler_int_add(const char *ev_name,
                                            const char *h_name,
                                            int priority, int signo);

/**
 * Remove handler
 * 
 * @param ev_name       Event name
 * @param h_name        Handler name
 */
extern void tapi_serial_handler_del(const char *ev_name,
                                    const char *h_name);

/**
 * Initialization of the parser id. The function calls the malloc. User
 * should call cleanup function (tapi_serial_id_cleanup()) after use.
 * By default conserver port value 3109 will be assigned.
 * 
 * @param agent     Agent name
 * @param c_name    Serial console name
 * @param name      The parser name
 */
extern tapi_parser_id *tapi_serial_id_init(const char *agent,
                                      const char *c_name, const char *name);

/**
 * Cleanup and free the parser id
 */
extern void tapi_serial_id_cleanup(tapi_parser_id *id);

/**
 * Create and launch a new parser
 * 
 * @param id            Location of the parser id
 * 
 * @return Status code
 */
extern te_errno tapi_serial_parser_add(tapi_parser_id *id);

/**
 * Stop and remove parser
 * 
 * @param id            Location of the parser id
 */
extern void tapi_serial_parser_del(tapi_parser_id *id);

/**
 * Start a parser thread
 * 
 * @param id            Location of the parser id
 * 
 * @return Status code
 */
extern te_errno tapi_serial_parser_enable(tapi_parser_id *id);

/**
 * Stop a parser thread
 * 
 * @param id            Location of the parser id
 * 
 * @return Status code
 */
extern te_errno tapi_serial_parser_disable(tapi_parser_id *id);

/**
 * Enable logging of the serial console to main log
 * 
 * @param id            Location of the parser id
 * @param level         Level name of the message for logging(logger_defs.h)
 *                      Use NULL to no change the value.
 */
extern te_errno tapi_serial_logging_enable(tapi_parser_id *id,
                                           const char *level);

/**
 * Disable logging of the serial console to main log
 * 
 * @param id            Location of the parser id
 */
extern te_errno tapi_serial_logging_disable(tapi_parser_id *id);

/**
 * Add parser event to the parser
 * 
 * @param id            Location of the parser id
 * @param name          The parser event name
 * @param t_name        The Tester event name. Use a name of one of the
 *                      events declarated in the Tester subtree.
 * 
 * @return Status code
 */
extern te_errno tapi_serial_parser_event_add(tapi_parser_id *id,
                                             const char *name,
                                             const char *t_name);

/**
 * Delete parser event
 * 
 * @param id            Location of the parser id
 * @param name          The parser event name
 */
extern void tapi_serial_parser_event_del(tapi_parser_id *id,
                                         const char *name);

/**
 * Add a pattern to the parser event
 * 
 * @param id            Location of the parser id
 * @param e_name        The parser event name
 * @param pattern       The pattern string
 * 
 * @return Index number of pattern or @c -1 in case of failure
 */
extern int tapi_serial_parser_pattern_add(tapi_parser_id *id,
                                          const char *e_name,
                                          const char *pattern);

/**
 * Remove a pattern from the parser event
 * 
 * @param id            Location of the parser id
 * @param e_name        The parser event name
 * @param pat_i         Index of the pattern
 */
extern void tapi_serial_parser_pattern_del(tapi_parser_id *id,
                                           const char *e_name, int pat_i);

/**
 * Reset status for each event of the parser
 * 
 * @param id            Location of the parser id
 * 
 * @return Status code
 */
extern te_errno tapi_serial_parser_reset(tapi_parser_id *id);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_TAPI_SERIAL_PARSE_H__ */
