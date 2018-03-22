/** @file
 * @brief Windows support
 *
 * Definitions missed in native Windows.
 *
 * This file should be used only on pre-processing stage.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __TE_WIN_DEFS_H__
#define __TE_WIN_DEFS_H__

typedef INT8   int8_t;
typedef INT16  int16_t;
typedef INT32  int32_t;
typedef INT64  int64_t;
typedef UINT8  uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

typedef int ssize_t;
typedef int pid_t;

#define inline
#define __const
#undef __stdcall

#define snprintf                _snprintf
#define vsnprintf               _vsnprintf
#define va_copy(_dest, _src)    _dest = _src;

#define ETHER_ADDR_LEN  6
#define ETHER_MIN_LEN   64
#define ETHER_CRC_LEN   4
#define ETHERTYPE_IP            0x0800          /* IP */
#define ETHERTYPE_ARP           0x0806          /* Address resolution */

#endif
