/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides some functions prototypes and macros
 * for internal using by Logger modules.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __LGR_LOGGER_INTERNAL_H__
#define __LGR_LOGGER_INTERNAL_H__
#ifdef _cplusplus
extern "C" {
#endif

#include "config.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "ipc_server.h"
#include "rcf_api.h"
#include "logger_api.h"


/** Default TA polling timeout */
#define LGR_TA_POLL_DEF         1000000     /* 1 second */

/** 
 * Maximum number of messages to be get during flush.
 * It is required to cope with permanent logging on TA
 * with broken time synchronization.
 */
#define LGR_FLUSH_TA_MSG_MAX    1000

/** Maximum length of the Logger IPC server name */
#define LGR_MAX_NAME            (strlen(LGR_SRV_FOR_TA_PREFIX) + \
                                 RCF_MAX_PATH)

/** Logger flush command */
#define LGR_FLUSH               "LGR-FLUSH"
/** Logger shutdown command */
#define LGR_SHUTDOWN            "LGR-SHUTDOWN"


#define LGR_TANAMES_LEN   1024


#define LGR_PUT_FLTR(_hdr, _ftype, _preg) \
    do {                                                            \
            re_fltr *tmp_el =                                       \
                (struct re_fltr *)malloc(sizeof(struct re_fltr));   \
            (_hdr)->last->next = tmp_el;                            \
            (_hdr)->last = tmp_el;                                  \
            tmp_el->type = (_ftype);                                \
            tmp_el->preg = (_preg);                                 \
            tmp_el->next = (_hdr);                                  \
    } while (0)

#define LGR_FREE_FLTR_LIST(_hdr) \
    do {                                        \
        while ((_hdr)->next != (_hdr))          \
        {                                       \
            (_hdr)->last = (_hdr)->next;        \
            (_hdr)->next = (_hdr)->next->next;  \
            regfree((_hdr)->last->preg);        \
            free((_hdr)->last);                 \
        }                                       \
        (_hdr)->next = (_hdr);                  \
        (_hdr)->last = (_hdr);                  \
    } while (0)


/** Filter type */
enum flt_type {
    LGR_EXCLUDE = 0,    /**< Exclude filter */
    LGR_INCLUDE         /**< Include filter */
};

/** Filter list should be in normal order not inverse */
typedef struct re_fltr {
    struct re_fltr *next;   /**< Pointer to the next structure */
    struct re_fltr *last;
    uint32_t        type;
    regex_t        *preg;   /**< Pattern buffer storage area */
} re_fltr;

/** Node of the TA single linked list */
typedef struct ta_inst {
    struct ta_inst *next;                /**< Pointer to the next
                                              structure */
    char            agent[RCF_MAX_NAME]; /**< TA name */
    char            type[RCF_MAX_NAME];  /**< Type assigned for TA or
                                              NULL */
    uint32_t        sequence;            /**< Incoming message sequence
                                              nmbr */
    int             polling;             /**< Polling parameter value */
    te_bool         thread_run;          /**< Is thread running? */
    pthread_t       thread;              /**< Thread identifier */
    int             flush_log;           /**< 0 - normal processing;
                                              1 - flush TA local log */
    struct re_fltr  filters;             /**< Filters formed within Logger
                                              configuration file parsing */
} ta_inst;


/** Node of the TEN entities single linked list */
typedef struct te_inst {
    struct te_inst *next;                 /**< Pointer to the next 
                                               structure */
    char            entity[RCF_MAX_NAME]; /**< TEN entity name */
    struct re_fltr  filters;              /**< Filters formed within Logger
                                               configuration file parsing */
} te_inst;


/**
 * Parse logger configuration file.
 *
 * @param  fileName  XML configuration file full name.
 *
 * @return  Status information
 *
 * @retval   0          Success.
 * @retval   Negative   Failure.
 */
extern int configParser(const char *file_name);

/**
 * Register the log message in the raw log file.
 *
 * @param buf_mess  Log message location.
 * @param buf_len   Log message length.
 */
extern void lgr_register_message(const void *buf_mess, size_t buf_len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __LGR_LOGGER_INTERNAL_H__ */
