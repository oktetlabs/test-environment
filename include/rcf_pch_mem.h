/** @file
 * @brief RCF Portable Command Handler
 *
 * Memory mapping library
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_PCH_MEM_H__
#define __TE_RCF_PCH_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Library for registering/unregistering of memory addresses */

/** An identifier corresponding to memory address */
typedef uint32_t rcf_pch_mem_id;

/**
 * Assign the identifier to memory.
 *
 * @param mem       location of real memory address (in)
 *
 * @return Memory identifier or 0 in the case of failure
 */
extern rcf_pch_mem_id rcf_pch_mem_alloc(void *mem);

/**
 * Mark the memory identifier as "unused".
 *
 * @param id       memory identifier returned by rcf_pch_mem_alloc
 */     
extern void rcf_pch_mem_free(rcf_pch_mem_id id);

/**
 * Mark the memory identifier corresponding to memory address as "unused".
 *
 * @param mem   memory address
 */     
extern void rcf_pch_mem_free_mem(void *mem);

/**
 * Obtain address of the real memory by its identifier.
 *
 * @param id       memory identifier returned by rcf_pch_mem_alloc
 *
 * @return Memory address or NULL
 */
extern void *rcf_pch_mem_get(rcf_pch_mem_id id);

/**
 * Find memory identifier by memory address.
 *
 * @param mem   memory address
 *
 * @return memory identifier or 0
 */     
extern rcf_pch_mem_id rcf_pch_mem_get_id(void *mem);

/*@}*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_PCH_MEM_H__ */
