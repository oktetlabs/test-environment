/** @file
 * @brief Tester Subsystem
 *
 * Auxiliary data types.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TESTER_TOOLS_H__
#define __TE_TESTER_TOOLS_H__

#include "te_queue.h"
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Types of run items */
typedef enum run_item_type {
    RUN_ITEM_NONE,
    RUN_ITEM_SCRIPT,
    RUN_ITEM_SESSION,
    RUN_ITEM_PACKAGE
} run_item_type;

/** Test ID */
typedef int test_id;

/** Element of the list of strings */
typedef struct tqe_string {
    TAILQ_ENTRY(tqe_string)     links;  /**< List links */
    char                       *v;      /**< Value */
} tqe_string;

/** Head of the list of strings */
typedef TAILQ_HEAD(tqh_strings, tqe_string) tqh_strings;


/** Is SIGINT signal received by Tester? */
extern te_bool tester_sigint_received;


/**
 * Free list of allocated strings.
 *
 * @param head          Head of the list
 * @param value_free    Function to be called to free value or NULL
 */
extern void tq_strings_free(tqh_strings *head, void (*value_free)(void *));


/**
 * Set specified bit in bitmask.
 *
 * @param mem           Bit mask memory
 * @param bit           Bit number to set (starting from 0)
 */
static inline void
bit_mask_set(uint8_t *mem, unsigned int bit)
{
    mem[bit >> 3] |= (1 << (bit & 0x7));
}

/**
 * Clear specified bit in bitmask.
 *
 * @param mem           Bit mask memory
 * @param bit           Bit number to cleared (starting from 0)
 */
static inline void
bit_mask_clear(uint8_t *mem, unsigned int bit)
{
    mem[bit >> 3] &= ~(1 << (bit & 0x7));
}

/**
 * Is specified bit in bitmask set?
 *
 * @param mem           Bit mask memory
 * @param bit           Bit number to test (starting from 0)
 */
static inline te_bool
bit_mask_is_set(const uint8_t *mem, unsigned int bit)
{
    return !!(mem[bit >> 3] & (1 << (bit & 0x7)));
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_TOOLS_H__ */
