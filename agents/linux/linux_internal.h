/** @file
 * @brief Linux Test Agent
 *
 * Linux TA definitions
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __LINUX_INTERNAL_H__
#define __LINUX_INTERNAL_H__

/** TA name pointer */
extern char *ta_name;

/** Command line */ 
extern char *ta_execname;

/* TA pid */
extern int ta_pid;

extern int ftp_open(char *uri, int flags, int passive, int offset);

/* Work-around for system/waitpid problem */
extern int ta_system(char *cmd);

#endif /* __LINUX_INTERNAL_H__ */
