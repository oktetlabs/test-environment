/** @file
 * @brief Sniffer process defenition
 *
 * Defenition of sniffer process function.
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
 * $Id: $
 */

#ifndef __TE_SNIFFER_PROC_H__
#define __TE_SNIFFER_PROC_H__

/**
 * Launch the sniffer process.
 */
extern int 
te_sniffer_process(int argc, char *argv[]);

#endif /* ndef __TE_SNIFFER_PROC_H__ */
