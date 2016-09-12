/** @file
 * @brief TAD API
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions which may be used outside RCF.
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 */

#ifndef __TE_TAD_API__
#define __TE_TAD_API__

#include "te_errno.h"
#include "tad_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create CSAP.
 *
 * @param stack         CSAP protocols stack
 * @param spec_str      CSAP specification string
 * @param new_csap_p    Location for CSAP instance
 *
 * @return Status code.
 */
extern te_errno tad_csap_create(const char *stack, const char *spec_str,
                                csap_p *new_csap_p);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_API__ */
