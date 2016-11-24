/** @file
 * @brief API to calculate hash used by RSS
 *
 * Definition of API to calculate hash used by RSS
 *
 * Based on the implementation of the Toeplitz hash function written
 * by Artem Andreev <Artem.Andreev@oktetlabs.ru> for Solarflare Solaris
 * GLDv3 network driver.
 *
 *
 * Copyright (c) 2008-2016 Solarflare Communications Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the FreeBSD Project.
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
