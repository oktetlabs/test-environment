/** @file
 * @brief Testing Results Comparator
 *
 * Definition of data types and API to use TRC.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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

#ifndef __TE_TRC_H__
#define __TE_TRC_H__


#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration of the Testing Results Comparator instance */
struct trc;

/**
 * Initialize TRC instance.
 *
 * @param db            Path to the database
 * @param iut_id        IUT identification tags
 * @param p_trci        Location for TRC instance handle
 *
 * @return Status code.
 */
extern te_errno trc_init(const char *db, const trc_tags *iut_id,
                         struct trc **p_trci);

/**
 * Free TRC instance.
 *
 * @param trci          TRC instance handle
 */
extern void trc_free(struct trc *trci);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_H__ */
