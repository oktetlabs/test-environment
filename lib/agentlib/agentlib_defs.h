/** @file
 * @brief Agent support
 *
 * Type definitions for agent libraries
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
