/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler *
 * Declarations of types and functions, used in common and protocol-specific 
 * modules implemnting TAD.
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
#ifndef __TE__TAD_CH__IMPL__H__
#define __TE__TAD_CH__IMPL__H__ 

#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "te_stdint.h"
#include "asn_usr.h" 
#include "tad_common.h"


#ifndef INSQUE
/* macros to insert element p into queue _after_ element q */
#define INSQUE(p, q) do {(p)->prev = q; (p)->next = (q)->next; \
                      (q)->next = p; (p)->next->prev = p; } while(0)
/* macros to remove element p from queue  */
#define REMQUE(p) do {(p)->prev->next = (p)->next; (p)->next->prev = (p)->prev; \
                   (p)->next = (p)->prev = p; } while(0)
#endif


#define WRITE_TIMEOUT_DEFAULT   {1, 0}
#define WRITE_RETRIES           128


#ifdef __cplusplus
extern "C" {
#endif



/* TODO: this constant should be placed to more appropriate header! */
#define MAX_ANS_PREFIX 16

/**
 * Init TAD Command Handler.
 */
extern void tad_ch_init(void);


typedef struct csap_instance *csap_p;

/**
 * Type for reference to callback for read parameter value of CSAP.
 *
 * @param csap_descr    CSAP descriptor structure. 
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
typedef char* (*csap_get_param_cb_t) 
       (csap_p csap_descr, int level, const char *param);

/**
 * Type for reference to callback for prepare/release low-level resources 
 * of CSAP used in traffic process.
 * Usually should open/close sockets, etc. 
 *
 * @param csap_descr    CSAP descriptor structure. 
 *
 * @return status code.
 */ 
typedef int (*csap_low_resource_cb_t) (csap_p csap_descr);

/**
 * Type for reference to callback for read data from media of CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data in microseconds.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
typedef int (*csap_read_cb_t)  (csap_p csap_descr, int timeout, 
                                char *buf, int buf_len);

/**
 * Type for reference to callback for write data to media of CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
typedef int (*csap_write_cb_t) (csap_p csap_descr, char *buf, int buf_len);

/**
 * Type for reference to callback for write data to media of CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data in microseconds.
 * @param w_buf         buffer with data to be written.
 * @param w_buf_len     length of data in w_buf.
 * @param r_buf         buffer for data to be read.
 * @param r_buf_len     available length r_buf.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
typedef int (*csap_write_read_cb_t) (csap_p csap_descr, int timeout,
                     char *w_buf, int w_buf_len,
                     char *r_buf, int r_buf_len);

/**
 * Type for reference to callback for check sequence of PDUs in template
 * or pattern, and fill absent layers if necessary. 
 *
 * @param csap_descr       identifier of CSAP.
 * @param pdus             ASN value with Traffic-Template or Traffic-Pattern, 
 *                         which field pdus will be checked to have sequence
 *                         of PDUs according with CSAP layer structure.
 *
 * @return zero on success, otherwise common TE error code.
 */ 
typedef int (*csap_check_pdus_cb_t) (csap_p csap_descr, asn_value *traffic_nds);


/**
 * Type for reference to echo CSAP method. 
 * Method should prepare binary data to be send as "echo" and call 
 * respective write method to send it. 
 * Method may change data stored at passed location.
 *
 * @param csap_descr    identifier of CSAP
 * @param pkt           Got packet, plain binary data. 
 *
 * @return zero on success or error code.
 */
typedef int (*csap_echo_method) (csap_p csap_descr, uint8_t *pkt, size_t len);

/**
 * CSAP parameters command and state are bitmasks with the following 
 * semantic:
 *
 *  CSAP command bits. 
 * 
 * 00000000
 * |||||||+- traffic send operation;
 * ||||||+- traffic receive operation;
 * |||||+- stop;
 * ||||+- get (valid only for "recv");
 * |||+- wait (valid only for "recv");
 * ||+- received packets requested (valid only for "recv");
 * ++- not used.
 *
 * complete (in background mode, all packets are precessed, wait stop)
 * answer is postponed, that is, operation in blocking mode 
 *
 * CSAP state bits 
 * 00000000
 * |||||||+- traffic send operation;
 * ||||||+- traffic receive operation;
 * |||||+- foreground (blocking) operation mode; 
 * ||||+- operation complete (in background mode), 
 * ||||   waiting for "stop" or "wait";
 * ++++- not used.
 *
 */

enum {
    /* These constants used for both "command" and "state" flags. */
    TAD_OP_SEND            =    1,
    TAD_OP_RECV            =    2,  

    /* Constants for "command" flag */
    TAD_COMMAND_STOP       =    4, 
    TAD_COMMAND_GET        =    8, 
    TAD_COMMAND_WAIT       = 0x10, 
    TAD_COMMAND_RESULTS    = 0x20, 

    /* Constants for "state" flag */
    TAD_STATE_FOREGROUND   =    4,
    TAD_STATE_COMPLETE     =    8,
};


typedef enum {
    TAD_RAW_CSAP = 0,
    TAD_DATA_CSAP
} tad_csap_type;

typedef struct csap_instance
{
    int           id;             /**< CSAP id */

    int           depth;          /**< number of layers in stack */
    char       ** proto;          /**< array of protocol layers labels */
    void       ** layer_data;     /**< array of pointer to layer-specific data*/
    tad_csap_type type;

    csap_get_param_cb_t 
                * get_param_cb;   /**< array of pointers to callbacks for 
                                      get CSAP parameters */

    csap_read_cb_t       read_cb;       /**< read data from CSAP media */
    csap_write_cb_t      write_cb;      /**< write data to CSAP media */ 
    csap_write_read_cb_t write_read_cb; /**< write data and read answer. */
    csap_check_pdus_cb_t check_pdus_cb; /**< check PDUs sequence */

    csap_low_resource_cb_t prepare_recv_cb; /**< prepare for receive */
    csap_low_resource_cb_t prepare_send_cb; /**< prepare for send */
    csap_low_resource_cb_t release_cb; /**< prepare for send */

    csap_echo_method     echo_cb;/**< method for echo */


    int         read_write_layer;/**< number of layer in protocol stack 
                                   responsible for read and write operations */

    int         last_errno;      /**< errno of last operation */
    int         timeout;         /**< timeout for read operations in microseconds */

    char        answer_prefix[MAX_ANS_PREFIX]; /**< prefix for test-protocol 
                                             answer to the current command */

    struct timeval  wait_for;    /**< Zero or moment of timeout current 
                                       CSAP operation */
    struct timeval  first_pkt;   /**< moment of first good packet processed:
                                       matched in RX mode or sent in TX mode. */ 
    struct timeval  last_pkt;    /**< moment of last good packet processed:
                                       matched in RX mode or sent in TX mode. */

    unsigned int    num_packets; /**< number of good packets to be processed. */
    size_t          total_bytes; /**< quantity of total processed bytes in 
                                      last operation, for some protocols it is
                                      not sensible, but it is more convenient
                                      to do it generic parameter */
    uint8_t         command;     /**< current command flag */
    uint8_t         state;       /**< current state bitmask */
    pthread_mutex_t data_access_lock; /**< mutex for lock CSAP data 
                                        changing from different threads */
} csap_instance;

/**
 * template argument iteration
 */ 

/**
 * Locks access to CSAP shared flags and data. 
 * If already locked, wait until would be unlocked. 
 *
 * @param _csap_descr   pointer to CSAP descriptor structure. 
 */
#define CSAP_DA_LOCK(_csap_descr)                               \
    do {                                                        \
        pthread_mutex_lock(&((_csap_descr)->data_access_lock)); \
    } while (0)

/**
 * Try to lock access to CSAP shared flags and data. 
 * If already locked, sets _rc to EBUSY.
 *
 * @param _csap_descr   pointer to CSAP descriptor structure. 
 * @param _rc           variable for return code.
 */
#define CSAP_DA_TRYLOCK(_csap_descr, _rc)                              \
    do {                                                                \
        (_rc) = pthread_mutex_trylock(&((_csap_descr)->data_access_lock)); \
    } while (0)

/**
 * Unlocks access to CSAP shared flags and data. 
 *
 * @param _csap_descr   pointer to CSAP descriptor structure. 
 */
#define CSAP_DA_UNLOCK(_csap_descr)                               \
    do {                                                        \
        pthread_mutex_unlock(&((_csap_descr)->data_access_lock)); \
    } while (0)

typedef enum {
    ARG_INT,
    ARG_STR,
    ARG_OCT,
} arg_type_t;

typedef struct { 
    arg_type_t  type;
    size_t      length;
    union {
        int     arg_int;
        char    *arg_str;
        uint8_t *arg_oct;
    };
} tad_template_arg_t;

typedef enum {
    ARG_TMPL_FOR,
    ARG_TMPL_INT_SEQ,
    ARG_TMPL_STR_SEQ,
} arg_tmpl_type_t;   

enum {
    TAD_ARG_SIMPLE_FOR_BEGIN_DEF = 1,
    TAD_ARG_SIMPLE_FOR_STEP_DEF = 1,
};

typedef struct {
    arg_tmpl_type_t type;
    union {
        struct {
            size_t length;
            int    last_index; /**< index of last value */ 
            int  **ints;
        } int_set;
        struct {
            size_t length;
            int    last_index; /**< index of last value */ 
            char **strings;
        } str_set;
        struct {
            int begin;
            int end;
            int step;
        } simple_for;
    };
} tad_template_arg_spec_t;

/**
 * Init argument iteration array template arguments specification.
 * Memory block for array assumed to be allocated.
 *
 * @param arg_specs     array of template argument specifications.
 * @param arg_specs_num length of array.
 * @param arg_iterated  array of template arguments (OUT).
 *
 * @retval      - positive on success itertaion.
 * @retval      - zero if iteration finished.
 * @retval      - negative if invalid arguments passed.
 */
extern int tad_init_tmpl_args(tad_template_arg_spec_t *arg_specs,
                                          size_t arg_specs_num, 
                                          tad_template_arg_t *arg_iterated);


/**
 * Perform next iteration for passed template arguments.
 *
 * @param arg_specs     array of template argument specifications.
 * @param arg_specs_num length of array.
 * @param arg_iterated  array of template arguments (OUT).
 *
 * @retval      - positive on success itertaion.
 * @retval      - zero if iteration finished.
 * @retval      - negative if invalid arguments passed.
 */
extern int tad_iterate_tmpl_args(tad_template_arg_spec_t *arg_specs,
                                          size_t arg_specs_num, 
                                          tad_template_arg_t *arg_iterated);

/**
 * Get argument set from template ASN value and put it into plain-C array
 *
 * @param arg_set       ASN value of type "SEQENCE OF Template-Parameter",
 *                      which is subvalue with label 'arg-sets' in
 *                      Traffic-Template.
 * @param arg_specs     memory block for arg_spec array. should be allocated by 
 *                      user. 
 * @param arg_num       length of arg_spec array, i.e. quantity of
 *                      Template-Arguments in template.
 *
 * @return zero on success, otherwise error code. 
 */
extern int tad_get_tmpl_arg_specs (const asn_value *arg_set, 
                   tad_template_arg_spec_t *arg_specs, size_t arg_num);
            



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
 *      This method does not perform any actions related to CSAP functionality,
 *      neither processing of CSAP init parameters, nor initialyzing some 
 *      communication media units (for example, sockets, etc.).
 *      It only allocates memory for csap_instance sturture, set fields
 *      'id', 'depth' and 'proto' in it and allocates memory for 'layer_data'.
 * 
 * @param type  Type of CSAP: dot-separated sequence of textual layer labels.
 *
 * @return identifier of new CSAP or zero if error occured.
 */ 
extern int csap_create(const char *type);

/**
 * Destroy CSAP.
 *      Before call this DB method, all protocol-specific data in 'layer-data'
 *      and underground media resources should be freed. 
 *      This method will free all non-NULL pointers in 'layer-data', but 
 *      does not know nothing about what structures are pointed by them, 
 *      therefore if there are some more pointers in that structures, 
 *      memory may be lost. 
 *
 * @param csap_id       Identifier of CSAP to be destroyed.
 *
 * @return zero on success, otherwise error code 
 */ 
extern int csap_destroy (int csap_id);

/**
 * Find CSAP by its identifier.
 *
 * @param csap_id       Identifier of CSAP 
 *
 * @return 
 *    Pointer to structure with internal CSAP information or NULL if not found. 
 *    Change data in this structure if you really know what does it mean!
 */ 
extern csap_p csap_find (int csap_id);


/**
 * Type for reference to callback for init CSAP layer.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
typedef int (*csap_nbr_init_cb_t) (int csap_id, 
                                   const asn_value *csap_nds, int layer);
/**
 * Type for reference to callback for destroy CSAP layer.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
typedef int (*csap_nbr_destroy_cb_t) (int csap_id, int layer);

/**
 * Type for reference to callback for confirm PDU with CSAP parameters and 
 *      possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
typedef int (*csap_confirm_pdu_cb_t) (int csap_id, int layer, 
                                        asn_value * tmpl_pdu); 




struct csap_pkts;
typedef struct csap_pkts * csap_pkts_p;
typedef struct csap_pkts
{
    csap_pkts_p   next;
    void            * data;
    int               len;
    void           (* free_data_cb)(void*);
} csap_pkts;

/**
 * Type for reference to callback for generate binary data to be sent to media.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU. 
 * @param args          Template iteration parameters array, may be used to 
 *                      prepare binary data.
 * @param arg_num       Length of array above. 
 * @param up_payload    pointer to data which is already generated for upper 
 *                      layers and is payload for this protocol level. 
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one element, 
 *                      but need in fragmentation sometimes may occur. 
 *                      Of cause, on up level only one PDU is passed, 
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have possibility 
 *                      to de-fragment payload.
 *                      Callback is responsible for freeing of memory, used in 
 *                      up_payload list. 
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
typedef int (* csap_gen_bin_cb_t)
    (int csap_id,   int layer,
     const asn_value *tmpl_pdu,
     const tad_template_arg_t *args,
     size_t         arg_num,
     csap_pkts_p    up_payload,
     csap_pkts_p    pkts
    );

/**
 * Type for reference to callback for parse received packet and match it 
 * with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type 'Generic-PDU'. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
typedef int (* csap_match_bin_cb_t)
    (int   csap_id, int layer,
     const asn_value * pattern_pdu,
     const csap_pkts * pkt,
     csap_pkts *       payload,
     asn_value *       parsed_packet
    );




/**
 * Type for reference to callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according 
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return zero on success or error code.
 */
typedef int (* csap_gen_pattern_cb_t)   (int csap_id, int layer,
                                         const asn_value * tmpl_pdu, 
                                         asn_value *   *pattern_pdu);

struct csap_layer_neighbour_list_t;
typedef struct csap_layer_neighbour_list_t * csap_layer_neighbour_list_p;
typedef struct csap_layer_neighbour_list_t
{
    char * nbr_type; /**< symbolic identifier of neighvour. 
              May have such values: 
              NULL    - this means that layer which neighbours are listed 
                          is single in stack;
              "data"  - for data-CSAPs;
              id of lower neighbour level. */
    csap_layer_neighbour_list_p next; /**< pointer to the next possible
                                           neighbour*/

    csap_nbr_init_cb_t    init_cb;
    csap_nbr_destroy_cb_t destroy_cb;
   
} csap_layer_neighbour_list_t;

typedef struct csap_spt_type 
{
    char *proto;     /**< symbolic label of related protocol level */

    /* protocol-specific callbacks: */ 
    csap_confirm_pdu_cb_t confirm_cb;
    csap_gen_bin_cb_t     generate_cb;
    csap_match_bin_cb_t   match_cb;
    csap_gen_pattern_cb_t generate_pattern_cb;

    /* link to the list with possible (lower) neighbours, see description of
       this structure for more details. */ 
    csap_layer_neighbour_list_p neighbours;
} * csap_spt_type_p, csap_spt_type_t;



/**
 * Init CSAP support database
 *
 * @param spt_descr     CSAP layer support structure. 
 *
 * @return zero on success, otherwise error code. 
 */
extern int init_csap_spt (void);


/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure. 
 *
 * @return zero on success, otherwise error code. 
 */
extern int add_csap_spt (csap_spt_type_p spt_descr);


/**
 * Find structure for CSAP support respective to passed protocol label.
 *
 * @param proto      protocol label.
 *
 * @return pointer to structure or NULL if not found. 
 */
extern csap_spt_type_p find_csap_spt (const char * proto);

typedef struct tad_task_context
{
    csap_p      csap;
    asn_value * nds;

    struct rcf_comm_connection * rcf_handle;
} tad_task_context;

/**
 * Start routine for tr_recv thread. 
 *
 * @param arg      start argument, should be pointer to tad_task_context struct.
 *
 * @return nothing. 
 */
extern void * tad_tr_recv_thread(void * arg);

/**
 * Start routine for tr_send thread. 
 *
 * @param arg      start argument, should be pointer to tad_task_context struct.
 *
 * @return nothing. 
 */
extern void * tad_tr_send_thread(void * arg);

typedef enum {
    TAD_PLD_UNKNOWN,
    TAD_PLD_BYTES,
    TAD_PLD_LENGTH,
    TAD_PLD_SCRIPT,
    TAD_PLD_FUNCTION,
} tad_payload_type;

/**
 * Prepare binary data by NDS.
 *
 * @param csap_descr    CSAP description structure;
 * @param nds           ASN value with traffic-template NDS, should be
 *                      preprocessed (all iteration and function calls
 *                      performed);
 * @param handle        handle of RCF connection;
 * @param args          Template iteration parameters array, may be used to 
 *                      prepare binary data.
 * @param arg_num       Length of array above. 
 * @param pld_type      Type of payload in nds, passed to make function more
 *                      fast.
 * @param pld_data      Payload data read from original NDS.
 * @param pkts          packets with generated binary data (OUT).
 *
 * @return zero on success, otherwise error code.  
 */
extern int tad_tr_send_prepare_bin(csap_p csap_descr, asn_value * nds, 
                        struct rcf_comm_connection *handle, 
                        const tad_template_arg_t *args, size_t arg_num,
                        tad_payload_type pld_type, const void *pld_data, 
                        csap_pkts_p pkts);


/**
 * Confirm traffic template or pattern PDUS set with CSAP settings and 
 * protocol defaults. 
 * This function changes passed ASN value, user have to ensure that changes
 * will be set in traffic template or pattern ASN value which will be used 
 * in next operation. This may be done by such ways:
 *
 * Pass pointer got by asn_get_subvalue method, or write modified value 
 * into original NDS. 
 *
 * @param csap_descr    CSAP descriptor.
 * @param pdus          ASN value with SEQUENCE OF Generic-PDU (IN/OUT).
 *
 * @return zero on success, otherwise error code.
 */
extern int tad_confirm_pdus(csap_p csap_descr, asn_value *pdus);

/**
 * Transform payload symbolic type label of ASN choice to enum.
 *
 * @param label         char string with ASN choice label.
 *
 * @return tad_payload_type enum value.
 */
extern tad_payload_type tad_payload_asn_label_to_enum(const char *label);


/**
 * Type for reference to user function for some magic processing 
 * with matched pkt
 *
 * @param usr_param   String passed by user
 * @param pkt         Packet binary data, as it was caught from net.
 * @param pkt_len     Length of pkt data.
 *
 * @return zero on success or error code.
 */
typedef int (* tad_processing_pkt_method) (const char *usr_param, 
                                      const uint8_t *pkt, size_t pkt_len);



/**
 * Type for reference to user function for generating data to be sent.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl          ASN value with template. 
 *                      function should replace that field (which it should
 *                      generate) with #plain (in headers) or #bytes (in payload)
 *                      choice (IN/OUT)
 *
 * @return zero on success or error code.
 */
typedef int (* tad_user_generate_method) (int csap_id, int layer,
                                          asn_value *tmpl);


typedef enum {
    CONSTANT = 0, 
    ARG_LINK,
    EXP_ADD,
    EXP_SUBSTR,
    EXP_MULT,
    EXP_DIV,
    EXP_U_MINUS, 
} tad_expr_node_type;

typedef struct tad_int_expr_t tad_int_expr_t;

/**
 * Struct for arithmetic (and boolean?) expressions in traffic operations
 */
struct tad_int_expr_t 
{ 
    tad_expr_node_type  n_type; 
    size_t              d_len;  /**< length of data: 
                                     - for node with operation is length
                                       of array with operands. 
                                     - for constant node is 'sizeof' integer variable,
                                       may be 4 or 8.  */
    union {
        int32_t val_i32;
        int64_t val_i64;
        int     arg_num;
        tad_int_expr_t *exprs; /**< array with operands. */
    };
};

/**
 * Parse textual presentation of expression. Syntax is Perl-like, references to 
 * template arguments noted as $1, $2, etc.
 *
 * @param string        text with expression. 
 * @param expr          place where pointer to new expression will be put (OUT).
 * @param syms          quantity of parsed symbols, if syntax error occured,
 *                      error position (OUT).  
 *
 * @return status code.
 */
extern int tad_int_expr_parse(const char *string, 
                              tad_int_expr_t **expr, int *syms);

/**
 * Initialize tad_int_expr_t structure with single constant value, storing
 * binary array up to 8 bytes length. Array is assumed as "network byte order"
 * and converted to "host byte order" while saveing in 64-bit integer.
 *
 * @param arr   pointer to binary array.
 * @param len   length of array.
 *
 * @return  pointer to new tad_int_expr_r structure, or NULL if no memory 
 *          or too beg length passed.
 */
extern tad_int_expr_t *tad_int_expr_constant_arr(uint8_t *arr, size_t len);

/**
 * Initialize tad_int_expr_t structure with single constant value.
 *
 * @param n     value.
 *
 * @return pointer to new tad_int_expr_r structure.
 */
extern tad_int_expr_t *tad_int_expr_constant(int64_t n);

/**
 * Free data allocated for expression. 
 */
extern void tad_int_expr_free(tad_int_expr_t *expr);

/**
 * Calculate value of expression as function of argument set
 *
 * @param expr          expression structure.
 * @param args          array with arguments.
 * @param result        location for result (OUT).
 *
 * @return status code.
 */
extern int tad_int_expr_calculate(const tad_int_expr_t *expr, 
                      const tad_template_arg_t *args, int64_t *result);

/**
 * Convert 64-bit integer from network order to the host and vise versa. 
 *
 * @param n     integer to be converted.
 *
 * @return converted integer
 */
extern uint64_t tad_ntohll(uint64_t n);

typedef struct {
    size_t      length;
    uint8_t    *mask;
    uint8_t    *pattern;
} tad_match_mask_t;

typedef struct {
    size_t      length;
    int32_t    *begin;
    int32_t    *end;
} tad_match_intervals_t;

typedef enum {
    TAD_DU_DATA_NM,/* label for octet string value which should not be matched
                      this is default zero value for "undef" data_unit.*/
    TAD_DU_INT_NM, /* label for int value which should not be matched */
    TAD_DU_I32,
    TAD_DU_I64, 
    TAD_DU_STRING,
    TAD_DU_DATA,
    TAD_DU_MASK, 
    TAD_DU_INTERVALS, 
    TAD_DU_EXPR,
    TAD_DU_SCRIPT,
} tad_du_type_t;

typedef struct {
    tad_du_type_t       du_type; 
    union {
        int32_t                val_i32;
        int64_t                val_i64;
        char                  *val_string;
        tad_match_mask_t       val_mask;  /**< used for both MASK and DATA */
        tad_match_intervals_t  val_intervals;
        tad_int_expr_t        *val_int_expr;
    };
} tad_data_unit_t;

/**
 * Convert DATA-UNIT ASN field of PDU to plain C structure.
 * Memory need to store dynamic data, is allocated in this method and
 * should be freed by user. 
 *
 * @param pdu_val       ASN value with pdu, which DATA-UNIT field should be 
 *                      converted.
 * @param label         label of DATA_UNIT field in PDU.
 * @param location      location for converted structure (OUT). 
 *
 * @return zero on success or error code. 
 */ 
extern int tad_data_unit_convert(const asn_value *pdu_val, const char *label,
                                 tad_data_unit_t *location);

/**
 * Constract data-unit structure from specified binary data for 
 * simple per-byte compare. 
 *
 * @param data          binary data which should be compared.
 * @param d_len         length of data.
 * @param location      location of data-unit structure to be initialized (OUT)
 *
 * @return error status.
 */
extern int tad_data_unit_from_bin(const uint8_t *data, size_t d_len, 
                                  tad_data_unit_t *location);

/**
 * Clear data_unit structure, e.g. free data allocated for internal usage. 
 * Memory block used by data_unit itself is not freed!
 * 
 * @param du    pointer to data_unit structure to be cleared. 
 */
extern void tad_data_unit_clear(tad_data_unit_t *du);

/**
 * Generic method to match data in incoming packet with DATA_UNIT pattern
 * field. If data matches, it will be written into respective field in pkt_pdu. 
 * Label of field should be provided by user. If pattern has "plain"
 * type, data will be simply compared. If it is integer, data will be
 * converted from "network byte order" to "host byte order" before matching. 
 *
 * @param pattern       pattern structure. 
 * @param pkt_pdu       ASN value with parsed packet PDU (OUT). 
 * @param data          binary data to be matched.
 * @param d_len         length of data packet to be matched, in bytes. 
 * @param label         textual label of desired field.
 *
 * @return zero on success (that means "data matches to the pattern"),
 *              otherwise error code. 
 */
extern int tad_univ_match_field (const tad_data_unit_t *pattern,
                                 asn_value *pkt_pdu, uint8_t *data, 
                                 size_t d_len, const char *label);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*  __TE__TAD_CH__IMPL__H__ */

