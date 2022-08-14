/** @file
 * @brief Unix Test Agent BPF support.
 *
 * Definition of unix TA BPF configuring support.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_AGENTS_UNIX_CONF_BPF_INNER_H__
#define __TE_AGENTS_UNIX_CONF_BPF_INNER_H__

#ifdef WITH_BPF

/**
 * Return BPF program descriptor according to program configurator
 * oid string.
 *
 * @param prog_oid_str Configurator oid string.
 *
 * @return Descriptor or -1 in case of error.
 */
extern int conf_bpf_fd_by_prog_oid(const char *prog_oid_str);

#else

#include "te_defs.h"
#include "logger_api.h"

static inline int
conf_bpf_fd_by_prog_oid(const char *prog_oid_str)
{
    UNUSED(prog_oid_str);

    ERROR("BPF is not supported");

    return -1;
}

#endif /* WITH_BPF */

#endif /* __TE_AGENTS_UNIX_CONF_BPF_INNER_H__ */
