/** @file
 * @brief Unix Test Agent BPF support.
 *
 * Definition of unix TA BPF configuring support.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Roman Zhukov <Roman.Zhukov@oktetlabs.ru>
 */

#ifndef __TE_CONF_BPF_H__
#define __TE_CONF_BPF_H__

#ifdef CFG_BPF_SUPPORT
#include "bpf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * On kernels < 4.18 struct bpf_info_prog hasn't requried fields,
 * so it's needed to define minimal required version for such kernels.
 */
#ifdef HAVE_STRUCT_BPF_PROG_INFO_JITED_FUNC_LENS

typedef struct bpf_prog_info bpf_prog_info_t;

#else

typedef struct bpf_prog_info_min_ver {
    unsigned int type;
    unsigned int id;
    unsigned char tag[BPF_TAG_SIZE];
    unsigned int jited_prog_len;
    unsigned int xlated_prog_len;
    unsigned long jited_prog_insns;
    unsigned long xlated_prog_insns;
    unsigned long load_time;
    unsigned int created_by_uid;
    unsigned int nr_map_ids;
    unsigned long map_ids;
    char name[BPF_OBJ_NAME_LEN];
    unsigned int ifindex;
    unsigned int gpl_compatible:1;
    unsigned long netns_dev;
    unsigned long netns_ino;
    unsigned int nr_jited_ksyms;
    unsigned int nr_jited_func_lens;
    unsigned long jited_ksyms;
    unsigned long jited_func_lens;
} bpf_prog_info_t;

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef __TE_CONF_BPF_H__ */
