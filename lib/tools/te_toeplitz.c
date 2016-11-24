/** @file
 * @brief API to calculate hash used by RSS
 *
 * Software implementation of the Toeplitz hash function used by RSS
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Based on the implementation of the Toeplitz hash function written
 * by Artem Andreev <Artem.Andreev@oktetlabs.ru> for Solarflare Solaris
 * GLDv3 network driver.
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
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
 */

/*
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
 */


#define TE_LGR_USER     "TOEPLITZ"

#include "te_toeplitz.h"
#include "te_config.h"

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif


/**
 * Maximum size of input data for Toeplitz function
 */
#define TE_TOEPLITZ_IN_MAX \
    (2 * (sizeof(struct in6_addr) + sizeof (in_port_t)))

/**
 * Maximum size of Toeplitz cache
 */
#define TE_TOEPLITZ_CACHE_SIZE (TE_TOEPLITZ_IN_MAX * (UINT8_MAX + 1))

struct te_toeplitz_hash_cache {
        unsigned char cache[TE_TOEPLITZ_CACHE_SIZE * sizeof(unsigned int)];
};

/* See description in te_toeplitz.h */
unsigned int
te_toeplitz_hash_data(const te_toeplitz_hash_cache *toeplitz_hash_cache,
                      const uint8_t *input, unsigned int pos,
                      unsigned int datalen)
{
    unsigned int hash = 0;

    for (; datalen != 0; datalen--, pos++, input++)
        hash ^= toeplitz_hash_cache->cache[pos * (UINT8_MAX + 1) + *input];

    return (hash);
}

/* See description in te_toeplitz.h */
unsigned int
te_toeplitz_hash(const te_toeplitz_hash_cache *toeplitz_hash_cache,
                 unsigned int addr_size,
                 const uint8_t *src_addr, uint16_t src_port,
                 const uint8_t *dst_addr, uint16_t dst_port)
{
    unsigned int hash = 0;
    unsigned int pos = 0;

    hash ^= te_toeplitz_hash_data(toeplitz_hash_cache, src_addr, pos,
                                  addr_size);
    pos += addr_size;
    hash ^= te_toeplitz_hash_data(toeplitz_hash_cache, dst_addr, pos,
                                  addr_size);
    pos += addr_size;

    if (src_port != 0 || dst_port != 0)
    {
        hash ^= te_toeplitz_hash_data(toeplitz_hash_cache,
                                      (const uint8_t *)&src_port,
                                      pos, sizeof(src_port));
        pos += sizeof(src_port);
        hash ^= te_toeplitz_hash_data(toeplitz_hash_cache,
                                      (const uint8_t *)&dst_port,
                                      pos, sizeof(dst_port));
    }

    return hash;
}

/* See description in te_toeplitz.h */
te_toeplitz_hash_cache *
te_toeplitz_cache_init(const uint8_t *key)
{
    unsigned int i;
    te_toeplitz_hash_cache *toeplitz_hash_cache;

    toeplitz_hash_cache = malloc(sizeof(*toeplitz_hash_cache));
    if (toeplitz_hash_cache == NULL)
        return NULL;

    for (i = 0; i < TE_TOEPLITZ_IN_MAX; i++, key++)
    {
        unsigned int key_bits[CHAR_BIT] = {};
        unsigned int j;
        unsigned int mask;
        unsigned int byte;

        key_bits[0] = htonl(*(unsigned int *)key);
        for (j = 1, mask = 1 << (CHAR_BIT - 1);
             j < CHAR_BIT;
             j++, mask >>= 1)
        {
            key_bits[j] = key_bits[j - 1] << 1;

            if ((key[sizeof(unsigned int)] & mask) != 0)
                key_bits[j] |= 1;
        }

        for (byte = 0; byte <= UINT8_MAX; byte++)
        {
            unsigned int res = 0;
            for (j = 0, mask = 1 << (CHAR_BIT - 1);
                 j < CHAR_BIT;
                 j++, mask >>= 1)
            {
                if (byte & mask)
                    res ^= key_bits[j];
            }
            toeplitz_hash_cache->cache[i * (UINT8_MAX + 1) + byte] = res;
        }
    }

    return toeplitz_hash_cache;
}

/* See description in te_toeplitz.h */
void
te_toeplitz_hash_fini(te_toeplitz_hash_cache *cache)
{
    free(cache);
}
