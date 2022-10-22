/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to calculate hash used by RSS
 *
 * Software implementation of the Toeplitz hash function used by RSS
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#define TE_LGR_USER     "TOEPLITZ"

#include "te_toeplitz.h"
#include "te_config.h"
#include "logger_api.h"

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

/** Default size of Toeplitz hash key */
#define TE_TOEPLITZ_DEF_KEY_SIZE 40

/** Maximum size of input data for a given hash key length */
#define TE_TOEPLITZ_IN_MAX(_key_len) ((_key_len) - 4)

/**
 * Hash key is 4 bytes longer than maximum input data length, so it
 * should be at least 5 bytes to be useful.
 */
#define TE_TOEPLITZ_MIN_KEY_SIZE 5

/**
 * Size of Toeplitz cache for a given key length.
 *
 * @param _key_len    Hash key length.
 */
#define TE_TOEPLITZ_CACHE_SIZE(_key_len) \
        (TE_TOEPLITZ_IN_MAX(_key_len) * (UINT8_MAX + 1))

/**
 * Length of hashed data for a given TCP or UDP connection.
 *
 * @param _addr_size    Address size
 */
#define TE_TOEPLITZ_TCPUDP_LEN(_addr_size) \
    (_addr_size * 2 + sizeof(uint16_t) * 2)

struct te_toeplitz_hash_cache {
    uint32_t *cache;
    size_t max_in_size;
};

/* See description in te_toeplitz.h */
uint32_t
te_toeplitz_hash_data(const te_toeplitz_hash_cache *toeplitz_hash_cache,
                      const uint8_t *input, unsigned int pos,
                      unsigned int datalen)
{
    uint32_t hash = 0;

    for (; datalen != 0; datalen--, pos++, input++)
        hash ^= toeplitz_hash_cache->cache[pos * (UINT8_MAX + 1) + *input];

    return (hash);
}

/* See description in te_toeplitz.h */
uint32_t
te_toeplitz_hash(const te_toeplitz_hash_cache *toeplitz_hash_cache,
                 unsigned int addr_size,
                 const uint8_t *src_addr, uint16_t src_port,
                 const uint8_t *dst_addr, uint16_t dst_port)
{
    uint32_t hash = 0;
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
uint32_t
te_toeplitz_hash_sym_or_xor(
    const te_toeplitz_hash_cache *cache,
    unsigned int addr_size,
    const uint8_t *src_addr, uint16_t src_port,
    const uint8_t *dst_addr, uint16_t dst_port)
{
    static const size_t max_addr_size = sizeof(struct in6_addr);
    uint8_t data[TE_TOEPLITZ_TCPUDP_LEN(max_addr_size)];
    unsigned int i;
    unsigned int pos = 0;

    assert(addr_size <= max_addr_size);
    assert(TE_TOEPLITZ_TCPUDP_LEN(addr_size) <= cache->max_in_size);

    for (i = 0; i < addr_size; i++)
        data[pos++] = src_addr[i] | dst_addr[i];

    for (i = 0; i < addr_size; i++)
        data[pos++] = src_addr[i] ^ dst_addr[i];

    *(uint16_t *)&data[pos] = src_port | dst_port;
    pos += sizeof(uint16_t);

    *(uint16_t *)&data[pos] = src_port ^ dst_port;
    pos += sizeof(uint16_t);

    return te_toeplitz_hash_data(cache, data, 0, pos);
}

/* See description in te_toeplitz.h */
te_errno
te_toeplitz_hash_sa(
    const te_toeplitz_hash_cache *toeplitz_hash_cache,
    const struct sockaddr *src_addr,
    const struct sockaddr *dst_addr,
    te_toeplitz_hash_variant hash_var,
    uint32_t *hash_out)
{
    unsigned int addr_size;
    const uint8_t *src_addr_ptr;
    uint16_t src_port;
    const uint8_t *dst_addr_ptr;
    uint16_t dst_port;

    if (src_addr->sa_family != dst_addr->sa_family)
    {
        ERROR("%s(): address families must be the same",
              __FUNCTION__);
        return TE_EINVAL;
    }
    if (src_addr->sa_family != AF_INET &&
        src_addr->sa_family != AF_INET6)
    {
        ERROR("%s(): not supported address family %d",
              __FUNCTION__, src_addr->sa_family);
        return TE_EINVAL;
    }

    src_addr_ptr = te_sockaddr_get_netaddr(src_addr);
    dst_addr_ptr = te_sockaddr_get_netaddr(dst_addr);
    src_port = te_sockaddr_get_port(src_addr);
    dst_port = te_sockaddr_get_port(dst_addr);

    addr_size = te_netaddr_get_size(src_addr->sa_family);
    if (TE_TOEPLITZ_TCPUDP_LEN(addr_size) > toeplitz_hash_cache->max_in_size)
    {
        ERROR("%s(): address size %u is too big for provided cache",
              __FUNCTION__, addr_size);
        return TE_ESMALLBUF;
    }

    switch (hash_var)
    {
        case TE_TOEPLITZ_HASH_STANDARD:
            *hash_out = te_toeplitz_hash(toeplitz_hash_cache, addr_size,
                                         src_addr_ptr, src_port,
                                         dst_addr_ptr, dst_port);
            break;

        case TE_TOEPLITZ_HASH_SYM_OR_XOR:
            *hash_out =  te_toeplitz_hash_sym_or_xor(toeplitz_hash_cache,
                                                     addr_size,
                                                     src_addr_ptr, src_port,
                                                     dst_addr_ptr, dst_port);
            break;

        default:
            ERROR("%s(): unknown algorithm variant %d",
                  __FUNCTION__, hash_var);
            return TE_EINVAL;
    }

    return 0;
}

/* See description in te_toeplitz.h */
te_toeplitz_hash_cache *
te_toeplitz_cache_init_size(const uint8_t *key, size_t key_size)
{
    unsigned int i;
    te_toeplitz_hash_cache *toeplitz_hash_cache;

    if (key_size < TE_TOEPLITZ_MIN_KEY_SIZE)
    {
        ERROR("%s(): too short hash key", __FUNCTION__);
        return NULL;
    }

    toeplitz_hash_cache = malloc(sizeof(*toeplitz_hash_cache));
    if (toeplitz_hash_cache == NULL)
        return NULL;

    toeplitz_hash_cache->cache = calloc(TE_TOEPLITZ_CACHE_SIZE(key_size),
                                        sizeof(uint32_t));
    if (toeplitz_hash_cache->cache == NULL)
    {
        free(toeplitz_hash_cache);
        return NULL;
    }

    toeplitz_hash_cache->max_in_size = TE_TOEPLITZ_IN_MAX(key_size);

    for (i = 0; i < TE_TOEPLITZ_IN_MAX(key_size); i++, key++)
    {
        uint32_t key_bits[CHAR_BIT] = {};
        unsigned int j;
        unsigned int mask;
        unsigned int byte;

        key_bits[0] = htonl(*(uint32_t *)key);
        for (j = 1, mask = 1 << (CHAR_BIT - 1);
             j < CHAR_BIT;
             j++, mask >>= 1)
        {
            key_bits[j] = key_bits[j - 1] << 1;

            if ((key[sizeof(uint32_t)] & mask) != 0)
                key_bits[j] |= 1;
        }

        for (byte = 0; byte <= UINT8_MAX; byte++)
        {
            uint32_t res = 0;
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
te_toeplitz_hash_cache *
te_toeplitz_cache_init(const uint8_t *key)
{
    return te_toeplitz_cache_init_size(key, TE_TOEPLITZ_DEF_KEY_SIZE);
}

/* See description in te_toeplitz.h */
void
te_toeplitz_hash_fini(te_toeplitz_hash_cache *cache)
{
    if (cache != NULL)
        free(cache->cache);
    free(cache);
}
