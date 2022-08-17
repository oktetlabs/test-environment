/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Remote Procedure Call
 *
 * Definition of RPC structures and functions
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
typedef int64_t     tarpc_rlim_t;
/** RPC clock_t analogue */
typedef int64_t     tarpc_clock_t;
/** RPC time_t analogue */
typedef int64_t     tarpc_time_t;
/** RPC suseconds_t analogue */
typedef int64_t     tarpc_suseconds_t;
/** Pointer to 'struct aiocb' */
typedef tarpc_ptr   tarpc_aiocb_t;
/** RPC pthread_t analogue */
typedef uint64_t    tarpc_pthread_t;

/** Handle of the 'WSAEvent' or 0 */
typedef tarpc_ptr   tarpc_wsaevent;
/** Handle of the window */
typedef tarpc_ptr   tarpc_hwnd;
/** WSAOVERLAPPED structure */
typedef tarpc_ptr   tarpc_overlapped;
/** HANDLE */
typedef tarpc_ptr   tarpc_handle;

/** Handle of the 'iomux_state' or 0 */
typedef tarpc_ptr   tarpc_iomux_state;

typedef uint32_t    tarpc_op;

/** RPC dynamic linking loader handlers */
typedef int64_t     tarpc_dlhandle;
typedef int64_t     tarpc_dlsymaddr;

/** Ethtool command, should be u32 on any Linux */
typedef uint32_t tarpc_ethtool_command;

/**
 * Flags to resolve function name.
 *
 * @attention   These are bit flags, with the exception of
 *              @c TARPC_LIB_DEFAULT, which is used as the
 *              initialization value.
 */
enum tarpc_lib_flags {
    TARPC_LIB_DEFAULT       = 0x0,
    TARPC_LIB_USE_LIBC      = 0x1,  /* 1 << 0 */
    TARPC_LIB_USE_SYSCALL   = 0x2   /* 1 << 1 */
};

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
    uint64_t        jobid;      /**< Job identifier (for async calls) */
    uint16_t        seqno;      /**< Sequence number of an RPC call */
    tarpc_lib_flags lib_flags;  /**< How to resolve function name */
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
    char        err_str<>;      /**< String explaining the error. */

    uint32_t    duration;   /**< Duration of the called routine
                                 execution (in microseconds) */
    uint64_t    jobid;    /**< Identifier of an asynchronous operation.
                               It should be passed as input when
                               RPC client want to check status or
                               wait for finish of the initiated
                               operation. */
    tarpc_bool  unsolicited; /**< Set to TRUE for RPC responses that are
                                  not paired to RPC requests.
                                  Currently only used for rpc_is_op_done
                                  notifications */
};

/* Just to make two-dimensional array of strings */
struct tarpc_string {
    string  str<>;
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

/** Ethernet-specific part of generic address */
struct tarpc_local {
    uint8_t     data[6];    /**< Ethernet address */
};

/** AF_LOCAL-specific part of generic address */
struct tarpc_sun {
    uint8_t     path[108];  /**< Path to pipe file */
};

/** AF_PACKET-specific part of generic address */
struct tarpc_ll {
    tarpc_int   sll_protocol;
    tarpc_int   sll_ifindex;
    tarpc_int   sll_hatype;
    tarpc_int   sll_pkttype;
    tarpc_int   sll_halen;
    uint8_t     sll_addr[8];
};

/**
 * Socket address family.
 * Values should correspond to values of
 * rpc_socket_addr_family enum.
 */
enum tarpc_socket_addr_family {
    TARPC_AF_INET = 1,
    TARPC_AF_INET6 = 2,
    TARPC_AF_PACKET = 3,
    TARPC_AF_LOCAL = 4,
    TARPC_AF_UNIX = 5,
    TARPC_AF_ETHER = 6,
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
    case TARPC_AF_ETHER:    struct tarpc_local  local;  /**< HW address */
    case TARPC_AF_LOCAL:    struct tarpc_sun    un;     /**< UNIX socket */
    case TARPC_AF_PACKET:   struct tarpc_ll     ll;     /**< Physical-layer
                                                             address */
    default:                void;                       /**< Nothing by
                                                             default */
};

enum tarpc_send_function {
    TARPC_SEND_FUNC_WRITE = 1,
    TARPC_SEND_FUNC_WRITEV,
    TARPC_SEND_FUNC_SEND,
    TARPC_SEND_FUNC_SENDTO,
    TARPC_SEND_FUNC_SENDMSG,
    TARPC_SEND_FUNC_SENDMMSG,
    TARPC_SEND_FUNC_MAX
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

/* gethostname() */
struct tarpc_gethostname_in {
    struct tarpc_in_arg common;

    char                name<>;
    tarpc_size_t        len;
};

struct tarpc_gethostname_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;

    char                 name<>;
};

/* Ethtool data types */
enum tarpc_ethtool_type {
    TARPC_ETHTOOL_UNKNOWN = 0,  /**< Unknown type */
    TARPC_ETHTOOL_CMD = 1,      /**< struct ethtool_cmd */
    TARPC_ETHTOOL_VALUE = 2,    /**< struct ethtool_value */
    TARPC_ETHTOOL_PADDR = 3,    /**< struct ethtool_perm_addr */
    TARPC_ETHTOOL_TS_INFO = 4   /**< struct ethtool_ts_info */
};

struct tarpc_ethtool_cmd {
    uint32_t cmd;           /* this field is defined only to keep
                               old tests using native constants
                               working */
    uint32_t supported;
    uint32_t advertising;
    uint16_t speed;
    uint8_t  duplex;
    uint8_t  port;
    uint8_t  phy_address;
    uint8_t  transceiver;
    uint8_t  autoneg;
    uint8_t  mdio_support;
    uint32_t maxtxpkt;
    uint32_t maxrxpkt;
    uint16_t speed_hi;
    uint8_t  eth_tp_mdix;
    uint8_t  reserved2;
    uint32_t lp_advertising;
    uint32_t reserved[2];
};

struct tarpc_ethtool_perm_addr {
    uint32_t    cmd;        /* this field is defined only to keep
                               old tests using native constants
                               working */
    uint32_t    size;
    tarpc_local data;
};

struct tarpc_ethtool_value {
    uint32_t    cmd;        /* this field is defined only to keep
                               old tests using native constants
                               working */
    uint32_t    data;
};

struct tarpc_ethtool_ts_info {
    uint32_t cmd;
    uint32_t so_timestamping;
    int32_t phc_index;
    uint32_t tx_types;
    uint32_t rx_filters;
};

/** struct ethtool_* */
union tarpc_ethtool_data
    switch (tarpc_ethtool_type type)
{
    case TARPC_ETHTOOL_CMD:     tarpc_ethtool_cmd cmd;
    case TARPC_ETHTOOL_VALUE:   tarpc_ethtool_value value;
    case TARPC_ETHTOOL_PADDR:   tarpc_ethtool_perm_addr paddr;
    case TARPC_ETHTOOL_TS_INFO: tarpc_ethtool_ts_info ts_info;
    default:                    void;
};

struct tarpc_ethtool {
    tarpc_ethtool_command   command;
    tarpc_ethtool_data      data;
};

/** tarpc_hwtstamp_config */
struct tarpc_hwtstamp_config {
    tarpc_int flags;        /* no flags defined right now, must be zero */
    tarpc_int tx_type;      /* HWTSTAMP_TX_* */
    tarpc_int rx_filter;    /* HWTSTAMP_FILTER_* */
};

/** struct ifreq */
struct tarpc_ifreq {
    char                    rpc_ifr_name<>; /**< Interface name */
    struct tarpc_sa         rpc_ifr_addr;   /**< Different interface
                                                 addresses */
    tarpc_int               rpc_ifr_flags;  /**< Interface flags */
    tarpc_int               rpc_ifr_ifindex;/**< Interface index */
    uint32_t                rpc_ifr_mtu;    /**< Interface MTU */
    tarpc_ethtool           rpc_ifr_ethtool; /**< Ethtool command
                                                  structure */
    tarpc_hwtstamp_config   rpc_ifr_hwstamp; /**< HW timestamping
                                                  configuration */
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

/** struct ptp_clock_caps */
struct tarpc_ptp_clock_caps {
    tarpc_int max_adj;
    tarpc_int n_alarm;
    tarpc_int n_ext_ts;
    tarpc_int n_per_out;
    tarpc_int pps;
    tarpc_int n_pins;
    tarpc_int cross_timestamping;
    tarpc_int adjust_phase;
};

/** struct timespec */
struct tarpc_timespec {
    int64_t     tv_sec;
    int64_t     tv_nsec;
};

/** struct scm_timestamping */
struct tarpc_scm_timestamping {
    tarpc_timespec systime;
    tarpc_timespec hwtimetrans;
    tarpc_timespec hwtimeraw;
};

/** Solarflare Onload specific struct onload_scm_timestamping_stream */
struct tarpc_onload_scm_timestamping_stream {
    tarpc_timespec first_sent;
    tarpc_timespec last_sent;
    tarpc_size_t len;
};

struct tarpc_ptp_clock_time {
    int64_t sec;
    uint32_t nsec;
};

struct tarpc_ptp_sys_offset {
    tarpc_uint n_samples;
    /*
     * In ptp_clock.h PTP_MAX_SAMPLES is defined to 25,
     * and in ptp_sys_offset 2 * PTP_MAX_SAMPLES + 1 is used.
     */
    tarpc_ptp_clock_time ts[51];
};

struct tarpc_ptp_three_ts {
    tarpc_ptp_clock_time sys1;
    tarpc_ptp_clock_time phc;
    tarpc_ptp_clock_time sys2;
};

struct tarpc_ptp_sys_offset_extended {
    tarpc_uint n_samples;
    tarpc_ptp_three_ts ts[25];
};

struct tarpc_ptp_sys_offset_precise {
    tarpc_ptp_clock_time device;
    tarpc_ptp_clock_time sys_realtime;
    tarpc_ptp_clock_time sys_monoraw;
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

/* rpc_find_func */

struct tarpc_rpc_find_func_in {
    struct tarpc_in_arg common;

    string func_name<>; /**< Name of function */
};

struct tarpc_rpc_find_func_out {
    struct tarpc_out_arg common;

    tarpc_int find_result;     /**< Result returned by tarpc_find_func */
};

/* rpc_is_op_done() */

typedef struct tarpc_void_in  tarpc_rpc_is_op_done_in;
struct tarpc_rpc_is_op_done_out {
    struct tarpc_out_arg common;

    tarpc_bool done;
};



/* rpc_is_alive() */

typedef struct tarpc_void_in  tarpc_rpc_is_alive_in;
typedef struct tarpc_void_out tarpc_rpc_is_alive_out;

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

/* dup3() */

struct tarpc_dup3_in {
    struct tarpc_in_arg common;

    tarpc_int   oldfd;
    tarpc_int   newfd;
    tarpc_int   flags;  /**< RPC_O_CLOEXEC */
};

typedef struct tarpc_socket_out tarpc_dup3_out;

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

/* fstat */

/* to be honest this looks like complete shit to me - I'm
 * not sure about types length etc. and it's not clear how
 * to convert 'long' value to 'short' one in the test.
 * macros for st_dev work and that's all we need right now.
 */
struct tarpc_stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_mode;
    uint64_t st_nlink;
    uint64_t st_uid;
    uint64_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint64_t st_blksize;
    uint64_t st_blocks;
    uint64_t te_atime;
    uint64_t te_mtime;
    uint64_t te_ctime;
    tarpc_bool ifsock;
    tarpc_bool iflnk;
    tarpc_bool ifreg;
    tarpc_bool ifblk;
    tarpc_bool ifdir;
    tarpc_bool ifchr;
    tarpc_bool ififo;
};

typedef struct tarpc_stat rpc_stat;

struct tarpc_te_fstat_in {
    struct tarpc_in_arg common;
    tarpc_int               fd;

    struct tarpc_stat buf;
};

struct tarpc_te_fstat_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    struct tarpc_stat buf;
};

struct tarpc_te_fstat64_in {
    struct tarpc_in_arg common;
    tarpc_int               fd;

    struct tarpc_stat buf;
};

struct tarpc_te_fstat64_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    struct tarpc_stat buf;
};

struct tarpc_te_stat_in {
    struct tarpc_in_arg common;
    char                path<>;

    struct tarpc_stat buf;
};

struct tarpc_te_stat_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    struct tarpc_stat buf;
};

/* link() */
struct tarpc_link_in {
    struct tarpc_in_arg common;

    char                path1<>;
    char                path2<>;
};

struct tarpc_link_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

/* symlink() */
struct tarpc_symlink_in {
    struct tarpc_in_arg common;

    char                path1<>;
    char                path2<>;
};

struct tarpc_symlink_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

/* unlink() */
struct tarpc_unlink_in {
    struct tarpc_in_arg common;

    char                path<>;
};

struct tarpc_unlink_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

/* rename() */
struct tarpc_rename_in {
    struct tarpc_in_arg common;

    char                path_old<>;
    char                path_new<>;
};

struct tarpc_rename_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

/* mkdir() */
struct tarpc_mkdir_in {
    struct tarpc_in_arg common;

    char                path<>;
    tarpc_int           mode;
};

struct tarpc_mkdir_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

/* mkdirp() */
typedef struct tarpc_mkdir_in tarpc_mkdirp_in;
typedef struct tarpc_mkdir_out tarpc_mkdirp_out;

/* rmdir() */
struct tarpc_rmdir_in {
    struct tarpc_in_arg common;

    char                path<>;
};

struct tarpc_rmdir_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

/* fstatvfs() */
struct tarpc_statvfs {
    uint32_t f_bsize; /* file system block size */
    uint64_t f_blocks; /* total number of blocks on file system in units of 'f_frsize' */
    uint64_t f_bfree; /* total number of free blocks */
};

typedef struct tarpc_statvfs tarpc_statvfs;

struct tarpc_fstatvfs_in {
    struct tarpc_in_arg  common;
    tarpc_int            fd;

    struct tarpc_statvfs buf;
};

struct tarpc_fstatvfs_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
    struct tarpc_statvfs buf;
};

/* statvfs() */
struct tarpc_statvfs_in {
    struct tarpc_in_arg  common;

    char                 path<>;
    struct tarpc_statvfs buf;
};

struct tarpc_statvfs_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
    struct tarpc_statvfs buf;
};

/* struct_dirent_props() */
struct tarpc_struct_dirent_props_in {
    struct tarpc_in_arg common;
};

struct tarpc_struct_dirent_props_out {
    struct tarpc_out_arg common;

    tarpc_uint           retval;
};

/* opendir() */
struct tarpc_opendir_in {
    struct tarpc_in_arg common;

    char                path<>;
};

struct tarpc_opendir_out {
    struct tarpc_out_arg common;

    tarpc_ptr            mem_ptr;
};

/* closedir() */
struct tarpc_closedir_in {
    struct tarpc_in_arg common;

    tarpc_ptr           mem_ptr;
};

struct tarpc_closedir_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

/* readdir() */
struct tarpc_dirent {
    uint64_t   d_ino; /* inode number (mandatory) */
    char       d_name<>; /* filename (mandatory) */

    uint64_t   d_off; /* offset to the next dirent (may be not supported) */
    tarpc_int  d_type; /* type of file (may be not supported) */
    tarpc_uint d_namelen; /* The size of the d_name member
                             (not including trailing zero).
                             (may be not supported) */
    tarpc_uint d_props; /* Properties of 'struct dirent' */
};

struct tarpc_readdir_in {
    struct tarpc_in_arg common;

    tarpc_ptr           mem_ptr;
};

struct tarpc_readdir_out {
    struct tarpc_out_arg common;

    tarpc_bool           ret_null;
    tarpc_dirent         dent<>;
};

/* read() / write() / write_and_close() */

struct tarpc_read_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    uint8_t         buf<>;
    tarpc_size_t    len;

    tarpc_bool chk_func;
};

struct tarpc_read_out {
    struct tarpc_out_arg    common;

    tarpc_ssize_t   retval;
    uint8_t         buf<>;
};

struct tarpc_pread_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    uint8_t         buf<>;
    tarpc_size_t    len;
    tarpc_off_t     offset;

    tarpc_bool chk_func;
};

typedef tarpc_read_out tarpc_pread_out;

typedef struct tarpc_read_in tarpc_write_in;

typedef struct tarpc_pread_in tarpc_pwrite_in;

typedef struct tarpc_ssize_t_retval_out tarpc_write_out;

typedef struct tarpc_write_out tarpc_pwrite_out;

typedef struct tarpc_write_in tarpc_write_and_close_in;

typedef struct tarpc_ssize_t_retval_out tarpc_write_and_close_out;

typedef struct tarpc_read_in  tarpc_read_via_splice_in;
typedef struct tarpc_read_out tarpc_read_via_splice_out;

typedef struct tarpc_write_in tarpc_write_via_splice_in;
typedef struct tarpc_write_out tarpc_write_via_splice_out;

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

struct tarpc_preadv_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;
    struct tarpc_iovec  vector<>;
    tarpc_size_t        count;
    tarpc_off_t         offset;
};

typedef struct tarpc_readv_in tarpc_writev_in;

typedef struct tarpc_preadv_in tarpc_pwritev_in;

typedef struct tarpc_readv_out tarpc_preadv_out;

typedef struct tarpc_ssize_t_retval_out tarpc_writev_out;

typedef tarpc_writev_out tarpc_pwritev_out;

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

    tarpc_send_function first_func;
    tarpc_send_function second_func;
    tarpc_bool          set_nodelay;
};

typedef struct tarpc_ssize_t_retval_out tarpc_send_msg_more_out;

/* send_one_byte_many() */

struct tarpc_send_one_byte_many_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    tarpc_int       duration;
};

typedef struct tarpc_ssize_t_retval_out tarpc_send_one_byte_many_out;

/* recv() */

struct tarpc_recv_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    uint8_t         buf<>;
    tarpc_size_t    len;
    tarpc_int       flags;

    tarpc_bool chk_func;
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

    tarpc_bool chk_func; /**< If @c TRUE, call __recvfrom_chk() */
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
    uint64_t        restorer;    /**< Opaque sa_restorer value */
    tarpc_sigset_t  mask;        /**< Handle (pointer in server
                                      context) of the allocated set */
    tarpc_int       flags;       /**< Flags to be passed to sigaction */
};

/* sigaltstack() */
struct tarpc_stack_t {
    tarpc_ptr       ss_sp;      /**< Base address of stack */
    tarpc_int       ss_flags;   /**< Flags */
    tarpc_size_t    ss_size;    /**< Number of bytes in stack */
};

/* sendmsg() / recvmsg() */
enum tarpc_cmsg_data_type {
    TARPC_CMSG_DATA_RAW = 0,            /* raw data */
    TARPC_CMSG_DATA_BYTE = 1,           /* uint8_t */
    TARPC_CMSG_DATA_INT = 2,            /* int */
    TARPC_CMSG_DATA_SOCK_EXT_ERR = 3,   /* struct sock_extended_err */
    TARPC_CMSG_DATA_PKTINFO = 4,        /* struct in_pktinfo */
    TARPC_CMSG_DATA_PKTINFO6 = 5,       /* struct in6_pktinfo */
    TARPC_CMSG_DATA_TV = 6,             /* struct timeval */
    TARPC_CMSG_DATA_TS = 7,             /* struct timespec */
    TARPC_CMSG_DATA_TSTAMP = 8,         /* struct scm_timestamping */
    TARPC_CMSG_DATA_TSTAMP_STREAM = 9   /* struct onload_scm_timestamping_stream */
};

/* struct sock_extended_err */
struct tarpc_sock_extended_err {
    tarpc_int   ee_errno;
    tarpc_int   ee_origin;
    tarpc_int   ee_type;
    tarpc_int   ee_code;
    tarpc_int   ee_pad;
    tarpc_int   ee_info;
    tarpc_int   ee_data;
    tarpc_sa    ee_offender;    /**< Address of network object
                                     where an error originated */
};

/* struct in_pktinfo */
struct tarpc_in_pktinfo {
    uint32_t                ipi_spec_dst;
    uint32_t                ipi_addr;
    tarpc_int               ipi_ifindex;
};

/* struct in6_pktinfo */
struct tarpc_in6_pktinfo {
    uint8_t                 ipi6_addr[16];  /**< Network address */
    tarpc_int               ipi6_ifindex;   /**< Interface index */
};

/* Data in control message */
union tarpc_cmsg_data switch (tarpc_cmsg_data_type type) {
    case TARPC_CMSG_DATA_RAW:   void;
    case TARPC_CMSG_DATA_INT:   tarpc_int int_data;   /**< Integer value */
    case TARPC_CMSG_DATA_BYTE:  uint8_t   byte_data;  /**< Byte value */

    case TARPC_CMSG_DATA_SOCK_EXT_ERR:
                tarpc_sock_extended_err     ext_err;  /**< IP_RECVERR */
    case TARPC_CMSG_DATA_PKTINFO:
                tarpc_in_pktinfo            pktinfo;  /**< IP_PKTINFO */
    case TARPC_CMSG_DATA_PKTINFO6:
                tarpc_in6_pktinfo           pktinfo6; /**< IPV6_PKTINFO */

    case TARPC_CMSG_DATA_TV:
                struct tarpc_timeval          tv;    /**< SO_TIMESTAMP */
    case TARPC_CMSG_DATA_TS:
                struct tarpc_timespec         ts;    /**< SO_TIMESTAMPNS */
    case TARPC_CMSG_DATA_TSTAMP:
                struct tarpc_scm_timestamping tstamp;/**< SO_TIMESTAMPING */
    case TARPC_CMSG_DATA_TSTAMP_STREAM:
                struct tarpc_onload_scm_timestamping_stream sf_txts; /**< SF Onload TX TCP SO_TIMESTAMPING */

};

struct tarpc_cmsghdr {
    uint32_t level;     /**< Originating protocol */
    uint32_t type;      /**< Protocol-specific type */
    uint8_t  data<>;    /**< Data */

    tarpc_cmsg_data data_aux;   /**< Non-raw representation if possible */
};

struct tarpc_msghdr {
    struct tarpc_sa      msg_name;       /**< Destination/source address */
    int64_t              msg_namelen;    /**< To be passed to
                                              recvmsg/sendmsg; in case of
                                              negative value will be
                                              determined from msg_name */
    struct tarpc_iovec   msg_iov<>;      /**< Vector */
    tarpc_size_t         msg_iovlen;     /**< Passed to recvmsg() */
    struct tarpc_cmsghdr msg_control<>;  /**< Control info array */
    int64_t              msg_controllen; /**< msg_controllen actually
                                              used/retrieved on TA. If
                                              negative value is passed
                                              by RPC caller, it is
                                              ignored; msg_controllen
                                              used on TA is computed
                                              from control messages
                                              lengths in that case. */

    uint8_t              msg_control_tail<>;  /**< Not parsed, will be
                                                   appended to msg_control
                                                   after control messages */

    tarpc_int            msg_flags;      /**< Flags on received message */
    tarpc_int            in_msg_flags;   /**< Original msg_flags value */
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

struct tarpc_mmsghdr {
    struct tarpc_msghdr msg_hdr;  /**< Message header */
    tarpc_uint          msg_len;  /**< Number of received bytes for
                                       header */
};

struct tarpc_recvmmsg_alt_in {
    struct tarpc_in_arg common;

    tarpc_int               fd;
    struct tarpc_mmsghdr    mmsg<>;
    tarpc_uint              vlen;
    tarpc_uint              flags;
    struct tarpc_timespec   timeout<>;
};

struct tarpc_recvmmsg_alt_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_mmsghdr    mmsg<>;
};

struct tarpc_sendmmsg_alt_in {
    struct tarpc_in_arg common;

    tarpc_int               fd;
    struct tarpc_mmsghdr    mmsg<>;
    tarpc_uint              vlen;
    tarpc_uint              flags;
};

struct tarpc_sendmmsg_alt_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_mmsghdr    mmsg<>;
};

/* cmsg_data_parse_ip_pktinfo() */
struct tarpc_cmsg_data_parse_ip_pktinfo_in {
    struct tarpc_in_arg common;

    uint8_t             data<>;
};

struct tarpc_cmsg_data_parse_ip_pktinfo_out {
    struct tarpc_out_arg    common;
    tarpc_int               retval;
    uint32_t                ipi_spec_dst;
    uint32_t                ipi_addr;
    tarpc_int               ipi_ifindex;
};


/* bind() */

struct tarpc_bind_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;         /**< TA-local socket */
    struct tarpc_sa     addr;       /**< Socket name */
    tarpc_socklen_t     len;        /**< Length to be passed to bind() */
    tarpc_bool          fwd_len;    /**< Use the specified length @a len */
};

typedef struct tarpc_int_retval_out tarpc_bind_out;

/* rpc_check_port_is_free() */

struct tarpc_check_port_is_free_in {
    struct tarpc_in_arg common;

    uint16_t    port;
};

struct tarpc_check_port_is_free_out {
    struct tarpc_out_arg common;

    tarpc_bool retval;
};

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

/* accept4() */

struct tarpc_accept4_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;     /**< TA-local socket */
    struct tarpc_sa     addr;   /**< Location for peer name */
    tarpc_socklen_t     len<>;  /**< Length of the location
                                     for peer name */
    tarpc_int           flags;  /**< RPC_SOCK_NONBLOCK and/or
                                     RPC_SOCK_CLOEXEC */
};

struct tarpc_accept4_out {
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

/* get_addr_by_id */
struct tarpc_get_addr_by_id_in {
    struct tarpc_in_arg  common;
    tarpc_ptr            id;     /**< A pointer in the TA address space */
};

struct tarpc_get_addr_by_id_out {
    struct tarpc_out_arg common;
    uint64_t             retval; /**< Address value */
};

/* raw2integer */
struct tarpc_raw2integer_in {
    struct tarpc_in_arg     common;

    uint8_t     data<>;     /**< Raw data */
};

struct tarpc_raw2integer_out {
    struct tarpc_out_arg    common;
    tarpc_int               retval;

    uint64_t    number;     /**< Converted value */
};

/* integer2raw */
struct tarpc_integer2raw_in {
    struct tarpc_in_arg     common;

    uint64_t        number;    /**< Number to be converted */
    tarpc_size_t    len;       /**< Size of integer type */
};

struct tarpc_integer2raw_out {
    struct tarpc_out_arg    common;
    tarpc_int               retval;

    uint8_t     data<>;     /**< Converted number */
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

struct tarpc_mmap_in {
    struct tarpc_in_arg  common;

    uint64_t             addr;
    uint64_t             length;
    tarpc_uint           prot;
    tarpc_uint           flags;
    tarpc_int            fd;
    tarpc_off_t          offset;
};

struct tarpc_mmap_out {
    struct tarpc_out_arg common;

    tarpc_ptr            retval;
};

struct tarpc_munmap_in {
    struct tarpc_in_arg  common;

    tarpc_ptr            addr;
    uint64_t             length;
};

struct tarpc_munmap_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
};

struct tarpc_madvise_in {
    struct tarpc_in_arg  common;

    tarpc_ptr            addr;
    uint64_t             length;
    tarpc_int            advise;
};

struct tarpc_madvise_out {
    struct tarpc_out_arg common;

    tarpc_int            retval;
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

struct tarpc_pselect_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_timespec   timeout<>;
};



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

    tarpc_bool chk_func;
};

struct tarpc_poll_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_pollfd     ufds<>;
};


/* ppoll() */

struct tarpc_ppoll_in {
    struct tarpc_in_arg common;

    struct tarpc_pollfd   ufds<>;
    tarpc_uint            nfds;
    struct tarpc_timespec timeout<>;
    tarpc_sigset_t        sigmask;

    tarpc_bool chk_func;
};

struct tarpc_ppoll_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_pollfd     ufds<>;
    struct tarpc_timespec   timeout<>;
};

/* epoll_create() */

struct tarpc_epoll_create_in {
    struct tarpc_in_arg common;

    tarpc_int           size;
};

typedef struct tarpc_int_retval_out tarpc_epoll_create_out;


/* epoll_create1() */

struct tarpc_epoll_create1_in {
    struct tarpc_in_arg common;

    tarpc_int           flags;
};

typedef struct tarpc_int_retval_out tarpc_epoll_create1_out;

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

/* epoll_pwait() */

struct tarpc_epoll_pwait_in {
    struct tarpc_in_arg common;

    tarpc_int                epfd;
    struct tarpc_epoll_event events<>;
    tarpc_int                maxevents;
    tarpc_int                timeout;
    tarpc_sigset_t           sigmask;
};

struct tarpc_epoll_pwait_out {
    struct tarpc_out_arg    common;

    tarpc_int               retval;

    struct tarpc_epoll_event events<>;
};

/*
 * Socket options
 */

enum option_type {
    OPT_INT             = 1,
    OPT_LINGER          = 2,
    OPT_TIMEVAL         = 3,
    OPT_MREQN           = 4,
    OPT_MREQ            = 5,
    OPT_MREQ6           = 6,
    OPT_IPADDR          = 7,
    OPT_IPADDR6         = 8,
    OPT_TCP_INFO        = 9,
    OPT_HANDLE          = 10,
    OPT_GROUP_REQ       = 11,
    OPT_IP_PKTOPTIONS   = 12,
    OPT_MREQ_SOURCE     = 13
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

struct option_value_mreq_source {
    uint32_t    imr_multiaddr;  /**< IP multicast group address */
    uint32_t    imr_interface;  /**< IP address of local interface */
    uint32_t    imr_sourceaddr; /**< IP address of multicast source */
};

struct tarpc_mreq_source {
    enum option_type    type;
    uint32_t            multiaddr;
    uint32_t            interface;
    uint32_t            sourceaddr;
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

    uint32_t    tcpi_rcv_rtt;
    uint32_t    tcpi_rcv_space;

    uint32_t    tcpi_total_retrans;
};

struct tarpc_group_req {
     tarpc_uint      gr_interface;
     struct tarpc_sa gr_group;
};

union option_value switch (option_type opttype) {
    case OPT_INT:           tarpc_int opt_int;
    case OPT_LINGER:        struct tarpc_linger opt_linger;
    case OPT_TIMEVAL:       struct tarpc_timeval opt_timeval;
    case OPT_MREQN:         struct option_value_mreqn opt_mreqn;
    case OPT_MREQ:          struct option_value_mreq opt_mreq;
    case OPT_MREQ_SOURCE:   struct option_value_mreq_source opt_mreq_source;
    case OPT_MREQ6:         struct option_value_mreq6 opt_mreq6;
    case OPT_IPADDR:        uint32_t opt_ipaddr;
    case OPT_IPADDR6:       uint32_t opt_ipaddr6[4];
    case OPT_TCP_INFO:      struct option_value_tcp_info opt_tcp_info;
    case OPT_HANDLE:        tarpc_int opt_handle;
    case OPT_GROUP_REQ:     struct tarpc_group_req opt_group_req;
    case OPT_IP_PKTOPTIONS: struct tarpc_cmsghdr opt_ip_pktoptions<>;
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
    IOCTL_TIMESPEC,
    IOCTL_IFREQ,
    IOCTL_IFCONF,
    IOCTL_ARPREQ,
    IOCTL_SGIO,
    IOCTL_HWTSTAMP_CONFIG,
    IOCTL_PTP_CLOCK_CAPS,
    IOCTL_PTP_SYS_OFFSET,
    IOCTL_PTP_SYS_OFFSET_EXTENDED,
    IOCTL_PTP_SYS_OFFSET_PRECISE
};

/* Access type of ioctl requests */
enum ioctl_access {
    IOCTL_RD,   /* read access (may include write) */
    IOCTL_WR    /* write-only access (no read) */
};

union ioctl_request switch (ioctl_type type) {
    case IOCTL_INT:      tarpc_int      req_int;
    case IOCTL_TIMEVAL:  tarpc_timeval  req_timeval;
    case IOCTL_TIMESPEC: tarpc_timespec req_timespec;
    case IOCTL_IFREQ:    tarpc_ifreq    req_ifreq;
    case IOCTL_IFCONF:   tarpc_ifconf   req_ifconf;
    case IOCTL_ARPREQ:   tarpc_arpreq   req_arpreq;
    case IOCTL_SGIO:     tarpc_sgio     req_sgio;

    case IOCTL_PTP_CLOCK_CAPS: tarpc_ptp_clock_caps
                                  req_ptp_clock_caps;
    case IOCTL_PTP_SYS_OFFSET: tarpc_ptp_sys_offset
                                  req_ptp_sys_offset;
    case IOCTL_PTP_SYS_OFFSET_EXTENDED: tarpc_ptp_sys_offset_extended
                                              req_ptp_sys_offset_extended;
    case IOCTL_PTP_SYS_OFFSET_PRECISE: tarpc_ptp_sys_offset_precise
                                              req_ptp_sys_offset_precise;
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

enum fcntl_type {
    FCNTL_UNKNOWN,
    FCNTL_INT,
    FCNTL_F_OWNER_EX

};

/** struct f_owner_ex */
struct tarpc_f_owner_ex {
    tarpc_int   type;
    tarpc_pid_t pid;
};

union fcntl_request switch (fcntl_type type) {
    case FCNTL_INT:         tarpc_int                req_int;
    case FCNTL_F_OWNER_EX:  struct tarpc_f_owner_ex  req_f_owner_ex;
};

struct tarpc_fcntl_in {
    struct tarpc_in_arg common;

    tarpc_int       fd;
    tarpc_int       cmd;
    fcntl_request   arg<>;
};

struct tarpc_fcntl_out {
    struct tarpc_out_arg common;

    fcntl_request   arg<>;
    tarpc_int       retval;
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

/* __sysv_signal() */

struct tarpc___sysv_signal_in {
    struct tarpc_in_arg common;

    tarpc_signum    signum;     /**< Signal */
    string          handler<>;  /**< Name of the signal handler function */
};

struct tarpc___sysv_signal_out {
    struct tarpc_out_arg    common;

    string handler<>;  /**< Name of the old signal handler function */
};

/* siginterrupt() */
struct tarpc_siginterrupt_in {
    struct tarpc_in_arg     common;

    tarpc_signum    signum; /**< The signal number */
    tarpc_int       flag;   /**< flag (on/off) */
};

struct tarpc_siginterrupt_out {
    struct tarpc_out_arg    common;

    tarpc_int   retval;
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

/* sigaltstack */

struct tarpc_sigaltstack_in {
    struct tarpc_in_arg     common;

    struct tarpc_stack_t    ss<>;   /**< New alternate signal stack */
    struct tarpc_stack_t    oss<>;  /**< State of existing alternate */
};

struct tarpc_sigaltstack_out {
    struct tarpc_out_arg    common;

    struct tarpc_stack_t    oss<>;  /**< State of existing alternate
                                         signal stack saved here if
                                         it's non-null */
    tarpc_int               retval;
};

/* kill() */

struct tarpc_kill_in {
    struct tarpc_in_arg common;

    tarpc_pid_t     pid;
    tarpc_signum    signum;
};

typedef struct tarpc_int_retval_out tarpc_kill_out;

/* pthread_kill() */

struct tarpc_pthread_kill_in {
    struct tarpc_in_arg common;

    tarpc_pthread_t tid;
    tarpc_signum    signum;
};

typedef struct tarpc_int_retval_out tarpc_pthread_kill_out;

/* call_tgkill() */

struct tarpc_call_tgkill_in {
    struct tarpc_in_arg common;

    tarpc_int       tgid;
    tarpc_int       tid;
    tarpc_signum    sig;
};

typedef struct tarpc_int_retval_out tarpc_call_tgkill_out;

/* call_gettid() */

typedef struct tarpc_void_in tarpc_call_gettid_in;
typedef struct tarpc_int_retval_out tarpc_call_gettid_out;

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

/* ta_kill_and_wait() */

struct tarpc_ta_kill_and_wait_in {
    struct tarpc_in_arg common;

    tarpc_pid_t     pid;
    tarpc_int       sig;
    tarpc_uint      timeout;
};

typedef struct tarpc_int_retval_out tarpc_ta_kill_and_wait_out;


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

/* sigset_cmp() */

struct tarpc_sigset_cmp_in {
    struct tarpc_in_arg common;

    tarpc_sigset_t      first_set;    /**< Handle of the first signal set */
    tarpc_sigset_t      second_set;   /**< Handle of the second signal set */
};

typedef struct tarpc_int_retval_out tarpc_sigset_cmp_out;

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

/* sigval_t, union sigval */
union tarpc_sigval switch (tarpc_bool pointer) {
    case 1: tarpc_ptr sival_ptr;
    case 0: tarpc_int sival_int;
};

/* siginfo_t */
struct tarpc_siginfo_t {
       tarpc_int        sig_signo;
       tarpc_int        sig_errno;
       tarpc_int        sig_code;
       tarpc_int        sig_trapno;
       tarpc_pid_t      sig_pid;
       tarpc_uid_t      sig_uid;
       tarpc_int        sig_status;
       tarpc_clock_t    sig_utime;
       tarpc_clock_t    sig_stime;
       tarpc_sigval     sig_value;
       tarpc_int        sig_int;
       tarpc_ptr        sig_ptr;
       tarpc_int        sig_overrun;
       tarpc_int        sig_timerid;
       tarpc_ptr        sig_addr;
       int64_t          sig_band;
       tarpc_int        sig_fd;
       tarpc_int        sig_addr_lsb;
};

/* siginfo_received() */

typedef struct tarpc_void_in tarpc_siginfo_received_in;

struct tarpc_siginfo_received_out {
    struct tarpc_out_arg  common;

    tarpc_siginfo_t     siginfo;
};

typedef struct tarpc_siginfo_received_out tarpc_siginfo_received_out;

/* signal_registrar_cleanup() */
typedef struct tarpc_void_in tarpc_signal_registrar_cleanup_in;

typedef struct tarpc_void_out tarpc_signal_registrar_cleanup_out;

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

/* pipe2() */

struct tarpc_pipe2_in {
    struct tarpc_in_arg common;

    tarpc_int   filedes<>;

    tarpc_int   flags;  /**< RPC_O_NONBLOCK and/or
                             RPC_O_CLOEXEC */
};

struct tarpc_pipe2_out {
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

/* chroot() */
struct tarpc_chroot_in {
    struct tarpc_in_arg common;

    char                path<>;
};

typedef struct tarpc_int_retval_out tarpc_chroot_out;

/* copy_ta_libs() */
struct tarpc_copy_ta_libs_in {
    struct tarpc_in_arg common;

    char                path<>;
};

typedef struct tarpc_int_retval_out tarpc_copy_ta_libs_out;

/* rm_ta_libs() */
struct tarpc_rm_ta_libs_in {
    struct tarpc_in_arg common;

    char                path<>;
};

typedef struct tarpc_int_retval_out tarpc_rm_ta_libs_out;

/* vlan_get_parent() */

struct tarpc_vlan_get_parent_in {
    struct tarpc_in_arg common;

    char ifname<>;
};

struct tarpc_vlan_get_parent_out {
    struct tarpc_out_arg    common;

    tarpc_int retval;

    char ifname<>;
};

/* bond_get_slaves() */
struct tarpc_ifname {
    char     ifname[16];    /**< Interface name */
};

struct tarpc_bond_get_slaves_in {
    struct tarpc_in_arg common;

    char      ifname<>;
};

struct tarpc_bond_get_slaves_out {
    struct tarpc_out_arg    common;

    tarpc_int retval;

    tarpc_ifname    slaves<>;
};

/* getenv() */
struct tarpc_getenv_in {
    struct tarpc_in_arg common;

    string name<>;
};

struct tarpc_getenv_out {
    struct tarpc_out_arg common;

    string val<>;
    tarpc_bool val_null;
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

/* exit() & _exit() */
struct tarpc_exit_in {
    struct tarpc_in_arg common;

    tarpc_int status;
};
typedef struct tarpc_exit_in tarpc__exit_in;

typedef struct tarpc_void_out tarpc_exit_out;
typedef struct tarpc_void_out tarpc__exit_out;

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

/* pthread_self() */
struct tarpc_pthread_self_in {
    struct tarpc_in_arg common;
};

struct tarpc_pthread_self_out {
    struct tarpc_out_arg    common;

    tarpc_pthread_t         retval;
};

typedef struct tarpc_pthread_self_in tarpc_pthread_self_in;
typedef struct tarpc_pthread_self_out tarpc_pthread_self_out;

/* pthread_cancel */
struct tarpc_pthread_cancel_in {
    struct tarpc_in_arg common;

    tarpc_pthread_t tid;
};

typedef struct tarpc_int_retval_out tarpc_pthread_cancel_out;

/* pthread_setcancelstate */
struct tarpc_pthread_setcancelstate_in {
    struct tarpc_in_arg common;

    tarpc_int state;
};

struct tarpc_pthread_setcancelstate_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    tarpc_int oldstate;
};

/* pthread_setcanceltype */
struct tarpc_pthread_setcanceltype_in {
    struct tarpc_in_arg common;

    tarpc_int type;
};

struct tarpc_pthread_setcanceltype_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    tarpc_int oldtype;
};

/* pthread_join */
struct tarpc_pthread_join_in {
    struct tarpc_in_arg common;

    tarpc_pthread_t tid;
};

struct tarpc_pthread_join_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    uint64_t  ret;
};

/* access */
struct tarpc_access_in {
    struct tarpc_in_arg common;
    char      path<>;
    tarpc_int mode;
};

typedef struct tarpc_int_retval_out tarpc_access_out;

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
    FUNC_PPOLL = 4,
    FUNC_EPOLL = 5,
    FUNC_EPOLL_PWAIT = 6,
    /** Value 7 is reserved */
    FUNC_DEFAULT_IOMUX = 8,
    FUNC_NO_IOMUX = 9
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

/**
 * @struct tarpc_pat_gen_arg
 * Arguments to pass to the pattern generator function.
 * Fields coefN can be used in different ways depending on a function.
 * This structure can also be modified, e.g. add some more fields.
 *
 * @param offset    Offset within pattern generator element, from which sender
 *                  must send and receiver must check the sequence.
 *                  It is used in generator functions which operates with
 *                  elements larger than a byte.
 * @param coef1,
 *        coef2,
 *        coef3     Arguments for a pattern generator function.
 */
struct tarpc_pat_gen_arg {
    uint32_t    offset;
    uint32_t    coef1;
    uint32_t    coef2;
    uint32_t    coef3;
};

typedef struct tarpc_pat_gen_arg tarpc_pat_gen_arg;

struct tarpc_pattern_sender_in {
    struct tarpc_in_arg common;

    tarpc_int   s;              /**< Socket to be used */
    char        fname<>;        /**< Name of function generating a
                                     pattern */
    char        swrapper<>;     /**< Send function wrapper (may be empty) */
    tarpc_ptr   swrapper_data;  /**< Data which should be passed to
                                     send function wrapper */
    iomux_func  iomux;          /**< Iomux function to be used **/
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
    uint32_t    time2run;       /**< How long to run (in seconds;
                                     the function can finish earlier
                                     if @b time2wait is positive) */
    uint32_t    time2wait;      /**< Maximum time to wait for writability
                                     before stopping sending (in
                                     milliseconds; if @c 0, the
                                     function will wait until @b time2run
                                     expires) */
    uint64_t    total_size;     /**< How many bytes to send (ignored
                                     if @c 0) */

    tarpc_bool  ignore_err;     /**< Ignore errors while run */

    char        pollerr_handler<>;      /**< Handler to call if @c POLLERR
                                             event is got (optional; if not
                                             set, receiving @c POLLERR
                                             instead of @c POLLOUT will
                                             result in error) */
    tarpc_ptr   pollerr_handler_data;   /**< Data to pass to @c POLLERR
                                             handler */

    tarpc_pat_gen_arg gen_arg;  /**< Pattern generator function
                                     arguments */
};

struct tarpc_pattern_sender_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;         /**< 0 (success) or -1 (failure) */

    uint64_t    bytes;          /**< Number of sent bytes */
    tarpc_bool  func_failed;    /**< TRUE if it was data transmitting
                                     function who failed */

    tarpc_pat_gen_arg gen_arg;  /**< Pattern generator function
                                     arguments */
};

struct tarpc_pattern_receiver_in {
    struct tarpc_in_arg common;

    tarpc_int   s;              /**< Socket to be used */
    char        fname<>;        /**< Pattern generating function */
    iomux_func  iomux;          /**< Iomux function to be used **/
    uint64_t    exp_received;   /**< How many bytes should be received
                                     (ignored if @c 0; if > @c 0, receiving
                                      will be stopped after reading this
                                      number of bytes) */
    uint32_t    time2run;       /**< How long to run (in seconds;
                                     the function can finish earlier
                                     if @b time2wait is positive) */
    uint32_t    time2wait;      /**< Maximum time to wait for readability
                                     before stopping receiving (in
                                     milliseconds; if @c 0, the
                                     function will wait until @b time2run
                                     expires) */
    tarpc_bool  ignore_pollerr; /**< If @c TRUE, @c POLLERR event should be
                                     ignored if it arrives instead of
                                     @c POLLIN */

    tarpc_pat_gen_arg gen_arg;  /**< Pattern generator function
                                     arguments */
};

typedef struct tarpc_pattern_sender_out tarpc_pattern_receiver_out;

struct tarpc_create_process_in {
    struct tarpc_in_arg common;

    tarpc_int  flags;           /**< RCF_RPC_SERVER_GET_* flags */
    char       name<>;          /**< RPC server name */
};

struct tarpc_create_process_out {
    struct tarpc_out_arg common;

    int32_t     pid;
};

/* vfork() */
struct tarpc_vfork_in {
    struct tarpc_in_arg common;

    char        name<>;          /**< RPC server name */
    uint32_t    time_to_wait;    /**< How much time to wait after
                                      vfork() call before
                                      returning from the function,
                                      in milliseconds */
};

struct tarpc_vfork_out {
    struct tarpc_out_arg common;

    int32_t     pid;
    uint32_t    elapsed_time;       /**< How much time elapsed until
                                         vfork() returned,
                                         in milliseconds */
};

struct tarpc_thread_create_in {
    struct tarpc_in_arg common;

    char name<>;
};

struct tarpc_thread_create_out {
    struct tarpc_out_arg common;

    tarpc_pthread_t   tid;
    tarpc_int         retval;
};

struct tarpc_thread_cancel_in {
    struct tarpc_in_arg common;

    tarpc_pthread_t    tid;
};

typedef struct tarpc_int_retval_out tarpc_thread_cancel_out;

struct tarpc_thread_join_in {
    struct tarpc_in_arg common;

    tarpc_pthread_t    tid;
};

typedef struct tarpc_int_retval_out tarpc_thread_join_out;

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

/* sendfile_via_splice() */
struct tarpc_sendfile_via_splice_in {
    struct tarpc_in_arg common;

    tarpc_int           out_fd;
    tarpc_int           in_fd;
    tarpc_off_t         offset<>;
    tarpc_size_t        count;
    tarpc_bool          force64;
};

struct tarpc_sendfile_via_splice_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t   retval;     /**< Returned length */
    tarpc_off_t     offset<>;   /**< new offset returned by the function */
};

/* splice() */
struct tarpc_splice_in {
    struct tarpc_in_arg common;

    tarpc_int           fd_in;
    tarpc_off_t         off_in<>;
    tarpc_int           fd_out;
    tarpc_off_t         off_out<>;
    tarpc_size_t        len;
    tarpc_int           flags;
};

struct tarpc_splice_out {
    struct tarpc_out_arg  common;

    tarpc_ssize_t   retval;     /**< Returned length */
    tarpc_off_t     off_in<>;   /**< new in offset returned by the
                                     function */
    tarpc_off_t     off_out<>;  /**< new out offset returned by the
                                     function */
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
    tarpc_bool      return_data;    /**< If @c TRUE, return data sent
                                         when overfilling buffers */
};

struct tarpc_overfill_buffers_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */

    uint64_t    bytes;      /**< Number of sent bytes */
    uint8_t     data<>;     /**< Sent data */
};

struct tarpc_drain_fd_in {
    struct tarpc_in_arg common;
    tarpc_int    fd;
    tarpc_size_t size;
    tarpc_int    time2wait;
    tarpc_uint   duration;
};

struct tarpc_drain_fd_out {
    struct tarpc_out_arg common;
    tarpc_int   retval;
    uint64_t    read;
};

/* read_fd() */
struct tarpc_read_fd_in {
    struct tarpc_in_arg common;
    tarpc_int    fd;
    tarpc_size_t size;
    tarpc_int    time2wait;
    tarpc_size_t amount;
};

struct tarpc_read_fd_out {
    struct tarpc_out_arg common;
    tarpc_int   retval;
    uint8_t     buf<>;
};

/* iomux_splice() */
struct tarpc_iomux_splice_in {
    struct tarpc_in_arg common;

    tarpc_int       fd_in;
    tarpc_int       fd_out;
    tarpc_size_t    len;
    tarpc_int       flags;
    uint32_t        time2run;        /**< How long run (in seconds) */
    iomux_func      iomux;
};

struct tarpc_iomux_splice_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */
};

/* overfill_fd() */
struct tarpc_overfill_fd_in {
    struct tarpc_in_arg common;

    tarpc_int       write_end;
};

struct tarpc_overfill_fd_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */

    uint64_t    bytes;      /**< Number of sent bytes */
};

/* multiple_iomux() */
struct tarpc_multiple_iomux_in {
    struct tarpc_in_arg common;

    tarpc_int   fd;         /* On which fd to call iomux */
    iomux_func  iomux;      /* Which iomux to call */
    tarpc_int   events;     /* With which events iomux should
                               be called */
    tarpc_int   count;      /* How many times to call iomux */
    tarpc_int   duration;   /* Call iomux during a specified time */
    tarpc_int   exp_rc;     /* Expected iomux return value */
};

struct tarpc_multiple_iomux_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */
    tarpc_int   number;     /**< Number of the last successfully
                                 called iomux */
    tarpc_int   last_rc;    /**< Value returned by the last call
                                 of iomux */
    tarpc_int   zero_rc;    /**< Number of zero code returned by iomux */
};

/* iomux_create_state() */
struct tarpc_iomux_create_state_in {
    struct tarpc_in_arg common;

    iomux_func          iomux;      /**< Which iomux to call */
};

struct tarpc_iomux_create_state_out {
    struct tarpc_out_arg common;

    tarpc_iomux_state   iomux_st;   /**< The multiplexer context */
    tarpc_int           retval;     /**< 0 (success) or -1 (failure) */
};

/* multiple_iomux_wait() */
struct tarpc_multiple_iomux_wait_in {
    struct tarpc_in_arg common;

    tarpc_int           fd;         /**< On which fd to call iomux */
    iomux_func          iomux;      /**< Which iomux to call */
    tarpc_iomux_state   iomux_st;   /**< The multiplexer context */
    tarpc_int           events;     /**< With which events iomux should
                                         be called */
    tarpc_int           count;      /**< How many times to call iomux */
    tarpc_int           duration;   /**< Call iomux during a specified time */
    tarpc_int           exp_rc;     /**< Expected iomux return value */
};

struct tarpc_multiple_iomux_wait_out {
    struct tarpc_out_arg common;

    tarpc_int   retval;     /**< 0 (success) or -1 (failure) */
    tarpc_int   number;     /**< Number of the last successfully
                                 called iomux */
    tarpc_int   last_rc;    /**< Value returned by the last call
                                 of iomux */
    tarpc_int   zero_rc;    /**< Number of zero code returned by iomux */
};

/* iomux_close_state() */
struct tarpc_iomux_close_state_in {
    struct tarpc_in_arg common;

    iomux_func          iomux;      /**< Which iomux to call */
    tarpc_iomux_state   iomux_st;   /**< The multiplexer context */
};

typedef struct tarpc_int_retval_out tarpc_iomux_close_state_out;

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

/* sysconf() */
struct tarpc_sysconf_in {
    struct tarpc_in_arg  common;

    tarpc_int            name;      /**< sysconf() constant name */
};

struct tarpc_sysconf_out {
    struct tarpc_out_arg  common;

    int64_t               retval;   /**< Value of the system resource */
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
    TARPC_MCAST_ADD_DROP = 1,   /* sockopt IP_ADD/DROP_MULTICAST */
    TARPC_MCAST_JOIN_LEAVE = 2, /* sockopt MCAST_JOIN/LEAVE_GROUP */
    TARPC_MCAST_WSA = 3,        /* WSAJoinLeaf, no leave */
    TARPC_MCAST_SOURCE_ADD_DROP = 4,   /* sockopt IP_ADD/DROP_SOURCE_MEMBERSHIP */
    TARPC_MCAST_SOURCE_JOIN_LEAVE = 5  /* sockopt MCAST_JOIN/LEAVE_SOURCE_GROUP */
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

struct tarpc_mcast_source_join_leave_in {
    struct tarpc_in_arg common;

    tarpc_int                   fd;           /**< TA-local socket */
    tarpc_int                   family;       /**< Address family */
    uint8_t                     multiaddr<>;  /**< Multicast group address */
    uint8_t                     sourceaddr<>; /**< Source address */
    tarpc_int                   ifindex;      /**< Interface index */
    tarpc_bool                  leave_group;  /**< Leave the group */
    tarpc_joining_method        how;          /**< How to join the group */
};

struct tarpc_mcast_source_join_leave_out {
    struct tarpc_out_arg common;
    tarpc_int            retval;
};


struct tarpc_power_sw_in {
    struct tarpc_in_arg common;

    tarpc_int           type;   /**< Device type */
    string              dev<>;  /**< Device name */
    tarpc_int           mask;   /**< Power lines bitmask */
    tarpc_int           cmd;    /**< Power switch command */
};

typedef struct tarpc_int_retval_out tarpc_power_sw_out;

/* dl functions */
/* dlopen() */
struct tarpc_ta_dlopen_in {
    struct tarpc_in_arg common;
    string              filename<>; /**< Dynamic library file name */
    tarpc_int           flag;       /**< dlopen() function flags */
};

struct tarpc_ta_dlopen_out {
    struct tarpc_out_arg    common;
    tarpc_dlhandle          retval; /**< Dynamic library "handle" */
};

/* dlerror() */
struct tarpc_ta_dlerror_in {
    struct tarpc_in_arg common;
};

struct tarpc_ta_dlerror_out {
    struct tarpc_out_arg    common;
    string                  retval<>; /**< Human readable error message */
};

/* dlsym() */
struct tarpc_ta_dlsym_in {
    struct tarpc_in_arg common;
    tarpc_dlhandle      handle;     /**< Dynamic library handle */
    string              symbol<>;   /**< Symbol name */
};

struct tarpc_ta_dlsym_out {
    struct tarpc_out_arg    common;
    tarpc_dlsymaddr         retval; /**< Symbol address */
};

typedef struct tarpc_ta_dlsym_in tarpc_ta_dlsym_call_in;
typedef struct tarpc_int_retval_out tarpc_ta_dlsym_call_out;

/* dlclose() */
struct tarpc_ta_dlclose_in {
    struct tarpc_in_arg common;
    tarpc_dlhandle      handle;     /**< Dynamic library handle */
};

typedef struct tarpc_int_retval_out tarpc_ta_dlclose_out;

/* socket_connect_close() */
struct tarpc_socket_connect_close_in {
    struct tarpc_in_arg common;

    tarpc_int           domain;
    struct tarpc_sa     addr;
    tarpc_socklen_t     len;
    uint32_t            time2run;
};
typedef struct tarpc_int_retval_out tarpc_socket_connect_close_out;

/* socket_listen_close() */
struct tarpc_socket_listen_close_in {
    struct tarpc_in_arg common;

    tarpc_int           domain;
    struct tarpc_sa     addr;
    tarpc_socklen_t     len;
    uint32_t            time2run;
};
typedef struct tarpc_int_retval_out tarpc_socket_listen_close_out;

/* vfork_pipe_exec() */
struct tarpc_vfork_pipe_exec_in {
    struct tarpc_in_arg common;

    tarpc_bool      use_exec;
};

typedef struct tarpc_int_retval_out tarpc_vfork_pipe_exec_out;

struct tarpc_execve_gen_in {
    struct tarpc_in_arg common;
    string              filename<>;
    struct tarpc_iovec  argv<>;
    struct tarpc_iovec  envp<>;
};

typedef struct tarpc_int_retval_out tarpc_execve_gen_out;

/* namespace_id2str() */
struct tarpc_namespace_id2str_in {
    struct tarpc_in_arg     common;
    tarpc_ptr               id;
};

struct tarpc_namespace_id2str_out {
    struct tarpc_out_arg    common;
    char                    str<>;
    tarpc_int               retval;
};

/* release_rpc_ptr() */
struct tarpc_release_rpc_ptr_in {
    struct tarpc_in_arg     common;
    tarpc_ptr               ptr;
    string                  ns_string<>;
};

typedef struct tarpc_void_out tarpc_release_rpc_ptr_out;

/* get_rw_ability() */
struct tarpc_get_rw_ability_in {
    struct tarpc_in_arg     common;
    tarpc_int               sock;
    tarpc_int               timeout;
    tarpc_bool              check_rd;
};

struct tarpc_get_rw_ability_out {
    struct tarpc_out_arg    common;
    tarpc_int               retval;
};

/* rpcserver_plugin_enable() */
struct tarpc_rpcserver_plugin_enable_in {
    struct tarpc_in_arg     common;
    string                  install<>;
    string                  action<>;
    string                  uninstall<>;
};

typedef struct tarpc_int_retval_out tarpc_rpcserver_plugin_enable_out;

/* rpcserver_plugin_disable() */
typedef struct tarpc_void_in tarpc_rpcserver_plugin_disable_in;
typedef struct tarpc_int_retval_out tarpc_rpcserver_plugin_disable_out;

struct tarpc_send_flooder_iomux_in {
    struct tarpc_in_arg common;
    tarpc_int           sock;
    iomux_func          iomux;
    tarpc_send_function send_func;
    tarpc_bool          msg_dontwait;
    uint32_t            packet_size;
    tarpc_int           duration;
};

struct tarpc_send_flooder_iomux_out {
    struct tarpc_out_arg    common;
    uint64_t packets;
    uint32_t errors;
    int      retval;
};

/* copy_fd2fd function arguments. */
struct tarpc_copy_fd2fd_in {
    struct tarpc_in_arg common;

    int             out_fd;     /**< File descriptor opened for writing.
                                     Can be a socket */
    int             in_fd;      /**< File descriptor opened for reading.
                                     Can be a socket */
    int             timeout;    /**< Number of milliseconds that function
                                     should block waiting for @b in_fd to
                                     become ready to read the next portion
                                     of data while all requested data will
                                     not be read */
    uint64_t        count;      /**< Number of bytes to copy between the
                                     file descriptors. If @c 0 then all
                                     available data should be copied, i.e.
                                     while @c EOF will not be gotten */
};
struct tarpc_copy_fd2fd_out {
    struct tarpc_out_arg  common;

    int64_t         retval;     /**< Number of bytes written to @b out_fd,
                                     or @c -1 on error. */
};

/* remove_dir_with_files() */
struct tarpc_remove_dir_with_files_in {
    struct tarpc_in_arg common;

    char                path<>;
};

typedef struct tarpc_int_retval_out tarpc_remove_dir_with_files_out;

/* clock_gettime() */

enum tarpc_clock_id_type {
    TARPC_CLOCK_ID_NAMED, /**< Named constant from rpc_clock_id */
    TARPC_CLOCK_ID_FD     /**< File descriptor of dynamic clock device */
};

struct tarpc_clock_gettime_in {
    struct tarpc_in_arg common;

    tarpc_clock_id_type id_type;
    tarpc_int id;
};

struct tarpc_clock_gettime_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    tarpc_timespec ts;
};

/* clock_settime() */

struct tarpc_clock_settime_in {
    struct tarpc_in_arg common;

    tarpc_clock_id_type id_type;
    tarpc_int id;
    tarpc_timespec ts;
};

typedef struct tarpc_int_retval_out tarpc_clock_settime_out;

/* clock_adjtime() */

struct tarpc_timex {
    tarpc_uint modes;
    tarpc_int status;
    int64_t offset;
    int64_t freq;
    tarpc_timeval time;
};

struct tarpc_clock_adjtime_in {
    struct tarpc_in_arg common;

    tarpc_clock_id_type id_type;
    tarpc_int id;
    tarpc_timex params;
};

struct tarpc_clock_adjtime_out {
    struct tarpc_out_arg common;

    tarpc_int retval;
    tarpc_timex params;
};

program tarpc
{
    version ver0
    {
        RPC_DEF(rpc_find_func)
        RPC_DEF(rpc_is_op_done)
        RPC_DEF(rpc_is_alive)
        RPC_DEF(setlibname)

        RPC_DEF(get_sizeof)
        RPC_DEF(get_addrof)
        RPC_DEF(protocol_info_cmp)

        RPC_DEF(get_var)
        RPC_DEF(set_var)

        RPC_DEF(create_process)
        RPC_DEF(vfork)
        RPC_DEF(thread_create)
        RPC_DEF(thread_cancel)
        RPC_DEF(thread_join)
        RPC_DEF(execve)
        RPC_DEF(execve_gen)
        RPC_DEF(getpid)
        RPC_DEF(pthread_self)
        RPC_DEF(pthread_cancel)
        RPC_DEF(pthread_setcancelstate)
        RPC_DEF(pthread_setcanceltype)
        RPC_DEF(pthread_join)
        RPC_DEF(gettimeofday)
        RPC_DEF(clock_gettime)
        RPC_DEF(clock_settime)
        RPC_DEF(clock_adjtime)
        RPC_DEF(gethostname)

        RPC_DEF(access)

        RPC_DEF(malloc)
        RPC_DEF(free)
        RPC_DEF(get_addr_by_id)
        RPC_DEF(raw2integer)
        RPC_DEF(integer2raw)
        RPC_DEF(memalign)
        RPC_DEF(mmap)
        RPC_DEF(munmap)
        RPC_DEF(madvise)
        RPC_DEF(memcmp)

        RPC_DEF(socket)
        RPC_DEF(wsa_socket)
        RPC_DEF(wsa_startup)
        RPC_DEF(wsa_cleanup)
        RPC_DEF(duplicate_socket)
        RPC_DEF(duplicate_handle)
        RPC_DEF(dup)
        RPC_DEF(dup2)
        RPC_DEF(dup3)
        RPC_DEF(close)
        RPC_DEF(shutdown)
        RPC_DEF(te_fstat)
        RPC_DEF(te_fstat64)
        RPC_DEF(te_stat)

        RPC_DEF(link)
        RPC_DEF(symlink)
        RPC_DEF(unlink)
        RPC_DEF(rename)
        RPC_DEF(mkdir)
        RPC_DEF(mkdirp)
        RPC_DEF(rmdir)
        RPC_DEF(fstatvfs)
        RPC_DEF(statvfs)
        RPC_DEF(struct_dirent_props)
        RPC_DEF(opendir)
        RPC_DEF(readdir)
        RPC_DEF(closedir)

        RPC_DEF(read)
        RPC_DEF(pread)
        RPC_DEF(write)
        RPC_DEF(pwrite)
        RPC_DEF(read_via_splice)
        RPC_DEF(write_via_splice)
        RPC_DEF(write_and_close)

        RPC_DEF(readbuf)
        RPC_DEF(writebuf)
        RPC_DEF(recvbuf)
        RPC_DEF(sendbuf)
        RPC_DEF(send_msg_more)
        RPC_DEF(send_one_byte_many)

        RPC_DEF(readv)
        RPC_DEF(preadv)
        RPC_DEF(writev)
        RPC_DEF(pwritev)

        RPC_DEF(lseek)

        RPC_DEF(fsync)

        RPC_DEF(send)
        RPC_DEF(recv)

        RPC_DEF(sendto)
        RPC_DEF(recvfrom)

        RPC_DEF(sendmsg)
        RPC_DEF(recvmsg)

        RPC_DEF(recvmmsg_alt)
        RPC_DEF(sendmmsg_alt)

        RPC_DEF(bind)
        RPC_DEF(check_port_is_free)
        RPC_DEF(connect)
        RPC_DEF(listen)
        RPC_DEF(accept)
        RPC_DEF(accept4)

        RPC_DEF(fd_set_new)
        RPC_DEF(fd_set_delete)
        RPC_DEF(do_fd_set)
        RPC_DEF(do_fd_zero)
        RPC_DEF(do_fd_clr)
        RPC_DEF(do_fd_isset)

        RPC_DEF(select)
        RPC_DEF(pselect)
        RPC_DEF(poll)
        RPC_DEF(ppoll)

        RPC_DEF(epoll_create)
        RPC_DEF(epoll_create1)
        RPC_DEF(epoll_ctl)
        RPC_DEF(epoll_wait)
        RPC_DEF(epoll_pwait)

        RPC_DEF(iomux_create_state)
        RPC_DEF(multiple_iomux_wait)
        RPC_DEF(iomux_close_state)
        RPC_DEF(multiple_iomux)

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
        RPC_DEF(siginterrupt)
        RPC_DEF(sysv_signal)
        RPC_DEF(__sysv_signal)
        RPC_DEF(sigaction)
        RPC_DEF(sigaltstack)
        RPC_DEF(kill)
        RPC_DEF(pthread_kill)
        RPC_DEF(call_gettid)
        RPC_DEF(call_tgkill)
        RPC_DEF(ta_kill_death)
        RPC_DEF(ta_kill_and_wait)

        RPC_DEF(sigset_new)
        RPC_DEF(sigset_delete)
        RPC_DEF(sigprocmask)
        RPC_DEF(sigset_cmp)
        RPC_DEF(sigemptyset)
        RPC_DEF(sigfillset)
        RPC_DEF(sigaddset)
        RPC_DEF(sigdelset)
        RPC_DEF(sigismember)
        RPC_DEF(sigpending)
        RPC_DEF(sigsuspend)
        RPC_DEF(sigreceived)
        RPC_DEF(siginfo_received)
        RPC_DEF(signal_registrar_cleanup)

        RPC_DEF(gethostbyname)
        RPC_DEF(gethostbyaddr)

        RPC_DEF(getaddrinfo)
        RPC_DEF(freeaddrinfo)

        RPC_DEF(pipe)
        RPC_DEF(pipe2)
        RPC_DEF(socketpair)
        RPC_DEF(open)
        RPC_DEF(open64)
        RPC_DEF(fopen)
        RPC_DEF(fdopen)
        RPC_DEF(fclose)
        RPC_DEF(te_shell_cmd)
        RPC_DEF(system)
        RPC_DEF(chroot)
        RPC_DEF(copy_ta_libs)
        RPC_DEF(rm_ta_libs)
        RPC_DEF(vlan_get_parent)
        RPC_DEF(bond_get_slaves)
        RPC_DEF(waitpid)
        RPC_DEF(fileno)
        RPC_DEF(getpwnam)
        RPC_DEF(exit)
        RPC_DEF(_exit)
        RPC_DEF(getuid)
        RPC_DEF(geteuid)
        RPC_DEF(setuid)
        RPC_DEF(seteuid)
        RPC_DEF(getenv)
        RPC_DEF(setenv)
        RPC_DEF(unsetenv)
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
        RPC_DEF(pattern_sender)
        RPC_DEF(pattern_receiver)
        RPC_DEF(wait_readable)
        RPC_DEF(recv_verify)
        RPC_DEF(flooder)
        RPC_DEF(echoer)
        RPC_DEF(iomux_splice)

        RPC_DEF(sendfile)
        RPC_DEF(sendfile_via_splice)
        RPC_DEF(splice)
        RPC_DEF(socket_to_file)

        RPC_DEF(ftp_open)
        RPC_DEF(ftp_close)

        RPC_DEF(overfill_buffers)
        RPC_DEF(overfill_fd)
        RPC_DEF(drain_fd)
        RPC_DEF(read_fd)

        RPC_DEF(create_event)
        RPC_DEF(create_event_with_bit)
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

        RPC_DEF(create_window)
        RPC_DEF(destroy_window)
        RPC_DEF(wsa_async_select)
        RPC_DEF(peek_message)
        RPC_DEF(wsa_join_leaf)

        RPC_DEF(setrlimit)
        RPC_DEF(getrlimit)
        RPC_DEF(sysconf)

        RPC_DEF(mcast_join_leave)
        RPC_DEF(mcast_source_join_leave)

        RPC_DEF(power_sw)

        RPC_DEF(ta_dlopen)
        RPC_DEF(ta_dlerror)
        RPC_DEF(ta_dlsym)
        RPC_DEF(ta_dlsym_call)
        RPC_DEF(ta_dlclose)

        RPC_DEF(socket_connect_close)
        RPC_DEF(socket_listen_close)

        RPC_DEF(vfork_pipe_exec)

        RPC_DEF(namespace_id2str)
        RPC_DEF(release_rpc_ptr)

        RPC_DEF(get_rw_ability)

        RPC_DEF(rpcserver_plugin_enable)
        RPC_DEF(rpcserver_plugin_disable)
        RPC_DEF(send_flooder_iomux)

        RPC_DEF(copy_fd2fd)

        RPC_DEF(remove_dir_with_files)
    } = 1;
} = 1;
