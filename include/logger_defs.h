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


/*
 * Default value should be overriden by user in user' module before 
 * inclusion of logger_api.h or like this:
 *     #undef   LGR_ENTITY
 *     #define  LGR_ENTITY <"SOMEENTITY">
 */
#ifndef LGR_ENTITY
/** Default entity name */
#define LGR_ENTITY  te_lgr_entity
#endif


/*
 * 16-bit field is provided for LOG_LEVEL masks
 */

/** Any abnormal/unexpected situation (ERROR macro) */    
#define ERROR_LVL         0x0001

/** It's not error situation, but may be an error (WARN macro) */    
#define WARNING_LVL       0x0002

/** Very important event in TE and tests (RING macro) */
#define RING_LVL          0x0004

/** Important events required for test debugging (INFO macro) */    
#define INFORMATION_LVL   0x0008

/** Verbose logging of entity internals (VERB macro) */    
#define VERBOSE_LVL       0x0010

/* 
 * Logging for function entry/exit point with function name 
 * and line number (for exit point only). (ENTRY,EXIT macros)
 */    
#define ENTRY_EXIT_LVL    0x0020



/* !!!!!!!!!  Temporary definitions !!!!!!!!!! */
#define DEBUG_LEVEL       1
#define TRACE_LEVEL       0x02
#define TRACE_ENTRYEXIT   0x80


/*
 * Override default level macros into the user' module for logging 
 * behaviour tunning at compilation time.
 * For example, into user' module the logging level can be modified 
 * as follows:
 *
 *     #define LOG_LEVEL   <level>
 *
 * Do NOT insert '#undef LOG_LEVEL' before in order to see warning 
 * during build and do not forget about overriden system wide 
 * LOG_LEVEL.
 */
#ifndef LOG_LEVEL
/** Default log level */
#define LOG_LEVEL    (ERROR_LVL | WARNING_LVL | RING_LVL)
#endif



/** Current log version */
#define LGR_LOG_VERSION  1

/* Appropriate fields length in the raw log file format */
#define LGR_NFL_FLD           1
#define LGR_VER_FLD           1
#define LGR_TIMESTAMP_FLD     8
#define LGR_LEVEL_FLD         2
#define LGR_MESS_LENGTH_FLD   2
#define LGR_FIELD_MAX         255
#define LGR_FILE_MAX          7


/** Convert value to the network order */
#define LGR_16_TO_NET(_val, _res) \
    (((int8_t *)(_res))[0] = (int8_t)(((_val)>>8) & 0xff), \
     ((int8_t *)(_res))[1] = (int8_t)((_val) & 0xff))

#define LGR_32_TO_NET(_val, _res) \
    (((int8_t *)(_res))[0] = (int8_t)(((_val)>>24) & 0xff), \
     ((int8_t *)(_res))[1] = (int8_t)(((_val)>>16) & 0xff), \
     ((int8_t *)(_res))[2] = (int8_t)(((_val)>>8) & 0xff),  \
     ((int8_t *)(_res))[3] = (int8_t)((_val) & 0xff))


/* ==== Test Agent Logger lib definitions */

/*
 * If it's needed override default value to release oldest message
 * and register new one when unused ring buffer resources is absent
 * as following:
 *  #undef LGR_RB_FORCE
 *  #define LGR_RB_FORCE 1
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Global variable with name of the Logger entity to be used from
 * libraries to log from this process context.
 *
 * @note It MUST be initialized to some value to avoid segmentation fault.
 */
extern const char *te_lgr_entity;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOGGER_DEFS_H__ */
