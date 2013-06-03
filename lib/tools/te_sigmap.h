/** @file
 * @brief Mapping of unix signal name->number and number->name
 *
 * Definition of the mapping functions.
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
 
#ifndef __TE_SIGMAP_H__
#define __TE_SIGMAP_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Map signal name to the number
 * 
 * @param name  Name of the signal
 * 
 * @return Signal number
 */
extern int map_name_to_signo(const char *name);


/**
 * Map signal number to the name
 * 
 * @param name  Number of the signal
 * 
 * @return Signal name or NULL
 */
extern char * map_signo_to_name(int signo);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef __TE_SIGMAP_H__ */
