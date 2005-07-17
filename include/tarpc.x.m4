/** @file
 * @brief Remote Procedure Call
 *
 * Definition of RPC structures and functions
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

%#include "te_config.h"


typedef int32_t     tarpc_int;
typedef uint32_t    tarpc_uint;
typedef uint8_t     tarpc_bool;
typedef uint32_t    tarpc_ptr;
typedef uint32_t    tarpc_signum;

/** RPC size_t analog */
typedef uint32_t    tarpc_size_t;
/** RPC pid_t analog */
typedef int32_t     tarpc_pid_t;
/** RPC ssize_t analog */
typedef int32_t     tarpc_ssize_t;
/** RPC socklen_t analog */
typedef uint32_t    tarpc_socklen_t;
/** Handle of the 'sigset_t' or 0 */
typedef tarpc_ptr   tarpc_sigset_t;
/** Handle of the 'fd_set' or 0 */
typedef tarpc_ptr   tarpc_fd_set;
/** RPC off_t analog */
typedef int32_t     tarpc_off_t;
/** RPC time_t analogue */
typedef int64_t     tarpc_time_t;
/** RPC suseconds_t analogue */
typedef int64_t     tarpc_suseconds_t;

/** Handle of the 'WSAEvent' or 0 */
typedef tarpc_ptr   tarpc_wsaevent;
/** Handle of the window */
typedef tarpc_ptr   tarpc_hwnd;
/** WSAOVERLAPPED structure */
typedef tarpc_ptr   tarpc_overlapped;
/** HANDLE */
typedef tarpc_ptr   tarpc_handle;

typedef uint32_t    tarpc_op;

/**
 * Input arguments common for all RPC calls.
 *
 * @attention It should be the first field of all routine-specific 
 *            parameters, since implementation casts routine-specific
 *            parameters to this structure.
 */
struct tarpc_in_arg {
    tarpc_op        op;         /**< RPC operation */
    uint64_t        start;
    tarpc_ptr       tid;        /**< Thread identifier (for checking and 
                                     waiting) */
    tarpc_ptr       done;       /**< Pointer to the boolean variable in
                                     TA context to be set when function
                                     which is called using non-blocking
                                     RPC call finishes */
    string          lib<>;      /**< If non-empty, library name */ 
};

/**
 * Output arguments common for all RPC calls.
 *
 * @attention It should be the first field of all routine-specific 
 *            parameters, since implementation casts routine-specific
 *            parameters to this structure.
 */
struct tarpc_out_arg {
    tarpc_int   _errno;     /**< @e errno of the operation from
                                 te_errno.h or rcf_rpc_defs.h */
    uint32_t    duration;   /**< Duration of the called routine
                                 execution (in microseconds) */
    tarpc_ptr   tid;        /**< Identifier of the thread which 
                                 performs possibly blocking operation,
                                 but caller does not want to block.
                                 It should be passed as input when
                                 RPC client want to check status or
                                 wait for finish of the initiated
                                 operation. */
    tarpc_ptr   done;       /**< Pointer to the boolean variable in
                                 TA context to be set when function
                                 which is called using non-blocking
                                 RPC call finishes */
    tarpc_int   win_error;  /**< Windows error as is */                                 
};


/** Generic address */
struct tarpc_sa {
    tarpc_int     sa_family; /**< TA-independent domain */
    uint8_t       sa_data<>;
};

/** struct timeval */
struct tarpc_timeval {
    tarpc_time_t        tv_sec;
    tarpc_suseconds_t   tv_usec;
};

/** struct timezone */
struct tarpc_timezone {
    tarpc_int   tz_minuteswest;
    tarpc_int   tz_dsttime;
};


/* gettimeofday() */
struct tarpc_gettimeofday_in {
    struct tarpc_in_arg common;

    struct tarpc_timeval    tv<>;
    struct tarpc_timezone   tz<>;
};
   
struct tarpc_gettimeofday_out {
    struct tarpc_out_arg common;

    tarpc_int               retval;

    struct tarpc_timeval    tv<>;
    struct tarpc_timezone   tz<>;
};


/** struct ifreq */
struct tarpc_ifreq {
    char            rpc_ifr_name<>; /**< Interface name */
    struct tarpc_sa rpc_ifr_addr;   /**< Different interface addresses */
    tarpc_int       rpc_ifr_flags;  /**< Interface flags */
    uint32_t        rpc_ifr_mtu;    /**< Interface MTU */
};

/** struct ifconf */
struct tarpc_ifconf {
    /** Length of the buffer to be passed to ioctl */
    tarpc_int       nmemb;      /**< Number of ifreq structures 
                                     to be fit into the buffer */
    tarpc_int       extra;      /**< Extra space */
    /** Interface list returned by the ioctl */
    tarpc_ifreq     rpc_ifc_req<>;  
};

/** struct arpreq */
struct tarpc_arpreq {
    struct tarpc_sa rpc_arp_pa;     /**< Protocol address */
    struct tarpc_sa rpc_arp_ha;     /**< Hardware address */
    tarpc_int       rpc_arp_flags;  /**< Flags */
    char            rpc_arp_dev<>;  /**< Device */
};

/** struct timespec */
struct tarpc_timespec {
    int32_t     tv_sec;
    int32_t     tv_nsec;
};

/** Function gets nothing */
struct tarpc_void_in {
    struct tarpc_in_arg     common;
};

/** Function outputs nothing */
struct tarpc_void_out {
    struct tarpc_out_arg    common;
};

/** Function outputs 'int' return value only */
struct tarpc_int_retval_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;
};



/** Function outputs 'ssize_t' return value only */
struct tarpc_ssize_t_retval_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;
};


/*
 * RPC arguments
 */

/* rpc_is_op_done() */

typedef struct tarpc_void_in  tarpc_rpc_is_op_done_in;
typedef struct tarpc_void_out tarpc_rpc_is_op_done_out;

/* setlibname() */

struct tarpc_setlibname_in {
    struct tarpc_in_arg common;
    
    char libname<>;
};

typedef struct tarpc_int_retval_out tarpc_setlibname_out;


/* socket() */

struct tarpc_socket_in {
    struct tarpc_in_arg common;
    
    tarpc_int   domain; /**< TA-independent domain */
    tarpc_int   type;   /**< TA-independent socket type */
    tarpc_int   proto;  /**< TA-independent socket protocol */

    char        info<>; /**< Protocol Info (for winsock2 only) */
    tarpc_int   flags;  /**< If 1, the socket is overlapped 
                             (for winsock2 only) */
};

struct tarpc_socket_out {
    struct tarpc_out_arg common;

    tarpc_int   fd;     /**< TA-local socket */
};

/* WSADuplicateSocket() */
struct tarpc_duplicate_socket_in {
    struct tarpc_in_arg common;
    
    tarpc_int   s;             /**< Old socket */
    tarpc_pid_t pid;           /**< Destination process PID */
    char        info<>;        /**< Location for protocol info */
};
   
struct tarpc_duplicate_socket_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;
    char        info<>;
};

/* dup() */

struct tarpc_dup_in {
    struct tarpc_in_arg common;

    tarpc_int   oldfd;
};

typedef struct tarpc_socket_out tarpc_dup_out;


/* dup2() */

struct tarpc_dup2_in {
    struct tarpc_in_arg common;

    tarpc_int   oldfd;
    tarpc_int   newfd;
};

typedef struct tarpc_socket_out tarpc_dup2_out;


/* close() */

struct tarpc_close_in {
    struct tarpc_in_arg common;

    tarpc_int   fd; /**< TA-local socket */
};

typedef struct tarpc_int_retval_out tarpc_close_out;


/* shutdown() */

struct tarpc_shutdown_in {
    struct tarpc_in_arg common;

    tarpc_int   fd;     /**< TA-local socket */
    tarpc_int   how;    /**< rpc_shut_how */
};

typedef struct tarpc_int_retval_out tarpc_shutdown_out;




/* read() / write() */

struct tarpc_read_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    uint8_t         buf<>;
    tarpc_size_t    len;
};

struct tarpc_read_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t   retval;
    uint8_t         buf<>;
};


typedef struct tarpc_read_in tarpc_write_in;

typedef struct tarpc_ssize_t_retval_out tarpc_write_out;



/* readv() / writev() */

struct tarpc_iovec {
    uint8_t         iov_base<>;
    tarpc_size_t    iov_len;
};

struct tarpc_readv_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;
    struct tarpc_iovec  vector<>;
    tarpc_size_t        count;
};

struct tarpc_readv_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;

    struct tarpc_iovec      vector<>;
};


typedef struct tarpc_readv_in tarpc_writev_in;

typedef struct tarpc_ssize_t_retval_out tarpc_writev_out;



/* send() */

struct tarpc_send_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    uint8_t         buf<>;
    tarpc_size_t    len;
    tarpc_int       flags;
};

typedef struct tarpc_ssize_t_retval_out tarpc_send_out;


/* recv() */

struct tarpc_recv_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    uint8_t         buf<>;
    tarpc_size_t    len;
    tarpc_int       flags;
};

struct tarpc_recv_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t   retval;

    uint8_t         buf<>;
};

/* WSARecvEx() */

struct tarpc_wsa_recv_ex_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;       /**< TA-local socket */
    uint8_t             buf<>;    /**< Buffer for received data */
    tarpc_size_t        len;      /**< Maximum length of expected data */
    tarpc_int           flags<>;  /**< TA-independent flags */
};

struct tarpc_wsa_recv_ex_out {
    struct tarpc_out_arg common;

    tarpc_ssize_t        retval;  /**< Returned length */

    uint8_t              buf<>;   /**< Returned buffer with received data */
    tarpc_int            flags<>; /**< TA-independent flags */
};

/* sendto() */

struct tarpc_sendto_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;
    uint8_t             buf<>;
    tarpc_size_t        len;
    tarpc_int           flags;
    struct tarpc_sa     to;
    tarpc_socklen_t     tolen;
};

typedef struct tarpc_ssize_t_retval_out tarpc_sendto_out;


/* recvfrom() */

struct tarpc_recvfrom_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;         /**< TA-local socket */
    uint8_t             buf<>;      /**< Buffer for received data */
    tarpc_size_t        len;        /**< Maximum length of expected data */
    tarpc_int           flags;      /**< TA-independent flags */
    struct tarpc_sa     from;       /**< Address to be passed as location
                                         for data source address */
    tarpc_socklen_t     fromlen<>;  /**< Maximum expected length of the
                                         address */
};

struct tarpc_recvfrom_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t   retval;     /**< Returned length */

    uint8_t         buf<>;      /**< Returned buffer with received 
                                     data */
    struct tarpc_sa from;       /**< Source address returned by the 
                                     function */
    tarpc_socklen_t fromlen<>;  /**< Length of the source address
                                     returned by the function */
};

/* sigaction() */
struct tarpc_sigaction {
    char                xx_handler<>;   /**< Name of the signal handler
                                             function */
    tarpc_sigset_t      xx_mask;        /**< Handle (pointer in server
                                             context) of the allocated set */
    tarpc_int           xx_flags;
    char                xx_restorer<>;  /**< Name of the restorer function */
};

/* sendmsg() / recvmsg() */

struct tarpc_cmsghdr {
    uint32_t level;     /**< Originating protocol */
    uint32_t type;      /**< Protocol-specific type */
    uint8_t  data<>;    /**< Data */
};    

struct tarpc_msghdr {
    struct tarpc_sa      msg_name;       /**< Destination/source address */
    tarpc_socklen_t      msg_namelen;    /**< To be passed to 
                                              recvmsg/sendmsg */
    struct tarpc_iovec   msg_iov<>;      /**< Vector */
    tarpc_size_t         msg_iovlen;     /**< Passed to recvmsg() */
    struct tarpc_cmsghdr msg_control<>;  /**< Control info array */
    tarpc_int            msg_flags;      /**< Flags on received message */
};

struct tarpc_sendmsg_in {
    struct tarpc_in_arg common;

    tarpc_int           s;
    struct tarpc_msghdr msg<>;
    tarpc_int           flags;
};

typedef struct tarpc_int_retval_out tarpc_sendmsg_out;

struct tarpc_recvmsg_in {
    struct tarpc_in_arg common;

    tarpc_int           s;
    struct tarpc_msghdr msg<>;
    tarpc_int           flags;
};                

struct tarpc_recvmsg_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t           retval;
    
    struct tarpc_msghdr     msg<>;
};



/* bind() */

struct tarpc_bind_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;     /**< TA-local socket */
    struct tarpc_sa     addr;   /**< Socket name */
    tarpc_socklen_t     len;    /**< Length to be passed to bind() */
};

typedef struct tarpc_int_retval_out tarpc_bind_out;


/* connect() */

struct tarpc_connect_in {
    struct tarpc_in_arg   common;

    tarpc_int           fd;     /**< TA-local socket */
    struct tarpc_sa     addr;   /**< Remote address */
    tarpc_socklen_t     len;    /**< Length to be passed to connect() */
};

typedef struct tarpc_int_retval_out tarpc_connect_out;

/* ConnectEx() */
struct tarpc_connect_ex_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               fd;       /**< TA-local socket */
    struct tarpc_sa         addr;     /**< Remote address */
    tarpc_socklen_t         len;      /**< Length to be passed to connectEx() */
    uint8_t                 buf<>;    /**< Buffer for data to be sent */
    tarpc_size_t            len_buf;  /**< Size of data passed to connectEx() */
    tarpc_overlapped        overlapped; /**< WSAOVERLAPPED structure */
    tarpc_size_t            len_sent<>; /**< Returned by the function
                                             size of sent data from buf */
};                                               

struct tarpc_connect_ex_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t  retval;     /**< Returned value (1 success, 0 failure) */
    tarpc_size_t   len_sent<>; /**< Returned by the function
                                    size of sent data from buf */
};

/* DisconnectEx */
struct tarpc_disconnect_ex_in {
    struct tarpc_in_arg    common;
    
    tarpc_int         fd;         /**< TA-local socket */   
    tarpc_overlapped  overlapped; /**< WSAOVERLAPPED structure */
    tarpc_int         flags;      /**< Function call processing flag */
};

typedef struct tarpc_int_retval_out tarpc_disconnect_ex_out;     

/* listen() */

struct tarpc_listen_in {
    struct tarpc_in_arg     common;

    tarpc_int               fd;         /**< TA-local socket */
    tarpc_int               backlog;    /**< Length of the backlog */
};

typedef struct tarpc_int_retval_out tarpc_listen_out;


/* accept() */

struct tarpc_accept_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;     /**< TA-local socket */
    struct tarpc_sa     addr;   /**< Location for peer name */
    tarpc_socklen_t     len<>;  /**< Length of the location 
                                     for peer name */
};

struct tarpc_accept_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_sa         addr;   /**< Location for peer name */
    tarpc_socklen_t         len<>;  /**< Length of the location 
                                         for peer name */
};

/* WSAAccept() */

enum tarpc_accept_verdict {
    TARPC_CF_REJECT,  
    TARPC_CF_ACCEPT,
    TARPC_CF_DEFER 
};

struct tarpc_accept_cond {
    uint16_t             port;       /**< Caller port             */
    tarpc_accept_verdict verdict;    /**< Verdict for this caller */
};

struct tarpc_wsa_accept_in {
    struct tarpc_in_arg common;

    tarpc_int                fd;      /**< TA-local socket */
    struct tarpc_sa          addr;    /**< Location for peer name */
    tarpc_socklen_t          len<>;   /**< Length of the peer name location */
    struct tarpc_accept_cond cond<>;  /**< Data for the callback function */
};

struct tarpc_wsa_accept_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;

    struct tarpc_sa      addr;   /**< Location for peer name */
    tarpc_socklen_t      len<>;  /**< Length of the peer name location */
};

/* AcceptEx() */
/* Also arguments for call of GetAcceptExSockAddrs function are specified */

struct tarpc_accept_ex_in {
    struct tarpc_in_arg common;
                                                                               
    tarpc_int               fd;           /**< TA-local socket */
    tarpc_int               fd_a;         /**< TA-local socket to wich the
                                               connection will be actually
                                               made */
    tarpc_size_t            buflen;       /**< Length of the buffer
                                               passed to the AcceptEx() */
    tarpc_overlapped        overlapped;   /**< WSAOVERLAPPED structure */
    tarpc_size_t            count<>;      /**< Location for
                                               count of received bytes */ 
};

struct tarpc_accept_ex_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    tarpc_size_t            count<>;      /**< Location for
                                               count of received bytes */ 
};

struct tarpc_get_accept_addr_in {
    struct tarpc_in_arg    common;

    tarpc_int       fd;          /**< TA-local socket */
    uint8_t         buf<>;       /**< Buffer with addresses */
    tarpc_size_t    buflen;      /**< Length of the buffer
                                      passed to the AcceptEx() */
    struct tarpc_sa laddr;       /**< Local address */
    struct tarpc_sa raddr;       /**< Remote address */
};

struct tarpc_get_accept_addr_out {
    struct tarpc_out_arg    common;

    struct tarpc_sa laddr;       /**< Local address */
    struct tarpc_sa raddr;       /**< Remote address */
};

/* TransmitFile() */

struct tarpc_transmit_file_in {
    struct tarpc_in_arg common;
                                                                               
    tarpc_int               fd;           /**< TA-local socket */
    tarpc_handle            file;         /**< Handle to the open file to be
                                               transmitted */
    tarpc_size_t            len;          /**< Number of file bytes to
                                               transmit */
    tarpc_size_t            len_per_send; /**< Number of bytes of each block of
                                               data sent in each send operarion */
    tarpc_size_t            offset;       /**< Offset to be passed
                                               to OVERLAPPED. File position
                                               at wich to start to transfer */
    tarpc_size_t            offset_high;  /**< OffsetHigh to be passed
                                               to OVERLAPPED. High-oreder
                                               word of the file position
                                               at wich to start to transfer */                                               
    tarpc_overlapped        overlapped;   /**< WSAOVERLAPPED structure */
    char                    head<>;       /**< Buffer to be transmitted before
                                               the file data is transmitted */
    char                    tail<>;       /**< Buffer to be transmitted after
                                               the file data is transmitted */
    tarpc_size_t            flags;        /**< Parameter of TransmitFile() */
};

typedef struct tarpc_int_retval_out tarpc_transmit_file_out;

/* TransmitFile(), 2nd version */

struct tarpc_transmitfile_tabufs_in {
    struct tarpc_in_arg common;
                                                                               
    tarpc_int           s;            /**< TA-local socket */
    tarpc_handle        file;         /**< Handle to the open file to be
                                           transmitted */
    tarpc_size_t        len;          /**< Number of file bytes to transmit */
    tarpc_size_t        bytes_per_send; /**< Number of bytes of each block of
                                             data sent in each send operarion */
    tarpc_overlapped    overlapped;   /**< WSAOVERLAPPED structure */
    tarpc_ptr           head;         /**< Buffer to be transmitted before
                                           the file data is transmitted */
    tarpc_ssize_t       head_len;
    tarpc_ptr           tail;         /**< Buffer to be transmitted after
                                           the file data is transmitted */
    tarpc_ssize_t       tail_len;
    tarpc_size_t        flags;        /**< Parameter of TransmitFile() */
};

typedef struct tarpc_int_retval_out tarpc_transmitfile_tabufs_out;

/* Windows CreateFile() */
struct tarpc_create_file_in {
    struct tarpc_in_arg  common;
    char                 name<>;       /**< File name */
    tarpc_uint           desired_access;
    tarpc_uint           share_mode;
    tarpc_ptr            security_attributes;
    tarpc_uint           creation_disposition;
    tarpc_uint           flags_attributes;
    tarpc_handle         template_file;
};

struct tarpc_create_file_out {
    struct tarpc_out_arg  common;
    tarpc_handle          handle;
};

/* Windows CloseHandle() */
struct tarpc_close_handle_in {
    struct tarpc_in_arg  common;
    tarpc_handle         handle;       /**< HANDLE to close */
};

struct tarpc_close_handle_out {
    struct tarpc_out_arg  common;
    int                   retval;
};

/* HasOverlappedIoCompleted() */

struct tarpc_has_overlapped_io_completed_in {
    struct tarpc_in_arg  common;
    tarpc_overlapped     overlapped;     /**< WSAOVERLAPPED structure */
};    

struct tarpc_has_overlapped_io_completed_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
};    

/* GetCurrentProcessId() */

struct tarpc_get_current_process_id_in {
    struct tarpc_in_arg  common;
};    

struct tarpc_get_current_process_id_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
};

/* rpc_get_ram_size() */
struct tarpc_get_ram_size_in {
    struct tarpc_in_arg  common;
};

struct tarpc_get_ram_size_out {
    struct tarpc_out_arg  common;
    uint64_t              ram_size;
};

/* rpc_vm_trasher() */
struct tarpc_vm_trasher_in {
    struct tarpc_in_arg  common;
    tarpc_bool           start;
};

typedef struct tarpc_int_retval_out tarpc_vm_trasher_out;

/* rpc_write_at_offset() */
struct tarpc_write_at_offset_in {
    struct tarpc_in_arg   common;
    tarpc_int             fd;
    uint8_t               buf<>;
    tarpc_off_t           offset;
};

struct tarpc_write_at_offset_out {
    struct tarpc_out_arg  common;
    tarpc_off_t           offset;
    tarpc_ssize_t         written;
};

/* getsockname() */

typedef struct tarpc_accept_in tarpc_getsockname_in;

typedef struct tarpc_accept_out tarpc_getsockname_out;


/* getpeername() */

typedef struct tarpc_accept_in tarpc_getpeername_in;

typedef struct tarpc_accept_out tarpc_getpeername_out;



/*
 * fd_set operations
 */

/* fd_set_new() */

typedef struct tarpc_void_in tarpc_fd_set_new_in;

struct tarpc_fd_set_new_out {
    struct tarpc_out_arg    common;

    tarpc_fd_set            retval;
};


/* fd_set_delete() */

struct tarpc_fd_set_delete_in {
    struct tarpc_in_arg common;

    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_fd_set_delete_out;


/* FD_ZERO() */

struct tarpc_do_fd_zero_in {
    struct tarpc_in_arg common;

    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_do_fd_zero_out;


/* WSACreateEvent() */
struct tarpc_create_event_in {
    struct tarpc_in_arg common;
};

struct tarpc_create_event_out {
    struct tarpc_out_arg common;
    tarpc_wsaevent       retval;
};

/* WSACloseEvent() */
struct tarpc_close_event_in {
    struct tarpc_in_arg common;
    tarpc_wsaevent      hevent;
};
typedef struct tarpc_int_retval_out tarpc_close_event_out;

/* WSAResetEvent() */
struct tarpc_reset_event_in {
    struct tarpc_in_arg common;
    tarpc_wsaevent      hevent;
};

typedef struct tarpc_int_retval_out tarpc_reset_event_out;

/* WSASetEvent() */
struct tarpc_set_event_in {
    struct tarpc_in_arg common;
    tarpc_wsaevent      hevent;
};

typedef struct tarpc_int_retval_out tarpc_set_event_out;

/* WSAAddressToString */
struct tarpc_wsa_address_to_string_in {
    struct tarpc_in_arg  common;
    struct tarpc_sa      addr;
    tarpc_size_t         addrlen;
    uint8_t              info<>; /**< Protocol Info */
    char                 addrstr<>;
    tarpc_size_t         addrstr_len<>;
};

struct tarpc_wsa_address_to_string_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    uint8_t              addrstr<>;
    tarpc_size_t         addrstr_len<>;
};

/* WSAStringToAddress */
struct tarpc_wsa_string_to_address_in {
    struct tarpc_in_arg  common;
    char                 addrstr<>;
    tarpc_int            address_family;
    uint8_t              info<>; /**< Protocol Info */
    tarpc_socklen_t      addrlen<>;
};

struct tarpc_wsa_string_to_address_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    struct tarpc_sa      addr;
    tarpc_socklen_t      addrlen<>;
};

/* WSACancelAsyncRequest */
struct tarpc_wsa_cancel_async_request_in {
    struct tarpc_in_arg  common;
    tarpc_handle         async_task_handle;
};

struct tarpc_wsa_cancel_async_request_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
};

/* malloc */
struct tarpc_malloc_in {
    struct tarpc_in_arg  common;
    tarpc_size_t         size; /**< Bytes to allocate */
};

struct tarpc_malloc_out {
    struct tarpc_out_arg common;
    tarpc_ptr            retval; /**< A pointer in the TA address space */
};

/* free */
struct tarpc_free_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            buf;    /**< A pointer in the TA address space */
};

struct tarpc_free_out {
    struct tarpc_out_arg common;
};

/* set_buf */
struct tarpc_set_buf_in {
    struct tarpc_in_arg  common;
    char                 src_buf<>;
    tarpc_ptr            dst_buf; /**< A pointer in the TA address space */
    tarpc_ptr            offset;  /**< A displacement in dest. buffer */
};

struct tarpc_set_buf_out {
    struct tarpc_out_arg common;
};

/* get_buf */
struct tarpc_get_buf_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            src_buf;  /**< A pointer in the TA address space */
    tarpc_ptr            offset;   /**< A displacement in source buffer */
    tarpc_size_t         len;      /**< How much to get */
};

struct tarpc_get_buf_out {
    struct tarpc_out_arg common;
    char                 dst_buf<>;
};

/* alloc_wsabuf */
struct tarpc_alloc_wsabuf_in {
    struct tarpc_in_arg  common;
    tarpc_ssize_t         len; /**< Length of buffer to allocate */
};

struct tarpc_alloc_wsabuf_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    tarpc_ptr            wsabuf;     /**< A pointer to WSABUF structure
                                          in the TA address space */
    tarpc_ptr            wsabuf_buf; /**< A pointer to buffer in the
                                          TA address space */
};

/* free_wsabuf */
struct tarpc_free_wsabuf_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            wsabuf;     /**< A pointer to WSABUF structure
                                          in the TA address space */
};

struct tarpc_free_wsabuf_out {
    struct tarpc_out_arg common;
};

/* Windows FLOWSPEC structure */
struct tarpc_flowspec {
    uint32_t TokenRate;
    uint32_t TokenBucketSize;
    uint32_t PeakBandwidth;
    uint32_t Latency;
    uint32_t DelayVariation;
    uint32_t ServiceType;
    uint32_t MaxSduSize;
    uint32_t MinimumPolicedSize;
};

/* Windows QOS structure */
struct tarpc_qos {
    struct tarpc_flowspec  sending;
    struct tarpc_flowspec  receiving;
    uint8_t                provider_specific_buf<>;
};

/* WSAConnect */
struct tarpc_wsa_connect_in {
    struct tarpc_in_arg    common;
    tarpc_int              s;
    struct tarpc_sa        addr;
    tarpc_size_t           addrlen;
    tarpc_ptr              caller_wsabuf; /**< A pointer to WSABUF structure
                                               in TA address space */
    tarpc_ptr              callee_wsabuf; /**< A pointer to WSABUF structure
                                               in TA address space */
    struct tarpc_qos       sqos;
    tarpc_bool             sqos_is_null;
};

struct tarpc_wsa_connect_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
};

/* Windows GUID structure */
struct tarpc_guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4<>;  /* 8 bytes array */
};

/* Windows tcp_keepalive structure */
struct tarpc_tcp_keepalive {
    uint32_t onoff;
    uint32_t keepalivetime;
    uint32_t keepaliveinterval;
};

/* WSAIoctl() */
enum wsa_ioctl_type {
    WSA_IOCTL_VOID = 1,      /* no data */
    WSA_IOCTL_INT,
    WSA_IOCTL_SA,            /* socket address */
    WSA_IOCTL_SAA,           /* socket addresses array */
    WSA_IOCTL_GUID,
    WSA_IOCTL_TCP_KEEPALIVE,
    WSA_IOCTL_QOS,
    WSA_IOCTL_PTR            /* a pointer valid in the TA address spase */
};

union wsa_ioctl_request switch (wsa_ioctl_type type) {
    case WSA_IOCTL_INT:            tarpc_int              req_int;
    case WSA_IOCTL_SA:             tarpc_sa               req_sa;
    case WSA_IOCTL_SAA:            tarpc_sa               req_saa<>;
    case WSA_IOCTL_GUID:           tarpc_guid             req_guid;
    case WSA_IOCTL_TCP_KEEPALIVE:  tarpc_tcp_keepalive    req_tka;
    case WSA_IOCTL_QOS:            tarpc_qos              req_qos;
    case WSA_IOCTL_PTR:            tarpc_ptr              req_ptr;
};

struct tarpc_wsa_ioctl_in {
    struct tarpc_in_arg common;
    tarpc_int           s;
    tarpc_int           code;
    wsa_ioctl_request   req;
    tarpc_size_t        inbuf_len;
    tarpc_size_t        outbuf_len;
    tarpc_overlapped    overlapped;
    tarpc_bool          callback;
};

struct tarpc_wsa_ioctl_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    wsa_ioctl_request    result;
    tarpc_uint           bytes_returned;
};

/* rpc_get_wsa_ioctl_overlapped_result() */
struct tarpc_get_wsa_ioctl_overlapped_result_in {
    struct tarpc_in_arg  common;
    tarpc_int            s;              /**< Socket    */
    tarpc_overlapped     overlapped;     /**< WSAOVERLAPPED structure */
    tarpc_int            wait;           /**< Wait flag */
    tarpc_int            bytes<>;        /**< Transferred bytes location */
    tarpc_int            flags<>;        /**< Flags location */
    tarpc_int            code;           /**< IOCTL control code */
};    

struct tarpc_get_wsa_ioctl_overlapped_result_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
    tarpc_int             bytes<>;    /**< Transferred bytes */
    tarpc_int             flags<>;    /**< Flags */
    wsa_ioctl_request     result;
};    

/* WSAAsyncGetHostByAddr */
struct tarpc_wsa_async_get_host_by_addr_in {
    struct tarpc_in_arg  common;
    tarpc_hwnd           hwnd;
    tarpc_uint           wmsg;
    char                 addr<>;
    tarpc_size_t         addrlen;
    tarpc_int            type;   /**< TA-independent socket type */    
    tarpc_ptr            buf;    /**< Buffer in TA address space */
    tarpc_size_t         buflen;
};

struct tarpc_wsa_async_get_host_by_addr_out {
    struct tarpc_out_arg common;
    tarpc_handle         retval;
};

/* WSAAsyncGetHostByName */
struct tarpc_wsa_async_get_host_by_name_in {
    struct tarpc_in_arg  common;
    tarpc_hwnd           hwnd;
    tarpc_uint           wmsg;
    char                 name<>;
    tarpc_ptr            buf;    /**< Buffer in TA address space */
    tarpc_size_t         buflen;
};

struct tarpc_wsa_async_get_host_by_name_out {
    struct tarpc_out_arg common;
    tarpc_handle         retval;
};

/* WSAAsyncGetProtoByName */
struct tarpc_wsa_async_get_proto_by_name_in {
    struct tarpc_in_arg  common;
    tarpc_hwnd           hwnd;
    tarpc_uint           wmsg;
    char                 name<>;
    tarpc_ptr            buf;    /**< Buffer in TA address space */
    tarpc_size_t         buflen;
};

struct tarpc_wsa_async_get_proto_by_name_out {
    struct tarpc_out_arg common;
    tarpc_handle         retval;
};

/* WSAAsyncGetProtoByNumber */
struct tarpc_wsa_async_get_proto_by_number_in {
    struct tarpc_in_arg  common;
    tarpc_hwnd           hwnd;
    tarpc_uint           wmsg;
    tarpc_int            number;
    tarpc_ptr            buf;    /**< Buffer in TA address space */
    tarpc_size_t         buflen;
};

struct tarpc_wsa_async_get_proto_by_number_out {
    struct tarpc_out_arg common;
    tarpc_handle         retval;
};

/* WSAAsyncGetServByName */
struct tarpc_wsa_async_get_serv_by_name_in {
    struct tarpc_in_arg  common;
    tarpc_hwnd           hwnd;
    tarpc_uint           wmsg;
    char                 name<>;
    char                 proto<>;
    tarpc_ptr            buf;    /**< Buffer in TA address space */
    tarpc_size_t         buflen;
};

struct tarpc_wsa_async_get_serv_by_name_out {
    struct tarpc_out_arg common;
    tarpc_handle         retval;
};

/* WSAAsyncGetServByPort */
struct tarpc_wsa_async_get_serv_by_port_in {
    struct tarpc_in_arg  common;
    tarpc_hwnd           hwnd;
    tarpc_uint           wmsg;
    tarpc_uint           port;
    char                 proto<>;
    tarpc_ptr            buf;    /**< Buffer in TA address space */
    tarpc_size_t         buflen;
};

struct tarpc_wsa_async_get_serv_by_port_out {
    struct tarpc_out_arg common;
    tarpc_handle         retval;
};

/* Create/delete WSAOVERLAPPED structure */
struct tarpc_create_overlapped_in {
    struct tarpc_in_arg common;
    tarpc_wsaevent      hevent;
    tarpc_uint          offset;
    tarpc_uint          offset_high;
};

struct tarpc_create_overlapped_out {
    struct tarpc_out_arg common;
    tarpc_overlapped     retval;
};

struct tarpc_delete_overlapped_in {
    struct tarpc_in_arg common;
    tarpc_overlapped     overlapped;
};
typedef struct tarpc_void_out tarpc_delete_overlapped_out;


/* WSAEventSelect() */

struct tarpc_event_select_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;           /**< TA-local socket */ 
    tarpc_wsaevent  hevent;       /**< Event object to be associated
                                       with set of network events */   
    uint32_t        event;        /**< Bitmask that specifies the set
                                         of network events */      
};

typedef struct tarpc_int_retval_out tarpc_event_select_out;

/* WSAEnumNetworkEvents() */

struct tarpc_enum_network_events_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;           /**< TA-local socket */ 
    tarpc_wsaevent  hevent;       /**< Event object to be reset */   
    uint32_t        event<>;      /**< Bitmask that specifies the set
                                       of network events occurred */      
};

struct tarpc_enum_network_events_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
    uint32_t             event<>;    /**< Bitmask that specifies the set
                                          of network events occurred */      
};

/* Create window */
struct tarpc_create_window_in {
    struct tarpc_in_arg common;
};

struct tarpc_create_window_out {
    struct tarpc_out_arg common;
    tarpc_hwnd          hwnd;
};

/* DestroyWindow */
struct tarpc_destroy_window_in {
    struct tarpc_in_arg common;
    tarpc_hwnd          hwnd;
};

typedef struct tarpc_void_out tarpc_destroy_window_out;

/* Get completion callback result */
struct tarpc_completion_callback_in {
    struct tarpc_in_arg common;
};

struct tarpc_completion_callback_out {
    struct tarpc_out_arg common;
    tarpc_int            called;        /**< 1 if callback was called    */
    tarpc_int            error;         /**< Error code (as is for now)  */
    tarpc_int            bytes;         /**< Number of bytes transferred */
    tarpc_overlapped     overlapped;    /**< Overlapped passed to callback */
};


/* WSAAsyncSelect */
struct tarpc_wsa_async_select_in {
    struct tarpc_in_arg common;
    tarpc_int           sock;   /**< Socket for WSAAsyncSelect() */
    tarpc_hwnd          hwnd;   /**< Window for messages receiving */
    tarpc_uint          event;  /**< Network event to be listened */
};

typedef struct tarpc_int_retval_out tarpc_wsa_async_select_out;

/* PeekMessage */
struct tarpc_peek_message_in {
    struct tarpc_in_arg common;
    tarpc_hwnd          hwnd;   /**< Window to be checked */
};

struct tarpc_peek_message_out {
    struct tarpc_out_arg common;
    tarpc_int            retval; /**< 0 if there are no messages */
    tarpc_int            sock;   /**< Socket about which the message is 
                                      received */
    tarpc_uint           event;  /**< Event about which the message is 
                                      received */
};

/* FD_SET() */

struct tarpc_do_fd_set_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;
    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_do_fd_set_out;


/* FD_CLR() */

struct tarpc_do_fd_clr_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;
    tarpc_fd_set        set;
};

typedef struct tarpc_void_out tarpc_do_fd_clr_out;


/* FD_ISSET() */

struct tarpc_do_fd_isset_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;
    tarpc_fd_set        set;
};

typedef struct tarpc_int_retval_out tarpc_do_fd_isset_out;



/*
 * I/O multiplexing
 */

/* select() */

struct tarpc_select_in {
    struct tarpc_in_arg     common;

    tarpc_int               n;
    tarpc_fd_set            readfds;
    tarpc_fd_set            writefds;
    tarpc_fd_set            exceptfds;
    struct tarpc_timeval    timeout<>;
};

struct tarpc_select_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_timeval    timeout<>;
};
  

/* pselect() */

struct tarpc_pselect_in {
    struct tarpc_in_arg     common;

    tarpc_int               n;
    tarpc_fd_set            readfds;
    tarpc_fd_set            writefds;
    tarpc_fd_set            exceptfds;
    struct tarpc_timespec   timeout<>;
    tarpc_sigset_t          sigmask;
};

typedef struct tarpc_int_retval_out tarpc_pselect_out;


/* poll() */

struct tarpc_pollfd {
    tarpc_int   fd;
    int16_t     events;
    int16_t     revents;
};

struct tarpc_poll_in {
    struct tarpc_in_arg common;

    struct tarpc_pollfd ufds<>;
    tarpc_uint          nfds;
    tarpc_int           timeout;
};

struct tarpc_poll_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_pollfd     ufds<>;
};


/*
 * Socket options
 */
 
enum option_type {
    OPT_INT      = 1,
    OPT_LINGER   = 2,
    OPT_TIMEVAL  = 3,
    OPT_MREQN    = 4,
    OPT_IPADDR   = 5,
    OPT_STRING   = 6,
    OPT_TCP_INFO = 7
};

struct option_value_linger {
    tarpc_int l_onoff; 
    tarpc_int l_linger;
};

struct option_value_mreqn {
    uint32_t    imr_multiaddr;  /**< IP multicast group address */
    uint32_t    imr_address;    /**< IP address of local interface */
    tarpc_int   imr_ifindex;    /**< Interpace index */
};

struct option_value_tcp_info {
    uint8_t     tcpi_state;
    uint8_t     tcpi_ca_state;
    uint8_t     tcpi_retransmits;
    uint8_t     tcpi_probes;
    uint8_t     tcpi_backoff;
    uint8_t     tcpi_options;
    uint8_t     tcpi_snd_wscale;
    uint8_t     tcpi_rcv_wscale;

    uint32_t    tcpi_rto;
    uint32_t    tcpi_ato;
    uint32_t    tcpi_snd_mss;
    uint32_t    tcpi_rcv_mss;

    uint32_t    tcpi_unacked;
    uint32_t    tcpi_sacked;
    uint32_t    tcpi_lost;
    uint32_t    tcpi_retrans;
    uint32_t    tcpi_fackets;

    /* Times. */
    uint32_t    tcpi_last_data_sent;
    uint32_t    tcpi_last_ack_sent;
    uint32_t    tcpi_last_data_recv;
    uint32_t    tcpi_last_ack_recv;

    /* Metrics. */
    uint32_t    tcpi_pmtu;
    uint32_t    tcpi_rcv_ssthresh;
    uint32_t    tcpi_rtt;
    uint32_t    tcpi_rttvar;
    uint32_t    tcpi_snd_ssthresh;
    uint32_t    tcpi_snd_cwnd;
    uint32_t    tcpi_advmss;
    uint32_t    tcpi_reordering;
};

union option_value switch (option_type opttype) {
    case OPT_INT:     tarpc_int opt_int;
    case OPT_LINGER:  struct option_value_linger opt_linger;
    case OPT_TIMEVAL: struct tarpc_timeval opt_timeval;
    case OPT_MREQN:   struct option_value_mreqn  opt_mreqn;
    case OPT_IPADDR:  uint32_t opt_ipaddr;
    case OPT_STRING:  char opt_string<>;
    case OPT_TCP_INFO: struct option_value_tcp_info opt_tcp_info;
};

/* setsockopt() */

struct tarpc_setsockopt_in {
    struct tarpc_in_arg  common;

    tarpc_int       s; 
    tarpc_int       level;
    tarpc_int       optname;
    option_value    optval<>;
    tarpc_socklen_t optlen;
};

typedef struct tarpc_int_retval_out tarpc_setsockopt_out;

/* getsockopt() */

struct tarpc_getsockopt_in {
    struct tarpc_in_arg  common;

    tarpc_int       s;
    tarpc_int       level; 
    tarpc_int       optname;
    option_value    optval<>;
    tarpc_socklen_t optlen<>;
};

struct tarpc_getsockopt_out {
    struct tarpc_out_arg  common;

    tarpc_int             retval;
    option_value          optval<>;
    tarpc_socklen_t       optlen<>;
};




/* ioctl() */

enum ioctl_type {
    IOCTL_INT = 1,
    IOCTL_TIMEVAL = 2,
    IOCTL_IFREQ = 3,
    IOCTL_IFCONF = 4,
    IOCTL_ARPREQ = 5
    
};

/* Access type of ioctl requests */
enum ioctl_access {
    IOCTL_RD = 1,
    IOCTL_WR = 2,
    IOCTL_RW = 3
};    

union ioctl_request switch (ioctl_type type) {
    case IOCTL_INT:     tarpc_int     req_int;
    case IOCTL_TIMEVAL: tarpc_timeval req_timeval;
    case IOCTL_IFREQ:   tarpc_ifreq   req_ifreq; 
    case IOCTL_IFCONF:  tarpc_ifconf  req_ifconf; 
    case IOCTL_ARPREQ:  tarpc_arpreq  req_arpreq;
};

struct tarpc_ioctl_in {
    struct tarpc_in_arg common;
    
    tarpc_int           s;
    tarpc_int           code;
    ioctl_access        access;
    ioctl_request       req<>;
};

struct tarpc_ioctl_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
    ioctl_request        req<>;
};


/* fcntl() */

struct tarpc_fcntl_in {
    struct tarpc_in_arg common;
    
    tarpc_int   fd;
    tarpc_int   cmd;
    tarpc_int   arg;
};

struct tarpc_fcntl_out {
    struct tarpc_out_arg common;
    
    tarpc_int   retval;
};


/*
 * Interface name/index
 */

/* if_nametoindex() */

struct tarpc_if_nametoindex_in {
    struct tarpc_in_arg common;

    char                ifname<>;
};

struct tarpc_if_nametoindex_out {
    struct tarpc_out_arg    common;

    tarpc_uint              ifindex;
};


/* if_indextoname() */

struct tarpc_if_indextoname_in {
    struct tarpc_in_arg common;

    tarpc_uint          ifindex;
    char                ifname<>;
};

struct tarpc_if_indextoname_out {
    struct tarpc_out_arg    common;

    char                    ifname<>;
};


/* if_nameindex() */

struct tarpc_if_nameindex {
    tarpc_uint      ifindex;
    char            ifname<>;
};

struct tarpc_if_nameindex_in {
    struct tarpc_in_arg common;
};

struct tarpc_if_nameindex_out {
    struct tarpc_out_arg        common;

    struct tarpc_if_nameindex   ptr<>;
    tarpc_ptr                   mem_ptr;
};


/* if_freenameindex() */

struct tarpc_if_freenameindex_in {
    struct tarpc_in_arg common;

    tarpc_ptr           mem_ptr;
};

typedef struct tarpc_void_out tarpc_if_freenameindex_out;

/*
 * Signal handlers are registered by name of the funciton
 * in RPC server context.
 */

/* signal() */

struct tarpc_signal_in {
    struct tarpc_in_arg common;

    tarpc_signum    signum;
    char            handler<>;  /**< Name of the signal handler function */
};

struct tarpc_signal_out {
    struct tarpc_out_arg    common;

    char            handler<>;  /**< Name of the old signal
                                     handler function */
};

/* sigaction() */
struct tarpc_sigaction_in {
    struct tarpc_in_arg     common;

    tarpc_signum            signum;   /**< The signal number */
    struct tarpc_sigaction  act<>;    /**< If it's non-null, the new action
                                           for signal signum is installed */
    struct tarpc_sigaction  oldact<>; /**< If it's non-null, the old action
                                           for signal signum was installed */
};

struct tarpc_sigaction_out {
    struct tarpc_out_arg    common;

    struct tarpc_sigaction  oldact<>;   /**< If it's non-null, the previous
                                             action will be saved */
    tarpc_int               retval;
};

/*
 * Is kill() RPC required? It seems RCF is sufficient.
 * It might be usefull, if IUT has it's own implementation,
 * but it look unreal.
 */

/* kill() */

struct tarpc_kill_in {
    struct tarpc_in_arg common;

    tarpc_pid_t     pid;
    tarpc_signum    signum;
};

typedef struct tarpc_int_retval_out tarpc_kill_out;



/*
 * Signal set are allocated/destroyed and manipulated in server context.
 * Two additional functions are defined to allocate/destroy signal set.
 * All standard signal set related functions get pointer to sigset_t.
 * Zero (0) handle corresponds to NULL pointer.
 */

/* sigset_new() */

typedef struct tarpc_void_in tarpc_sigset_new_in;

struct tarpc_sigset_new_out {
    struct tarpc_out_arg    common;

    tarpc_sigset_t          set;    /**< Handle (pointer in server 
                                         context) of the allocated set */
};


/* sigset_delete() */

struct tarpc_sigset_delete_in {
    struct tarpc_in_arg common;

    tarpc_sigset_t      set;    /**< Handle of the set to be freed */
                                  
};

typedef struct tarpc_void_out tarpc_sigset_delete_out;


/* sigprocmask() */

struct tarpc_sigprocmask_in {
    struct tarpc_in_arg common;

    tarpc_int           how;
    tarpc_sigset_t      set;    /**< Handle of the signal set */
    tarpc_sigset_t      oldset; /**< Handle of the old signal set */
};

typedef struct tarpc_int_retval_out tarpc_sigprocmask_out;

/* sigemptyset() */

struct tarpc_sigemptyset_in {
    struct tarpc_in_arg common;

    tarpc_sigset_t      set;    /**< Handle of the signal set */
};

typedef struct tarpc_int_retval_out tarpc_sigemptyset_out;


/* sigfillset() */

typedef struct tarpc_sigemptyset_in tarpc_sigfillset_in;

typedef struct tarpc_sigemptyset_out tarpc_sigfillset_out;


/* sigaddset() */

struct tarpc_sigaddset_in {
    struct tarpc_in_arg common;

    tarpc_sigset_t  set;    /**< Handle of the signal set */
    tarpc_signum    signum;
};

typedef struct tarpc_sigemptyset_out tarpc_sigaddset_out;


/* sigdelset() */

typedef struct tarpc_sigaddset_in tarpc_sigdelset_in;

typedef struct tarpc_sigemptyset_out tarpc_sigdelset_out;
 

/* sigismember() */

typedef struct tarpc_sigaddset_in tarpc_sigismember_in;

typedef struct tarpc_sigemptyset_out tarpc_sigismember_out;


/* sigpending() */

typedef struct tarpc_sigemptyset_in tarpc_sigpending_in;

typedef struct tarpc_int_retval_out tarpc_sigpending_out;


/* sigsuspend() */

typedef struct tarpc_sigemptyset_in tarpc_sigsuspend_in;

typedef struct tarpc_int_retval_out tarpc_sigsuspend_out;


/* sigreceived() */

typedef struct tarpc_void_in tarpc_sigreceived_in;

typedef struct tarpc_sigset_new_out tarpc_sigreceived_out;



/* gethostbyname */

struct tarpc_h_addr {
    char val<>;
};

struct tarpc_h_alias {
    char name<>;
};

struct tarpc_hostent {
    char          h_name<>;         /**< Host name */
    tarpc_h_alias h_aliases<>;      /**< List of aliases */
    tarpc_int     h_addrtype;       /**< RPC domain */
    tarpc_int     h_length;         /**< Address length */
    tarpc_h_addr  h_addr_list<>;    /**< List of addresses */
};    

struct tarpc_gethostbyname_in {
    struct tarpc_in_arg common;
    char                name<>;
};    

struct tarpc_gethostbyname_out {
    struct tarpc_out_arg common;
    tarpc_hostent        res<>;
};

/* gethostbyaddr */

struct tarpc_gethostbyaddr_in {
    struct tarpc_in_arg common;
    tarpc_h_addr        addr; /**< Addresses */
    tarpc_int           type; /**< RPC domain */
};    

struct tarpc_gethostbyaddr_out {
    struct tarpc_out_arg common;
    tarpc_hostent        res<>;
};

struct tarpc_ai {
    tarpc_int     flags;
    tarpc_int     family;
    tarpc_int     socktype;
    tarpc_int     protocol;
    tarpc_size_t  addrlen;
    tarpc_sa      addr;
    char          canonname<>;
};

struct tarpc_getaddrinfo_in {
    struct tarpc_in_arg common;
    char                node<>;
    char                service<>;
    tarpc_ai            hints<>;
};

struct tarpc_getaddrinfo_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    tarpc_ptr            mem_ptr;
    tarpc_ai             res<>;
};

struct tarpc_freeaddrinfo_in {
    struct tarpc_in_arg common;
    tarpc_ptr           mem_ptr;
};

typedef struct tarpc_void_out tarpc_freeaddrinfo_out;

/* pipe() */

struct tarpc_pipe_in {
    struct tarpc_in_arg common;

    tarpc_int   filedes<>;
};

struct tarpc_pipe_out {
    struct tarpc_out_arg common;
    tarpc_int   retval;
    tarpc_int   filedes<>;
};

/* socketpair() */

struct tarpc_socketpair_in {
    struct tarpc_in_arg common;
    
    tarpc_int   domain; /**< TA-independent domain */
    tarpc_int   type;   /**< TA-independent socket type */
    tarpc_int   proto;  /**< TA-independent socket protocol */

    tarpc_int   sv<>;   /**< Socket pair */
};

struct tarpc_socketpair_out {
    struct tarpc_out_arg common;
    
    tarpc_int   retval; /**< Returned value */

    tarpc_int   sv<>;   /**< Socket pair */
};

/* fopen() */
struct tarpc_fopen_in {
    struct tarpc_in_arg common;
    
    char path<>;
    char mode<>;
};

struct tarpc_fopen_out {
    struct tarpc_out_arg common;
    
    tarpc_ptr   mem_ptr;
};

/* fclose() */
struct tarpc_fclose_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            mem_ptr;
};

struct tarpc_fclose_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
};

/* popen() */
struct tarpc_popen_in {
    struct tarpc_in_arg common;
    
    char cmd<>;
    char mode<>;
};

struct tarpc_popen_out {
    struct tarpc_out_arg common;
    
    tarpc_ptr   mem_ptr;
};

/* getenv() */
struct tarpc_getenv_in {
    struct tarpc_in_arg common;
    
    string name<>;
};

struct tarpc_getenv_out {
    struct tarpc_out_arg common;
    
    string val<>;
};

/* setenv() */
struct tarpc_setenv_in {
    struct tarpc_in_arg common;
    
    string     name<>;
    string     val<>;
    tarpc_bool overwrite;
};


struct tarpc_setenv_out {
    struct tarpc_out_arg common;
    
    tarpc_int retval;
};

/* fileno() */
struct tarpc_fileno_in {
    struct tarpc_in_arg common;
    
    tarpc_ptr   mem_ptr;
};

struct tarpc_fileno_out {
    struct tarpc_out_arg common;
    
    tarpc_int fd;
};

/* struct passwd */
struct tarpc_passwd {
    char       name<>;      /* User name */
    char       passwd<>;    /* User password */
    tarpc_uint uid;         /* User id */
    tarpc_uint gid;         /* Group id */
    char       gecos<>;     /* Real name */
    char       dir<>;       /* Home directory */
    char       shell<>;     /* Shell program */
};

/* getpwnam() */
struct tarpc_getpwnam_in {
    struct tarpc_in_arg common;
    
    char name<>;
};

struct tarpc_getpwnam_out {
    struct tarpc_out_arg common;
    
    struct tarpc_passwd passwd;   /* name is NULL if entry is not found */
};

/* getuid() */
struct tarpc_getuid_in {
    struct tarpc_in_arg common;
};

struct tarpc_getuid_out {
    struct tarpc_out_arg common;
    
    tarpc_uint  uid;
};

/* geteuid */
typedef struct tarpc_getuid_in tarpc_geteuid_in;
typedef struct tarpc_getuid_out tarpc_geteuid_out;

/* setuid() */
struct tarpc_setuid_in {
    struct tarpc_in_arg common;
    
    tarpc_uint  uid;
};

typedef struct tarpc_int_retval_out tarpc_setuid_out;


/* geteuid */
typedef struct tarpc_setuid_in tarpc_seteuid_in;
typedef struct tarpc_setuid_out tarpc_seteuid_out;


/* getpid() */
struct tarpc_getpid_in {
    struct tarpc_in_arg common;
};

typedef struct tarpc_int_retval_out tarpc_getpid_out;


struct tarpc_simple_sender_in {
    struct tarpc_in_arg common;
    
    tarpc_int   s;              /**< Socket to be used */
    uint32_t    size_min;       /**< Minimum size of the message */
    uint32_t    size_max;       /**< Maximum size of the message */
    tarpc_bool  size_rnd_once;  /**< If true, random size should be
                                     calculated only once and used for
                                     all messages; if false, random size
                                     is calculated for each message */
    uint32_t    delay_min;      /**< Minimum delay between messages in 
                                     microseconds */
    uint32_t    delay_max;      /**< Maximum delay between messages in 
                                     microseconds */
    tarpc_bool  delay_rnd_once; /**< If true, random delay should be
                                     calculated only once and used for
                                     all messages; if false, random
                                     delay is calculated for each
                                     message */
    uint32_t    time2run;       /**< How long run (in seconds) */
    tarpc_bool  ignore_err;     /**< Ignore errors while run */
};

struct tarpc_simple_sender_out {
    struct tarpc_out_arg common;
    
    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */
    
    uint64_t    bytes;      /**< Number of sent bytes */
};

struct tarpc_simple_receiver_in {
    struct tarpc_in_arg common;
    
    tarpc_int   s;               /**< Socket to be used */

    uint32_t    time2run;        /**< Receiving duration (in seconds) */
};

struct tarpc_simple_receiver_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */
    
    uint64_t    bytes;      /**< Number of received bytes */
};


struct tarpc_recv_verify_in {
    struct tarpc_in_arg common;
    
    tarpc_int   s;        /**< Socket to be used */
    char        fname<>;  /**< Name of function to generate data */

    uint64_t    start;    /**< Position in stream of first byte
                               which should be received. */
};

struct tarpc_recv_verify_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< bytes received (success) or -1 (failure) */
};


struct tarpc_send_traffic_in {
    struct tarpc_in_arg common;
    tarpc_int           num;    /**< Number of packets to be sent */
    tarpc_int           fd<>;     /**< List of sockets */
    uint8_t             buf<>;  /**< Buffer */
    tarpc_size_t        len;    /**< Buffer length */
    tarpc_int           flags;  /**< Flags */ 
    struct tarpc_sa     to<>;     /**< List of addresses */
    tarpc_socklen_t     tolen;  /**< Address length */
};
typedef struct tarpc_ssize_t_retval_out tarpc_send_traffic_out;

struct tarpc_timely_round_trip_in {
    struct tarpc_in_arg common;
    tarpc_int           sock_num;    /**< Number of sockets */
    tarpc_int           fd<>;        /**< Socket */
    tarpc_size_t        size;        /**< Buffer length */
    tarpc_size_t        vector_len;  /**< Vector length */
    uint32_t            timeout;     /**< Timeout passed to select */
    uint32_t            time2wait;   /**< Time during for a roundtrip 
                                          should be performed */
    tarpc_int           flags;       /**< Flags */
    tarpc_int           addr_num;    /**< Number of addresses */
    struct tarpc_sa     to<>;        /**< Adresses list */
    tarpc_socklen_t     tolen;       /**< Address length */
};    

enum round_trip_error {
    ROUND_TRIP_ERROR_OTHER = 1,
    ROUND_TRIP_ERROR_SEND = 2,
    ROUND_TRIP_ERROR_RECV = 3,
    ROUND_TRIP_ERROR_TIMEOUT = 4,
    ROUND_TRIP_ERROR_TIME_EXPIRED = 5
};    

struct tarpc_timely_round_trip_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
    tarpc_int             index;  /**< Index in addresses list
                                       for which address 
                                       error occured */
};

struct tarpc_round_trip_echoer_in {
    struct tarpc_in_arg common;
    tarpc_int           sock_num;    /**< Number of sockets */
    tarpc_int           fd<>;        /**< Sockets list */
    tarpc_int           addr_num;    /**< Number of addresses for which echo
                                          must be done */
    tarpc_size_t        size;        /**< Buffer length */
    tarpc_size_t        vector_len;  /**< Vector length */
    uint32_t            timeout;     /**< Timeout passed to select */
    tarpc_int           flags;       /**< Flags */
};

struct tarpc_round_trip_echoer_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
    tarpc_int             index;  /**< How many times echoes 
                                       were done */
};


/*
 * IOMUX functions
 */
enum iomux_func {
    FUNC_SELECT = 1,
    FUNC_PSELECT = 2,
    FUNC_POLL = 3
};


struct tarpc_flooder_in {
    struct tarpc_in_arg common;

    /** Input */
    tarpc_int       sndrs<>;
    tarpc_int       rcvrs<>;
    tarpc_size_t    bulkszs;
    uint32_t        time2run;        /**< How long run (in seconds) */
    uint32_t        time2wait;       /**< How long wait (in seconds) */
    iomux_func      iomux;
    tarpc_bool      rx_nonblock;

    uint64_t        tx_stat<>;
    uint64_t        rx_stat<>;
};

struct tarpc_flooder_out {
    struct tarpc_out_arg common;

    tarpc_int       retval;  /**< 0 (success) or -1 (failure) */

    uint64_t        tx_stat<>;
    uint64_t        rx_stat<>;
};


struct tarpc_echoer_in {
    struct tarpc_in_arg common;

    tarpc_int   sockets<>;
    uint32_t    time2run;        /**< How long run (in seconds) */
    iomux_func  iomux;

    uint64_t    tx_stat<>;
    uint64_t    rx_stat<>;
};

struct tarpc_echoer_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;  /**< 0 (success) or -1 (failure) */
    uint64_t    tx_stat<>;
    uint64_t    rx_stat<>;
};


struct tarpc_aio_read_test_in {
    struct tarpc_in_arg common;
    
    tarpc_int       s;      /**< Socket to be used */
    tarpc_signum    signum; /**< Signal to be used */
    tarpc_int       t;      /**< Timeout for select() */
    uint32_t        buflen; /**< Buffer length to be passed to the read */
    uint8_t         buf<>;  /**< Read data */
    char            diag<>; /**< Error message */
};

struct tarpc_aio_read_test_out {
    struct tarpc_out_arg common;
    
    tarpc_int   retval; /**< Status returned by the test routine */
    uint8_t     buf<>;  /**< Data buffer */
    char        diag<>; /**< Error message */
};

struct tarpc_aio_error_test_in {
    struct tarpc_in_arg common;

    char        diag<>; /**< Error message */
};

struct tarpc_aio_error_test_out {
    struct tarpc_out_arg common;
    
    tarpc_int   retval; /**< Status returned by the test routine */
    char        diag<>; /**< Error message */
};

struct tarpc_aio_write_test_in {
    struct tarpc_in_arg common;
    
    tarpc_int       s;      /**< Socket to be used */
    tarpc_signum    signum; /**< Signal to be used */
    uint8_t         buf<>;  /**< Data to be sent*/
    char            diag<>; /**< Error message */
};

struct tarpc_aio_write_test_out {
    struct tarpc_out_arg common;
    
    tarpc_int       retval; /**< Status returned by the test routine */
    char            diag<>; /**< Error message */
};

struct tarpc_aio_suspend_test_in {
    struct tarpc_in_arg common;
    
    tarpc_int       s;      /**< Socket to be used */
    tarpc_int       s_aux;  /**< Additional dummy socket */
    tarpc_signum    signum; /**< Signal to be used */
    int32_t         t;      /**< Timeout for suspend */
    uint8_t         buf<>;  /**< Data buffer */
    char            diag<>; /**< Error message */
};

struct tarpc_aio_suspend_test_out {
    struct tarpc_out_arg common;
    
    int32_t     retval; /**< Status returned by the test routine */
    char        buf<>;  /**< Read data */
    char        diag<>; /**< Error message */
};

struct tarpc_fork_in {
    struct tarpc_in_arg common;
    
    char name<>;        /**< RPC server name */
};

struct tarpc_fork_out {
    struct tarpc_out_arg common;
    
    int32_t     pid;
};

struct tarpc_pthread_create_in {
    struct tarpc_in_arg common;
    
    char name<>;
};

struct tarpc_pthread_create_out {
    struct tarpc_out_arg common;
    
    tarpc_ptr   tid;
    tarpc_int   retval;
};

struct tarpc_pthread_cancel_in {
    struct tarpc_in_arg common;
    
    tarpc_ptr           tid;
};

struct tarpc_pthread_cancel_out {
    struct tarpc_out_arg common;
    
    tarpc_int   retval;
};

struct tarpc_execve_in {
    struct tarpc_in_arg common;

    string name<>;        /**< RPC server name */
};

struct tarpc_execve_out {
    struct tarpc_out_arg common;
};


/* sendfile() */
struct tarpc_sendfile_in {
    struct tarpc_in_arg common;

    tarpc_int           out_fd;
    tarpc_int           in_fd;
    tarpc_off_t         offset<>;
    tarpc_size_t        count;
};

struct tarpc_sendfile_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t   retval;     /**< Returned length */
    tarpc_off_t     offset<>;   /**< new offset returned by the function */
};


/* socket_to_file() */
struct tarpc_socket_to_file_in {
    struct tarpc_in_arg common;

    tarpc_int   sock;
    char        path<>;
    uint32_t    timeout;
};

typedef struct tarpc_ssize_t_retval_out tarpc_socket_to_file_out;

/* WSASend */
struct tarpc_wsa_send_in {
    struct tarpc_in_arg common;

    tarpc_int           s;              /**< Socket */
    struct tarpc_iovec  vector<>;       /**< Buffers */
    tarpc_size_t        count;          /**< Number of buffers */
    tarpc_int           bytes_sent<>;   /**< Location for sent bytes num */
    tarpc_int           flags;          /**< Flags */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    tarpc_bool          callback;       /**< If 1, completion callback should be 
                                             specified */
};

struct tarpc_wsa_send_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    tarpc_int            bytes_sent<>;
};

/* WSARecv */
struct tarpc_wsa_recv_in {
    struct tarpc_in_arg common;

    tarpc_int           s;              /**< Socket */
    struct tarpc_iovec  vector<>;       /**< Buffers */
    tarpc_size_t        count;          /**< Number of buffers */
    tarpc_int           bytes_received<>;   
                                        /**< Location for received bytes num */
    tarpc_int           flags<>;        /**< Flags */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    tarpc_bool          callback;       /**< If 1, completion callback
                                             should be specified */
};

struct tarpc_wsa_recv_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    struct tarpc_iovec   vector<>;       
    tarpc_int            bytes_received<>;   
    tarpc_int            flags<>;        
};


/* WSAGetOverlappedResult */
struct tarpc_get_overlapped_result_in {
    struct tarpc_in_arg common;

    tarpc_int           s;              /**< Socket    */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure */
    tarpc_int           wait;           /**< Wait flag */
    tarpc_int           bytes<>;        /**< Transferred bytes location */
    tarpc_int           flags<>;        /**< Flags location */
    tarpc_bool          get_data;       /**< Whether the data should be 
                                         *   returned in out.vector */
};    

struct tarpc_get_overlapped_result_out {
    struct tarpc_out_arg common;

    tarpc_int           retval;
    tarpc_int           bytes<>;    /**< Transferred bytes */
    tarpc_int           flags<>;    /**< Flags             */
    struct tarpc_iovec  vector<>;   /**< Buffer to receive buffers resulted
                                         from overlapped operation */
};    


/* WSAWaitForMultipleEvents */

/** Return codes for rpc_wait_multiple_events */
enum tarpc_wait_code {
    TARPC_WSA_WAIT_FAILED,
    TARPC_WAIT_IO_COMPLETION,
    TARPC_WSA_WAIT_TIMEOUT,
    TARPC_WSA_WAIT_EVENT_0
};

struct tarpc_wait_multiple_events_in {
    struct tarpc_in_arg common; 
    
    tarpc_wsaevent  events<>;   /**< Events array */
    tarpc_int       wait_all;   /**< WaitAll flag */
    tarpc_uint      timeout;    /**< Timeout (in milliseconds) */
    tarpc_int       alertable;  /**< Alertable flag */
};

struct tarpc_wait_multiple_events_out {
    struct tarpc_out_arg common;

    tarpc_wait_code      retval;
};    

/* WSASendTo */
struct tarpc_wsa_send_to_in {
    struct tarpc_in_arg common;
    tarpc_int           s;              /**< TA-local socket */
    struct tarpc_iovec  vector<>;       /**< Buffers */
    tarpc_size_t        count;          /**< Number of buffers */
    tarpc_int           bytes_sent<>;   /**< Location for sent bytes num */
    tarpc_int           flags;          /**< Flags */
    struct tarpc_sa     to;             /**< Target address */
    tarpc_socklen_t     tolen;          /**< Target address lenght */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    tarpc_bool          callback;       /**< If 1, completion callback should be 
                                             specified */
};

struct tarpc_wsa_send_to_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    tarpc_int            bytes_sent<>;
};

/* WSARecvFrom */
struct tarpc_wsa_recv_from_in {
    struct tarpc_in_arg common;
    tarpc_int           s;              /**< TA-local socket */
    struct tarpc_iovec  vector<>;       /**< Buffers */
    tarpc_size_t        count;          /**< Number of buffers */
    tarpc_int           bytes_received<>;   
                                        /**< Location for received bytes num */
    tarpc_int           flags<>;        /**< Flags */
    struct tarpc_sa     from;           /**< Address to be passed as location
                                             for data source address */
    tarpc_int           fromlen<>;      /**< Maximum expected length of the
                                             address */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    tarpc_bool          callback;       /**< If 1, completion callback
                                             should be specified */
};

struct tarpc_wsa_recv_from_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    struct tarpc_iovec   vector<>;       
    tarpc_int            bytes_received<>;   
    tarpc_int            flags<>;
    struct tarpc_sa      from;          /**< Source address returned by the 
                                             function */
    tarpc_int            fromlen<>;     /**< Length of the source address
                                             returned by the function */
};

/* WSASendDisconnect */
struct tarpc_wsa_send_disconnect_in {
    struct tarpc_in_arg common;
    tarpc_int           s;              /**< TA-local socket */
    struct tarpc_iovec  vector<>;       /**< Outgoing disconnect data */
};

typedef struct tarpc_ssize_t_retval_out tarpc_wsa_send_disconnect_out;

/* WSARecvDisconnect */
struct tarpc_wsa_recv_disconnect_in {
    struct tarpc_in_arg common;
    tarpc_int           s;              /**< TA-local socket */
    struct tarpc_iovec  vector<>;       /**< Buffer for incoming
                                             disconnect data */
};

struct tarpc_wsa_recv_disconnect_out {
    struct tarpc_out_arg common;
    tarpc_ssize_t        retval;
    struct tarpc_iovec   vector<>;      /**< Incoming disconnect data */
};

/* WSARecvMsg */
struct tarpc_wsa_recv_msg_in {
    struct tarpc_in_arg common;

    tarpc_int           s;
    struct tarpc_msghdr msg<>;
    tarpc_int           bytes_received<>;   
                                        /**< Location for received bytes num */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    tarpc_bool          callback;       /**< If 1, completion callback
                                             should be specified */
};                

struct tarpc_wsa_recv_msg_out {
    struct tarpc_out_arg common;

    tarpc_ssize_t        retval;
    struct tarpc_msghdr  msg<>;
    tarpc_int            bytes_received<>;
};

/* ftp_open() */
struct tarpc_ftp_open_in {
    struct tarpc_in_arg common;
    
    char        uri<>;         /**< URI to open */
    tarpc_bool  rdonly;        /**< If 1, open to get file */
    tarpc_bool  passive;       /**< If 1, use passive mode */
    int32_t     offset;        /**< File offset */
    tarpc_int   sock<>;        /**< Pointer on a socket */
};

struct tarpc_ftp_open_out {
    struct tarpc_out_arg common;
    
    tarpc_int fd;     /**< TCP socket file descriptor */
    tarpc_int sock;   /**< Value of returning socket */
};    

/* many_send() */
struct tarpc_many_send_in {
    struct tarpc_in_arg common;

    tarpc_int       sock;
    tarpc_size_t    vector<>;
};

struct tarpc_many_send_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */

    uint64_t    bytes;      /**< Number of sent bytes */
};

/* overfill_buffers() */
struct tarpc_overfill_buffers_in {
    struct tarpc_in_arg common;

    tarpc_int       sock;
};

struct tarpc_overfill_buffers_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */

    uint64_t    bytes;      /**< Number of sent bytes */
};

program tarpc
{
    version ver0
    {
changequote([,])
define([cnt], 1)
define([counter], [cnt[]define([cnt],incr(cnt))])
define([RPC_DEF], [tarpc_$1_out _$1(tarpc_$1_in *) = counter;])

        RPC_DEF(rpc_is_op_done)
        RPC_DEF(setlibname)

        RPC_DEF(fork)
        RPC_DEF(pthread_create)
        RPC_DEF(pthread_cancel)
        RPC_DEF(execve)
        RPC_DEF(getpid)
        RPC_DEF(gettimeofday)
        
        RPC_DEF(socket)
        RPC_DEF(duplicate_socket)
        RPC_DEF(dup)
        RPC_DEF(dup2)
        RPC_DEF(close)
        RPC_DEF(shutdown)

        RPC_DEF(read)
        RPC_DEF(write)

        RPC_DEF(readv)
        RPC_DEF(writev)

        RPC_DEF(send)
        RPC_DEF(recv)

        RPC_DEF(sendto)
        RPC_DEF(recvfrom)

        RPC_DEF(sendmsg)
        RPC_DEF(recvmsg)

        RPC_DEF(bind)
        RPC_DEF(connect)
        RPC_DEF(listen)
        RPC_DEF(accept)

        RPC_DEF(fd_set_new)
        RPC_DEF(fd_set_delete)
        RPC_DEF(do_fd_set)
        RPC_DEF(do_fd_zero)
        RPC_DEF(do_fd_clr)
        RPC_DEF(do_fd_isset)

        RPC_DEF(select)
        RPC_DEF(pselect)
        RPC_DEF(poll)

        RPC_DEF(getsockopt)
        RPC_DEF(setsockopt)

        RPC_DEF(ioctl) 
        RPC_DEF(fcntl)

        RPC_DEF(getsockname)
        RPC_DEF(getpeername)

        RPC_DEF(if_nametoindex)
        RPC_DEF(if_indextoname)
        RPC_DEF(if_nameindex)
        RPC_DEF(if_freenameindex)

        RPC_DEF(signal)
        RPC_DEF(sigaction)
        RPC_DEF(kill)

        RPC_DEF(sigset_new)
        RPC_DEF(sigset_delete)
        RPC_DEF(sigprocmask)
        RPC_DEF(sigemptyset)
        RPC_DEF(sigfillset)
        RPC_DEF(sigaddset)
        RPC_DEF(sigdelset)
        RPC_DEF(sigismember)
        RPC_DEF(sigpending)
        RPC_DEF(sigsuspend)
        RPC_DEF(sigreceived) 

        RPC_DEF(gethostbyname)
        RPC_DEF(gethostbyaddr)
        
        RPC_DEF(getaddrinfo)
        RPC_DEF(freeaddrinfo) 
        
        RPC_DEF(pipe)
        RPC_DEF(socketpair)
        RPC_DEF(fopen)
        RPC_DEF(fclose)
        RPC_DEF(popen)
        RPC_DEF(fileno)
        RPC_DEF(getpwnam)
        RPC_DEF(getuid)
        RPC_DEF(geteuid)
        RPC_DEF(setuid)
        RPC_DEF(seteuid)
        RPC_DEF(getenv)
        RPC_DEF(setenv)

        RPC_DEF(simple_sender)
        RPC_DEF(simple_receiver)
        RPC_DEF(recv_verify)
        RPC_DEF(send_traffic)
        RPC_DEF(timely_round_trip)
        RPC_DEF(round_trip_echoer)
        RPC_DEF(flooder)
        RPC_DEF(echoer)
        
        RPC_DEF(aio_read_test)
        RPC_DEF(aio_error_test)
        RPC_DEF(aio_write_test)
        RPC_DEF(aio_suspend_test)
        
        RPC_DEF(sendfile)
        RPC_DEF(socket_to_file)

        RPC_DEF(create_event)
        RPC_DEF(close_event)
        RPC_DEF(create_overlapped)
        RPC_DEF(delete_overlapped)

        RPC_DEF(connect_ex)
        RPC_DEF(wsa_accept)
        RPC_DEF(accept_ex)
        RPC_DEF(get_accept_addr)
        RPC_DEF(disconnect_ex)
        RPC_DEF(reset_event)     
        RPC_DEF(set_event)
        RPC_DEF(event_select)
        RPC_DEF(enum_network_events)
        RPC_DEF(transmit_file)
        RPC_DEF(transmitfile_tabufs)
        RPC_DEF(create_file)
        RPC_DEF(close_handle)
        RPC_DEF(has_overlapped_io_completed)
        RPC_DEF(get_current_process_id)
        RPC_DEF(get_ram_size)
        RPC_DEF(vm_trasher)
        RPC_DEF(write_at_offset)
        RPC_DEF(completion_callback)
        RPC_DEF(wsa_send)
        RPC_DEF(wsa_recv)
        RPC_DEF(wsa_recv_ex)
        RPC_DEF(get_overlapped_result)
        RPC_DEF(wait_multiple_events)
        RPC_DEF(wsa_send_to)
        RPC_DEF(wsa_recv_from)
        RPC_DEF(wsa_send_disconnect)
        RPC_DEF(wsa_recv_disconnect)
        RPC_DEF(wsa_recv_msg)
        RPC_DEF(wsa_address_to_string)
        RPC_DEF(wsa_string_to_address)
        RPC_DEF(wsa_async_get_host_by_addr)
        RPC_DEF(wsa_async_get_host_by_name)
        RPC_DEF(wsa_async_get_proto_by_name)
        RPC_DEF(wsa_async_get_proto_by_number)
        RPC_DEF(wsa_async_get_serv_by_name)
        RPC_DEF(wsa_async_get_serv_by_port)
        RPC_DEF(wsa_cancel_async_request)
        RPC_DEF(malloc)
        RPC_DEF(free)
        RPC_DEF(set_buf)
        RPC_DEF(get_buf)
        RPC_DEF(alloc_wsabuf)
        RPC_DEF(free_wsabuf)
        RPC_DEF(wsa_connect)
        RPC_DEF(wsa_ioctl)
        RPC_DEF(get_wsa_ioctl_overlapped_result)

        RPC_DEF(create_window)
        RPC_DEF(destroy_window)
        RPC_DEF(wsa_async_select)
        RPC_DEF(peek_message)
        
        RPC_DEF(ftp_open)

        RPC_DEF(many_send)
        RPC_DEF(overfill_buffers)

    } = 1;
} = 1;
