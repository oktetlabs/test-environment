/** @file
 * @brief Tester Subsystem
 *
 * Auxiliary data types.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TESTER_DEFS_H__
#define __TE_TESTER_DEFS_H__

#include "te_queue.h"
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Test identification number for
 * prologues, epilogues, sessions, packages.
 */
#define TE_TIN_INVALID  ((unsigned int)(-1))


/** Types of run items */
typedef enum run_item_type {
    RUN_ITEM_NONE,
    RUN_ITEM_SCRIPT,
    RUN_ITEM_SESSION,
    RUN_ITEM_PACKAGE
} run_item_type;

/** Run item role values */
typedef enum run_item_role {
    RI_ROLE_NORMAL,
    RI_ROLE_PROLOGUE,
    RI_ROLE_EPILOGUE,
    RI_ROLE_KEEPALIVE,
} run_item_role;

/** Convert role value to string */
static inline const char *
ri_role2str(run_item_role role)
{
    static const char * const role_names[] = {
        [RI_ROLE_NORMAL] = NULL,
        [RI_ROLE_PROLOGUE] = "prologue",
        [RI_ROLE_EPILOGUE] = "epilogue",
        [RI_ROLE_KEEPALIVE] = "keepalive",
    };

    return role_names[role];
}

/** Test ID */
typedef int test_id;

/** Is SIGINT signal received by Tester? */
extern te_bool tester_sigint_received;


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
#endif /* !__TE_TESTER_DEFS_H__ */
