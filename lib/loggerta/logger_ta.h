/** @file
 * @brief Logger subsystem API - TA side 
 *
 * API provided by Logger subsystem to users on TA side.
 * Set of macros provides compete functionality for debugging
 * and tracing and highly recommended for using in the source
 * code. 
 *
 * There are facilities for both fast and slow  logging. 
 * Fast logging(LOGF_... macros) does not copy
 * bytes arrays and strings into local log buffer. Only start 
 * memory address and length of dumped memory are regestered.
 * So, user has to take care about validity of logged data 
 * (logged data should not be volatile).
 * Slow logging (LOGS_... macros) parses log message format string
 * and copy dumped memory and strings into local log buffer and user 
 * has not to take care about validity of logged data.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 * @author Igor B. Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __LGR_LOGGER_TA_H__
#define __LGR_LOGGER_TA_H__

#include "logger_ta_internal.h"
#include "logger_defs.h"
#include "logger_api.h"

#define LGRF_MESSAGE(_lvl, _lgruser, _fs, _args...) \
    do {                                                            \
        if (TE_LOG_LEVEL & (_lvl))                                  \
        {                                                           \
            log_message_fast(_lvl, _lgruser, _fs, LARG12(_args));   \
        }                                                           \
    } while (0) 
    
/**< Logging abnormal/unexpected situations */
#define LOGF_ERROR(_us, _fs...)  LGRF_MESSAGE(TE_LL_ERROR, _us, _fs)
#define F_ERROR(_fs...)          LOGF_ERROR(TE_LGR_USER, _fs)

/**< Logging situations same failed initialization of the optional feature */
#define LOGF_WARN(_us, _fs...)  LGRF_MESSAGE(TE_LL_WARN, _us, _fs)
#define F_WARN(_fs...)          LOGF_WARN(TE_LGR_USER, _fs)
  
/**< Logging very important event in TE and tests */ 
#define LOGF_RING(_us, _fs...)  LGRF_MESSAGE(TE_LL_RING, _us, _fs)
#define F_RING(_fs...)          LOGF_RING(TE_LGR_USER, _fs)

/**< Logging important event for debugging of the test */
#define LOGF_INFO(_us, _fs...)  LGRF_MESSAGE(TE_LL_INFO, _us, _fs)
#define F_INFO(_fs...)          LOGF_INFO(TE_LGR_USER, _fs)
   
/**< Logging additional events for detalization of processing */
#define LOGF_VERB(_us, _fs...)  LGRF_MESSAGE(TE_LL_VERB, _us, _fs)
#define F_VERB(_fs...)          LOGF_VERB(TE_LGR_USER, _fs)


    
/**< Logging of entry to and exit from function */    
#define LOGF_SUPENTRY(_us, _fs, _args...) \
    do {                                              \
        log_message_fast(TE_LL_ENTRY_EXIT, _us,       \
                         "ENTRY to %s(): " _fs,       \
                         4, (uint32_t)__FUNCTION__,   \
                         LARG10(_args));              \
    } while (0)


#define LOGF_ENTRY(_us, _fs...) \
    do {                                                                \
        if (TE_LOG_LEVEL & TE_LL_ENTRY_EXIT)                            \
        {                                                               \
            if (!!(#_fs[0]))                                            \
                LOGF_SUPENTRY(_us, _fs);                                \
            else                                                        \
                log_message_fast(TE_LL_ENTRY_EXIT, _us,                 \
                                 "ENTRY to %s()",                       \
                                 4, (uint32_t)__FUNCTION__,             \
                                 LARG10());                             \
        }                                                               \
    } while (0)
    
#define F_ENTRY(_fs...)  LOGF_ENTRY(TE_LGR_USER, _fs)    
    
#define LOGF_SUPEXIT(_us, _fs, _args...) \
    do {                                                        \
        log_message_fast(TE_LL_ENTRY_EXIT, _us,                 \
                         "EXIT in line %d from %s(): " _fs,     \
                         4, __LINE__,                           \
                         4, (uint32_t)__FUNCTION__,             \
                         LARG9(_args));                         \
    } while (0)
  
#define LOGF_EXIT(_us, _fs...)                                  \
    do {                                                        \
        if (TE_LOG_LEVEL & TE_LL_ENTRY_EXIT)                    \
        {                                                       \
            if (!!(#_fs[0]))                                    \
                LOGF_SUPEXIT(_us, _fs);                         \
            else                                                \
                log_message_fast(TE_LL_ENTRY_EXIT, _us,         \
                                 "EXIT in line %d from %s()",   \
                                 4, __LINE__,                   \
                                 4, (uint32_t)__FUNCTION__,     \
                                 LARG9());                      \
        }                                                       \
    } while (0)

                
#define F_EXIT(_fs...)  LOGF_EXIT(TE_LGR_USER, _fs)     
   

#ifdef _cplusplus
extern "C" {
#endif

/* Some functions for interacting with logger resourcese on TA side */

/**
 * Initialize Logger resources on the Test Agent side (log buffer, 
 * log file and so on).
 *     
 * @return  Operation status.
 *
 * @retval  0  Success.
 * @retval -1  Failure.       
 */
extern int log_init(void);

/**
 * Finish Logger activity on the Test Agent side (fluhes buffers 
 * in the file if that means exists and so on).
 *     
 * @return  Operation status.
 *
 * @retval  0  Success.
 * @retval -1  Failure.        
 */ 
extern int log_shutdown(void);

/**
 * Request the log messages accumulated in the Test Agent local log 
 * buffer. Passed messages will be deleted from local log.
 *     
 * @param  buf_length   Length of the transfer buffer.
 * @param  transfer_buf Pointer to the transfer buffer.
 * 
 * @retval  Length of the filled part of the transfer buffer in bytes
 */
extern uint32_t log_get(uint32_t buf_length, uint8_t *transfer_buf);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* __TE_LOGGER_TA_H__ */
