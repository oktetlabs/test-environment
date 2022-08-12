/** @file
 * @brief CS Common Definitions
 *
 * CS-related definitions used on TA and Engine applications
 * (including tests).
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

#ifndef __TE_CS_COMMON_H__
#define __TE_CS_COMMON_H__

/** Separator in values in which there are substitutions */
#define CS_SUBSTITUTION_DELIMITER "$$"

/** Neighbour entry states, see /agent/interface/neigh_dynamic/state */
typedef enum {
    CS_NEIGH_INCOMPLETE = 1, /**< Incomplete entry */
    CS_NEIGH_REACHABLE = 2,  /**< Complete up-to-date entry */
    CS_NEIGH_STALE = 3,      /**< Complete, but possibly out-of-date - entry
                                  can be used but should be validated */
    CS_NEIGH_DELAY = 4,      /**< Intermediate state between stale and
                                  probe */
    CS_NEIGH_PROBE = 5,      /**< Entry is validating now */
    CS_NEIGH_FAILED = 6,     /**< Neighbour is not reachable */
    CS_NEIGH_NOARP = 7       /**< Complete but without validating */
} cs_neigh_entry_state;

/**
 * String representation of neighbour entry state.
 */
static inline const char *
cs_neigh_entry_state2str(cs_neigh_entry_state state)
{
    switch (state)
    {
#define NEIGH_STATE2STR(_state) \
        case CS_NEIGH_##_state: return #_state

        NEIGH_STATE2STR(INCOMPLETE);
        NEIGH_STATE2STR(REACHABLE);
        NEIGH_STATE2STR(STALE);
        NEIGH_STATE2STR(DELAY);
        NEIGH_STATE2STR(PROBE);
        NEIGH_STATE2STR(FAILED);
        NEIGH_STATE2STR(NOARP);

#undef NEIGH_STATE2STR

        default:
            return "<UNKNOWN>";
    }
}

#endif /* !__TE_CS_COMMON_H__ */
