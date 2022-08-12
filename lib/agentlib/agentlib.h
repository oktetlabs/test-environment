/** @file
 * @brief Agent support
 *
 * Common agent routines
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_AGENTLIB_H__
#define __TE_AGENTLIB_H__

#include "agentlib_defs.h"

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "tq_string.h"

/**
 * Get parent device name of VLAN interface.
 * If passed interface is not VLAN, method sets 'parent' to empty string
 * and return success.
 *
 * @param ifname        interface name
 * @param parent        location of parent interface name,
 *                      IF_NAMESIZE buffer length(OUT)
 *
 * @return status
 */
extern te_errno ta_vlan_get_parent(const char *ifname, char *parent);

/**
 * Get slaves devices names for bonding interface.
 * If passed interface is not bonding, method sets 'slaves_num' to zero
 * and return success.
 *
 * @param ifname        Interface name
 * @param slaves        Where to save slaves interfaces names
 * @param slaves_num    Where to save number of slaves interfaces
 *                      (may be @c NULL)
 * @param is_team       If not @c NULL, will be set to @c TRUE if
 *                      this is teaming device, and to @c FALSE
 *                      otherwise.
 *
 * @return Status code.
 */
extern te_errno ta_bond_get_slaves(const char *ifname,
                                   tqh_strings *slaves,
                                   int *slaves_num,
                                   te_bool *is_team);

#if defined(ENABLE_FTP)

/**
 * Open FTP connection for reading/writing the file.
 *
 * @param uri           FTP uri: ftp://user:password@server/directory/file
 * @param flags         O_RDONLY or O_WRONLY
 * @param passive       if 1, passive mode
 * @param offset        file offset
 * @param sock          pointer on socket
 *
 * @return file descriptor, which may be used for reading/writing data
 */
extern int ftp_open(const char *uri, int flags, int passive,
                    int offset, int *sock);

/**
 * Close FTP control connection.
 *
 * @param control_socket socket to close
 *
 * @retval 0 success
 * @retval -1 failure
 */
extern int ftp_close(int control_socket);

#endif /* ENABLE_FTP */


/**
 * Initialize process management subsystem
 */
extern te_errno ta_process_mgmt_init(void);

/**
 * waitpid() analogue, with the same parameters/return value.
 * Only WNOHANG option is supported for now.
 * Process groups are not supported for now.
 */
extern pid_t ta_waitpid(pid_t pid, int *status, int options);

/**
 * system() analogue, with the same parameters/return value.
 *
 * @sa ta_system_fmt
 */
extern int ta_system(const char *cmd);

/**
 * system() analogue, with the same return value. The function builds
 * a command line with a printf-like format string.
 *
 * @param fmt Command format string
 * @param ... Format string arguments
 *
 * @sa ta_system
 */
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
extern int ta_system_fmt(const char *fmt, ...);

/**
 * popen('r') analogue, with slightly modified parameters.
 */
extern te_errno ta_popen_r(const char *cmd, pid_t *cmd_pid, FILE **f);

/**
 * Perform cleanup actions for ta_popen_r() function.
 */
extern te_errno ta_pclose_r(pid_t cmd_pid, FILE *f);

/**
 * Kill a child process.
 *
 * @param pid PID of the child to be killed
 *
 * @return Status code
 * @retval 0 child was exited or killed successfully
 * @retval -1 there is no such child.
 *
 * @sa ta_kill_and_wait
 */
extern int ta_kill_death(pid_t pid);

/**
 * Kill a child process and wait for process to change state
 *
 * @param pid           PID of the child process to be killed
 * @param sig           Signal to be sent to child process
 * @param timeout_s     Time to wait for process to change state
 *
 * @return Status code
 * @retval  0   Successful result
 * @retval -1   Failed to kill the child process
 * @retval -2   Timed out to wait for changed state of the child process
 *
 * @sa ta_kill_death
 */
extern int ta_kill_and_wait(pid_t pid, int sig, unsigned int timeout_s);

/**
 * Create the directory(ies), if they do not already exist
 *
 * @param path      Path of the directory to be created
 * @param mode      The permission bits to assign to a new directory(ies)
 *
 * @return Status code
 * @retval @c 0     The directory has been created successfully or existing
 */
extern te_errno mkdirp(const char *path, int mode);

#if defined(ENABLE_TELEPHONY)
#include "telephony.h"
#endif /* ENABLE_TELEPHONY */

#if defined(ENABLE_POWER_SW)
/**
 * Turn ON, turn OFF or restart power lines specified by mask
 *
 * @param type      power switch device type tty/parport
 * @param dev       power switch device name
 * @param mask      power lines bitmask
\ * @param cmd       power switch command turn ON, turn OFF or restart
 *
 * @return          0, otherwise -1
 */
extern int power_sw(int type, const char *dev, int mask, int cmd);
#endif /* ENABLE_POWER_SW */

#if defined(ENABLE_UPNP)
#include "tarpc_upnp_cp.h"
#endif /* ENABLE_UPNP */


/** @defgroup rcf_ch_addr Command Handler: Symbol name and address resolver support
 * @ingroup rcf_ch
 * @{
 */

/**
 * Register symbol table
 *
 * @param entries  An array of symbol entries, the last element must have
 *                 NULL name
 * @note The @p entries must point to a static memory, as it is not copied
 *       by the function
 */
extern te_errno rcf_ch_register_symbol_table(
    const rcf_symbol_entry *entries);

/**
 * This function may be used by Portable Commands Handler to resolve
 * name of the variable or function to its address if rcf_ch_vread,
 * rcf_ch_vwrite or rcf_ch_call function returns -1. In this case
 * default command processing is performed by caller: it is assumed that
 * variable or function are in TA address space and variable is
 * unsigned 32 bit integer.
 *
 * @param name          symbol name
 * @param is_func       if TRUE, function name is required
 *
 * @return symbol address or NULL
 */
extern void *rcf_ch_symbol_addr(const char *name, te_bool is_func);

/**
 * This function may be used by Portable Commands Handler to symbol address
 * to its name.
 *
 * @param addr          symbol address
 *
 * @return symbol name or NULL
 */
extern const char *rcf_ch_symbol_name(const void *addr);

/**@} <!-- END rcf_ch_addr --> */


/**
 * This function is an equivalent for pthread_atfork(), but it sets up
 * hooks to be called _explicitly_ around vfork() via run_vhook_hooks().
 *
 * @param prepare   A hook to run before vfork()
 * @param child     A hook to run after vfork() in the child process
 * @param parent    A hook to run after vfork() in the parent process
 *
 * @note @p child and @p parent hooks need to obey all restrictions imposed
 * by vfork()
 */
extern te_errno register_vfork_hook(void (*prepare)(void),
                                    void (*child)(void),
                                    void (*parent)(void));

/** Phases for running vfork hooks, as by run_vfork_hooks() */
enum vfork_hook_phase {
    VFORK_HOOK_PHASE_PREPARE, /**< Before vfork() */
    VFORK_HOOK_PHASE_CHILD,   /**< After vfork() in child */
    VFORK_HOOK_PHASE_PARENT,  /**< After vfork() in parent */
    VFORK_HOOK_N_PHASES,      /**< Total number of phases */
};

/**
 * Run hooks registered by register_vfork_hook()
 *
 * @param phase   Hook phase (see #vfork_hook_phase)
 * @note This function is merely a convenience routine, it does not
 * in itself have anything to do with vfork(), so it's totally the caller's
 * responsibility to call it at appropriate places.
 * The typical scenario would look like this:
 * ~~~~~~~~~~
 * run_vfork_hook(VFORK_HOOK_PHASE_PREPARE);
 * child = vfork();
 * if (child == -1)
 * {
 *    run_vfork_hook(VFORK_HOOK_PHASE_PARENT);
 *    ....
 *    return error;
 * }
 * if (child == 0)
 * {
 *    run_vfork_hook(VFORK_HOOK_PHASE_CHILD);
 *    ....
 *    _exit(0);
 * }
 * run_vfork_hook(VFORK_HOOK_PHASE_PARENT);
 * ~~~~~~~~~~
 */
extern void run_vfork_hooks(enum vfork_hook_phase phase);

/**
 * Check that a given TCP or UDP port is not bound.
 *
 * @param socket_family     Socket family to use for checking, @c AF_INET
 *                          for IPv4, @c AF_INET6 for IPv6 or @c 0 for IPv6
 *                          with fallback to IPv4 if IPv6 is not supported.
 * @param socket_type       Socket type to use, @c SOCK_STREAM, @c SOCK_DGRAM,
 *                          or @c 0 to check both.
 * @param port              Port number in host endian
 *
 * @return @c TRUE - Port is free
 */
extern te_bool agent_check_l4_port_is_free(int socket_family, int socket_type,
                                           uint16_t port);

/**
 * Allocate a TCP/UDP port for the TA.
 *
 * @param socket_family     Socket family to use, @c AF_INET
 *                          for IPv4, @c AF_INET6 for IPv6 or @c 0 for IPv6
 *                          with fallback to IPv4 if IPv6 is not supported.
 * @param socket_type       Socket type to use, @c SOCK_STREAM, @c SOCK_DGRAM,
 *                          or @c 0 to check both.
 * @param port[out]         Port number in host endian
 *
 * @return                  Status code
 */
extern te_errno agent_alloc_l4_port(int socket_family, int socket_type,
                                    uint16_t *port);

/**
 * Free a TCP/UDP port for the TA.
 * The API is used to free the ports allocated by agent_alloc_l4_port().
 *
 * @param port              Port number in host endian
 */
extern void agent_free_l4_port(uint16_t port);

/**
 * Allocate the specified TCP/UDP port for TA.
 *
 * @param socket_family     Socket family to use, @c AF_INET
 *                          for IPv4, @c AF_INET6 for IPv6 or @c 0 for IPv6
 *                          with fallback to IPv4 if IPv6 is not supported.
 * @param socket_type       Socket type to use, @c SOCK_STREAM, @c SOCK_DGRAM,
 *                          or @c 0 to check both.
 * @param port              Port number in host endian
 *
 * @return                  Status code
 */
extern te_errno agent_alloc_l4_specified_port(int socket_family,
                                              int socket_type,
                                              uint16_t port);


/** Supported key managers */
typedef enum agent_key_manager {
    AGENT_KEY_MANAGER_SSH, /**< ssh-keygen */
} agent_key_manager;

/**
 * Generate a key with a given @p manager.
 *
 * @param manager           Key manager to use
 * @param type              Key type.
 *                          For AGENT_KEY_MANAGER_SSH:
 *                          the value of -t option of `ssh-keygen`
 * @param bitsize           Bit length of a key
 * @param user              The name of the key owner
 *                          (@c NULL means the current user)
 * @param private_key_file  The path to a file where the new private key
 *                          will be stored
 *
 * @return                  Status code
 */
extern te_errno agent_key_generate(agent_key_manager manager,
                                   const char *type,
                                   unsigned bitsize,
                                   const char *user,
                                   const char *private_key_file);

#endif /* __TE_AGENTLIB_H__ */
