/** @file
 * @brief CS Common Definitions 
 *
 * CS-related definitions used on TA and Engine applications 
 * (including tests).
 *
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_CS_COMMON_H__
#define __TE_CS_COMMON_H__

/** Neighbour entry states, see /agent/interface/neigh_dynamic/state */
typedef enum {
    CS_NEIGH_INCOMPLETE = 1, /**< Incomplete entry */
    CS_NEIGH_REACHABLE = 2,  /**< Complete up-to-date entry */
    CS_NEIGH_STALE = 3,      /**< Complete, but possibly out-of-date - entry
                                  can be used but should be validated */
    CS_NEIGH_DELAY = 4,      /**< Intermediate state between stale and 
                                  probe */
    CS_NEIGH_PROBE = 5,      /**< Entry is validating now */
    CS_NEIGH_FAILED = 6      /**< Neighbour is not reachable */
} cs_neigh_entry_state;

#endif /* !__TE_CS_COMMON_H__ */
