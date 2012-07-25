/** @file
 * @brief RCF Engine - TA-specific library interface
 *
 * Definition of RCF TA-specific library interface.
 * 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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

#ifndef __TE_RCF_METHODS_H__
#define __TE_RCF_METHODS_H__

#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

    
/** Handle returned by the "start" method and used to control the TA */
typedef void *rcf_talib_handle;


/** @name Test Agent flags */
#define TA_LOCAL        0x01    /**< Test Agent runs on the same station
                                     with TEN, so it's prohibited to
                                     reboot it if it is not proxy */
#define TA_PROXY        0x02    /**< Test Agent is proxy agent, so reboot
                                     should not lead to loss of
                                     the connectivity */
#define TA_REBOOTABLE   0x04    /**< Test Agent host may be rebooted */
#define TA_FAKE         0x08    /**< TA is started manually */
#define TA_NO_HKEY_CHK  0x10    /**< TA is copied using
                                     StrictHostKeyChecking=no
                                     SSH option */
/*@}*/
/** @name Test Agent flags for RCF engine internal use */
#define TA_DOWN         0x0100  /**< For internal RCF use */
#define TA_CHECKING     0x0200  /**< TA checking is in progress, to not
                                     forward new requests */
#define TA_DEAD         0x0400  /**< TA is dead, but can be recovered */
#define TA_UNRECOVER    0x0800  /**< TA is dead, but can't be recovered */
#define TA_REBOOTING    0x1000  /**< TA is performing cold reboot */
/*@}*/


/**
 * Start the Test Agent. Note that it's not necessary
 * to restart the proxy Test Agents after rebooting of
 * the NUT, which it serves.
 *
 * @param ta_name       Test Agent name
 * @param ta_type       Test Agent type (Test Agent executable is equal
 *                      to ta_type and is located in TE_INSTALL/agents/bin)
 * @param conf_str      TA-specific configuration string
 * @param handle        location for TA handle
 * @param flags         IN/OUT location of TA flags;
 *                      these location is shared between RCF and library
 *
 * @return Error code.
 */
typedef te_errno (* rcf_talib_start)(const char       *ta_name,
                                     const char       *ta_type, 
                                     const char       *conf_str,
                                     rcf_talib_handle *handle,
                                     unsigned int     *flags);

/**
 * Kill all processes related to TA on the station where it is run.
 * Reboot station which TA is runing on (if it's allowed).
 * Handle should not be freed.
 *
 * @param handle        TA handle locaton, may already contain memory
 *                      pointer in the case of TA restart
 * @param parms         library-specific parameters
 *
 * @return Error code.
 */
typedef te_errno (* rcf_talib_finish)(rcf_talib_handle  handle,
                                      const char       *parms);

/**
 * Establish connection with the Test Agent. Note that it's not necessary
 * to perform real reconnect to proxy Test Agents after rebooting of
 * the NUT, which it serves.
 *
 * @param handle        TA handle
 * @param select_set    FD_SET to be updated with the TA connection file 
 *                      descriptor (for Test Agents supporting listening
 *                      mode) (IN/OUT)
 *
 * @param select_tm     timeout value for the select to be updated with
 *                      TA polling interval (for Test Agents supporting
 *                      polling mode only)
 *                      (IN/OUT)
 *
 * @return Error code.
 */
typedef te_errno (* rcf_talib_connect)(rcf_talib_handle  handle, 
                                       fd_set           *select_set, 
                                       struct timeval   *select_tm);

/**
 * Transmit data to the Test Agent.
 *
 * @param handle        TA handle
 * @param data          data to be transmitted
 * @param len           data length
 *
 * @return Error code.
 */
typedef te_errno (* rcf_talib_transmit)(rcf_talib_handle handle, 
                                        const void *data, size_t len);
 
/**
 * Check pending data on the Test Agent connection.
 *
 * @param handle        TA handle
 *
 * @return TRUE, if data are pending; FALSE otherwise
 */
typedef te_bool (* rcf_talib_is_ready)(rcf_talib_handle handle);

/**
 * Receive one command (possibly with attachment) from the Test Agent 
 * or its part.
 *
 * @param handle        TA handle
 * @param buf           location for received data
 * @param len           should be filled by the caller to length of the
 *                      buffer; is filled by the routine to length of
 *                      received data
 * @param pba           location for address of first byte after answer 
 *                      end marker (is set only if binary attachment
 *                      presents)
 *
 * @return Error code.
 * @retval 0            success
 *
 * @retval TE_ESMALLBUF Buffer is too small for the command. The part
 *                      of the command is written to the buffer. Other
 *                      part(s) of the message can be read by the subsequent
 *                      routine calls. ETSMALLBUF is returned until last
 *                      part of the message is read.
 *
 * @retval TE_EPENDING  Attachment is too big to fit into the buffer.
 *                      The command and a part of the attachment is written
 *                      to the buffer. Other part(s) can be read by the
 *                      subsequent routine calls. TE_EPENDING is returned
 *                      until last part of the message is read.
 *
 * @retval other        OS errno
 */
typedef te_errno (* rcf_talib_receive)(rcf_talib_handle handle, 
                                       char *buf, size_t *len, char **pba);

/**
 * Close interactions with TA.
 *
 * @param handle        TA handle
 * @param select_set    FD_SET to be updated with the TA connection file 
 *                      descriptor (for Test Agents supporting listening
 *                      mode) (IN/OUT)
 *
 * @return Error code.
 */
typedef te_errno (* rcf_talib_close)(rcf_talib_handle  handle,
                                     fd_set           *select_set);

/**
 * Structure to keep RCF TA methods.
 * A library that implements RCF TA communication type
 * shall export this data structure via RCF_TALIB_METHODS_DEFINE() macro.
 */
struct rcf_talib_methods {
    rcf_talib_start     start;    /**< Start TA */
    rcf_talib_close     close;    /**< Close TA */
    rcf_talib_finish    finish;   /**< Stop TA */
    rcf_talib_connect   connect;  /**< Connect to TA */
    rcf_talib_transmit  transmit; /**< Transmit to TA */
    rcf_talib_is_ready  is_ready; /**< Is data from TA pending */
    rcf_talib_receive   receive;  /**< Receive from TA */
};

/**
 * Export RCF TA communication library methods.
 *
 * @param talib_prefix_  Prefix name used in method functions
 *
 * @note This macro expects TE_LIB_NAME is defined with the name
 * of the library. For the case of rcfunix library TE_LIB_NAME
 * is defined as a part of command line options in Makefile.am
 */
#define RCF_TALIB_METHODS_DEFINE(talib_prefix_) \
extern struct rcf_talib_methods TE_CONCAT(TE_LIB_NAME, _methods);        \
struct rcf_talib_methods TE_CONCAT(TE_LIB_NAME, _methods) = {            \
    talib_prefix_ ## _start,                                            \
    talib_prefix_ ## _close,                                            \
    talib_prefix_ ## _finish,                                           \
    talib_prefix_ ## _connect,                                          \
    talib_prefix_ ## _transmit,                                         \
    talib_prefix_ ## _is_ready,                                         \
    talib_prefix_ ## _receive                                           \
}

#ifdef __cplusplus
}
#endif
#endif /* !__TE_RCF_METHODS_H__ */
