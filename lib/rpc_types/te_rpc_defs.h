/** @file
 * @brief RPC types definitions
 *
 * Macros to be used for all RPC types definitions.
 * 
 * 
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_RPC_DEFS_H__
#define __TE_RPC_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Coverts system native constant to its mirror in RPC namespace
 */
#define H2RPC(name_) \
    case name_: return RPC_ ## name_

/**
 * Coverts a constant from RPC namespace to system native constant
 */
#define RPC2H(name_) \
    case RPC_ ## name_: return name_

/**
 * Coverts a constant from RPC namespace to string representation 
 */
#define RPC2STR(name_) \
    case RPC_ ## name_: return #name_

/** Entry for mapping a bit of bitmask to its string value */
struct rpc_bit_map_entry {
    const char   *str_val; /**< String value */
    unsigned int  bit_val; /**< Numerical value */
};


/** Define one entry in the list of maping entries */
#define RPC_BIT_MAP_ENTRY(entry_val_) \
            { #entry_val_, (int)RPC_ ## entry_val_ }

#define RPCBITMAP2STR(bitmap_name_, mapping_list_) \
static inline const char *                         \
bitmap_name_ ## _rpc2str(int bitmap_name_)         \
{                                                  \
    struct rpc_bit_map_entry maps_[] = {           \
        mapping_list_,                             \
        { NULL, 0 }                                \
    };                                             \
    return bitmask2str(maps_, bitmap_name_);       \
}


/**
 * Convert an arbitrary bitmask to string according to 
 * the mapping passed
 *
 * @param maps  an array of mappings
 * @param val   bitmask value to be mapped
 *
 * @return String representation of bit mask
 */
extern const char *bitmask2str(struct rpc_bit_map_entry *maps,
                               unsigned int val);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_DEFS_H__ */
