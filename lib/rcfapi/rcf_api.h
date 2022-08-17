/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * Definition of the C API provided by RCF to TE subsystems and tests
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RCF_API_H__
#define __TE_RCF_API_H__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "rcf_common.h"
#include "tad_common.h"
#include "te_vector.h"

/** @defgroup rcfapi_base API: RCF
 * @ingroup rcfapi
 * @{
 */

/** Discover name of the RCF IPC server */
static inline const char *
rcf_server_name(void)
{
    static const char *rcf_name = NULL;

    if ((rcf_name == NULL) && (rcf_name = getenv("TE_RCF")) == NULL)
        rcf_name = "TE_RCF";

    return rcf_name;
}

/** IPC RCF Server name */
#define RCF_SERVER      rcf_server_name()

/**
 * List of modes that can be used while calling RCF functions especially
 * for those that deal with traffic application domain
 */
typedef enum rcf_call_mode {
    RCF_MODE_NONBLOCKING, /**< Used for non-blocking calls */
    RCF_MODE_BLOCKING, /**< Used for blocking call */
} rcf_call_mode_t;

/** Bit mask for report flag of receiving mode */
#define RCF_TRRECV_REPORT_MASK 0x3

/**
 * List of modes that can be used while calling RCF traffic receive
 * operation.
 */
typedef enum rcf_trrecv_mode {
    RCF_TRRECV_COUNT,              /**< Count received packets only */
    RCF_TRRECV_NO_PAYLOAD,         /**< Get packet headers only, do not get
                                       payload */
    RCF_TRRECV_PACKETS,            /**< Store packets to get from test later */
    RCF_TRRECV_SEQ_MATCH = 0x04,   /**< Pattern sequence matching */
    RCF_TRRECV_MISMATCH = 0x08,    /**< Store mismatch packets
                                        to get from test later */
} rcf_trrecv_mode;

/**
 * Convert mode of RCF traffic operation to string.
 *
 * @param mode      Mode to convert
 *
 * @return Mode as string
 */
static inline const char *
rcf_call_mode2str(rcf_call_mode_t mode)
{
    switch (mode)
    {
        case RCF_MODE_NONBLOCKING:  return "non-blocking";
        case RCF_MODE_BLOCKING:     return "blocking";
        default:                    return "UNKNOWN";
    }
}


#ifdef __cplusplus
extern "C" {
#endif

/* All routines block caller until response from RCF */

/** Test Agent control flags */
enum rcf_ta_flags {
    /* Generic flags */
    RCF_TA_REBOOTABLE   = 0x01, /**< Test agent is rebootable */
    RCF_TA_NO_SYNC_TIME = 0x02, /**< Disable time synchronisation
                                     on Test Agent start-up */

    RCF_TA_NO_HKEY_CHK  = 0x04, /**< TA is copied to destination host
                                     using StrictHostKeyChecking=no
                                     SSH option */

    /* rcfunix-specific flags */
    RCF_TA_UNIX_SUDO    = 0x010000, /**< Start agent under sudo */
};

/** Reboot types */
typedef enum {
    RCF_REBOOT_TYPE_AGENT, /**< Reboot TA process */
    RCF_REBOOT_TYPE_WARM,  /**< Reboot TA host */
    RCF_REBOOT_TYPE_COLD,  /**< Cold reboot for the TA host */
    RCF_REBOOT_TYPE_FORCE  /**< Reboot TA in any possible way */
} rcf_reboot_type;

/**
 * Add a new Test Agent to RCF.
 *
 * @param name          Test Agent name
 * @param type          Test Agent type
 * @param rcflib        Name of RCF TA-specific shared library to be
 *                      used to control Test Agent
 * @param confstr       TA-specific configuration string (kvpairs)
 * @param flags         Test Agent control flags (see ::rcf_ta_flags)
 *
 * @return Error code
 * @retval 0            success
 * @retval TE_EEXIST    Test Agent with such name exists and running
 * @retval TE_ETOOMANY  Too many Test Agents are added, no more space
 */
extern te_errno rcf_add_ta(const char *name, const char *type,
                           const char *rcflib, const char *confstr,
                           unsigned int flags);

/**
 * Add a new Test Agent controlled using rcfunix TA-specific shared
 * library.
 *
 * @param name          Test Agent name
 * @param type          Test Agent type
 * @param host          Host name or IPv4 address in dotted notation
 * @param port          TCP port to be used on Test Agent to listen to
 * @param copy_timeout  Test Agent image coping timeout or 0 for default
 * @param kill_timeout  Test Agent kill timeout or 0 for default
 * @param flags         Test Agent control flags (see ::rcf_ta_flags)
 *
 * @return Error code   See 'rcf_add_ta' error codes
 */
extern te_errno rcf_add_ta_unix(const char *name, const char *type,
                                const char *host, uint16_t port,
                                unsigned int copy_timeout,
                                unsigned int kill_timeout,
                                unsigned int flags);

/**
 * Delete Test Agent from RCF.
 *
 * @param name          Test Agent name
 *
 * @return Error code
 * @retval 0            success
 * @retval TE_ENOENT    Test Agent with such name does not exist
 * @retval TE_EPERM     Test Agent with such name exists but is
 *                      specified in RCF configuration file
 */
extern te_errno rcf_del_ta(const char *name);

/**
 * This function returns list of Test Agents (names) running.
 *
 * @param buf           location for the name list;
 *                      names are put to the buffer one-by-one and are
 *                      separated by "'\0'"; the list is finished by
 *                      "'\0'" as well
 *
 * @param len           pointer to length variable (should be set
 *                      to the length the buffer by caller;
 *                      is filled to amount of space used for the name list
 *                      by the routine)
 *
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_ESMALLBUF     the buffer is too small
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ENOMEM        out of memory
 */
extern te_errno rcf_get_ta_list(char *buf, size_t *len);

/**
 * This function maps name of the Test Agent to its type.
 *
 * @param ta_name       Test Agent name
 * @param ta_type       Test Agent type location (should point to
 *                      the buffer at least of RCF_MAX_NAME length)
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running Test Agent is provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ENOMEM        out of memory
 */
extern te_errno rcf_ta_name2type(const char *ta_name, char *ta_type);

/**
 * This function returns unique session identifier for the Test Agent.
 * This session identifier may be passed to all other TA-related routines.
 * Command with session identifier X will be passed to the Test Agent
 * before obtaining of answer on the command with session identifier Y
 * (X != Y). However if the Test Agent does not support simultaneous
 * processing of several commands, this mechanism does not give any
 * advantage.
 *
 * @param ta_name       Test Agent name
 * @param session       location for the session identifier
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running Test Agent is provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ENOMEM        out of memory
 */
extern te_errno rcf_ta_create_session(const char *ta_name, int *session);

/**
 * This function reboots the Test Agent or NUT served by it. It sends
 * "reboot" command to the Test Agent, calls reboot function provided by
 * RCF TA-specific library, restarts the TA and re-connects to it.
 * The function may be called by Configurator only.
 * It is prohibited to call the function for the TA running on the Testing
 * Node.
 *
 * @param ta_name       Test Agent name
 * @param boot_params   complete parameter string to be passed to the TA
 *                      in the "reboot" command and to the reboot function
 *                      of RCF TA-specific library (without quotes) or NULL
 *
 * @param image         name of the bootable image (without path) to be
 *                      passed to the Test Agent as binary attachment or
 *                      NULL (it is assumed that NUT images are located in
 *                      ${TE_INSTALL_NUT}/bin or ${TE_INSTALL}/nuts/bin)
 * @param reboot_type   Reboot type
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running or located on TN Test Agent
 *                         is provided or parameter string is too long
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EINPROGRESS  operation is in progress
 * @retval TE_ENOENT       cannot open NUT image file
 * @retval TE_ENOMEM       out of memory
 * @retval TE_EPERM        operation is not allowed (please check that you
 *                         enabled @attr_name{rebootable} attribute in RCF
 *                         configuration file)
 */
extern te_errno rcf_ta_reboot(const char *ta_name,
                              const char *boot_params,
                              const char *image,
                              rcf_reboot_type reboot_type);

/**
 * Enable/disable logging of TA configuration changes.
 *
 * @param enable        if TRUE, enable loging
 */
extern void rcf_log_cfg_changes(te_bool enable);

/**
 * This function is used to obtain value of object instance by its
 * identifier.  The function may be called by Configurator only.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param oid           object instance identifier
 * @param val_buf       location for the object instance value
 * @param len           location length
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or OID string is
 *                      too long
 * @retval TE_EIPC      cannot interact with RCF
 * @retval TE_ESMALLBUF the buffer is too small
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern te_errno rcf_ta_cfg_get(const char *ta_name, int session,
                               const char *oid,
                               char *val_buf, size_t len);

/**
 * This function is used to change value of object instance.
 * The function may be called by Configurator only.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param oid           object instance identifier
 * @param val           object instance value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or too OID or value
 *                      strings are too long
 * @retval TE_EIPC      cannot interact with RCF
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern te_errno rcf_ta_cfg_set(const char *ta_name, int session,
                               const char *oid, const char *val);

/**
 * This function is used to create new object instance and assign the
 * value to it.
 * The function may be called by Configurator only.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param oid           object instance identifier
 * @param val           object instance value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or too OID or value
 *                      strings are too long
 * @retval TE_EIPC      cannot interact with RCF
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern te_errno rcf_ta_cfg_add(const char *ta_name, int session,
                               const char *oid, const char *val);

/**
 * This function is used to remove the object instance.
 * The function may be called by Configurator only.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param oid           object instance identifier
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or OID string is
 *                      too long
 * @retval TE_EIPC      cannot interact with RCF
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
extern te_errno rcf_ta_cfg_del(const char *ta_name, int session,
                               const char *oid);

/**
 * This function is used to begin/finish group of configuration commands.
 * The function may be called by Configurator only.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param is_start      Is start(TRUE) or end(FALSE) of a group?
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EIPC      cannot interact with RCF
 * @retval other        error returned by command handler on the TA
 */
extern te_errno rcf_ta_cfg_group(const char *ta_name, int session,
                                 te_bool is_start);

/**
 * This function is used to pull out capture logs from the sniffer. The only
 * user of this calls is Logger.
 *
 * @param ta_name       Test agent name
 * @param snif_id       The sniffer ID
 * @param fname         File name for the capture logs (IN/OUT)
 * @param offset        The absolute offset of the received part of capture.
 *
 * @return Status code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent or bad
 *                          sniffer ID are provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 * @retval other            error returned by command handler on the TA
 */
extern te_errno rcf_get_sniffer_dump(const char *ta_name,
                                     const char *snif_id, char *fname,
                                     unsigned long long *offset);

/**
 * This function is used to get list of sniffers from the Test Agent.
 * The function may be called by Logger only.
 *
 * @param ta_name       Test Agent name
 * @param snif_id       Used to get offset of last captured packet for
 *                      the sniffer
 * @param buf           Pointer to buffer for the list of sniffers (OUT).
 *                      Memory for this buffer should be allocated by caller
 *                      from heap, size should be specified in 'len' param.
 *                      The memory may be reallocated by function. Caller is
 *                      responsible to free this memory.
 * @param len           Size of incoming buffer and length of
 *                      outgoing (IN/OUT)
 * @param sync          Sync sniffers offsets.
 *
 * @return Status code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent or bad
 *                          sniffer ID are provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 * @retval TE_ENOPROTOOPT   Agent side sniffer support is not available
 * @retval other            error returned by command handler on the TA
 */
extern te_errno rcf_ta_get_sniffers(const char *ta_name,
                                    const char *snif_id, char **buf,
                                    size_t *len, te_bool sync);

/**
 * This function is used to get bulk of log from the Test Agent.
 * The function may be called by Logger only.
 *
 * @param ta_name       Test Agent name
 * @param log_file      name of the local file where log should be put
 *                      (the file is truncated to zero length before
 *                      updating)
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent or bad file
 *                          name are provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 * @retval other            error returned by command handler on the TA
 */
extern te_errno rcf_ta_get_log(const char *ta_name, char *log_file);

/**
 * This function is used to obtain value of the variable from the Test Agent
 * or NUT served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param var_name      name of the variable to be read
 * @param var_type      type according to which string returned from the TA
 *                      should be converted
 *
 * @param var_len       length of the location buffer
 * @param val           location for variable value
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        one of arguments is invalid (NULL, too long or
 *                          has inappropriate value)
 * @retval TE_ENOENT        no such variable
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ESMALLBUF     the buffer is too small
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 * @retval other            error returned by command handler on the TA
 */
extern te_errno rcf_ta_get_var(const char *ta_name, int session,
                               const char *var_name,
                               rcf_var_type_t var_type,
                               size_t var_len, void *val);

/**
 * This function is used to change value of the variable from the Test Agent
 * or NUT served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param var_name      name of the variable to be changed
 * @param var_type      type according to which variable value should
 *                      appear in the protocol command
 * @param val           new value of the variable
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        one of arguments is invalid (NULL, too long or
 *                          has inappropriate value)
 * @retval TE_ENOENT        no such variable
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 * @retval other            error returned by command handler on the TA
 */
extern te_errno rcf_ta_set_var(const char *ta_name, int session,
                               const char *var_name,
                               rcf_var_type_t var_type, const char *val);

/**
 * This function loads file from Test Agent or NUT served by it to the
 * testing node.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param rfile         full name of the file in the TA/NUT file system
 * @param lfile         full name of the file in the TN file system
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent, non-existent
 *                          session identifier or bad file name are provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ENOENT        no such file
 * @retval TE_EPERM         operation not permitted
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 */
extern te_errno rcf_ta_get_file(const char *ta_name, int session,
                                const char *rfile, const char *lfile);

/**
 * This function loads file from the testing node to Test Agent or NUT
 * served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param lfile         full name of the file in the TN file system
 * @param rfile         full name of the file in the TA/NUT file system
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent, non-existent
 *                          session identifier or bad file name are provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ENOENT        no such file
 * @retval TE_EPERM         operation not permitted
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 */
extern te_errno rcf_ta_put_file(const char *ta_name, int session,
                                const char *lfile, const char *rfile);

/**
 * This function deletes file from the Test Agent or NUT served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param rfile         full name of the file in the TA/NUT file system
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent, non-existent
 *                          session identifier or bad file name are provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ENOENT        no such file
 * @retval TE_EPERM         operation not permitted
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 */
extern te_errno rcf_ta_del_file(const char *ta_name, int session,
                                const char *rfile);

/**
 * Switch traffic operations logging among RING (enabled) and INFO
 * (disabled) levels.
 *
 * @param ring      TRUE - log using RING level,
 *                  FALSE - log using INFO level
 *
 * @return Status code.
 */
extern te_errno rcf_tr_op_log(te_bool ring);

/**
 * Get traffic operations logging level.
 *
 * @return @c TRUE - log using RING level, @c FALSE - log using INFO level.
 */
extern te_bool rcf_tr_op_log_get(void);


/**
 * This function creates CSAP (communication service access point) on the
 * Test Agent.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param stack_id      protocol stack identifier
 * @param params        parameters necessary for CSAP creation (string
 *                      or file name); if file with *params name exists,
 *                      it is attached to the CSAP creation command as
 *                      a binary attachment; otherwise the string is
 *                      appended to the command
 *
 * @param csap_id       location for unique CSAP handle
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent or
 *                          non-existent session identifier is provided
 * @retval TE_EIPC          cannot interact with RCF
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 * @retval other            error returned by command handler on the TA
 *
 * @sa rcf_ta_csap_destroy
 */
extern te_errno rcf_ta_csap_create(const char *ta_name, int session,
                                   const char *stack_id,
                                   const char *params,
                                   csap_handle_t *csap_id);

/**
 * This function destroys CSAP.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap_id       CSAP handle
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_csap_create
 */
extern te_errno rcf_ta_csap_destroy(const char *ta_name, int session,
                                    csap_handle_t csap_id);

/**
 * This function is used to obtain CSAP parameter value.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap_id       CSAP handle
 * @param var           parameter name
 * @param var_len       length of the location buffer
 * @param val           location for variable value
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ESMALLBUF    the buffer is too small
 */
extern te_errno rcf_ta_csap_param(const char *ta_name, int session,
                                  csap_handle_t csap_id,
                                  const char *var, size_t var_len,
                                  char *val);

/**
 * This function is used to force sending of traffic via already created
 * CSAP.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap_id       CSAP handle
 * @param templ         full name of the file with traffic template
 * @param blk_mode      mode of the operation:
 *                      in blocking mode it suspends the caller
 *                      until sending of all the traffic finishes
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EINPROGRESS  operation is already in progress
 * @retval TE_EBUSY        CSAP is used for receiving now
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other           error returned by command handler on the TA
 *
 * @se It may block caller according to "blk_mode" parameter value
 *
 * @sa rcf_ta_trsend_stop
 */
extern te_errno rcf_ta_trsend_start(const char *ta_name, int session,
                                    csap_handle_t csap_id,
                                    const char *templ,
                                    rcf_call_mode_t blk_mode);

/**
 * This function is used to stop sending of traffic started by
 * rcf_ta_trsend_start called in non-blocking mode.
 *
 * @param ta_name       Test Agent name
 * @param session       session identifier
 * @param csap_id       CSAP handle
 * @param num           location where number of sent packets should be
 *                      placed
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EALREADY     traffic sending is not in progress now
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_trsend_start
 */
extern te_errno rcf_ta_trsend_stop(const char *ta_name, int session,
                                   csap_handle_t csap_id, int *num);

/** Function - handler of received packets */
typedef void (*rcf_pkt_handler)(
    const char *pkt,        /**< File name where received packet
                                 was saved. The file is deleted after the
                                 handler invocation, so the caller should
                                 rename or copy to keep it. */
    void       *user_param  /**< Parameter provided by caller of
                                 rcf_ta_trrecv_start */
);

/**
 * This function is used to force receiving of traffic via already created
 * CSAP.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap_id       CSAP handle
 * @param pattern       Full name of the file with traffic pattern
 * @param timeout       Timeout (in milliseconds) for traffic receive
 *                      operation. After this time interval CSAP stops
 *                      capturing any traffic on the agent and will be
 *                      waiting until rcf_ta_trrecv_stop() or
 *                      rcf_ta_trrecv_wait() are called
 * @param num           Number of packets that needs to be captured;
 *                      if it is zero, the number of received packets
 *                      is not limited
 * @param mode          The flags allows to specify the receive mode.
 *                      Count received packets only, store packets
 *                      to get to the test side later or use pattern sequence
 *                      matching.
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EINPROGRESS  operation is already in progress
 * @retval TE_EBUSY        CSAP is used for sending now
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       not enough memory in the system
 * @retval other           error returned by command handler on the TA
 *
 * @sa rcf_ta_trrecv_stop rcf_ta_trrecv_wait
 */
extern te_errno rcf_ta_trrecv_start(const char      *ta_name,
                                    int              session,
                                    csap_handle_t    csap_id,
                                    const char      *pattern,
                                    unsigned int     timeout,
                                    unsigned int     num,
                                    unsigned int     mode);

/**
 * Blocks the caller until all the traffic, which is being captured,
 * is received by the CSAP or timeout occurres (timeout specified
 * with rcf_ta_trrecv_start()).
 * If @a handler is specified, it is called for all received packets.
 * This function can only be called after rcf_ta_trrecv_start().
 *
 * @param ta_name       Test Agent name
 * @param session       Session identifier
 * @param csap_id       CSAP handle
 * @param handler       Address of function to be used to process
 *                      received packets or NULL
 * @param user_param    User parameter to be passed to the @a handler
 * @param num           Number of received packets (OUT)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_ETIMEDOUT timeout occured before all the requested traffic
 *                      received - this means that TAD has captured less
 *                      than the number of packets specified with
 *                      rcf_ta_trrecv_start.
 *                      Nevertheless, number of captured packets is set to
 *                      "num" parameter.
 *
 * @sa rcf_ta_trrecv_start
 */
extern te_errno rcf_ta_trrecv_wait(const char      *ta_name,
                                   int              session,
                                   csap_handle_t    csap_id,
                                   rcf_pkt_handler  handler,
                                   void            *user_param,
                                   unsigned int    *num);

/**
 * This function is used to stop receiving of traffic started by
 * rcf_ta_trrecv_start.
 * If @a handler is specified, it is called for all received packets.
 *
 * @param ta_name       Test Agent name
 * @param session       Session identifier
 * @param csap_id       CSAP handle
 * @param handler       Address of function to be used to process
 *                      received packets or NULL
 * @param user_param    User parameter to be passed to the @a handler
 * @param num           Location where number of received packets
 *                      should be placed (OUT)
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EALREADY     traffic receiving is not in progress now
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_trrecv_start
 */
extern te_errno rcf_ta_trrecv_stop(const char      *ta_name,
                                   int              session,
                                   csap_handle_t    csap_id,
                                   rcf_pkt_handler  handler,
                                   void            *user_param,
                                   unsigned int    *num);

/**
 * This function is used to force processing of received packets
 * without stopping of traffic receiving.
 * If @a handler is specified, it is called for all received packets.
 *
 * @param ta_name       Test Agent name
 * @param session       Session identifier
 * @param csap_id       CSAP handle
 * @param handler       Address of function to be used to process
 *                      received packets or NULL
 * @param user_param    User parameter to be passed to the @a handler
 * @param num           Location where number of processed packets
 *                      should be placed
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EALREADY     traffic receiving is not in progress now
 * @retval TE_ENODATA      no data available on TA, because handler was not
 *                         specified in rcf_ta_trrecv_start
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_trrecv_start
 */
extern te_errno rcf_ta_trrecv_get(const char      *ta_name,
                                  int              session,
                                  csap_handle_t    csap_id,
                                  rcf_pkt_handler  handler,
                                  void            *user_param,
                                  unsigned int    *num);

/**
 * This function is used to send exactly one packet via CSAP and receive
 * an answer (it may be used for CLI, SNMP, ARP, ICMP, DNS, etc.)
 * This function blocks the caller until the packet is received by
 * traffic application domain or timeout occures.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap_id       CSAP handle
 * @param templ         Full name of the file with traffic template
 * @param handler       Callback function used in processing of
 *                      received packet or NULL
 * @param user_param    User parameter to be passed to the handler
 * @param timeout       Timeout for answer waiting (in milliseconds)
 * @param error         Location for protocol-specific error extracted
 *                      from the answer or NULL (if error is 0, then answer
 *                      is positive; if error is -1, then it's not possible
 *                      to extract the error)
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_ETIMEDOUT    timeout occured before a packet that matches
 *                         the template received
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided or flag blocking
 *                         is used together with num 0
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EBUSY        CSAP is busy
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other           error returned by command handler on the TA
 */
extern te_errno rcf_ta_trsend_recv(const char      *ta_name,
                                   int              session,
                                   csap_handle_t    csap_id,
                                   const char      *templ,
                                   rcf_pkt_handler  handler,
                                   void            *user_param,
                                   unsigned int     timeout,
                                   int             *error);


/** CSAP parameters to be used for polling of many CSAPs. */
typedef struct rcf_trpoll_csap {
    const char     *ta;         /**< Test Agent name */
    csap_handle_t   csap_id;    /**< CSAP handle */
    te_errno        status;     /**< Returned status of the CSAP */
} rcf_trpoll_csap;

/**
 * Wait for completion of send and/or receive operation on one of CSAPs.
 *
 * @param csaps         Array with CSAPs to be polled
 * @param n_csaps       Number of entries in @a csaps array
 * @param timeout       Timeout (in milliseconds) to wait for send or
 *                      receive processing (may be TAD_TIMEOUT_INF)
 *
 * If @a csap_id in rcf_trpoll_csap structure is CSAP_INVALID_HANDLE,
 * the entry is ignored (does not cause break of timeout) and
 * TE_ETADCSAPNOTEX is returned in @a status.
 *
 * @a status in rcf_trpoll_csap structure contains status code of the
 * request to Test Agent. The following values are expeted:
 *  - TE_ETADCSAPNOTEX - CSAP does not exist;
 *  - TE_ETADCSAPSTATE - CSAP is idle, no send and/or receive operations
 *                       have been executed;
 *  - TE_ETIMEDOUT     - send or receive operation is in progress and it
 *                       has not finished until timeout or completion of
 *                       another request;
 *  - 0                - send or receive operation has been finished.
 *
 * @return Status code.
 * @retval 0            One of requests has been finished before timeout
 * @retval TE_ETIMEDOUT Requests to all CSAPs are timed out
 */
extern te_errno rcf_trpoll(rcf_trpoll_csap *csaps, unsigned int n_csaps,
                           unsigned int timeout);


/**
 * This function is used to call remotely a routine on the Test Agent or
 * NUT served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param rtn           routine name
 * @param error         location for the code returned by the routine
 * @param argc          number of parameters
 * @param argv          if TRUE, rest of parameters are char *;
 *                      otherwise pairs: type, value.
 *
 * Usage example:
 * rcf_ta_call("loki.oktetlabs.ru", 25, "foo_rtn", &rc, 3, TRUE,
 *             str, "foo", "bar");
 * rcf_ta_call("thor", 13, "bar_rtn", &rc, 3, FALSE,
 *             RCF_INT8, 7, RCF_STRING, "foo", RCF_STRING, arg)
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_ENOENT       no such routine
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 */
extern te_errno rcf_ta_call(const char *ta_name, int session,
                            const char *rtn, int *error,
                            int argc, te_bool argv, ...);

/**
 * This function is used to start a process on the Test Agent or
 * NUT served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param priority      priority of the new process
 * @param rtn           routine name (task entry point)
 * @param pid           location for process identifier
 * @param argc          number of parameters
 * @param argv          if TRUE, rest of parameters are char *;
 *                      otherwise pairs: type, value.
 *
 * Usage example:
 * rcf_ta_start_task("loki.oktetlabs.ru", 25, HIGH_PRIORITY,
 *                   "foo_entry_pont", &pid, 3, TRUE,
 *                   str, "foo", "bar");
 * rcf_ta_start_task("thor", 13, LOW_PRIORITY, "bar_entry_point",
 *                   &pid, 3, FALSE,
 *                   RCF_INT8, 7, RCF_STRING, "foo", RCF_STRING, arg)
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_ENOENT       no such routine
 * @retval TE_EIPC         cannot interact with RCF
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_kill_process
 */
extern te_errno rcf_ta_start_task(const char *ta_name, int session,
                                  int priority,  const char *rtn,
                                  pid_t *pid, int argc, te_bool argv, ...);

/**
 * This function is similar to rcf_ta_start_task, but
 * the task should be started as a separate thread, not a process
 *
 * @sa rcf_ta_start_task
 */
extern te_errno rcf_ta_start_thread(const char *ta_name, int session,
                                    int priority,  const char *rtn,
                                    int *tid, int argc, te_bool argv, ...);

/**
 * This function is used to kill a process on the Test Agent or
 * NUT served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param pid           identifier of the process to be killed
 *
 * @return Status code
 */
extern te_errno rcf_ta_kill_task(const char *ta_name, int session,
                                 pid_t pid);

/**
 * This function is used to kill a thread on the Test Agent or
 * NUT served by it.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param tid           identifier of the thread to be killed
 *
 * @return Status code
 */
extern te_errno rcf_ta_kill_thread(const char *ta_name, int session,
                                   int tid);

/**
 * Call SUN RPC on the TA.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param rpcserver     Name of the RPC server
 * @param timeout       RPC timeout in milliseconds or 0 (unlimited)
 * @param rpc_name      Name of the RPC (e.g. "bind")
 * @param in            Input parameter C structure
 * @param out           Output parameter C structure
 *
 * @return Status code
 */
extern te_errno rcf_ta_call_rpc(const char *ta_name, int session,
                                const char *rpcserver, int timeout,
                                const char *rpc_name, void *in, void *out);

/**
 * Check that given agent is still working.
 *
 * @param ta_name       Test Agent name
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_ETAREBOOTED  if the test agent has been normally rebooted
 * @retval TE_ETADEAD   if the agent was dead
 * @retval TE_EIPC      cannot interact with RCF
 */
extern te_errno rcf_check_agent(const char *ta_name);

/**
 * Check that all running agents are still working.
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_ETAREBOOTED  if at least one test agent has been normally
 *                         rebooted
 * @retval TE_ETADEAD      if at least one agent was dead
 * @retval TE_EIPC         cannot interact with RCF
 */
extern te_errno rcf_check_agents(void);

/**
 * This function is used to shutdown the RCF.
 *
 * @return error code
 */
extern te_errno rcf_shutdown_call(void);

/**
 * Clean up resources allocated by RCF API.
 * This routine should be called from each thread used RCF API.
 *
 * Usually user should not worry about calling of the function, since
 * it is called automatically using atexit() or similar mechanism.
 */
extern void rcf_api_cleanup(void);

/**
 * Prototype of the function to be called for each Test Agent.
 *
 * @param ta            Test agent
 * @param cookie        Callback opaque data
 *
 * @return Status code.
 *
 * @note Iterator terminates if callback returns non zero.
 */
typedef te_errno rcf_ta_cb(const char *ta, void *cookie);

/**
 * Execute callback for each Test Agent
 *
 * @param cb            Callback function
 * @param cookie        Callback opaque data
 *
 * @return Status code.
 */
extern te_errno rcf_foreach_ta(rcf_ta_cb *cb, void *cookie);

/**
 * Get vector of the dead agents.
 *
 * @param dead_agents Vector of the dead agents
 *
 * @return Status code
 */
extern te_errno rcf_get_dead_agents(te_vec *dead_agents);

/**@} <!-- END rcfapi_base --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RCF_API_H__ */
