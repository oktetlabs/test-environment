/** @file
 * @brief API to calculate hash used by RSS
 *
 * Software implementation of the Toeplitz hash function used by RSS
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Denis Pryazhennikov <Denis.Pryazhennikov@oktetlabs.ru>
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
        unsigned int cache[TE_TOEPLITZ_CACHE_SIZE];
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
