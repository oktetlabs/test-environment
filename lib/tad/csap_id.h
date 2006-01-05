/** @file
 * @brief TAD CSAP IDs
 *
 * Traffic Application Domain Command Handler.
 * Definition of CSAP IDs database functions.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_CSAP_ID_H__
#define __TE_TAD_CSAP_ID_H__ 

#include "tad_common.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize CSAP IDs database.
 */
extern void csap_id_init(void);

/**
 * Destroy CSAP IDs database.
 */
extern void csap_id_destroy(void);


/**
 * Allocate a new CSAP ID and associate it with specified pointer.
 *
 * @param ptr       Associated pointer (must be not equal to NULL)
 *
 * @return A new CSAP ID or CSAP_INVALID_HANDLE
 */
extern csap_handle_t csap_id_new(void *ptr);

/**
 * Get pointer associated with CSAP ID.
 *
 * @param csap_id   CSAP ID
 *
 * @return Associated pointer or NULL
 */
extern void *csap_id_get(csap_handle_t csap_id);

/**
 * Forget CSAP ID.
 *
 * @param csap_id   CSAP ID
 *
 * @return Associated pointer or NULL
 */
extern void *csap_id_delete(csap_handle_t csap_id);


/**
 * Prototype of callback function for CSAP IDs enumeration.
 *
 * @param csap_id   CSAP ID
 * @param ptr       Associated pointer
 *
 * @retval TRUE     Continue
 * @retval FALSE    Break
 */
typedef void (*csap_id_enum_cb)(csap_handle_t csap_id, void *ptr);

/**
 * Enumerate all known CSAP IDs.
 *
 * @param cb        Function to be called for each CSAP ID
 *
 * It is allowed to delete enumerated item from callback.
 */
extern void csap_id_enum(csap_id_enum_cb cb);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAD_CSAP_ID_H__ */
