/** @file
 * @brief Logger subsystem API - TA side 
 *
 * TA side Logger functionality for
 * forked TA processes and newly created threads
 *
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Mamadou Ngom <Mamadou.Ngom@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_LOGFORK_H__
#define __TE_LOGFORK_H__

    
#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of the logging message */
#define LOGFORK_MAXLEN  256

/**
 * Register process name and pid, so it would be 
 * possible to know from which process or thread message
 * has been sent.
 *
 * @param name  process or thread name
 */
extern void logfork_register_user(const char *name);

/** 
 * Entry point for log gathering.
 */
extern void logfork_entry(void);

/** 
 * Function for logging to be used by forked processes
 *
 * @param level logging level
 * @param fmt   format string
 */
extern void logfork_log_message(int level, char *lgruser, const char *fmt, ...);


#ifdef __cplusplus
}  /* extern "C" */
#endif  
#endif  /* __TE_LOGFORK_H__*/

#ifdef LOGFORK_LOG

#undef LGR_MESSAGE
#define LGR_MESSAGE(_lvl, _lgruser, _fs...)  logfork_log_message(_lvl, _lgruser, _fs) 

#endif
