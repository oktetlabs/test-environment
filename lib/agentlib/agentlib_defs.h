/** @file
 * @brief Agent support
 *
 * Type definitions for agent libraries
 *
 * Copyright (C) 2004-2016 Test Environment authors (see file AUTHORS
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
 * @author Artem V. Andreev   <Artem.Andreev@oktetlabs.ru>
 *
 * @note This header is separated from principal `agentlib.h`, so that
 * it can be included into a automatically generated symbol table
 * definitions, without causing symbol definition mismatch
 * $Id$
 */
 
#ifndef __TE_AGENTLIB_DEFS_H__
#define __TE_AGENTLIB_DEFS_H__

#include "te_config.h"
#include "te_defs.h"
#include "te_errno.h"
#include "agentlib_config.h"

/**
 * Manual symbol table entry
 */
typedef struct rcf_symbol_entry {
    const char *name;     /**< Name of a symbol */
    void       *addr;     /**< Symbol address */
    te_bool     is_func;  /**< Whether the symbol is function or variable */
} rcf_symbol_entry;

#endif /* __TE_AGENTLIB_DEFS_H__ */
