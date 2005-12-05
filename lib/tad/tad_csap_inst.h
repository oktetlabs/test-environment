/** @file
 * @brief TAD Command Handler
 *
 * Traffic Application Domain Command Handler
 *
 * Declarations of types and functions, used in common and 
 * protocol-specific modules implemnting TAD.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_CSAP_INST_H__
#define __TE_TAD_CSAP_INST_H__ 

#ifndef PACKAGE_VERSION
#include "config.h"
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "asn_usr.h" 
#include "tad_common.h"


/* ============= Macros definitions =============== */


/**
 * Insert element p into queue _after_ element q 
 *
 * @param new_    Pointer to new element, to be inserted.
 * @param place_  Pointer to the element, after which new should be placed.
 */
#ifndef INSQUE
#define INSQUE(new_, place_) \
    do {                                \
        (new_)->prev = place_;          \
        (new_)->next = (place_)->next;  \
        (place_)->next = new_;          \
        (new_)->next->prev = new_;      \
    } while (0)

/**
 * Remove element p from queue (double linked list)
 *
 * @param elem_         Pointer to the element to be removed.
 */
#define REMQUE(elem_) \
    do {                                        \
        (elem_)->prev->next = (elem_)->next;    \
        (elem_)->next->prev = (elem_)->prev;    \
        (elem_)->next = (elem_)->prev = elem_;  \
    } while (0)
#endif


/**
 * Default timeout for waiting write possibility. This macro should 
 * be used only for initialization of 'struct timeval' variables. 
 */
#define TAD_WRITE_TIMEOUT_DEFAULT   {1, 0}
/**
 * Number of retries to write data in low layer
 */
#define TAD_WRITE_RETRIES           128

/**
 * Locks access to CSAP shared flags and data. 
 * If already locked, wait until unlocked. 
 *
 * @param csap_inst_   Pointer to CSAP instance structure. 
 */
#define CSAP_DA_LOCK(csap_inst_)\
    do {                                                                \
        int rc = pthread_mutex_lock(&((csap_inst_)->data_access_lock)); \
                                                                        \
        if (rc != 0)                                                    \
            ERROR("%s: mutex_lock fails %d", __FUNCTION__, rc);         \
    } while (0)

/**
 * Try to lock access to CSAP shared flags and data. 
 * If already locked, sets _rc to EBUSY.
 *
 * @param csap_inst_   Pointer to CSAP instance structure. 
 * @param rc_          Variable for return code.
 */
#define CSAP_DA_TRYLOCK(csap_inst_, rc_)\
    do {                                                                  \
        (rc_) = pthread_mutex_trylock(&((csap_inst_)->data_access_lock)); \
        if (rc_ != 0 && rc_ != EBUSY)                                     \
            ERROR("%s: mutex_trylock fails %d", __FUNCTION__, rc_);       \
    } while (0)

/**
 * Unlocks access to CSAP shared flags and data. 
 *
 * @param csap_inst_   Pointer to CSAP descriptor structure. 
 */
#define CSAP_DA_UNLOCK(csap_inst_)\
    do {                                                                  \
        int rc = pthread_mutex_unlock(&((csap_inst_)->data_access_lock)); \
                                                                          \
        if (rc != 0)                                                      \
            ERROR("%s: mutex_unlock fails %d", __FUNCTION__, rc);         \
    } while (0)


/* TODO: this constant should be placed to more appropriate header! */
/**
 * Maximum length of RCF message prefix
 */
#define MAX_ANS_PREFIX 16



/* ============= Types and structures definitions =============== */

/**
 * Pointer type to CSAP instance
 */
typedef struct csap_instance *csap_p;

struct csap_spt_type_t;

/**
 * Callback type to read parameter value of CSAP.
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param layer         Index of layer in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
typedef char *(*csap_get_param_cb_t)(csap_p        csap_descr,
                                     unsigned int  layer, 
                                     const char   *param);

/**
 * Callback type to prepare/release low-layer resources 
 * of CSAP used in traffic process.
 * Usually should open/close sockets, etc. 
 *
 * @param csap_descr    CSAP descriptor structure. 
 *
 * @return Status code.
 */ 
typedef te_errno (*csap_low_resource_cb_t)(csap_p csap_descr);

/**
 * Callback type to read data from media of CSAP. 
 *
 * @param csap_descr    CSAP descriptor structure. 
 * @param timeout       Timeout of waiting for data in microseconds.
 * @param buf           Buffer for read data.
 * @param buf_len       Length of available buffer.
 *
 * @return Quantity of read octets, or -1 if error occured, 
 *         0 if timeout expired. 
 */ 
typedef int (*csap_read_cb_t)(csap_p csap_descr, int timeout, 
                              char *buf, size_t buf_len);

/**
 * Callback type to write data to media of CSAP. 
 *
 * @param csap_descr    CSAP descriptor structure. 
 * @param buf           Buffer with data to be written.
 * @param buf_len       Length of data in buffer.
 *
 * @return Quantity of written octets, or -1 if error occured. 
 */ 
typedef int (*csap_write_cb_t)(csap_p csap_descr, const char *buf,
                               size_t buf_len);

/**
 * Callback type to write data to media of CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_descr    CSAP descriptor structure. 
 * @param timeout       Timeout of waiting for data in microseconds.
 * @param w_buf         Buffer with data to be written.
 * @param w_buf_len     Length of data in w_buf.
 * @param r_buf         Buffer for data to be read.
 * @param r_buf_len     Available length r_buf.
 *
 * @return Quantity of read octets, or -1 if error occured, 
 *         0 if timeout expired. 
 */ 
typedef int (*csap_write_read_cb_t)(csap_p csap_descr, int timeout,
                                    const char *w_buf, size_t w_buf_len,
                                    char *r_buf, size_t r_buf_len);

/**
 * Callback type to check sequence of PDUs in template
 * or pattern, and fill absent layers if necessary. 
 *
 * @param csap_descr    CSAP descriptor structure. 
 * @param pdus          ASN value with Traffic-Template or 
 *                      Traffic-Pattern, which field pdus will 
 *                      be checked to have sequence of PDUs according
 *                      with CSAP layer structure.
 *
 * @return Zero on success, otherwise common TE error code.
 */ 
typedef te_errno (*csap_check_pdus_cb_t)(csap_p csap_descr, 
                                         asn_value *traffic_nds);


/**
 * Callback type to echo CSAP method. 
 * Method should prepare binary data to be send as "echo" and call 
 * respective write method to send it. 
 * Method may change data stored at passed location.
 *
 * @param csap_descr    CSAP descriptor structure. 
 * @param pkt           Got packet, plain binary data. 
 * @param len           Length of packet.
 *
 * @return Zero on success or error code.
 */
typedef te_errno (*csap_echo_method)(csap_p csap_descr, uint8_t *pkt, 
                                     size_t len);


/**
 * Collection of common protocol layer attributes of CSAP.
 */
typedef struct csap_layer_t { 
    char        *proto;          /**< procolol layer text label */
    void        *specific_data;  /**< protocol-specific data */ 
    asn_value   *csap_layer_pdu; /**< ASN value with CSAP specification
                                      layer PDU */

    struct csap_spt_type_t *proto_support; /**< protocol layer 
                                                support descroptor */
    csap_get_param_cb_t     get_param_cb;  /**< callbacks to get
                                                CSAP parameters */
} csap_layer_t;

/**
 * Constants for last unprocessed traffic command to the CSAP 
 */
typedef enum {
    TAD_OP_IDLE,        /**< no traffic operation, waiting for command */
    TAD_OP_SEND,        /**< trsend_start */
    TAD_OP_SEND_RECV,   /**< trsend_recv */
    TAD_OP_RECV,        /**< trrecv_start */
    TAD_OP_GET,         /**< trrecv_get */
    TAD_OP_WAIT,        /**< trrecv_wait */
    TAD_OP_STOP,        /**< tr{send|recv}_stop */
    TAD_OP_DESTROY,     /**< csap_destroy */
} tad_traffic_op_t;

/** @name CSAP processing flags */
enum {
    TAD_STATE_SEND       =    1, /**< CSAP is sending */
    TAD_STATE_RECV       =    2, /**< CSAP is receiving */
    TAD_STATE_FOREGROUND =    4, /**< RCF answer is pending */
    TAD_STATE_COMPLETE   =    8, /**< traffic operation complete */
    TAD_STATE_RESULTS    = 0x10, /**< receive results are required */
};
/*@}*/


/**
 * Type of CSAP: raw or data 
 */
typedef enum {
    TAD_CSAP_RAW,  /**< Raw CSAP, which allows full control of 
                        byte flow */
    TAD_CSAP_DATA, /**< Data CSAP, which have 'data' prefix in its 
                        type string and provides only service
                        to transport data over some protocol */
} tad_csap_type_t;


/**
 * CSAP instance support resources and attributes.
 */
typedef struct csap_instance {
    int             id;         /**< CSAP id */

    unsigned int    depth;      /**< number of layers in stack */
    char           *csap_type;  /**< pointer to original CSAP type, proto[]
                                     entries are blocks of this string. */

    tad_csap_type_t type;  /**< type of CSAP */

    csap_layer_t   *layers;/**< array of protocol layer descroptors */

    csap_read_cb_t       read_cb;       /**< read data from CSAP media */
    csap_write_cb_t      write_cb;      /**< write data to CSAP media */ 
    csap_write_read_cb_t write_read_cb; /**< write data and read answer.*/
    csap_check_pdus_cb_t check_pdus_cb; /**< check PDUs sequence */
    csap_echo_method     echo_cb;       /**< method for echo */

    csap_low_resource_cb_t prepare_recv_cb; /**< prepare CSAP for receive */
    csap_low_resource_cb_t prepare_send_cb; /**< prepare CSAP for send */
    csap_low_resource_cb_t release_cb;      /**< release all lower non-TAD
                                                 send/recv resources */



    int         read_write_layer;/**< index of layer in protocol stack 
                                      responsible for read and write 
                                      operations, usually upper or lower */

    int         last_errno;      /**< errno of last operation */
    int         timeout;         /**< timeout for read operations in 
                                      microseconds */

    char        answer_prefix[MAX_ANS_PREFIX]; 
                                 /**< prefix for test-protocol answer to 
                                      the current command */

    struct timeval  wait_for;    /**< Zero or moment of timeout current 
                                       CSAP operation */
    struct timeval  first_pkt;   /**< moment of first good packet processed:
                                      matched or sent. */ 
    struct timeval  last_pkt;    /**< moment of last good packet processed:
                                      matched or sent. */

    unsigned int    num_packets; /**< number of good packets to be 
                                      processed. */
    size_t          total_bytes; /**< quantity of total processed bytes in
                                      last operation, for some protocols 
                                      it is not sensible */
    size_t          total_sent;  /**< quantity of total sent bytes in
                                      all CSAP live, for some protocols 
                                      it is not sensible */
    size_t          total_received;/**< quantity of total received bytes in
                                      all CSAP live, for some protocols 
                                      it is not sensible */
    tad_traffic_op_t command;    /**< last unprocessed command */
    uint8_t          state;      /**< current state bitmask */
    pthread_t        traffic_thread; 
                                 /**< ID of traffic operation thread */
    pthread_mutex_t  data_access_lock; 
                                 /**< mutex for lock CSAP data changing 
                                      from different threads */
} csap_instance;



/**
 * Type for reference to user function for some magic processing 
 * with matched pkt
 *
 * @param csap_descr  CSAP descriptor structure.
 * @param usr_param   String passed by user.
 * @param pkt         Packet binary data, as it was caught from net.
 * @param pkt_len     Length of pkt data.
 *
 * @return zero on success or error code.
 */
typedef int (*tad_processing_pkt_method)(csap_p csap_descr,
                                         const char *usr_param, 
                                         const uint8_t *pkt, 
                                         size_t pkt_len);


/**
 * Type for reference to user function for generating data to be sent.
 *
 * @param csap_id       Identifier of CSAP
 * @param layer         Numeric index of layer in CSAP type to be processed.
 * @param tmpl          ASN value with template. 
 *                      function should replace that field (which it should
 *                      generate) with #plain (in headers) or #bytes 
 *                      (in payload) choice (IN/OUT)
 *
 * @return zero on success or error code.
 */
typedef int (*tad_user_generate_method)(int csap_id, int layer,
                                        asn_value *tmpl);




/* ============= Function prototypes declarations =============== */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Init TAD Command Handler.
 */
extern void tad_ch_init(void);


/**
 * Initialize CSAP database.
 *
 * @return zero on success, otherwise error code 
 */ 
extern int csap_db_init();

/**
 * Clear CSAP database.
 *
 * @return zero on success, otherwise error code 
 */ 
extern int csap_db_clear();

/**
 * Create new CSAP. 
 * This method does not perform any actions related to CSAP functionality,
 * neither processing of CSAP init parameters, nor initialyzing some 
 * communication media units (for example, sockets, etc.).
 * It only allocates memory for csap_instance sturture, set fields
 * 'id', 'depth' and 'proto' in it and allocates memory for 'layer_data'.
 * 
 * @param type  Type of CSAP: dot-separated sequence of textual layer 
 *              labels.
 *
 * @return identifier of new CSAP or zero if error occured.
 */ 
extern int csap_create(const char *type);

/**
 * Destroy CSAP.
 *      Before call this DB method, all protocol-specific data in 
 *      'layer-data' and underground media resources should be freed. 
 *      This method will free all non-NULL pointers in 'layer-data', but 
 *      does not know nothing about what structures are pointed by them, 
 *      therefore if there are some more pointers in that structures, 
 *      memory may be lost. 
 *
 * @param csap_id       Identifier of CSAP to be destroyed.
 *
 * @return zero on success, otherwise error code 
 */ 
extern int csap_destroy(int csap_id);

/**
 * Find CSAP by its identifier.
 *
 * @param csap_id       Identifier of CSAP 
 *
 * @return      Pointer to structure with internal CSAP information 
 *              or NULL if not found. 
 *
 * Change data in this structure if you really know what does it mean!
 */ 
extern csap_p csap_find(int csap_id);


/**
 * Traffic operation thread special data.
 */
typedef struct tad_task_context {
    csap_p     csap; /**< Pointer to CSAP descriptor */
    asn_value *nds;  /**< ASN value with NDS */

    struct rcf_comm_connection *rcf_handle; /**< RCF handle to answer */
} tad_task_context;


/**
 * Start routine for tr_recv thread. 
 *
 * @param arg      start argument, should be pointer to 
 *                 tad_task_context struct.
 *
 * @return NULL 
 */
extern void *tad_tr_recv_thread(void *arg);

/**
 * Start routine for tr_send thread. 
 *
 * @param arg           start argument, should be pointer to 
 *                      tad_task_context struct.
 *
 * @return NULL 
 */
extern void *tad_tr_send_thread(void *arg);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_CSAP_INST_H__ */
