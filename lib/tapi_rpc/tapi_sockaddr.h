/** @file
 * @brief Functions to opearate with generic "struct sockaddr"
 *
 * @defgroup ts_tapi_sockaddr Tools to work with "struct sockaddr"
 * @ingroup te_ts_tapi
 * @{
 *
 * Definition of test API for working with struct sockaddr.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_SOCKADDR_H__
#define __TE_TAPI_SOCKADDR_H__

#include "te_stdint.h"
#include "te_errno.h"
#include "te_sockaddr.h"
#include "rcf_rpc.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Possible address types.
 */
typedef enum {
    TAPI_ADDRESS_SPECIFIC = 0,       /**< A specific IP address. */
    TAPI_ADDRESS_SPECIFIC_ZERO_PORT, /**< A specific IP address with zero
                                          port. */
    TAPI_ADDRESS_WILDCARD,           /**< @c INADDR_ANY. */
    TAPI_ADDRESS_WILDCARD_ZERO_PORT, /**< @c INADDR_ANY and zero port. */
    TAPI_ADDRESS_NULL                /**< @c NULL. */
} tapi_address_type;

/**
 * Address types list, can be passed to macro @b TEST_GET_ENUM_PARAM.
 */
#define TAPI_ADDRESS_TYPE  \
    { "specific",           TAPI_ADDRESS_SPECIFIC }, \
    { "specific_zero_port", TAPI_ADDRESS_SPECIFIC_ZERO_PORT }, \
    { "wildcard",           TAPI_ADDRESS_WILDCARD }, \
    { "wildcard_zero_port", TAPI_ADDRESS_WILDCARD_ZERO_PORT }, \
    { "null",               TAPI_ADDRESS_NULL }

/**
 * Get address type and allocate required address.
 *
 * @param _base_addr Pointer to the base address.
 * @param _type_arg  Variable to get address type.
 * @param _res_addr  Ponter to save the requested address.
 */
#define TEST_GET_TYPED_ADDR(_base_addr, _type_arg, _res_addr) \
do {                                                                \
    TEST_GET_ENUM_PARAM(_type_arg, TAPI_ADDRESS_TYPE);              \
    _res_addr = tapi_sockaddr_clone_typed(_base_addr, _type_arg);   \
} while (0)

/**
 * Retrieve unused in system port in host order.
 *
 * @param pco       RPC server to check that port is free
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 *
 * @sa tapi_get_port
 */
extern te_errno tapi_allocate_port(struct rcf_rpc_server *pco,
                                   uint16_t *p_port);

/**
 * Retrieve range of ports unused in system, in host order.
 *
 * @param pco       RPC server to check that port is free
 * @param p_port    Location for allocated ports, pointer to array,
 *                  should have enough place for @p items.
 * @param num       Number of ports requests, i.e. length of range.
 *
 * @return Status code.
 */
extern te_errno tapi_allocate_port_range(struct rcf_rpc_server *pco,
                                         uint16_t *p_port, int num);

/**
 * Retrieve unused in system port in network order.
 *
 * @param pco       RPC server to check that port is free
 * @param p_port    Location for allocated port
 *
 * @return Status code.
 *
 * @sa tapi_get_port_htons
 */
extern te_errno tapi_allocate_port_htons(rcf_rpc_server *pco,
                                         uint16_t *p_port);

/**
 * Retrieve unused in system port in host order. It calls tapi_allocate_port()
 * to get a port, but unlike that function it jumps to cleanup on failure.
 *
 * @param rpcs      RPC server to check that port is free.
 *
 * @return Port number.
 *
 * @sa tapi_get_port_htons, tapi_allocate_port
 */
extern uint16_t tapi_get_port(rcf_rpc_server *rpcs);

/**
 * Retrieve unused in system port in network order.
 *
 * @note It jumps to cleanup on failure.
 *
 * @param rpcs      RPC server to check that port is free.
 *
 * @return Port number.
 *
 * @sa tapi_get_port, tapi_allocate_port_htons
 */
static inline uint16_t
tapi_get_port_htons(rcf_rpc_server *rpcs)
{
    return htons(tapi_get_port(rpcs));
}


/**
 * Generate new sockaddr basing on existing one (copy data and
 * allocate new port).
 *
 * @param pco   RPC server to check that port is free
 * @param src   existing sockaddr
 * @param dst   location for new sockaddr
 *
 * @return Status code.
 */
static inline te_errno
tapi_sockaddr_clone(rcf_rpc_server *pco,
                    const struct sockaddr *src,
                    struct sockaddr_storage *dst)
{
    memcpy(dst, src, te_sockaddr_get_size(src));
    return tapi_allocate_port_htons(pco, te_sockaddr_get_port_ptr(SA(dst)));
}

/**
 * Obtain an exact copy of a given socket address.
 *
 * @param src   existing sockaddr
 * @param dst   location for a clone
 *
 * @return Status code.
 */
static inline void
tapi_sockaddr_clone_exact(const struct sockaddr *src,
                          struct sockaddr_storage *dst)
{
    memcpy(dst, src, te_sockaddr_get_size(src));
}

/**
 * Get address of the specified type based on @p addr. New address instance
 * is allocated from the heap.
 *
 * @param addr  Base address - port is copied from it.
 * @param type  Required address type.
 *
 * @return Pointer to the address of the specified type.
 */
extern struct sockaddr *tapi_sockaddr_clone_typed(
                                                const struct sockaddr *addr,
                                                tapi_address_type type);

/**
 * Allocate a free port and set it to @p addr.
 *
 * @param rpcs      RPC server handle.
 * @param addr      Address to set the new port.
 *
 * @return Status code.
 */
extern te_errno tapi_allocate_set_port(rcf_rpc_server *rpcs,
                                       const struct sockaddr *addr);

/**
 * Compair the content of two 'struct sockaddr' structures.
 *
 * @param addr1     The first address
 * @param addr2     The second address
 *
 * @return The comparison result:
 * @retval 0        equal
 * @retval -1       not equal
 * @retval -2       comparison of addresses of unsupported family
 */
static inline int
tapi_sockaddr_cmp(const struct sockaddr *addr1,
                  const struct sockaddr *addr2)
{
    return te_sockaddrcmp(addr1, te_sockaddr_get_size(addr1),
                          addr2, te_sockaddr_get_size(addr2));
}

/**
 * Allocate memory of @b sockaddr_storage size and copy @p src data to
 * there. @p dst should be released with @p free(3) when it is no longer
 * needed.
 *
 * @param[in]  src      Existing sockaddr.
 * @param[out] dst      Copy of sockaddr.
 *
 * @return Status code.
 */
extern te_errno tapi_sockaddr_clone2(const struct sockaddr  *src,
                                     struct sockaddr       **dst);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SOCKADDR_H__ */

/**@} <!-- END ts_tapi_sockaddr --> */
