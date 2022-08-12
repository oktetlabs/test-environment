/** @file
 * @brief RPC types definitions
 *
 * Macros to be used for all RPC types definitions.
 * 
 * 
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */
 
#ifndef __TE_RPC_DEFS_H__
#define __TE_RPC_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Converts system native constant to its mirror in RPC namespace
 */
#define H2RPC(name_) \
    case name_: return RPC_ ## name_

/**
 * Converts a constant from RPC namespace to system native constant
 */
#define RPC2H(name_) \
    case RPC_ ## name_: return name_

/**
 * Converts a constant from RPC namespace to string representation 
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
