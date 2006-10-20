#if HAVE_LINUX_ETHTOOL_H
#include <asm/types.h> /* For >= 2.6.18 */

#include "te_stdint.h" /* For <= 2.6.17 */
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

#include <linux/ethtool.h>
#endif
