/** @file
 * @brief Testing Results Comparator
 *
 * Logging macros
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TOOLS_TRC_LOG_H__
#define __TE_TOOLS_TRC_LOG_H__

#include <stdio.h>

#define LOG(fmt_...) \
    do {                        \
        fprintf(stderr, fmt_);  \
        fprintf(stderr, "\n");  \
    } while (0)

#define ERROR(fmt_...)     LOG("ERROR: " fmt_)
#define INFO(fmt_...)

#endif /* !__TE_TOOLS_TRC_LOG_H__ */
