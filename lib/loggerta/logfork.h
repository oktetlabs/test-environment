/** @file
 * @brief Logger subsystem API - TA side 
 *
 * TA side Logger functionality for
 * forked TA processes and newly created threads
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Mamadou Ngom <Mamadou.Ngom@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_LIB_LOGFORK_H__
#define __TE_LIB_LOGFORK_H__
    
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Register process name and pid, so it would be 
 * possible to know from which process or thread message
 * has been sent.
 *
 * @param name  process or thread name
 *
 * @retval  0 success
 * @retval -1 failure
 */
extern int logfork_register_user(const char *name);

/** 
 * Entry point for log gathering.
 */
extern void logfork_entry(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif  
#endif /* !__TE_LIB_LOGFORK_H__ */
