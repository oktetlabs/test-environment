/** @file
 * @brief API to modify target requirements from prologues
 *
 * Declaration of API to modify target requirements from prologues.
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

#ifndef __TE_TAPI_REQS_H__
#define __TE_TAPI_REQS_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add exclusion of test which match corresponding requirements
 * expression.
 *
 * @param reqs          Requirements expression
 *
 * @return Status code.
 */
extern te_errno tapi_reqs_exclude(const char *reqs);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_REQS_H__ */
