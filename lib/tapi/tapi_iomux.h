/** @file
 * @brief Test API to work with I/O multiplexers via RPC
 *
 * This API is dedicated to work with arbitrary I/O multiplexer functions in
 * the single way.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_IOMUX_H_
#define __TE_TAPI_IOMUX_H_

#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_rpc.h"
#include "te_rpc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type of iomux call, for usage as parameter of function in this library.
 *
 * @note It must be synchronized with definitions in .x file.
 */
typedef enum {
    TAPI_IOMUX_UNKNOWN,
    TAPI_IOMUX_SELECT           = 1,
    TAPI_IOMUX_PSELECT          = 2,
    TAPI_IOMUX_POLL             = 3,
    TAPI_IOMUX_PPOLL            = 4,
    TAPI_IOMUX_EPOLL            = 5,
    TAPI_IOMUX_EPOLL_PWAIT      = 6,
    TAPI_IOMUX_RESERVED         = 7,
    TAPI_IOMUX_DEFAULT          = 8,
} tapi_iomux_type;

/**
 * Type of events passed to iomux_call and iomux_common_steps functions
 * and returned by them.
 */
typedef enum tapi_iomux_evt {
    EVT_NONE    = 0x000,           /**< None event */
    EVT_RD      = 0x001,           /**< Read event */
    EVT_PRI     = 0x002,           /**< Read event */
    EVT_WR      = 0x004,           /**< Write event */
    EVT_RDWR    = EVT_RD | EVT_WR, /**< Read and write event */
    EVT_RD_NORM = 0x008,           /**< Normal data may be read */
    EVT_WR_NORM = 0x010,           /**< Writing now will not block */
    EVT_RD_BAND = 0x020,           /**< Priority data may be read */
    EVT_WR_BAND = 0x040,           /**< Priority data may be written */
    EVT_EXC     = 0x080,           /**< Generic exception was occured */
    EVT_ERR     = 0x100,           /**< Error condition */
    EVT_HUP     = 0x200,           /**< Hung up */
    EVT_RDHUP   = 0x400,           /**< Stream socket peer closed
                                        connection, or shut down
                                        writing half of connection. */
    EVT_NVAL    = 0x800,           /**< Invalid request */
} tapi_iomux_evt;

/** Define one entry in the list of maping entries */
#define BIT_MAP_ENTRY(entry_val_) \
            { #entry_val_, (int)entry_val_ }

/** List of mapping numerical value to string for 'tapi_iomux_evt' */
#define IOMUX_EVENT_MAPPING_LIST \
            BIT_MAP_ENTRY(EVT_RD),      \
            BIT_MAP_ENTRY(EVT_PRI),     \
            BIT_MAP_ENTRY(EVT_WR),      \
            BIT_MAP_ENTRY(EVT_RD_NORM), \
            BIT_MAP_ENTRY(EVT_WR_NORM), \
            BIT_MAP_ENTRY(EVT_RD_BAND), \
            BIT_MAP_ENTRY(EVT_WR_BAND), \
            BIT_MAP_ENTRY(EVT_EXC),     \
            BIT_MAP_ENTRY(EVT_ERR),     \
            BIT_MAP_ENTRY(EVT_HUP),     \
            BIT_MAP_ENTRY(EVT_RDHUP),   \
            BIT_MAP_ENTRY(EVT_NVAL)
/**
 * tapi_iomux_event_rpc2str()
 */
RPCBITMAP2STR(tapi_iomux_event, IOMUX_EVENT_MAPPING_LIST);

/**
 * Structure for event request entry for iomux API.
 */
typedef struct {
    int fd;             /**< File descriptor */
    uint16_t events;    /**< Requested events */
    uint16_t revents;   /**< Returned events */
} tapi_iomux_evt_fd;

/**
 * Convert string name of iomux function to enum constant.
 *
 * @param iomux         name of function: "select", "pselect", or "poll".
 *
 * @return respecive value from tapi_iomux_type enum.
 */
extern tapi_iomux_type tapi_iomux_call_str2en(const char *iomux);

/**
 * Convert constant from #tapi_iomux_type to human string
 *
 * @param iomux_type    Value to be converted
 *
 * @return static character string
 */
extern const char *tapi_iomux_call_en2str(tapi_iomux_type iomux_type);

/**
 * Convert bitmask constructed with constants from #tapi_iomux_evt type to
 * bitmask constructed with standard POLL* macros from poll.h
 *
 * @param iomux_evt_mask Event bitmask from #tapi_iomux_evt constants.
 *
 * @return event bitmask from POLL* constants.
 */
extern short int tapi_iomux_evt_to_poll(uint16_t iomux_evt_mask);

/**
 * Convert bitmask constructed with constants from #tapi_iomux_evt type to
 * bitmask constructed with standard EPOLL* macros from epoll.h
 *
 * @param iomux_evt_mask Event bitmask from #tapi_iomux_evt constants.
 *
 * @return event bitmask from EPOLL* constants.
 */
extern short int tapi_iomux_evt_to_epoll(uint16_t iomux_evt_mask);

/**
 * Convert bitmask constructed with standard POLL* macros from poll.h
 * to bitmask constructed with constants from #tapi_iomux_evt type.
 *
 * @param poll_evt_mask        - event bitmask from POLL* constants.
 *
 * @return event bitmask from #tapi_iomux_evt constants.
 */
extern uint16_t tapi_iomux_poll_to_evt(short int poll_evt_mask);

/**
 * Convert bitmask constructed with standard EPOLL* macros from epoll.h
 * to bitmask constructed with constants from #tapi_iomux_evt type.
 *
 * @param poll_evt_mask        - event bitmask from EPOLL* constants.
 *
 * @return event bitmask from #tapi_iomux_evt constants.
 */
extern uint16_t tapi_iomux_epoll_to_evt(short int poll_evt_mask);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_IOMUX_H_ */
