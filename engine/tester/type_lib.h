/** @file
 * @brief Tester Subsystem
 *
 * Types support library declaration.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TESTER_TYPE_LIB_H__
#define __TE_TESTER_TYPE_LIB_H__

#include "te_errno.h"

#include "tester_conf.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize types support library.
 *
 * @return Status code.
 */
extern te_errno tester_init_types(void);

/**
 * Find type by name in current context.
 *
 * @param session       Test session context
 * @param name          Name of the type to find
 *
 * @return Pointer to found type or NULL.
 */
extern const test_value_type * tester_find_type(const test_session *session,
                                                const char         *name);

/**
 * Register new type in the current context.
 *
 * @param session       Test session context
 * @param type          Type description
 */
extern void tester_add_type(test_session *session, test_value_type *type);


/**
 * Check that plain value belongs to type.
 *
 * @param type          Type description
 * @param plain         Plain value
 *
 * @return Pointer to corresponding value description or NULL.
 */
extern const test_entity_value * tester_type_check_plain_value(
                                     const test_value_type *type,
                                     const char            *plain);

#endif /* !__TE_TESTER_TYPE_LIB_H__ */
