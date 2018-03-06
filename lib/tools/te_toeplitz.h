/** @file
 * @brief API to calculate hash used by RSS
 *
 * @defgroup te_tools_te_toeplitz Toeplitz hash
 * @ingroup te_tools
 * @{
 *
 * Definition of API to calculate hash used by RSS
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

#ifndef __TE_TOOLS_TOEPLITZ_H__
#define __TE_TOOLS_TOEPLITZ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_config.h"
#include "te_defs.h"
#include "te_stdint.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif


typedef struct te_toeplitz_hash_cache te_toeplitz_hash_cache;


/**
 * Calculate a Toeplitz hash of input data
 *
 * @param cache        Pre-constructed cache
 * @param input        Pointer to input data
 * @param pos          Position of input data
 * @param datalen      Length of input data in bytes
 *
 * @retval RSS hash value of input data
 */
extern unsigned int te_toeplitz_hash_data(
    const te_toeplitz_hash_cache *toeplitz_hash_cache,
    const uint8_t *input, unsigned int pos, unsigned int datalen);


/**
 * Calculate a RSS hash using pre-calculated cache
 *
 * @param cache        Pre-constructed cache
 * @param addr_size    IPv4 / IPv6 address length
 * @param src_addr     Pointer to source address
 * @param src_port     Source port number in network byte order
 * @param dst_addr     Pointer to destination address
 * @param dst_port     Desination port number in network byte order
 *
 * @retval RSS hash value
 */
extern unsigned int te_toeplitz_hash(
    const te_toeplitz_hash_cache *toeplitz_hash_cache,
    unsigned int addr_size, const uint8_t *src_addr, uint16_t src_port,
    const uint8_t *dst_addr, uint16_t dst_port);

/**
 * Free pre-constructed cache
 *
 * @param cache          Pointer to cache
 */
extern void te_toeplitz_hash_fini(
    te_toeplitz_hash_cache *toeplitz_hash_cache);

/**
 * Pre-construct cache to use it for faster calculating of hash
 * used by Toeplitz hash function
 *
 * @param key        40-byte hash key
 *
 * @retval cache or NULL in case of error
 */
extern te_toeplitz_hash_cache *te_toeplitz_cache_init(const uint8_t *key);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_TOEPLITZ_H__ */
/**@} <!-- END te_tools_te_toeplitz --> */
