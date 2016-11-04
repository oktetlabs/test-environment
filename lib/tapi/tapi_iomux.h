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
#include "te_queue.h"

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

/** Minimum supported iomux type value.  */
#define TAPI_IOMUX_MIN TAPI_IOMUX_SELECT

/** Maximum supported iomux type value.  */
#define TAPI_IOMUX_MAX TAPI_IOMUX_EPOLL_PWAIT

/**
 * The list of values allowed for test parameter
 * defining iomux function.
 */
#define TAPI_IOMUX_MAPPING_LIST \
    { "select",      TAPI_IOMUX_SELECT },      \
    { "pselect",     TAPI_IOMUX_PSELECT },     \
    { "poll",        TAPI_IOMUX_POLL },        \
    { "ppoll",       TAPI_IOMUX_PPOLL },       \
    { "epoll",       TAPI_IOMUX_EPOLL },       \
    { "epoll_pwait", TAPI_IOMUX_EPOLL_PWAIT }

/**
 * Get the value of parameter defining iomux function.
 *
 * @param var_name_  Name of the variable used to get the value of
 *                   "var_name_" parameter.
 */
#define TEST_GET_TE_IOMUX_FUNC(var_name_) \
    TEST_GET_ENUM_PARAM(var_name_, TAPI_IOMUX_MAPPING_LIST)

/**
 * Type of events used in iomux API.
 */
typedef enum tapi_iomux_evt {
    EVT_NONE    = 0x000,           /**< None event */
    EVT_RD      = 0x001,           /**< Read event */
    EVT_PRI     = 0x002,           /**< Urgent data available for read */
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


/**
 * Handle for a multiplexer context.
 */
struct tapi_iomux_handle;
typedef struct tapi_iomux_handle tapi_iomux_handle;

/**
 * A file descriptor events.
 */
struct tapi_iomux_evt_fd;
typedef struct tapi_iomux_evt_fd tapi_iomux_evt_fd;

/**
 * Function prototype to create a multiplexer.
 *
 * @param iomux     The multiplexer handle.
 */
typedef void (* tapi_iomux_method_create)(tapi_iomux_handle *iomux);

/**
 * Function prototype to add a file descriptor to a multiplexer set.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor to add to the multiplexer set.
 * @param evt       Requested events.
 */
typedef void (* tapi_iomux_method_add)(tapi_iomux_handle *iomux, int fd,
                                       tapi_iomux_evt evt);

/**
 * Function prototype to modify a file descriptor events.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor to modify events for.
 * @param evt       New events.
 */
typedef void (* tapi_iomux_method_mod)(tapi_iomux_handle *iomux, int fd,
                                       tapi_iomux_evt evt);

/**
 * Function prototype to delete a file descriptor from a multiplexer set.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor which should be deleted.
 */
typedef void (* tapi_iomux_method_del)(tapi_iomux_handle *iomux, int fd);

/**
 * Function prototype to perform a multiplexer call.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param revts     Returned events.
 *
 * @return Events number or @c -1 in case of fail, actualy return code is
 *         forwarded from the system multiplexer function call.
 */
typedef int (* tapi_iomux_method_call)(tapi_iomux_handle *iomux,
                                       int timeout,
                                       tapi_iomux_evt_fd **revts);

/**
 * Function prototype to destroy a multiplexer.
 *
 * @param iomux     The multiplexer handle.
 */
typedef void (* tapi_iomux_method_destroy)(tapi_iomux_handle *iomux);

/**
 * A multiplexer methods.
 */
typedef struct tapi_iomux_methods {
    tapi_iomux_method_create  create;
    tapi_iomux_method_add     add;
    tapi_iomux_method_mod     mod;
    tapi_iomux_method_del     del;
    tapi_iomux_method_call    call;
    tapi_iomux_method_destroy destroy;
} tapi_iomux_methods;

/**
 * Context data for @c select() API.
 */
typedef struct tapi_iomux_select_context {
    rpc_fd_set_p read_fds;  /**< RPC pointer to file descriptors set
                                 watching for read events. */
    rpc_fd_set_p write_fds; /**< RPC pointer to file descriptors set
                                 watching for write events. */
    rpc_fd_set_p exc_fds;   /**< RPC pointer to file descriptors set
                                 watching for exception events. */
} tapi_iomux_select_context;

/**
 * Context data for @c poll() API.
 */
typedef struct tapi_iomux_poll_context {
    struct rpc_pollfd *fds;   /**< Poll file descriptors set. */
} tapi_iomux_poll_context;

/**
 * Context data for @c epoll() API.
 */
typedef struct tapi_iomux_epoll_context {
    int epfd;                       /**< Epoll file descriptor. */
    struct rpc_epoll_event *events; /**< Epoll events set. */
} tapi_iomux_epoll_context;


/**
 * A file descriptor events.
 */
struct tapi_iomux_evt_fd {
    int            fd;      /**< A file descriptor. */
    tapi_iomux_evt events;  /**< Requested events. */
    tapi_iomux_evt revents; /**< Returned events. */
};

/**
 * Events list for internal iomux API using.
 */
typedef struct tapi_iomux_evts_list {
    tapi_iomux_evt_fd evt;
    SLIST_ENTRY(tapi_iomux_evts_list)     tapi_iomux_evts_ent;
} tapi_iomux_evts_list;
SLIST_HEAD(tapi_iomux_evts_list_h, tapi_iomux_evts_list);
typedef struct tapi_iomux_evts_list_h tapi_iomux_evts_list_h;

/**
 * Handle for a multiplexer context.
 */
struct tapi_iomux_handle {
    rcf_rpc_server               *rpcs;    /**< RPC server handle. */
    tapi_iomux_type               type;    /**< Multiplexor type. */
    const tapi_iomux_methods     *methods; /**< Multiplexor methods. */
    int                           fds_num; /**< File descriptors number in
                                                the set. */
    tapi_iomux_evts_list_h        evts;    /**< Events list. */
    tapi_iomux_evt_fd            *revts;   /**< Pointer to the returned
                                                events array. */
    rpc_sigset_p                  sigmask; /**< RPC pointer to a signal
                                                mask. */
    void                         *opaque;  /**< Opaque pointer for possible
                                                extensions. */
    union {
        tapi_iomux_select_context select;  /**< 'select' API context. */
        tapi_iomux_poll_context   poll;    /**< 'poll' API context. */
        tapi_iomux_epoll_context  epoll;   /**< 'epoll' API context. */
    };
};

/**
 * Create a multiplexer.
 *
 * @param rpcs  RPC server handle.
 * @param type  The multiplexer type.
 *
 * @return The multiplexer handle.
 */
extern tapi_iomux_handle * tapi_iomux_create(rcf_rpc_server *rpcs,
                                             tapi_iomux_type type);

/**
 * Add a file descriptor to a multiplexer set.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor to add to the multiplexer set.
 * @param evt       Requested events.
 */
extern void tapi_iomux_add(tapi_iomux_handle *iomux, int fd,
                           tapi_iomux_evt evt);

/**
 * Modify a file descriptor events.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor to modify events for.
 * @param evt       New events.
 */
extern void tapi_iomux_mod(tapi_iomux_handle *iomux, int fd,
                           tapi_iomux_evt evt);

/**
 * Delete a file descriptor from a multiplexer set.
 *
 * @param iomux     The multiplexer handle.
 * @param fd        The file descriptor which should be deleted.
 */
extern void tapi_iomux_del(tapi_iomux_handle *iomux, int fd);

/**
 * Specify a signal mask for a multiplexer.
 *
 * @note This call makes sense for iomux types @c TAPI_IOMUX_PSELECT,
 *       @c TAPI_IOMUX_PPOLL and @c TAPI_IOMUX_EPOLL_PWAIT, the signal mask
 *       is ignored by other muxers.
 *
 * @param iomux     The multiplexer handle.
 * @param sigmask   RPC pointer to the signal mask.
 */
extern void tapi_iomux_set_sigmask(tapi_iomux_handle *iomux,
                                   rpc_sigset_p sigmask);

/**
 * Perform a multiplexer call.
 *
 * @note The call can be done expecting a fail using @c RPC_AWAIT_IUT_ERROR
 *       as for usual RPC call, return code of the muxer call is forwarded
 *       and returned by the function.
 *
 * @note This call can be executed in non-blocking mode using
 *       @c RCF_RPC_CALL as usual RPC call.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param revts     Returned events.
 *
 * @return Events number or @c -1 in case of fail, actualy return code is
 *         forwarded from the system multiplexer function call.
 */
extern int tapi_iomux_call(tapi_iomux_handle *iomux, int timeout,
                           tapi_iomux_evt_fd **revts);

/**
 * Perform a multiplexer call specifying a signal mask.
 *
 * @note See also @c tapi_iomux_call() description for details.
 *
 * @param iomux     The multiplexer handle.
 * @param timeout   Timeout to block in the call in milliseconds.
 * @param sigmask   Signal mask.
 * @param revts     Returned events.
 *
 * @return Events number or @c -1 in case of fail, actualy return code is
 *         forwarded from the system multiplexer function call.
 */
extern int tapi_iomux_pcall(tapi_iomux_handle *iomux, int timeout,
                            rpc_sigset_p sigmask,
                            tapi_iomux_evt_fd **revts);

/**
 * Destroy a multiplexer.
 *
 * @param iomux     The multiplexer handle.
 */
extern void tapi_iomux_destroy(tapi_iomux_handle *iomux);

/**
 * Process returned events by @c epoll_wait() or @c epoll_pwait() call,
 * convert and save them in the iomux context.
 *
 * @param iomux     The multiplexer handle.
 * @param events    Returned epoll events array.
 * @param evts_num  The array length.
 *
 * @return Returned events array converted to the generic representation.
 */
extern tapi_iomux_evt_fd * tapi_iomux_epoll_get_events(
                                            tapi_iomux_handle *iomux,
                                            struct rpc_epoll_event *events,
                                            int evts_num);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_IOMUX_H_ */
