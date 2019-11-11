/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides some functions prototypes and macros
 * for internal using by Logger modules.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOGGER_INTERNAL_H__
#define __TE_LOGGER_INTERNAL_H__
#ifdef _cplusplus
extern "C" {
#endif

#include "te_config.h"

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
#include "te_log_sniffers.h"


/** Default TA polling timeout in milliseconds */
#define LGR_TA_POLL_DEF         1000     /* 1 second */

/** 
 * Maximum number of messages to be get during flush.
 * It is required to cope with permanent logging on TA
 * with broken time synchronization.
 */
#define LGR_FLUSH_TA_MSG_MAX    1000

/** Maximum length of the Logger IPC server name */
#define LGR_MAX_NAME            (strlen(LGR_SRV_FOR_TA_PREFIX) + \
                                 RCF_MAX_PATH)

/** Logger shutdown command */
#define LGR_SHUTDOWN            "LGR-SHUTDOWN"


#define LGR_TANAMES_LEN   1024

/** Overfill type constans */
typedef enum overfill_type {
    ROTATION   = 0, /**< Overfill type rotation */
    TAIL_DROP  = 1, /**< Overfill type tail drop */
} overfill_type;

/** Cpature logs polling variables */
typedef struct snif_polling_sets_t {
    char            dir[RCF_MAX_PATH];  /**< Cpature logs directory */
    char            name[RCF_MAX_PATH]; /**< File name template */
    unsigned        osize;              /**< Max logs cumulative size */
    unsigned        sn_space;           /**< Max total capture files size
                                             for one sniffer */
    unsigned        fsize;              /**< Max file size for each
                                             sniffer */
    unsigned        rotation;           /**< Rotate agent side temporary
                                             logs across *x* files */
    unsigned        period;             /**< Period for capture logs
                                             polling */
    overfill_type   ofill;              /**< Overfill handle method */
    te_bool         errors;             /**< Errors flag */
} snif_polling_sets_t;

/** Node of the TA single linked list */
typedef struct ta_inst {
    struct ta_inst *next;                /**< Pointer to the next
                                              structure */
    char            agent[RCF_MAX_NAME]; /**< TA name */
    char            type[RCF_MAX_NAME];  /**< Type assigned for TA or
                                              NULL */
    uint32_t        sequence;            /**< Incoming message sequence
                                              nmbr */
    int             polling;             /**< Polling parameter value
                                              (in milliseconds) */
    te_bool         thread_run;          /**< Is thread running? */
    pthread_t       thread;              /**< Thread identifier */
    int             flush_log;           /**< 0 - normal processing;
                                              1 - flush TA local log */
} ta_inst;


/** Create message and register it in the raw log file. */
extern te_log_message_f lgr_log_message;


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

/**
 * Check the logger shutdown flag.
 * 
 * @return TRUE if sthe flag is active else FALSE.
 */
extern te_bool te_log_check_shutdown(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_INTERNAL_H__ */
