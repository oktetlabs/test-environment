/** @file
 * @breaf TE TAD Forwarder additional module internal declarations
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_FORWARDER_H__
#define __TE_FORWARDER_H__ 

#include "te_stdint.h" 
#include "tad_csap_inst.h"

/* 
 * =============== Route manipulation routines ==================
 */

/**
 * Create new forw task with specified parameters, associate it with 
 * specified send queue.
 *
 * @param forw_asn_spec         Textual presentation of ASN value with
 *                              parameters for new forwareder task
 * @param sendq_id              ID of send queue
 *
 * @return zero on success or error status
 */
extern int tadf_add_forw_task(const char *forw_asn_spec, int sendq_id);

/**
 * Destroy specified forw task. 
 *
 * @param forw_name     Name of forw task to be destroyed
 *
 * @return zero on success or error status
 */
extern int tadf_del_forw_task(const char *forw_name);

/**
 * Set parameter of specified forw task, not stopping its functionality.
 *
 * @param forw_name     Name of forw task whose parameter should be 
 *                      changed
 * @param param_spec    Name of parameter to be changed, it should be 
 *                      correct ASN label of field in forw_task ASN 
 *                      specification, passed during forw task creation
 * @param value         New value of parameter
 *
 * @return zero on success or error status
 */
extern int tadf_forw_task_set_param(const char *forw_name, 
                                    const char *param_spec, int value);

/**
 * Process packet via specified forw task. 
 * It includes: make drop, delay, and reorder (if specifed) and 
 * put it into send queue, which is attached to this forw task. 
 *
 * @param forw_name     Name of forw task 
 * @param pkt           Pointer to the packet data
 * @param pkt_len       Length of packet
 *
 * @return zero on success or error status
 */
extern int tadf_forw_packet(csap_p csap_descr, const char *forw_name,
                            const uint8_t *pkt, size_t pkt_len);


#endif /*  __TE_FORWARDER_H__ */

