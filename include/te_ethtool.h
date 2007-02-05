/** @file
 * @brief Wrapper to include <linux/ethtool.h>
 *
 * It is not easy to include linux/ethtool.h for kernels <= 2.6.17.
 * This wrapper solves the problem for users.
 *
 * @author Alexandra Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *
 * $Id$
 */
#include <asm/types.h> /* For >= 2.6.18 */

#include "te_stdint.h" /* For <= 2.6.17 */
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

#include <linux/ethtool.h>
