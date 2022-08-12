/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from sys/mman.h.
 *
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_RPC_SYS_MMAN_H__
#define __TE_RPC_SYS_MMAN_H__

#include "te_rpc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** TA-independent memory protection flags for mmap(). */
typedef enum rpc_prot_flags {
    RPC_PROT_READ          = 0x1,  /**< Page can be read */
    RPC_PROT_WRITE         = 0x2,  /**< Page can be written */
    RPC_PROT_EXEC          = 0x4,  /**< Page can be executed */
    RPC_PROT_SEM           = 0x8,  /**< Page may be used for
                                        atomic ops */
    RPC_PROT_NONE          = 0x10, /**< Page cannot be accessed */
    RPC_PROT_GROWSDOWN     = 0x20, /**< "mprotect flag: extend change
                                        to start of growsdown vma" */
    RPC_PROT_GROWSUP       = 0x40, /**< "mprotect flag: extend change
                                        to end of growsup vma" */
    RPC_PROT_UNKNOWN       = 0x80, /**< Unknown flag */
} rpc_prot_flags;

#define PROT_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(PROT_READ),        \
            RPC_BIT_MAP_ENTRY(PROT_WRITE),       \
            RPC_BIT_MAP_ENTRY(PROT_EXEC),        \
            RPC_BIT_MAP_ENTRY(PROT_SEM),         \
            RPC_BIT_MAP_ENTRY(PROT_NONE),        \
            RPC_BIT_MAP_ENTRY(PROT_GROWSDOWN),   \
            RPC_BIT_MAP_ENTRY(PROT_GROWSUP),     \
            RPC_BIT_MAP_ENTRY(PROT_UNKNOWN)

/** prot_flags_rpc2str() */
RPCBITMAP2STR(prot_flags, PROT_FLAGS_MAPPING_LIST)

/** Convert RPC memory protection flags to native flags */
extern unsigned int prot_flags_rpc2h(unsigned int flags);

/** Convert native memory protection flags to RPC flags */
extern unsigned int prot_flags_h2rpc(unsigned int flags);

/** TA-independent flags for mmap(). */
typedef enum rpc_map_flags {
    RPC_MAP_SHARED              = 0x01, /**< Shared mapping */
    RPC_MAP_PRIVATE             = 0x02, /**< Private mapping */
    RPC_MAP_FIXED               = 0x04, /**< Place mapping exactly
                                             at specified addr */
    RPC_MAP_ANONYMOUS           = 0x08, /**< Mapping not backed by
                                             a file */
    RPC_MAP_GROWSDOWN           = 0x10, /**< Mapping should extend
                                             downward in memory */
    RPC_MAP_LOCKED              = 0x20, /**< Pages are locked */
    RPC_MAP_POPULATE            = 0x40, /**< Populate (prefault)
                                             page tables */
    RPC_MAP_NONBLOCK            = 0x80, /**< Do not block on I/O */
    RPC_MAP_STACK               = 0x100, /**< Allocate at address
                                              suitable for process
                                              or thread stack */
    RPC_MAP_HUGETLB             = 0x200, /**< Huge page mapping */
    RPC_MAP_UNKNOWN             = 0x800, /**< Unknown flag */
} rpc_map_flags;


#define MAP_FLAGS_MAPPING_LIST \
            RPC_BIT_MAP_ENTRY(MAP_SHARED),        \
            RPC_BIT_MAP_ENTRY(MAP_PRIVATE),       \
            RPC_BIT_MAP_ENTRY(MAP_FIXED),         \
            RPC_BIT_MAP_ENTRY(MAP_ANONYMOUS),     \
            RPC_BIT_MAP_ENTRY(MAP_GROWSDOWN),     \
            RPC_BIT_MAP_ENTRY(MAP_LOCKED),        \
            RPC_BIT_MAP_ENTRY(MAP_POPULATE),      \
            RPC_BIT_MAP_ENTRY(MAP_NONBLOCK),      \
            RPC_BIT_MAP_ENTRY(MAP_STACK),         \
            RPC_BIT_MAP_ENTRY(MAP_HUGETLB),       \
            RPC_BIT_MAP_ENTRY(MAP_UNKNOWN)

/** map_flags_rpc2str() */
RPCBITMAP2STR(map_flags, MAP_FLAGS_MAPPING_LIST)

/** Convert RPC memory mapping flags to native flags */
extern unsigned int map_flags_rpc2h(unsigned int flags);

/** Convert native memory mapping flags to RPC flags */
extern unsigned int map_flags_h2rpc(unsigned int flags);

/** Value passed to rpc_madvise() */
typedef enum rpc_madv_value {
    RPC_MADV_NORMAL = 1,    /**< No special treatment */
    RPC_MADV_RANDOM,        /**< Expect page references in random order */
    RPC_MADV_SEQUENTIAL,    /**< Expect page references in sequential
                                 order */
    RPC_MADV_WILLNEED,      /**< Expect access in the near future */
    RPC_MADV_DONTNEED,      /**< Do not expect access in the near future */
    RPC_MADV_REMOVE,        /**< Free up a given range of pages and its
                                 associated backing store */
    RPC_MADV_DONTFORK,      /**< Pages should not be available to a
                                 child after fork() */
    RPC_MADV_DOFORK,        /**< Undo effect of @c RPC_MADV_DONTFORK */
    RPC_MADV_HWPOISON,      /**< Poison the pages (so that references
                                 to them will be handled like a hardware
                                 memory corruption) */
    RPC_MADV_MERGEABLE,     /**< Enable Kernel Samepage Merging */
    RPC_MADV_UNMERGEABLE,   /**< Undo the effect of @c RPC_MADV_MERGEABLE */
    RPC_MADV_SOFT_OFFLINE,  /**< Soft offline the pages */
    RPC_MADV_HUGEPAGE,      /**< Enable Transparent Huge Pages */
    RPC_MADV_NOHUGEPAGE,    /**< Ensure that pages will not be collapsed
                                 into huge pages */
    RPC_MADV_DONTDUMP,      /**< Exclude pages from a core dump */
    RPC_MADV_DODUMP,        /**< Undo the effect of @c RPC_MADV_DONTDUMP */
    RPC_MADV_FREE,          /**< The application no longer requires these
                                 pages, kernel can free them */
    RPC_MADV_WIPEONFORK,    /**< After fork() a child will find these
                                 pages zero-filled */
    RPC_MADV_KEEPONFORK,    /**< Undo the effect of
                                 @c RPC_MADV_WIPEONFORK */
    RPC_MADV_UNKNOWN,       /**< Unknown value */
} rpc_madv_value;

/** Convert @ref rpc_madvise_value to string */
extern const char *madv_value_rpc2str(rpc_madv_value value);

/** Convert @ref rpc_madvise_value to native value */
extern int madv_value_rpc2h(rpc_madv_value value);

/** Convert native value to @ref rpc_madv_value */
extern rpc_madv_value madv_value_h2rpc(int value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_SYS_MMAN_H__ */
