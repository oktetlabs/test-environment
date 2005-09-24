/** @file
 * @brief Logger Common Definitions
 *
 * Some common definitions is used by Logger process library, TEN side
 * Logger lib, TA side Logger lib.
 *
 * DO NOT include this file directly.
 * 
 * 
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Igor Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_DEFS_H__
#define __TE_LOGGER_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Every TE process (engine application, test, test agent) must call
 * this macro in its main C file to define 'te_lgr_entity' global
 * variable with specified initial value.
 *
 * @name name       Logger entity name (e.g. RCF)
 */
#define DEFINE_LGR_ENTITY(name) \
const char *te_lgr_entity = name

/**
 * Global variable with name of the Logger entity to be used from
 * libraries to log from this process context.
 *
 * @note It MUST be initialized to some value to avoid segmentation fault.
 */
extern const char *te_lgr_entity;

/*
 * Default value should be overriden by user in user' module before 
 * inclusion of logger_api.h or like this:
 *     #undef   TE_LGR_ENTITY
 *     #define  TE_LGR_ENTITY <"SOMEENTITY">
 *
 * @attention It's highly not recommended to use/define it.
 *            Use DEFINE_LGR_ENTITY().
 */
#ifndef TE_LGR_ENTITY
/** Default entity name */
#define TE_LGR_ENTITY  te_lgr_entity
#endif

/*
 * Default value should be overriden by user in user' module before 
 * inclusion of logger_api.h or like this:
 *     #undef   TE_LGR_USER
 *     #define  TE_LGR_USER <"SOMEUSER">
 */
#ifndef TE_LGR_USER
/** Default entity name */
#define TE_LGR_USER     "Self"
#endif


/*
 * 16-bit field is provided for TE_LOG_LEVEL masks
 */

/** Any abnormal/unexpected situation (ERROR macro) */    
#define TE_LL_ERROR         0x0001

/** It's not error situation, but may be an error (WARN macro) */    
#define TE_LL_WARN          0x0002

/** Very important event in TE and tests (RING macro) */
#define TE_LL_RING          0x0004

/** Important events required for test debugging (INFO macro) */    
#define TE_LL_INFO          0x0008

/** Verbose logging of entity internals (VERB macro) */    
#define TE_LL_VERB          0x0010

/* 
 * Logging for function entry/exit point with function name 
 * and line number (for exit point only). (ENTRY,EXIT macros)
 */    
#define TE_LL_ENTRY_EXIT    0x0020


/*
 * Override default level macros into the user' module for logging 
 * behaviour tunning at compilation time.
 * For example, into user' module the logging level can be modified 
 * as follows:
 *
 *     #define TE_LOG_LEVEL   <level>
 *
 * Do NOT insert '#undef TE_LOG_LEVEL' before in order to see warning 
 * during build and do not forget about overriden system wide 
 * TE_LOG_LEVEL.
 */
#ifndef TE_LOG_LEVEL
/** Default log level */
#define TE_LOG_LEVEL    (TE_LL_ERROR | TE_LL_WARN | TE_LL_RING)
#endif


/**
 * Logging backend function.
 *
 * @param file      Name of the file with the log message
 * @param line      Line in the @a file with the log message
 * @param level     Log level
 * @param entity    Entity name whose user generates this message
 * @param user      Arbitrary "user name"
 * @param fmt       Log message format string. This string should contain
 *                  conversion specifiers if some arguments follows
 * @param ...       Arguments passed into the function according
 *                  to log message format string
 */
typedef void (* te_log_message_f)(const char   *file,
                                  unsigned int  line,
                                  unsigned int  level,
                                  const char   *entity,
                                  const char   *user,
                                  const char   *fmt, ...);

/** Logging backend */
extern te_log_message_f te_log_message;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGGER_DEFS_H__ */
