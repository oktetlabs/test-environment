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
typedef uint32_t    tarpc_waitpid_opts;
typedef uint32_t    tarpc_wait_status_flag;
typedef uint32_t    tarpc_wait_status_value;
typedef uint32_t    tarpc_uid_t;

typedef uint8_t     tarpc_uchar;
typedef uint16_t    tarpc_usint;

/** RPC size_t analog */
typedef uint32_t    tarpc_size_t;
/** RPC pid_t analog */
typedef int32_t     tarpc_pid_t;
/** RPC ssize_t analog */
typedef int32_t     tarpc_ssize_t;
/** RPC socklen_t analog */
typedef uint32_t    tarpc_socklen_t;
/** Handle of the 'sigset_t' or 0 */
typedef tarpc_ptr    tarpc_sigset_t;
/** Handle of the 'fd_set' or 0 */
typedef tarpc_ptr    tarpc_fd_set;
/** RPC off_t analog */
typedef int64_t     tarpc_off_t;
/** RPC rlim_t analog */
typedef int32_t     tarpc_rlim_t;
/** RPC time_t analogue */
typedef int64_t     tarpc_time_t;
/** RPC suseconds_t analogue */
typedef int64_t     tarpc_suseconds_t;
/** Pointer to 'struct aiocb' */
typedef tarpc_ptr   tarpc_aiocb_t;
/** RPC pthread_t analogue */
typedef tarpc_ptr   tarpc_pthread_t;

/** Handle of the 'WSAEvent' or 0 */
typedef tarpc_ptr   tarpc_wsaevent;
/** Handle of the window */
typedef tarpc_ptr   tarpc_hwnd;
/** WSAOVERLAPPED structure */
typedef tarpc_ptr   tarpc_overlapped;
/** HANDLE */
typedef tarpc_ptr   tarpc_handle;

typedef uint32_t    tarpc_op;

/** Ethtool command, should be u32 on any Linux */
typedef uint32_t tarpc_ethtool_command;

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
    tarpc_pthread_t tid;        /**< Thread identifier (for checking and 
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
    tarpc_int   _errno;         /**< @e errno of the operation from
                                     te_errno.h or rcf_rpc_defs.h */
    tarpc_bool  errno_changed;  /**< Was errno modified by the call? */

    uint32_t    duration;   /**< Duration of the called routine
                                 execution (in microseconds) */
    tarpc_pthread_t tid;    /**< Identifier of the thread which 
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
};


/** IPv4-specific part of generic address */
struct tarpc_sin {
    uint16_t    port;       /**< Port in host byte order */
    uint8_t     addr[4];    /**< Network address */
};

/** IPv6-specific part of generic address */
struct tarpc_sin6 {
    uint16_t    port;       /**< Port in host byte order */
    uint32_t    flowinfo;   /**< Flow information */
    uint8_t     addr[16];   /**< Network address */
    uint32_t    scope_id;   /**< Scope identifier (since RFC 2553) */
    uint32_t    src_id;     /**< Solaris extension */
};

/** AF_LOCAL-specific part of generic address */
struct tarpc_local {
    uint8_t     data[6];    /**< Ethernet address */
};

enum tarpc_socket_addr_family {
    TARPC_AF_INET = 1,
    TARPC_AF_INET6 = 2,
    TARPC_AF_LOCAL = 4,
    TARPC_AF_UNSPEC = 7
};

enum tarpc_flags {
    TARPC_SA_NOT_NULL = 0x1,
    TARPC_SA_RAW      = 0x2,
    TARPC_SA_LEN_AUTO = 0x4
};

/** 'type' is the same as 'family', but defines real representation */
union tarpc_sa_data switch (tarpc_socket_addr_family type) {
    case TARPC_AF_INET:     struct tarpc_sin    in;     /**< IPv4 */
    case TARPC_AF_INET6:    struct tarpc_sin6   in6;    /**< IPv6 */
    case TARPC_AF_LOCAL:    struct tarpc_local  local;  /**< Ethernet
                                                             address */
    default:                void;                       /**< Nothing by
                                                             default */
};

/** Generic address */
struct tarpc_sa {
    tarpc_flags         flags;          /**< Flags */
    tarpc_socklen_t     len;            /**< Address length parameter
                                             value */
#if 0
    uint8_t             sa_len;         /**< 'sa_len' field value */
#endif
    tarpc_int           sa_family;      /**< 'sa_family' field value */
    tarpc_sa_data       data;           /**< Family specific data */
    uint8_t             raw<>;          /**< Sequence of bytes */
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

/* Ethtool data types */
enum tarpc_ethtool_type {
    TARPC_ETHTOOL_CMD = 1,      /**< struct ethtool_cmd */
    TARPC_ETHTOOL_VALUE = 2     /**< struct ethtool_value */
};

struct tarpc_ethtool_cmd {
    uint32_t supported;
    uint32_t advertising;
    uint16_t speed;
    uint8_t  duplex;
    uint8_t  port;
    uint8_t  phy_address;
    uint8_t  transceiver;
    uint8_t  autoneg;
    uint32_t maxtxpkt;
    uint32_t maxrxpkt;
};

struct tarpc_ethtool_value {
    uint32_t data;
};

/** struct ethtool_* */
union tarpc_ethtool_data
    switch (tarpc_ethtool_type type)
{
    case TARPC_ETHTOOL_CMD:     tarpc_ethtool_cmd cmd;
    case TARPC_ETHTOOL_VALUE:   tarpc_ethtool_value value;
    default:                    void;
};

struct tarpc_ethtool {
    tarpc_ethtool_command   command;
    tarpc_ethtool_data      data;
};

/** struct ifreq */
struct tarpc_ifreq {
    char            rpc_ifr_name<>; /**< Interface name */
    struct tarpc_sa rpc_ifr_addr;   /**< Different interface addresses */
    tarpc_int       rpc_ifr_flags;  /**< Interface flags */
    tarpc_int       rpc_ifr_ifindex;/**< Interface index */
    uint32_t        rpc_ifr_mtu;    /**< Interface MTU */
    tarpc_ethtool   rpc_ifr_ethtool; /**< Ethtool command structure */
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

/** struct tarpc_sgio */
struct tarpc_sgio {
    tarpc_int   interface_id;    /**< [i] 'S' for SCSI generic (required) */
    tarpc_int   dxfer_direction; /**< [i] data transfer direction */
    tarpc_uchar cmd_len;         /**< [i] SCSI command length ( <= 16 bytes) */
    tarpc_uchar mx_sb_len;       /**< [i] max length to write to sbp */
    tarpc_usint iovec_count;     /**< [i] 0 implies no scatter gather */
    tarpc_uint  dxfer_len;       /**< [i] byte count of data transfer */
    tarpc_uchar dxferp<>;        /**< [i], [*io] */
    tarpc_uchar cmdp<>;          /**< [i], [*i] points to command to perform */
    tarpc_uchar sbp<>;           /**< [i], [*o] points to sense_buffer memory */
    tarpc_uint  timeout;         /**< [i] MAX_UINT->no timeout (in millisec) */
    tarpc_uint  flags;           /**< [i] 0 -> default, see SG_FLAG... */
    tarpc_int   pack_id;         /**< [i->o] unused internally (normally) */
    tarpc_uchar usr_ptr<>;       /**< [i->o] unused internally */
    tarpc_uchar status;          /**< [o] scsi status */
    tarpc_uchar masked_status;   /**< [o] shifted, masked scsi status */
    tarpc_uchar msg_status;      /**< [o] messaging level data (optional) */
    tarpc_uchar sb_len_wr;       /**< [o] byte count actually written to sbp */
    tarpc_usint host_status;     /**< [o] errors from host adapter */
    tarpc_usint driver_status;   /**< [o] errors from software driver */
    tarpc_int   resid;           /**< [o] dxfer_len - actual_transferred */
    tarpc_uint  duration;        /**< [o] time taken by cmd (unit: millisec) */
    tarpc_uint  info;            /**< [o] auxiliary information */
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

/** struct rlimit */
struct tarpc_rlimit {
    tarpc_rlim_t rlim_cur;
    tarpc_rlim_t rlim_max;
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
};

struct tarpc_socket_out {
    struct tarpc_out_arg common;

    tarpc_int   fd;     /**< TA-local socket */
};


/* WSACleanup */

struct tarpc_wsa_cleanup_in{
    struct tarpc_in_arg common;
};

struct tarpc_wsa_cleanup_out{
    struct tarpc_out_arg common;
    tarpc_int   retval;
};

/* WSAStartup */

struct tarpc_wsa_startup_in{
    struct tarpc_in_arg common;
};

struct tarpc_wsa_startup_out{
    struct tarpc_out_arg common;
    tarpc_int   retval;
};


/* WSASocket() */

struct tarpc_wsa_socket_in {
    struct tarpc_in_arg common;
    
    tarpc_int   domain; /**< TA-independent domain */
    tarpc_int   type;   /**< TA-independent socket type */
    tarpc_int   proto;  /**< TA-independent socket protocol */

    char        info<>; /**< Protocol Info (for winsock2 only) */
    tarpc_int   flags;  /**< flags (for winsock2 only) */
};

struct tarpc_wsa_socket_out {
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

/* DuplicateHandle; used for sockets only with option DUPLICATE_SAME_ACCESS */
struct tarpc_duplicate_handle_in {
    struct tarpc_in_arg common;
    
    tarpc_pid_t src;  /**< Source process */
    tarpc_pid_t tgt;  /**< Target process */
    tarpc_int   fd;   /**< File descriptor to be duplicated */
};

struct tarpc_duplicate_handle_out {
    struct tarpc_out_arg common;
    
    tarpc_bool retval;   /**< Value returned by DuplicateHandle */
    tarpc_int  fd;       /**< New file descriptor */
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


/* ReadFile() / WriteFile() */

struct tarpc_read_file_in {
    struct tarpc_in_arg common;

    tarpc_int        fd;
    uint8_t          buf<>;
    tarpc_size_t     len;
    tarpc_size_t     received<>;
    tarpc_overlapped overlapped;
};

struct tarpc_read_file_out {
    struct tarpc_out_arg    common;

    tarpc_bool      retval;
    uint8_t         buf<>;
    tarpc_size_t    received<>;
};

struct tarpc_read_file_ex_in {
    struct tarpc_in_arg common;

    tarpc_int        fd;
    uint8_t          buf<>;
    tarpc_size_t     len;
    tarpc_overlapped overlapped;
    string           callback<>;
};

struct tarpc_read_file_ex_out {
    struct tarpc_out_arg common;

    tarpc_bool      retval;
};

struct tarpc_write_file_in {
    struct tarpc_in_arg common;

    tarpc_int        fd;
    uint8_t          buf<>;
    tarpc_size_t     len;
    tarpc_size_t     sent<>;
    tarpc_overlapped overlapped;
};

struct tarpc_write_file_out {
    struct tarpc_out_arg    common;

    tarpc_bool      retval;
    tarpc_size_t    sent<>;
};

struct tarpc_write_file_ex_in {
    struct tarpc_in_arg common;

    tarpc_int        fd;
    uint8_t          buf<>;
    tarpc_size_t     len;
    tarpc_overlapped overlapped;
    string           callback<>;
};

struct tarpc_write_file_ex_out {
    struct tarpc_out_arg common;

    tarpc_bool      retval;
};

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

/* readbuf() / writebuf() */

struct tarpc_readbuf_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    tarpc_ptr       buf;
    tarpc_size_t    off;
    tarpc_size_t    len;
};

typedef struct tarpc_ssize_t_retval_out tarpc_readbuf_out;

typedef struct tarpc_readbuf_in tarpc_writebuf_in;

typedef struct tarpc_ssize_t_retval_out tarpc_writebuf_out;


/* lseek() */

struct tarpc_lseek_in {
    struct tarpc_in_arg common;
    
    tarpc_int           fd;
    tarpc_off_t         pos;
    tarpc_int           mode; 
};

struct tarpc_lseek_out {
    struct tarpc_out_arg common;
    
    tarpc_off_t          retval;
};

/* fsync() */

struct tarpc_fsync_in {
    struct tarpc_in_arg common;
    
    tarpc_int           fd;
};

typedef struct tarpc_int_retval_out tarpc_fsync_out;

/* send() */

struct tarpc_send_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    uint8_t         buf<>;
    tarpc_size_t    len;
    tarpc_int       flags;
};

typedef struct tarpc_ssize_t_retval_out tarpc_send_out;

/* sendbuf() */

struct tarpc_sendbuf_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    tarpc_ptr       buf;
    tarpc_size_t    off;
    tarpc_size_t    len;
    tarpc_int       flags;
};

typedef struct tarpc_ssize_t_retval_out tarpc_sendbuf_out;

/* send_msg_more() */

struct tarpc_send_msg_more_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    tarpc_ptr       buf;
    tarpc_size_t    first_len;
    tarpc_size_t    second_len;
};

typedef struct tarpc_ssize_t_retval_out tarpc_send_msg_more_out;


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

/* recvbuf() */

struct tarpc_recvbuf_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    tarpc_ptr       buf;
    tarpc_size_t    off;
    tarpc_size_t    len;
    tarpc_int       flags;
};

typedef struct tarpc_ssize_t_retval_out tarpc_recvbuf_out;


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
    string          handler<>;   /**< sa_handler name */
    string          restorer<>;  /**< sa_restorer name */              
    tarpc_sigset_t  mask;        /**< Handle (pointer in server
                                      context) of the allocated set */
    tarpc_int       flags;       /**< Flags to be passed to sigaction */
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

struct tarpc_cmsg_data_parse_ip_pktinfo_in {
    struct tarpc_in_arg common;

    uint8_t             data<>;
};

struct tarpc_cmsg_data_parse_ip_pktinfo_out {
    struct tarpc_out_arg    common;
    tarpc_int               retval;
    uint32_t                ipi_addr;
    tarpc_int               ipi_ifindex;
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

/* telephony_open_channel() */
struct tarpc_telephony_open_channel_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               port;       /**< TA-local telephony card port */   
};                                               

typedef struct tarpc_int_retval_out tarpc_telephony_open_channel_out;

/* telephony_close_channel() */
struct tarpc_telephony_close_channel_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               chan;       /**< TA-local telephony channel */   
};                                               

typedef struct tarpc_int_retval_out tarpc_telephony_close_channel_out;

/* telephony_pickup() */
struct tarpc_telephony_pickup_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               chan;       /**< TA-local telephony channel*/   
};                                               

typedef struct tarpc_int_retval_out tarpc_telephony_pickup_out;

/* telephony_hangup() */
struct tarpc_telephony_hangup_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               chan;       /**< TA-local telephony channel */
};                                               

typedef struct tarpc_int_retval_out tarpc_telephony_hangup_out;

/* telephony_check_dial_tone() */
struct tarpc_telephony_check_dial_tone_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               chan;       /**< TA-local telephony channel */   
};                                               

typedef struct tarpc_int_retval_out tarpc_telephony_check_dial_tone_out;

/* telephony_dial_number() */
struct tarpc_telephony_dial_number_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               chan;       /**< TA-local telephony channel */
    string                  number<>;   /**< Telephone number to dial */
};

typedef struct tarpc_int_retval_out tarpc_telephony_dial_number_out;

/* telephony_call_wait() */
struct tarpc_telephony_call_wait_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               chan;       /**< TA-local telephony channel */   
};                                               

typedef struct tarpc_int_retval_out tarpc_telephony_call_wait_out;

/* ConnectEx() */
struct tarpc_connect_ex_in {
    struct tarpc_in_arg     common;
    
    tarpc_int               fd;       /**< TA-local socket */
    struct tarpc_sa         addr;     /**< Remote address */
    tarpc_ptr               send_buf; /**< RPC pointer for data to be sent */
    tarpc_size_t            buflen;   /**< Size of data passed to connectEx() */
    tarpc_size_t            len_sent<>; /**< Returned by the function
                                             size of sent data from buf */
    tarpc_overlapped        overlapped; /**< WSAOVERLAPPED structure */
    
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
    tarpc_int            timeout;    /**< Timeout to sleep, ms */
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
    tarpc_ptr               out_buf;      /**< Pointer to a buffer that receives
                                               the first block of data, the 
                                               local address, and the remote
                                               address.*/
    tarpc_size_t            buflen;       /**< Length of the buffer
                                               passed to the AcceptEx() */
                                               
    tarpc_size_t            laddr_len;    /**< Number of bytes reserved for 
                                               the local address information */
    tarpc_size_t            raddr_len;    /**< Number of bytes reserved for 
                                               the remote address information */
                                                                                       
    tarpc_size_t            count<>;      /**< Location for
                                               count of received bytes */
    tarpc_overlapped        overlapped;   /**< WSAOVERLAPPED structure */

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
    tarpc_ptr       buf;         /**< Buffer with addresses */
    tarpc_size_t    buflen;      /**< Length of the buffer
                                      passed to the AcceptEx() */
    tarpc_size_t    laddr_len;   /**< Number of bytes reserved for 
                                      the local address information */
    tarpc_size_t    raddr_len;   /**< Number of bytes reserved for 
                                      the remote address information */

    tarpc_bool      l_sa_null;   /**< LocalSockaddr is NULL */
    tarpc_bool      r_sa_null;   /**< RemoteSockaddr is NULL */
    struct tarpc_sa laddr;       /**< Local address */
    struct tarpc_sa raddr;       /**< Remote address */
    tarpc_size_t    l_sa_len<>;  /**< LocalSockaddrLength (transparent) */
    tarpc_size_t    r_sa_len<>;  /**< RemoteSockaddrLength (transparent) */
};

struct tarpc_get_accept_addr_out {
    struct tarpc_out_arg    common;

    struct tarpc_sa laddr;       /**< Local address */
    tarpc_size_t    l_sa_len<>;  /**< LocalSockaddrLength (transparent) */
    struct tarpc_sa raddr;       /**< Remote address */
    tarpc_size_t    r_sa_len<>;  /**< RemoteSockaddrLength (transparent) */
};

/* TransmitFile() */

struct tarpc_transmit_file_in {
    struct tarpc_in_arg common;
                                                                               
    tarpc_int               fd;           /**< TA-local socket */
    tarpc_int               file;         /**< Handle to the open file to be
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
    tarpc_int           file;         /**< Handle to the open file to be
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


/* TransmitPackets() */
enum tarpc_transmit_packet_type {
    TARPC_TP_MEM = 1,                /**< Transmit data from buffer */
    TARPC_TP_FILE = 2                /**< Transmit data from file */
};

struct tarpc_file_data {
    tarpc_off_t offset; /**< Data offset in the file */
    tarpc_int   file;   /**< Handle to the open file to be transmitted */
};

union tarpc_transmit_packet_source
    switch (tarpc_transmit_packet_type type)
{
    case TARPC_TP_MEM:  char            buf<>;
    case TARPC_TP_FILE: tarpc_file_data file_data;
};
    
struct tarpc_transmit_packets_element {
    tarpc_int           length;      /**< Number of bytes to transmit */
    tarpc_transmit_packet_source 
                        packet_src;  /**< Where to get transmission data */
};

struct tarpc_transmit_packets_in {
    struct tarpc_in_arg             common;
    
    tarpc_int                       s;              
                                    /**< TA-local socket */
    tarpc_transmit_packets_element  packet_array<>;
                                    /**< Data to transmit */
    tarpc_int                       send_size;
                                    /**< Size of data sent per operation */
    tarpc_overlapped                overlapped;
                                    /**< Overlapped structure */
    tarpc_size_t                    flags;
                                    /** Flags parameter */
};

struct tarpc_transmit_packets_out {
    struct tarpc_out_arg            common;

    tarpc_bool                      retval;
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
    tarpc_int            template_file;
};

struct tarpc_create_file_out {
    struct tarpc_out_arg  common;
    tarpc_int             handle;
};

/* Windows closesocket() */
struct tarpc_closesocket_in {
    struct tarpc_in_arg  common;
    tarpc_int            s;       /**< socket to close */
};

struct tarpc_closesocket_out {
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
    tarpc_bool            retval;
};    


/* CancelIo() */

struct tarpc_cancel_io_in {
    struct tarpc_in_arg  common;
    tarpc_int            fd;
};    

struct tarpc_cancel_io_out {
    struct tarpc_out_arg  common;
    tarpc_bool            retval;
};    

/* CreateIoCompletionPort() */

struct tarpc_create_io_completion_port_in {
    struct tarpc_in_arg  common;
    tarpc_int            file_handle;
    tarpc_int            existing_completion_port;
    uint64_t             completion_key;
    tarpc_uint           number_of_concurrent_threads;
};    

struct tarpc_create_io_completion_port_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
};    

/* GetQueuedCompletionStatus() */

struct tarpc_get_queued_completion_status_in {
    struct tarpc_in_arg  common;
    tarpc_int            completion_port;
    tarpc_uint           milliseconds;
};    

struct tarpc_get_queued_completion_status_out {
    struct tarpc_out_arg  common;
    tarpc_size_t          number_of_bytes;
    uint64_t              completion_key;
    tarpc_overlapped      overlapped;
    tarpc_bool            retval;
};    

/* PostQueuedCompletionStatus() */

struct tarpc_post_queued_completion_status_in {
    struct tarpc_in_arg   common;
    tarpc_int             completion_port;
    tarpc_uint            number_of_bytes;
    uint64_t              completion_key;
    tarpc_overlapped      overlapped;
};    

struct tarpc_post_queued_completion_status_out {
    struct tarpc_out_arg  common;
    tarpc_bool            retval;
};    

/* GetCurrentProcessId() */

struct tarpc_get_current_process_id_in {
    struct tarpc_in_arg  common;
};    

struct tarpc_get_current_process_id_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
};

/* rpc_get_sys_info() */
struct tarpc_get_sys_info_in {
    struct tarpc_in_arg  common;
};

struct tarpc_get_sys_info_out {
    struct tarpc_out_arg  common;
    uint64_t              ram_size;
    tarpc_uint            page_size;
    tarpc_uint            number_of_processors;
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

/* WSACreateEvent() */
struct tarpc_create_event_with_bit_in {
    struct tarpc_in_arg common;
};

struct tarpc_create_event_with_bit_out {
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

/* memalign */
struct tarpc_memalign_in {
    struct tarpc_in_arg  common;
    tarpc_size_t         alignment; /**< Alignment of a block */    
    tarpc_size_t         size;      /**< Bytes to allocate */
};

struct tarpc_memalign_out {
    struct tarpc_out_arg common;
    tarpc_ptr            retval; /**< A pointer in the TA address space */
};


/* set_buf */
struct tarpc_set_buf_in {
    struct tarpc_in_arg  common;
    char                 src_buf<>;
    tarpc_ptr            dst_buf;
    tarpc_size_t         dst_off;
};

struct tarpc_set_buf_out {
    struct tarpc_out_arg common;
};

/* get_buf */
struct tarpc_get_buf_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            src_buf;  /**< A pointer in the TA address space */
    tarpc_size_t         src_off;  /**< Offset from the buffer */
    tarpc_size_t         len;      /**< How much to get */
};

struct tarpc_get_buf_out {
    struct tarpc_out_arg common;
    char                 dst_buf<>;
};

/* set_buf_pattern */
struct tarpc_set_buf_pattern_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            dst_buf;
    tarpc_size_t         dst_off;
    tarpc_int            pattern;
    tarpc_size_t         len;      
};

struct tarpc_set_buf_pattern_out {
    struct tarpc_out_arg common;
};

/* memcmp */
struct tarpc_memcmp_in {
    struct tarpc_in_arg  common;

    tarpc_ptr       s1_base;
    tarpc_size_t    s1_off;
    tarpc_ptr       s2_base;
    tarpc_size_t    s2_off;
    tarpc_size_t    n;
};

typedef struct tarpc_int_retval_out tarpc_memcmp_out;

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
    uint8_t  data4[8];  
};

/* Windows tcp_keepalive structure */
struct tarpc_tcp_keepalive {
    uint32_t onoff;
    uint32_t keepalivetime;
    uint32_t keepaliveinterval;
};

/* WSAIoctl() */
enum wsa_ioctl_type {
    WSA_IOCTL_VOID = 0,      /* no data */
    WSA_IOCTL_INT,
    WSA_IOCTL_SA,            /* socket address */
    WSA_IOCTL_SAA,           /* socket addresses array */
    WSA_IOCTL_GUID,
    WSA_IOCTL_TCP_KEEPALIVE,
    WSA_IOCTL_QOS,
    WSA_IOCTL_PTR
};

union wsa_ioctl_request switch (wsa_ioctl_type type) {
    case WSA_IOCTL_VOID:           tarpc_int              req_void;
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
    wsa_ioctl_request   inbuf<>;
    tarpc_size_t        inbuf_len;
    wsa_ioctl_request   outbuf<>;
    tarpc_size_t        outbuf_len;
    tarpc_overlapped    overlapped;
    string              callback<>;
    tarpc_size_t        bytes_returned<>;
};

struct tarpc_wsa_ioctl_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    wsa_ioctl_request    outbuf<>;
    tarpc_uint           bytes_returned<>;
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
    tarpc_uint          cookie1;
    tarpc_uint          cookie2;
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

/* Windows WSANETWORKEVENTS structure */

struct tarpc_network_events {
    uint32_t network_events;
    tarpc_int error_code[10];
};
        
/* WSAEnumNetworkEvents() */

struct tarpc_enum_network_events_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;           /**< TA-local socket */ 
    tarpc_wsaevent  hevent;       /**< Event object to be reset */   
    struct tarpc_network_events events<>;    /**< Structure that specifies
                                                  network events occurred */      
};

struct tarpc_enum_network_events_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
    struct tarpc_network_events events<>;    /**< Structure that specifies
                                                  network events occurred */      
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
    tarpc_ssize_t        bytes;         /**< Number of bytes transferred */
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

/* WSAJoinLeaf */
struct tarpc_wsa_join_leaf_in {
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
    tarpc_int              flags;  

};

struct tarpc_wsa_join_leaf_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;

    struct tarpc_sa      addr;   /**< Location for peer name */
    tarpc_socklen_t      len<>;  /**< Length of the peer name location */

};


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


/* epoll_create() */

struct tarpc_epoll_create_in {
    struct tarpc_in_arg common;

    tarpc_int           size;
};

typedef struct tarpc_int_retval_out tarpc_epoll_create_out;

/* epoll_ctl() */

/* For epoll_data union */
enum tarpc_epoll_data_type {
    TARPC_ED_PTR = 1,
    TARPC_ED_INT = 2,
    TARPC_ED_U32 = 3,
    TARPC_ED_U64 = 4
};

union tarpc_epoll_data
    switch (tarpc_epoll_data_type type)
{
    case TARPC_ED_PTR: tarpc_ptr    ptr;
    case TARPC_ED_INT: tarpc_int    fd;
    case TARPC_ED_U32: uint32_t     u32;
    case TARPC_ED_U64: uint64_t     u64;
};

struct tarpc_epoll_event {
    uint32_t             events;
    tarpc_epoll_data     data;
};

struct tarpc_epoll_ctl_in {
    struct tarpc_in_arg common;

    tarpc_int                epfd;
    tarpc_int                op;
    tarpc_int                fd;
    struct tarpc_epoll_event event<>;
};

typedef struct tarpc_int_retval_out tarpc_epoll_ctl_out;

/* epoll_wait() */

struct tarpc_epoll_wait_in {
    struct tarpc_in_arg common;

    tarpc_int                epfd;
    struct tarpc_epoll_event events<>;
    tarpc_int                maxevents;
    tarpc_int                timeout;
};

struct tarpc_epoll_wait_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_epoll_event events<>;
};

/*
 * Socket options
 */
 
enum option_type {
    OPT_INT         = 1,
    OPT_LINGER      = 2,
    OPT_TIMEVAL     = 3,
    OPT_MREQN       = 4,
    OPT_MREQ        = 5,
    OPT_MREQ6       = 6,
    OPT_IPADDR      = 7,
    OPT_IPADDR6     = 8,
    OPT_TCP_INFO    = 9,
    OPT_HANDLE      = 10
};

struct tarpc_linger {
    tarpc_int l_onoff; 
    tarpc_int l_linger;
};

struct option_value_mreqn {
    uint32_t    imr_multiaddr;  /**< IP multicast group address */
    uint32_t    imr_address;    /**< IP address of local interface */
    tarpc_int   imr_ifindex;    /**< Interface index */
};

struct tarpc_mreqn {
    enum option_type    type;
    uint32_t            multiaddr;
    uint32_t            address;
    tarpc_int           ifindex;
};

struct option_value_mreq {
    uint32_t    imr_multiaddr;  /**< IP multicast group address */
    uint32_t    imr_address;    /**< IP address of local interface */
};

struct option_value_mreq6 {
     uint32_t   ipv6mr_multiaddr<>;
     tarpc_int  ipv6mr_ifindex;
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
    case OPT_INT:       tarpc_int opt_int;
    case OPT_LINGER:    struct tarpc_linger opt_linger;
    case OPT_TIMEVAL:   struct tarpc_timeval opt_timeval;
    case OPT_MREQN:     struct option_value_mreqn opt_mreqn;
    case OPT_MREQ:      struct option_value_mreq opt_mreq;
    case OPT_MREQ6:     struct option_value_mreq6 opt_mreq6;
    case OPT_IPADDR:    uint32_t opt_ipaddr;
    case OPT_IPADDR6:   uint32_t opt_ipaddr6[4];
    case OPT_TCP_INFO:  struct option_value_tcp_info opt_tcp_info;
    case OPT_HANDLE:    tarpc_int opt_handle;
};

/* setsockopt() */

struct tarpc_setsockopt_in {
    struct tarpc_in_arg  common;

    tarpc_int       s; 
    tarpc_int       level;
    tarpc_int       optname;
    option_value    optval<>;
    uint8_t         raw_optval<>;
    tarpc_socklen_t raw_optlen;
};

typedef struct tarpc_int_retval_out tarpc_setsockopt_out;

/* getsockopt() */

struct tarpc_getsockopt_in {
    struct tarpc_in_arg  common;

    tarpc_int       s;
    tarpc_int       level; 
    tarpc_int       optname;
    option_value    optval<>;
    uint8_t         raw_optval<>;
    tarpc_socklen_t raw_optlen<>;
};

struct tarpc_getsockopt_out {
    struct tarpc_out_arg  common;

    tarpc_int       retval;
    option_value    optval<>;
    uint8_t         raw_optval<>;
    tarpc_socklen_t raw_optlen<>;
};



/* ioctl() */

enum ioctl_type {
    IOCTL_UNKNOWN,
    IOCTL_INT,
    IOCTL_TIMEVAL,
    IOCTL_IFREQ,
    IOCTL_IFCONF,
    IOCTL_ARPREQ,
    IOCTL_SGIO
    
};

/* Access type of ioctl requests */
enum ioctl_access {
    IOCTL_RD,   /* read access (may include write) */
    IOCTL_WR    /* write-only access (no read) */
};    

union ioctl_request switch (ioctl_type type) {
    case IOCTL_INT:     tarpc_int     req_int;
    case IOCTL_TIMEVAL: tarpc_timeval req_timeval;
    case IOCTL_IFREQ:   tarpc_ifreq   req_ifreq; 
    case IOCTL_IFCONF:  tarpc_ifconf  req_ifconf; 
    case IOCTL_ARPREQ:  tarpc_arpreq  req_arpreq;
    case IOCTL_SGIO:    tarpc_sgio    req_sgio;
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

    tarpc_signum    signum;     /**< Signal */
    string          handler<>;  /**< Name of the signal handler function */
};

struct tarpc_signal_out {
    struct tarpc_out_arg    common;

    string handler<>;  /**< Name of the old signal handler function */
};

/* bsd_signal() */

struct tarpc_bsd_signal_in {
    struct tarpc_in_arg common;

    tarpc_signum    signum;     /**< Signal */
    string          handler<>;  /**< Name of the signal handler function */
};

struct tarpc_bsd_signal_out {
    struct tarpc_out_arg    common;

    string handler<>;  /**< Name of the old signal handler function */
};

/* sysv_signal() */

struct tarpc_sysv_signal_in {
    struct tarpc_in_arg common;

    tarpc_signum    signum;     /**< Signal */
    string          handler<>;  /**< Name of the signal handler function */
};

struct tarpc_sysv_signal_out {
    struct tarpc_out_arg    common;

    string handler<>;  /**< Name of the old signal handler function */
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

/* kill() */

struct tarpc_kill_in {
    struct tarpc_in_arg common;

    tarpc_pid_t     pid;
    tarpc_signum    signum;
};

typedef struct tarpc_int_retval_out tarpc_kill_out;


/* waitpid() */

struct tarpc_waitpid_in {
    struct tarpc_in_arg common;

    tarpc_pid_t         pid;
    tarpc_waitpid_opts  options;
};

struct tarpc_waitpid_out {
    struct tarpc_out_arg    common;

    tarpc_pid_t              pid;
    tarpc_wait_status_flag   status_flag;
    tarpc_wait_status_value  status_value;
};

/* ta_kill_death() */

struct tarpc_ta_kill_death_in {
    struct tarpc_in_arg common;

    tarpc_pid_t     pid;
};

typedef struct tarpc_int_retval_out tarpc_ta_kill_death_out;


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


/* open() */
struct tarpc_open_in {
    struct tarpc_in_arg common;
    
    char        path<>;
    tarpc_int   flags;
    tarpc_int   mode;
};

struct tarpc_open_out {
    struct tarpc_out_arg common;
    
    tarpc_int   fd;
};

/* open64() */
struct tarpc_open64_in {
    struct tarpc_in_arg common;
    
    char        path<>;
    tarpc_int   flags;
    tarpc_int   mode;
};

struct tarpc_open64_out {
    struct tarpc_out_arg common;
    
    tarpc_int   fd;
};

/* fopen() */
struct tarpc_fopen_in {
    struct tarpc_in_arg common;
    
    string path<>;
    string mode<>;
};

struct tarpc_fopen_out {
    struct tarpc_out_arg common;
    
    tarpc_ptr mem_ptr;
};

/* fdopen() */
struct tarpc_fdopen_in {
    struct tarpc_in_arg common;
    
    tarpc_int fd;
    string    mode<>;
};

struct tarpc_fdopen_out {
    struct tarpc_out_arg common;
    
    tarpc_ptr mem_ptr;
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
    struct tarpc_in_arg  common;
    string cmd<>;
    string mode<>;
};
 
struct tarpc_popen_out {
    struct tarpc_out_arg  common;
    tarpc_ptr   mem_ptr;
};

/* pclose() */
struct tarpc_pclose_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            mem_ptr;
};

struct tarpc_pclose_out {
    struct tarpc_out_arg  common;
    tarpc_int             retval;
};

/* te_shell_cmd() */
struct tarpc_te_shell_cmd_in {
    struct tarpc_in_arg common;
    
    char        cmd<>;
    tarpc_uid_t uid;
    tarpc_bool  in_fd;
    tarpc_bool  out_fd;
    tarpc_bool  err_fd;
};

struct tarpc_te_shell_cmd_out {
    struct tarpc_out_arg common;
    
    tarpc_pid_t pid;
    tarpc_int   in_fd;
    tarpc_int   out_fd;
    tarpc_int   err_fd;
};

/* system() */

struct tarpc_system_in {
    struct tarpc_in_arg common;

    char cmd<>;
};

struct tarpc_system_out {
    struct tarpc_out_arg    common;

    tarpc_wait_status_flag   status_flag;
    tarpc_wait_status_value  status_value;
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

/* unsetenv() */
struct tarpc_unsetenv_in {
    struct tarpc_in_arg common;
    
    string     name<>;
};


struct tarpc_unsetenv_out {
    struct tarpc_out_arg common;
    
    tarpc_int retval;
};

/* uname() */

struct tarpc_utsname {
    char sysname<>;
    char nodename<>;
    char release<>;
    char osversion<>;
    char machine<>;
};

struct tarpc_uname_in {
    struct tarpc_in_arg common;
};

struct tarpc_uname_out {
    struct tarpc_out_arg common;
    
    tarpc_int     retval;
    tarpc_utsname buf;
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
    
    tarpc_uid_t uid;
};

/* geteuid */
typedef struct tarpc_getuid_in tarpc_geteuid_in;
typedef struct tarpc_getuid_out tarpc_geteuid_out;

/* setuid() */
struct tarpc_setuid_in {
    struct tarpc_in_arg common;
    
    tarpc_uid_t uid;
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

/* access */
struct tarpc_access_in {
    struct tarpc_in_arg common;
    char      path<>;
    tarpc_int mode;
};

typedef struct tarpc_int_retval_out tarpc_access_out;

/* sigval_t, union sigval */
union tarpc_sigval switch (tarpc_bool pointer) {
    case 1: tarpc_ptr sival_ptr;
    case 0: tarpc_int sival_int;
};

/* sigevent */
struct tarpc_sigevent {
    tarpc_sigval value;         /* Value to be passed to handler  */
    tarpc_signum signo;         /* Signal number                  */
    tarpc_int    notify;        /* Notification type: RPC_SIGEV_* */
    string       function<>;    /* Callback function name         */
    /* pthread_attr_t is not supported at the moment */
};

/* Allocate aio control block filled by garbage */
struct tarpc_create_aiocb_in {
    struct tarpc_in_arg common;
};

struct tarpc_create_aiocb_out {
    struct tarpc_out_arg common;
    
    tarpc_aiocb_t cb;
};

/* Change user-accessible data in the aio control block */
struct tarpc_fill_aiocb_in {
    struct tarpc_in_arg common;
    
    tarpc_aiocb_t  cb;

    tarpc_int      fildes;      /* File descriptor   */
    tarpc_int      lio_opcode;  /* Opcode: RPC_LIO_* */
    tarpc_int      reqprio;     /* Priority          */
    tarpc_ptr      buf;         /* Allocated buffer  */
    tarpc_size_t   nbytes;      /* Buffer length     */
    tarpc_sigevent sigevent;    /* Notification data */
};

typedef struct tarpc_void_out tarpc_fill_aiocb_out;

/* Free aio control block */
struct tarpc_delete_aiocb_in {
    struct tarpc_in_arg common;
    
    tarpc_aiocb_t cb;
};

typedef struct tarpc_void_out tarpc_delete_aiocb_out;


/* aio_read */
struct tarpc_aio_read_in {
    struct tarpc_in_arg common;
    
    tarpc_aiocb_t cb;
};

typedef struct tarpc_int_retval_out tarpc_aio_read_out;

/* aio_write */
struct tarpc_aio_write_in {
    struct tarpc_in_arg common;
    
    tarpc_aiocb_t cb;
};

typedef struct tarpc_int_retval_out tarpc_aio_write_out;

/* aio_return */
struct tarpc_aio_return_in {
    struct tarpc_in_arg common;
    
    tarpc_aiocb_t cb;
};

typedef struct tarpc_ssize_t_retval_out tarpc_aio_return_out;

/* aio_error */
struct tarpc_aio_error_in {
    struct tarpc_in_arg common;
    
    tarpc_aiocb_t cb;
};

typedef struct tarpc_int_retval_out tarpc_aio_error_out;

/* aio_cancel */
struct tarpc_aio_cancel_in {
    struct tarpc_in_arg common;
    
    tarpc_int     fd;
    tarpc_aiocb_t cb;
};

typedef struct tarpc_int_retval_out tarpc_aio_cancel_out;

/* aio_fsync */
struct tarpc_aio_fsync_in {
    struct tarpc_in_arg common;
    
    int           op;   /* Operation: RPC_O_* */
    tarpc_aiocb_t cb;
};

typedef struct tarpc_ssize_t_retval_out tarpc_aio_fsync_out;

/* aio_suspend */
struct tarpc_aio_suspend_in {
    struct tarpc_in_arg common;
    
    tarpc_aiocb_t          cb<>;
    tarpc_int              n;
    struct tarpc_timespec  timeout<>;
};

typedef struct tarpc_int_retval_out tarpc_aio_suspend_out;


/* lio_listio */
struct tarpc_lio_listio_in {
    struct tarpc_in_arg common;
    
    tarpc_int      mode;   /* Mode: RPC_LIO_*                      */
    tarpc_aiocb_t  cb<>;   /* Control blocks array                 */
    tarpc_int      nent;   /* 'netnt' to be passed to the function */
    tarpc_sigevent sig<>;  /* Notification type                    */
};

typedef struct tarpc_int_retval_out tarpc_lio_listio_out;


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

struct tarpc_wait_readable_in {
    struct tarpc_in_arg common;
    
    tarpc_int   s;               /**< Socket to be used */
    uint32_t    timeout;         /**< Receive timeout (in milliseconds) */
};

typedef struct tarpc_int_retval_out tarpc_wait_readable_out;


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


/*
 * IOMUX functions
 */
enum iomux_func {
    FUNC_SELECT = 1,
    FUNC_PSELECT = 2,
    FUNC_POLL = 3,
    FUNC_EPOLL = 4
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

struct tarpc_create_process_in {
    struct tarpc_in_arg common;
    
    tarpc_bool inherit;         /**< Inherit file handles */
    tarpc_bool net_init;        /**< Initialize network */
    char       name<>;          /**< RPC server name */
};

struct tarpc_create_process_out {
    struct tarpc_out_arg common;
    
    int32_t     pid;
};

struct tarpc_thread_create_in {
    struct tarpc_in_arg common;
    
    char name<>;
};

struct tarpc_thread_create_out {
    struct tarpc_out_arg common;
    
    tarpc_ptr   tid;
    tarpc_int   retval;
};

struct tarpc_thread_cancel_in {
    struct tarpc_in_arg common;
    
    tarpc_ptr           tid;
};

struct tarpc_thread_cancel_out {
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
    tarpc_bool          force64;
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
    tarpc_ssize_t       bytes_sent<>;   /**< Location for sent bytes num */
    tarpc_int           flags;          /**< Flags */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    string              callback<>;     /**< Callback name */
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
    tarpc_ssize_t       bytes_received<>;   
                                        /**< Location for received bytes num */
    tarpc_int           flags<>;        /**< Flags */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    string              callback<>;     /**< Callback name */
};

struct tarpc_wsa_recv_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
    struct tarpc_iovec   vector<>;       
    tarpc_int            bytes_received<>;   
    tarpc_int            flags<>;        
};


/* WSAGetOverlappedResult */
struct tarpc_wsa_get_overlapped_result_in {
    struct tarpc_in_arg common;

    tarpc_int           s;              /**< Socket    */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure */
    tarpc_int           wait;           /**< Wait flag */
    tarpc_size_t        bytes<>;        /**< Transferred bytes location */
    tarpc_int           flags<>;        /**< Flags location */
    tarpc_bool          get_data;       /**< Whether the data should be 
                                         *   returned in out.vector */
};    

struct tarpc_wsa_get_overlapped_result_out {
    struct tarpc_out_arg common;

    tarpc_int           retval;
    tarpc_int           bytes<>;    /**< Transferred bytes */
    tarpc_int           flags<>;    /**< Flags             */
    struct tarpc_iovec  vector<>;   /**< Buffer to receive buffers resulted
                                         from overlapped operation */
};    


/* WSAWaitForMultipleEvents */

/** Return codes for rpc_wait_for_multiple_events */
enum tarpc_wait_code {
    TARPC_WSA_WAIT_FAILED,
    TARPC_WAIT_IO_COMPLETION,
    TARPC_WSA_WAIT_TIMEOUT,
    TARPC_WSA_WAIT_EVENT_0
};

struct tarpc_wait_for_multiple_events_in {
    struct tarpc_in_arg common; 
    
    tarpc_wsaevent  events<>;   /**< Events array */
    tarpc_int       wait_all;   /**< WaitAll flag */
    tarpc_uint      timeout;    /**< Timeout (in milliseconds) */
    tarpc_int       alertable;  /**< Alertable flag */
};

struct tarpc_wait_for_multiple_events_out {
    struct tarpc_out_arg common;

    tarpc_wait_code      retval;
};    

/* WSASendTo */
struct tarpc_wsa_send_to_in {
    struct tarpc_in_arg common;
    tarpc_int           s;              /**< TA-local socket */
    struct tarpc_iovec  vector<>;       /**< Buffers */
    tarpc_size_t        count;          /**< Number of buffers */
    tarpc_ssize_t       bytes_sent<>;   /**< Location for sent bytes num */
    tarpc_int           flags;          /**< Flags */
    struct tarpc_sa     to;             /**< Target address */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    string              callback<>;     /**< Callback name */
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
    tarpc_ssize_t       bytes_received<>;   
                                        /**< Location for received bytes num */
    tarpc_int           flags<>;        /**< Flags */
    struct tarpc_sa     from;           /**< Address to be passed as location
                                             for data source address */
    tarpc_int           fromlen<>;      /**< Maximum expected length of the
                                             address */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    string              callback<>;     /**< Callback name */
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
    tarpc_ssize_t       bytes_received<>;   
                                        /**< Location for received bytes num */
    tarpc_overlapped    overlapped;     /**< WSAOVERLAPPED structure pointer */
    string              callback<>;     /**< Callback name */
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

/* ftp_close() */
struct tarpc_ftp_close_in {
    struct tarpc_in_arg common;

    tarpc_int   sock;   /**< Socket to close */
};

struct tarpc_ftp_close_out {
    struct tarpc_out_arg common;

    tarpc_int   ret;    /**< Return value */
};


/* overfill_buffers() */
struct tarpc_overfill_buffers_in {
    struct tarpc_in_arg common;

    tarpc_int       sock;
    tarpc_bool      is_nonblocking;
    iomux_func      iomux;
};

struct tarpc_overfill_buffers_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */

    uint64_t    bytes;      /**< Number of sent bytes */
};


/* setrlimit() */

struct tarpc_setrlimit_in {
    struct tarpc_in_arg  common;

    tarpc_int            resource; /**< resource type */
    struct tarpc_rlimit  rlim<>; /**< struct rlimit */
};

typedef struct tarpc_int_retval_out tarpc_setrlimit_out;

/* getrlimit() */

struct tarpc_getrlimit_in {
    struct tarpc_in_arg  common;

    tarpc_int            resource; /**< resource type */
    struct tarpc_rlimit  rlim<>; /**< struct rlimit */
};

struct tarpc_getrlimit_out {
    struct tarpc_out_arg  common;

    tarpc_int             retval;

    struct tarpc_rlimit   rlim<>; /**< struct rlimit */
};


/* get_sizeof() */
struct tarpc_get_sizeof_in {
    struct tarpc_in_arg common;

    string typename<>;
};

struct tarpc_get_sizeof_out {
    struct tarpc_out_arg common;

    tarpc_ssize_t size;
};

/* protocol_info_cmp() */

struct tarpc_protocol_info_cmp_in {
    struct tarpc_in_arg common;

    uint8_t buf1<>;
    uint8_t buf2<>;
    tarpc_bool is_wide1;
    tarpc_bool is_wide2;
};

struct tarpc_protocol_info_cmp_out {
    struct tarpc_out_arg common;
    tarpc_bool retval;
};

/* get_addrof() */
struct tarpc_get_addrof_in {
    struct tarpc_in_arg common;
       
    string name<>;
};

struct tarpc_get_addrof_out {
    struct tarpc_out_arg common;

    tarpc_ptr addr;
};

/* get_var() */
struct tarpc_get_var_in {
    struct tarpc_in_arg common;
       
    string       name<>; /**< Name of the variable */
    tarpc_size_t size;   /**< size of the variable: 1, 2, 4, 8 */
};    

struct tarpc_get_var_out {
    struct tarpc_out_arg common;
    
    uint64_t   val;   /**< Variable value */
    tarpc_bool found; /**< If TRUE, variable is found */
};

/* set_var() */
struct tarpc_set_var_in {
    struct tarpc_in_arg common;
       
    string       name<>; /**< Name of the variable */
    tarpc_size_t size;   /**< size of the variable: 1, 2, 4, 8 */
    uint64_t     val; /**< Variable value */
};    

struct tarpc_set_var_out {
    struct tarpc_out_arg common;
    
    tarpc_bool found; /**< If TRUE, variable is found */
};

/* mcast_join_leave() */
enum tarpc_joining_method {
    TARPC_MCAST_OPTIONS = 1,
    TARPC_MCAST_WSA = 2
};

struct tarpc_mcast_join_leave_in {
    struct tarpc_in_arg common;
    
    tarpc_int                  fd;          /**< TA-local socket */
    tarpc_int                  family;      /**< Address family */
    uint8_t                    multiaddr<>; /**< Multicast group address */
    tarpc_int                  ifindex;     /**< Interface index */
    tarpc_bool                 leave_group; /**< Leave the group */
    tarpc_joining_method       how;         /**< How to join the group */
};

struct tarpc_mcast_join_leave_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
};

/* cpe_get_rpc_methods() */
typedef struct tarpc_void_in tarpc_cpe_get_rpc_methods_in;

typedef char tarpc_string32_t[33];
typedef char tarpc_string64_t[65];
typedef char tarpc_string256_t[257];
typedef char *tarpc_xsd_any_simple_type_t;
typedef tarpc_string64_t tarpc_base64_t;

struct tarpc_cpe_get_rpc_methods_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    tarpc_string64_t method_list<>;
};

/* cpe_set_parameter_values() */
struct parameter_value_struct_t {
    tarpc_string256_t           name;
    tarpc_xsd_any_simple_type_t value;
};

struct tarpc_cpe_set_parameter_values_in {
    struct tarpc_in_arg common;

    parameter_value_struct_t parameter_list<>;
    tarpc_string32_t         parameter_key;
};

struct tarpc_cpe_set_parameter_values_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    tarpc_int status;
};

/* cpe_get_parameter_values() */
struct tarpc_cpe_get_parameter_values_in {
    struct tarpc_in_arg common;

    tarpc_string256_t parameter_names<>;
};

struct tarpc_cpe_get_parameter_values_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    parameter_value_struct_t parameter_list<>;
};

/* cpe_get_parameter_names() */
struct tarpc_cpe_get_parameter_names_in {
    struct tarpc_in_arg common;

    tarpc_string256_t parameter_path;
    tarpc_bool        next_level;
};

struct parameter_info_struct_t {
    tarpc_string256_t name;
    tarpc_bool        writable;
};

struct tarpc_cpe_get_parameter_names_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    parameter_info_struct_t parameter_list<>;
};

/* cpe_set_parameter_attributes() */
struct set_parameter_attributes_struct_t {
    tarpc_string256_t name;
    tarpc_bool        notification_change;
    tarpc_int         notification;
    tarpc_bool        access_list_change;
    tarpc_string64_t  access_list<>;
};

struct tarpc_cpe_set_parameter_attributes_in {
    struct tarpc_in_arg common;

    set_parameter_attributes_struct_t parameter_list<>;
};

typedef struct tarpc_int_retval_out tarpc_cpe_set_parameter_attributes_out;

/* cpe_get_parameter_attributes() */
struct tarpc_cpe_get_parameter_attributes_in {
    struct tarpc_in_arg common;

    tarpc_string256_t parameter_names<>;
};

struct parameter_attribute_struct_t {
    tarpc_string256_t name;
    tarpc_int         notification;
    tarpc_string64_t  access_list<>;
};

struct tarpc_cpe_get_parameter_attributes_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    parameter_attribute_struct_t parameter_list<>;
};

/* cpe_add_object() */
struct tarpc_cpe_add_object_in {
    struct tarpc_in_arg common;

    tarpc_string256_t object_name;
    tarpc_string32_t  parameter_key;
};

struct tarpc_cpe_add_object_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    tarpc_uint instance_number;
    tarpc_int  status;
};

/* cpe_delete_object() */
struct tarpc_cpe_delete_object_in {
    struct tarpc_in_arg common;

    tarpc_string256_t object_name;
    tarpc_string32_t  parameter_key;
};

struct tarpc_cpe_delete_object_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    tarpc_int status;
};

/* cpe_reboot() */
struct tarpc_cpe_reboot_in {
    struct tarpc_in_arg common;

    tarpc_string32_t command_key;
};

typedef struct tarpc_int_retval_out tarpc_cpe_reboot_out;

/* cpe_download() */
struct tarpc_cpe_download_in {
    struct tarpc_in_arg common;

    tarpc_string32_t  command_key;
    tarpc_string64_t  file_type;
    tarpc_string256_t url;
    tarpc_string256_t username;
    tarpc_string256_t password;
    tarpc_uint        file_size;
    tarpc_string256_t target_file_name;
    tarpc_uint        delay_seconds;
    tarpc_string256_t success_url;
    tarpc_string256_t failure_url;
};

struct tarpc_cpe_download_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    tarpc_int    status;
    tarpc_time_t start_time;
    tarpc_time_t complete_time;
};

/* cpe_upload() */
struct tarpc_cpe_upload_in {
    struct tarpc_in_arg common;

    tarpc_string32_t  command_key;
    tarpc_string64_t  file_type;
    tarpc_string256_t url;
    tarpc_string256_t username;
    tarpc_string256_t password;
    tarpc_uint        delay_seconds;
};

struct tarpc_cpe_upload_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    tarpc_int    status;
    tarpc_time_t start_time;
    tarpc_time_t complete_time;
};

/* cpe_factory_reset() */
typedef struct tarpc_void_in tarpc_cpe_factory_reset_in;

typedef struct tarpc_int_retval_out tarpc_cpe_factory_reset_out;

/* cpe_get_queued_transfers() */
typedef struct tarpc_void_in tarpc_cpe_get_queued_transfers_in;

struct tarpc_cpe_get_queued_transfers_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    tarpc_string32_t command_key;
    tarpc_int        state;
};

/* cpe_get_all_queued_transfers() */
typedef struct tarpc_void_in tarpc_cpe_get_all_queued_transfers_in;

struct all_queued_transfer_struct_t {
    tarpc_string32_t  command_key;
    tarpc_int         state;
    tarpc_bool        is_download;
    tarpc_string64_t  file_type;
    tarpc_uint        file_size;
    tarpc_string256_t target_file_name;
};

struct tarpc_cpe_get_all_queued_transfers_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    all_queued_transfer_struct_t transfer_list[16];
};

/* cpe_schedule_inform() */
struct tarpc_cpe_schedule_inform_in {
    struct tarpc_in_arg common;

    tarpc_uint       delay_seconds;
    tarpc_string32_t command_key;
};

typedef struct tarpc_int_retval_out tarpc_cpe_schedule_inform_out;

/* cpe_set_vouchers() */

struct tarpc_cpe_set_vouchers_in {
    struct tarpc_in_arg common;

    tarpc_base64_t voucher_list<>;
};

typedef struct tarpc_int_retval_out tarpc_cpe_set_vouchers_out;

/* cpe_get_options() */
struct tarpc_cpe_get_options_in {
    struct tarpc_in_arg common;

    tarpc_base64_t voucher_list<>;
};

struct option_struct_t {
    tarpc_string64_t option_name;
    tarpc_uint       voucher_s_n;
    tarpc_uint       state;
    tarpc_int        mode;
    tarpc_time_t     start_date;
    tarpc_time_t     expiration_date;
    tarpc_bool       is_transferable;
};

struct tarpc_cpe_get_options_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;

    option_struct_t option_list<>;
};



struct tarpc_cwmp_op_call_in {
    struct tarpc_in_arg common;

    string              acs_name<>; /**< Name of ACS object */ 
    string              cpe_name<>; /**< Name of CPE record */ 
    tarpc_uint          cwmp_rpc;   /**< CWMP RPC call type */
    uint8_t             buf<>;      /**< Buffer with CWMP call data */
};


struct tarpc_cwmp_op_call_out {
    struct tarpc_out_arg  common;

    tarpc_uint         status;    /**< status of operation - te_errno */
    tarpc_uint         request_id;/**< Index of CWMP RPC */
};


struct tarpc_cwmp_op_check_in {
    struct tarpc_in_arg common;

    string              acs_name<>; /**< Name of ACS object */ 
    string              cpe_name<>; /**< Name of CPE record */ 
    tarpc_uint          request_id; /**< Index of CWMP RPC */
    tarpc_uint          cwmp_rpc;   /**< CWMP ACS RPC call type, 
                            that is, value from enum #te_cwmp_rpc_acs_t.
                            Should be zero (CWMP_RPC_ACS_NONE) for check
                            status of RPC, sent to CPE, and get its
                            response. */
};

struct tarpc_cwmp_op_check_out {
    struct tarpc_out_arg  common;

    tarpc_uint          status;     /**< status of operation - te_errno */
    tarpc_uint          cwmp_rpc;   /**< CWMP RPC call type */
    tarpc_uint          request_id; /**< Index of CWMP RPC */
    uint8_t             buf<>;      /**< Buffer with CWMP response data */
};


struct tarpc_cwmp_conn_req_in {
    struct tarpc_in_arg common;

    string              acs_name<>; /**< Name of ACS object */ 
    string              cpe_name<>; /**< Name of CPE record */ 
};

struct tarpc_cwmp_conn_req_out {
    struct tarpc_out_arg  common;

    tarpc_int           status;     /**< status of operation */
};


struct tarpc_cwmp_get_inform_in {
    struct tarpc_in_arg common;

    string              acs_name<>; /**< Name of ACS object */ 
    string              cpe_name<>; /**< Name of CPE record */ 
    tarpc_uint          index; /**< Index of CWMP Inform */
};

struct tarpc_cwmp_get_inform_out {
    struct tarpc_out_arg  common;

    tarpc_int           status;     /**< status of operation */
    uint8_t             buf<>;      /**< Buffer with CWMP Inform data */
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

        RPC_DEF(get_sizeof)
        RPC_DEF(get_addrof)
        RPC_DEF(protocol_info_cmp)

        RPC_DEF(get_var)
        RPC_DEF(set_var)
        
        RPC_DEF(create_process)
        RPC_DEF(thread_create)
        RPC_DEF(thread_cancel)
        RPC_DEF(execve)
        RPC_DEF(getpid)
        RPC_DEF(gettimeofday)
        
        RPC_DEF(access)

        RPC_DEF(malloc)
        RPC_DEF(free)
        RPC_DEF(memalign)
        RPC_DEF(memcmp)

        RPC_DEF(socket)
        RPC_DEF(wsa_socket)
        RPC_DEF(wsa_startup)
        RPC_DEF(wsa_cleanup)
        RPC_DEF(duplicate_socket)
        RPC_DEF(duplicate_handle)
        RPC_DEF(dup)
        RPC_DEF(dup2)
        RPC_DEF(close)
        RPC_DEF(shutdown)

        RPC_DEF(read)
        RPC_DEF(write)

        RPC_DEF(readbuf)
        RPC_DEF(writebuf)
        RPC_DEF(recvbuf)
        RPC_DEF(sendbuf)
        RPC_DEF(send_msg_more)

        RPC_DEF(readv)
        RPC_DEF(writev)

        RPC_DEF(lseek)

        RPC_DEF(fsync)

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

        RPC_DEF(epoll_create)
        RPC_DEF(epoll_ctl)
        RPC_DEF(epoll_wait)

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
        RPC_DEF(bsd_signal)
        RPC_DEF(sysv_signal)
        RPC_DEF(sigaction)
        RPC_DEF(kill)
        RPC_DEF(ta_kill_death)

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
        RPC_DEF(open)
        RPC_DEF(open64)
        RPC_DEF(fopen)
        RPC_DEF(fdopen)
        RPC_DEF(fclose)
        RPC_DEF(te_shell_cmd)
        RPC_DEF(system)
        RPC_DEF(waitpid)
        RPC_DEF(fileno)
        RPC_DEF(getpwnam)
        RPC_DEF(getuid)
        RPC_DEF(geteuid)
        RPC_DEF(setuid)
        RPC_DEF(seteuid)
        RPC_DEF(getenv)
        RPC_DEF(setenv)
        RPC_DEF(uname)
        
        RPC_DEF(create_aiocb)
        RPC_DEF(fill_aiocb)
        RPC_DEF(delete_aiocb)
        RPC_DEF(aio_read)
        RPC_DEF(aio_write)
        RPC_DEF(aio_error)
        RPC_DEF(aio_return)
        RPC_DEF(aio_suspend)
        RPC_DEF(aio_cancel)
        RPC_DEF(aio_fsync)
        RPC_DEF(lio_listio)

        RPC_DEF(simple_sender)
        RPC_DEF(simple_receiver)
        RPC_DEF(wait_readable)
        RPC_DEF(recv_verify)
        RPC_DEF(flooder)
        RPC_DEF(echoer)

        RPC_DEF(sendfile)
        RPC_DEF(socket_to_file)

        RPC_DEF(ftp_open)
        RPC_DEF(ftp_close)

        RPC_DEF(overfill_buffers)

        RPC_DEF(create_event)
        RPC_DEF(create_event_with_bit)
        RPC_DEF(close_event)
        RPC_DEF(create_overlapped)
        RPC_DEF(delete_overlapped)

        RPC_DEF(telephony_open_channel)
        RPC_DEF(telephony_close_channel)
        RPC_DEF(telephony_pickup)
        RPC_DEF(telephony_hangup)
        RPC_DEF(telephony_check_dial_tone)
        RPC_DEF(telephony_dial_number)
        RPC_DEF(telephony_call_wait)

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
        RPC_DEF(transmit_packets)
        RPC_DEF(create_file)
        RPC_DEF(closesocket)
        RPC_DEF(has_overlapped_io_completed)
        RPC_DEF(cancel_io)
        RPC_DEF(create_io_completion_port)
        RPC_DEF(get_queued_completion_status)
        RPC_DEF(post_queued_completion_status)
        RPC_DEF(get_current_process_id)
        RPC_DEF(get_sys_info)
        RPC_DEF(vm_trasher)
        RPC_DEF(write_at_offset)
        RPC_DEF(completion_callback)
        RPC_DEF(wsa_send)
        RPC_DEF(wsa_recv)
        RPC_DEF(wsa_recv_ex)
        RPC_DEF(wsa_get_overlapped_result)
        RPC_DEF(wait_for_multiple_events)
        RPC_DEF(wsa_send_to)
        RPC_DEF(wsa_recv_from)
        RPC_DEF(wsa_send_disconnect)
        RPC_DEF(wsa_recv_disconnect)
        RPC_DEF(wsa_recv_msg)
        RPC_DEF(read_file)
        RPC_DEF(read_file_ex)
        RPC_DEF(write_file)
        RPC_DEF(write_file_ex)
        RPC_DEF(wsa_address_to_string)
        RPC_DEF(wsa_string_to_address)
        RPC_DEF(wsa_async_get_host_by_addr)
        RPC_DEF(wsa_async_get_host_by_name)
        RPC_DEF(wsa_async_get_proto_by_name)
        RPC_DEF(wsa_async_get_proto_by_number)
        RPC_DEF(wsa_async_get_serv_by_name)
        RPC_DEF(wsa_async_get_serv_by_port)
        RPC_DEF(wsa_cancel_async_request)
        RPC_DEF(set_buf)
        RPC_DEF(get_buf)
        RPC_DEF(set_buf_pattern)
        RPC_DEF(alloc_wsabuf)
        RPC_DEF(free_wsabuf)
        RPC_DEF(wsa_connect)
        RPC_DEF(wsa_ioctl)
        RPC_DEF(get_wsa_ioctl_overlapped_result)
        RPC_DEF(cmsg_data_parse_ip_pktinfo)

        RPC_DEF(create_window)
        RPC_DEF(destroy_window)
        RPC_DEF(wsa_async_select)
        RPC_DEF(peek_message)
        RPC_DEF(wsa_join_leaf)
        
        RPC_DEF(setrlimit)
        RPC_DEF(getrlimit)

        RPC_DEF(mcast_join_leave)

        RPC_DEF(cwmp_op_call)
        RPC_DEF(cwmp_op_check)
        RPC_DEF(cwmp_conn_req)
        RPC_DEF(cwmp_get_inform)
    } = 1;
} = 1;
