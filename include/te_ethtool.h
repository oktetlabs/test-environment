#if HAVE_LINUX_ETHTOOL_H
#include "te_stdint.h"
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef uint64_t __u64;
typedef uint32_t __u32;
typedef uint16_t __u16;
typedef uint8_t  __u8;
#include <linux/ethtool.h>
#endif
