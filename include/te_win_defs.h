/** @file
 * @brief Windows support
 *
 * Definitions missed in native Windows.
 *
 * This file should be used only on pre-processing stage.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
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
