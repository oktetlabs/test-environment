/** @file
 * @brief RCF Portable Command Handler
 *
 * Memory mapping library
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#ifndef __TE_RCF_PCH_MEM_H__
#define __TE_RCF_PCH_MEM_H__

#include "te_errno.h"
#include "te_rpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Library for registering/unregistering of memory addresses */

/**
 * An identifier corresponding to memory address
 *
 * @deprecated Type to maintain backward compatibility
 */
typedef rpc_ptr rcf_pch_mem_id;

/**
 * Initialize RCF PCH memory mapping
 */
extern void rcf_pch_mem_init(void);

/**
 * Assign the identifier to memory.
 *
 * @param [in] mem          Location of real memory address
 * @param [in] ns           Namespace
 * @param [in] caller_func  Name of function (for more detailed error messages)
 * @param [in] caller_line  Line in file (for more detailed error messages)
 *
 * @return Memory identifier or @c 0 in the case of failure
 */
extern rpc_ptr rcf_pch_mem_index_alloc(
        void *mem, rpc_ptr_id_namespace ns, const char *caller_func, int caller_line);

/** Wrapper for @b rcf_pch_mem_index_alloc() with details for error messages */
#define RCF_PCH_MEM_INDEX_ALLOC(_mem, _ns)                          \
    rcf_pch_mem_index_alloc((_mem), (_ns), __FUNCTION__, __LINE__)

/** Wrapper for @b RCF_PCH_MEM_INDEX_ALLOC() with generic @b ns */
#define rcf_pch_mem_alloc(_mem)                                 \
    RCF_PCH_MEM_INDEX_ALLOC((_mem), rcf_pch_mem_ns_generic())

/**
 * Mark the memory identifier corresponding to the memory address with
 * the @b ns as "unused".
 *
 * @param [in] id           Memory id returned by @b rcf_pch_mem_index_alloc
 * @param [in] ns           Namespace
 * @param [in] caller_func  Name of function (for more detailed error messages)
 * @param [in] caller_line  Line in file (for more detailed error messages)
 *
 * @return Status code
 */
extern te_errno rcf_pch_mem_index_free(
        rpc_ptr id, rpc_ptr_id_namespace ns, const char *caller_func, int caller_line);

/** Wrapper for @b rcf_pch_mem_index_free() with details for error messages */
#define RCF_PCH_MEM_INDEX_FREE(_id, _ns)                            \
    rcf_pch_mem_index_free((_id), (_ns), __FUNCTION__, __LINE__)

/** Wrapper for @b RCF_PCH_MEM_INDEX_FREE() with generic @b ns */
#define rcf_pch_mem_free(_id)                               \
    RCF_PCH_MEM_INDEX_FREE((_id), rcf_pch_mem_ns_generic())

/**
 * Mark the memory identifier corresponding to memory address @p mem and
 * namespace @p ns as "unused".
 *
 * @param [in] mem          Memory address
 * @param [in] ns           Namespace
 * @param [in] caller_func  Name of function (for more detailed error messages)
 * @param [in] caller_line  Line in file (for more detailed error messages)
 *
 * @return Status code
 */
extern te_errno rcf_pch_mem_index_free_mem(
        void *mem, rpc_ptr_id_namespace ns, const char *caller_func, int caller_line);

/**
 * Wrapper for @b rcf_pch_mem_index_free_mem() with details for
 * error messages
 */
#define RCF_PCH_MEM_INDEX_FREE_MEM(_mem, _ns)                           \
    rcf_pch_mem_index_free_mem((_mem), (_ns), __FUNCTION__, __LINE__)

/** Wrapper for @b RCF_PCH_MEM_INDEX_FREE_MEM() with generic @b ns */
#define rcf_pch_mem_free_mem(_mem)                                  \
    RCF_PCH_MEM_INDEX_FREE_MEM((_mem), rcf_pch_mem_ns_generic())

/**
 * Obtain address of the real memory by its identifier and namespace.
 *
 * @param [in] id           Memory id returned by @b rcf_pch_mem_index_alloc
 * @param [in] ns           Namespace
 * @param [in] caller_func  Name of function (for more detailed error messages)
 * @param [in] caller_line  Line in file (for more detailed error messages)
 *
 * @return Memory address or @c NULL
 */
extern void *rcf_pch_mem_index_mem_to_ptr(
        rpc_ptr id, rpc_ptr_id_namespace ns, const char *caller_func, int caller_line);

/**
 * Wrapper for @b rcf_pch_mem_index_mem_to_ptr() with details for
 * error messages
 */
#define RCF_PCH_MEM_INDEX_MEM_TO_PTR(_id, _ns)                          \
    rcf_pch_mem_index_mem_to_ptr((_id), (_ns), __FUNCTION__, __LINE__)

/** Alias for macro @b RCF_PCH_MEM_INDEX_MEM_TO_PTR. */
#define RPC_PTR_ID_TO_MEM RCF_PCH_MEM_INDEX_MEM_TO_PTR

/** Wrapper for @b RCF_PCH_MEM_INDEX_MEM_TO_PTR() with generic @b ns */
#define rcf_pch_mem_get(_id)                                        \
    RCF_PCH_MEM_INDEX_MEM_TO_PTR((_id), rcf_pch_mem_ns_generic())

/**
 * Wrapper for @b rcf_pch_mem_index_mem_to_ptr() with details for error
 * messages. The macro sets @b _errno inside the variable @b out and
 * calls @b return value @b _rc (may be unspecified for void return)
 * in case of fail.
 *
 * @note This macro can be called in a context where there is
 * the variable @b out->common._errno.
 *
 * @note This macro cannot be called in the macro @b MAKE_CALL. That macro
 * resets the variable @b out->common._errno at the end.
 */
#define RCF_PCH_MEM_INDEX_TO_PTR_RPC(_mem, _id, _ns, _rc)       \
do {                                                            \
    if ((_id) == 0)                                             \
        (_mem) = NULL;                                          \
    else                                                        \
    {                                                           \
        (_mem) = rcf_pch_mem_index_mem_to_ptr(                  \
                    (_id), (_ns), __FUNCTION__, __LINE__);      \
        if ((_mem) == NULL)                                     \
        {                                                       \
            out->common._errno = TE_RC(TE_RCF_PCH, TE_EFAULT);  \
            return _rc;                                         \
        }                                                       \
    }                                                           \
} while (0)

/**
 * Find memory identifier by memory address and namespace.
 *
 * @param [in] mem  Memory address
 * @param [in] ns   Namespace
 * @param [out] id  RPC pointer identifier
 *
 * @return Status code
 */
extern te_errno rcf_pch_mem_index_ptr_to_mem_gen(void *mem,
                                                 rpc_ptr_id_namespace ns,
                                                 rpc_ptr *id);

/**
 * Find memory identifier by memory address and namespace. Reports an error
 * if identifier is not found.
 *
 * @param [in] mem          Memory address
 * @param [in] ns           Namespace
 * @param [in] caller_func  Name of function (for more detailed error messages)
 * @param [in] caller_line  Line in file (for more detailed error messages)
 *
 * @return memory identifier or @c 0
 */
extern rpc_ptr rcf_pch_mem_index_ptr_to_mem(
        void *mem, rpc_ptr_id_namespace ns, const char *caller_func, int caller_line);

/**
 * Wrapper for @b rcf_pch_mem_index_ptr_to_mem() with details for error
 * messages
 */
#define RCF_PCH_MEM_INDEX_PTR_TO_MEM(_mem, _ns)                         \
    rcf_pch_mem_index_ptr_to_mem((_mem), (_ns), __FUNCTION__, __LINE__)


/** Alias for macro @b RCF_PCH_MEM_INDEX_PTR_TO_MEM. */
#define RPC_PTR_MEM_TO_ID RCF_PCH_MEM_INDEX_PTR_TO_MEM

/** Wrapper for @b RCF_PCH_MEM_INDEX_PTR_TO_MEM() with generic @b ns */
#define rcf_pch_mem_get_id(_mem)                                    \
    RCF_PCH_MEM_INDEX_PTR_TO_MEM((_mem), rcf_pch_mem_ns_generic())

/**
 * Determine a namespace id for generic namespace
 *
 * @return  namespace id
 */
extern rpc_ptr_id_namespace rcf_pch_mem_ns_generic(void);

/**
 * Determine a namespace id by the string of namespace
 *
 * @param [in]  ns_string   Namespace as string
 * @param [out] ns_id       Namespace id
 *
 * @return Status code
 */
extern te_errno rcf_pch_mem_ns_get_index(
        const char *ns_string, rpc_ptr_id_namespace *ns_id);

/**
 * Determine a string of namespace by the namespace id
 *
 * @param [in]  ns_id       Namespace id
 * @param [out] ns_string   Namespace as string
 *
 * @return Status code
 */
extern te_errno rcf_pch_mem_ns_get_string(
        rpc_ptr_id_namespace ns_id, const char **ns_string);


/**
 * Associate a variable with the namespace id.
 *
 * @param [out] ns_id           Variable name for namespace id
 * @param [in]  ns_str          Namespace as string
 * @param [in]  caller_func     Name of function (for more detailed error
 *                              messages)
 * @param [in]  caller_line     Line in file (for more detailed error messages)
 */
static inline te_errno
rcf_pch_mem_ns_create_if_needed(
        rpc_ptr_id_namespace *ns_id, const char *ns_str,
        const char *caller_func, int caller_line)
{
    te_errno rc;

    if (*ns_id != RPC_PTR_ID_NS_INVALID)
        return 0;

    rc = rcf_pch_mem_ns_get_index(ns_str, ns_id);
    if (rc == 0)
        return 0;

    ERROR("%s:%d: Cannot get a namespace id ('%s', rc=%r)",
          caller_func, caller_line, ns_str, rc);
    return rc;
}

/**
 * Wrapper for @b rcf_pch_mem_ns_create_if_needed() with details for error
 * messages
 */
#define RCF_PCH_MEM_NS_CREATE_IF_NEEDED(_ns_id, _ns_str)    \
    rcf_pch_mem_ns_create_if_needed(                        \
        (_ns_id), (_ns_str), __FUNCTION__, __LINE__)

/**
 * Wrapper for @b rcf_pch_mem_ns_create_if_needed() with details for error
 * messages. The macro sets @b errno and calls @b return value @c _rc
 * (may be unspecified for void return) in case of fail.
 */
#define RCF_PCH_MEM_NS_CREATE_IF_NEEDED_RETURN(_ns_id, _ns_str, _rc) \
do {                                                                    \
    if (rcf_pch_mem_ns_create_if_needed((_ns_id), (_ns_str),            \
                                        __FUNCTION__, __LINE__) != 0)   \
    {                                                                   \
        errno = ENOENT;                                                 \
        return _rc;                                                     \
    }                                                                   \
} while (0)

/**
 * Performs @p _actions with a defined namespace
 *
 * @param [in] _ns_id       Variable name for namespace id
 * @param [in] _ns_str      Namespace as string
 * @param [in] _actions     Some actions
 *
 * @note In case of an error this macro changes the @b errno for processing
 *       outside
 */
#define RPC_PCH_MEM_WITH_NAMESPACE(_ns_id, _ns_str, _actions)           \
    do {                                                                \
        static rpc_ptr_id_namespace (_ns_id) = RPC_PTR_ID_NS_INVALID;   \
        {                                                               \
            te_errno rc = rcf_pch_mem_ns_create_if_needed(              \
                        &(_ns_id), (_ns_str), __FUNCTION__, __LINE__);  \
            if (rc != 0) {                                              \
                errno = ENOENT;                                         \
                break;                                                  \
            }                                                           \
        }                                                               \
        { _actions }                                                    \
    } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RCF_PCH_MEM_H__ */
