/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * SNMP protocol implementaion internal declarations.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_TAPI_SNMP__H__
#define __TE_TAPI_SNMP__H__ 

#if HAVE_NET_SNMP_DEFINITIONS_H
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/definitions.h> 
#include <net-snmp/mib_api.h> 
#endif

#if 0
/* If there is no NET-SNMP package on your system, but there is UCD-SNMP, 
 * include these headers: */
#include <sys/types.h>
#include <ucd-snmp/asn1.h>
#include<ucd-snmp/snmp_impl.h>
#endif


#include <sys/types.h>
#include <time.h>
#include <sys/time.h>


#include "te_errno.h"

#include "asn_impl.h"
#include "asn_usr.h" 
#include "ndn_snmp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Strongly restricted maximal length of Object Identifier - this value 
is not succeded for all MIBs which are under testging. Really, it seems that 
this length can be exceeded only for "string" table indixes. */

#ifndef MAX_OID_LEN
#define MAX_OID_LEN 40
#endif

#ifndef MAX_LABLE_LEN
#define MAX_LABLE_LEN 50
#endif
	
	
/** SNMP Object Identifier */
typedef struct {
    size_t length;
    oid    id[MAX_OID_LEN];
} tapi_snmp_oid_t;


/** Type codes for SNMP variable binding values. Really SNMP uses some of 
    ASN.1 codes. Not all possible codes are defined, only usually used 
    and supported in TAD SNMP module. */
typedef enum {
    TAPI_SNMP_OTHER     = 0,	
    TAPI_SNMP_INTEGER   = ASN_INTEGER,  /**<    2 */
    TAPI_SNMP_OCTET_STR = ASN_OCTET_STR,/**<    4 */
    TAPI_SNMP_OBJECT_ID = ASN_OBJECT_ID,/**<    6 */
    TAPI_SNMP_IPADDRESS = ASN_IPADDRESS,/**< 0x40 */
    TAPI_SNMP_COUNTER   = ASN_COUNTER,  /**< 0x41 */
    TAPI_SNMP_UNSIGNED  = ASN_UNSIGNED, /**< 0x42 */
    TAPI_SNMP_TIMETICKS = ASN_TIMETICKS,/**< 0x43 */
    TAPI_SNMP_NOSUCHOBJ = SNMP_NOSUCHOBJECT,   /**< 0x80  = 128*/
    TAPI_SNMP_NOSUCHINS = SNMP_NOSUCHINSTANCE, /**< 0x81  = 129*/
    TAPI_SNMP_ENDOFMIB  = SNMP_ENDOFMIBVIEW,   /**< 0x82  = 130*/
} tapi_snmp_vartypes_t;

typedef enum {
    TAPI_SNMP_MIB_ACCESS_READONLY   = MIB_ACCESS_READONLY, /* 18 */
    TAPI_SNMP_MIB_ACCESS_READWRITE  = MIB_ACCESS_READWRITE, /* 19 */
    TAPI_SNMP_MIB_ACCESS_CREATE     = MIB_ACCESS_CREATE, /* 48 */
    
} tapi_snmp_mib_access;	

typedef enum {
    TAPI_SNMP_EXACT,
    TAPI_SNMP_NEXT, 
} tapi_snmp_get_type_t;

/** SNMP variable binding */
typedef struct { 
    tapi_snmp_oid_t      name;   /**< Object identifier of variable. */
    tapi_snmp_vartypes_t type;   /**< Type of variable. */
    size_t               v_len;  /**< length of data in variable value: 
                                   quantity of bytes for octet-sting types,
                                   number of sub-identifiers for OID type, 
                                   not used for integer types. */
    union {
        unsigned char   * oct_string;
        tapi_snmp_oid_t * obj_id;
        int               integer;
    };
} tapi_snmp_varbind_t;



typedef struct {
    ndn_snmp_msg_t type;    /**< type of message */
    int err_status;         /**< SNMP error, values: SNMP_ERR_* from 
                                  <net-snmp/library/snmp.h>; 
                                  used only  in SNMP replies. */
    int err_index;          /**< Index of wrong variable binding in request. 
                                 used only  in SNMP replies. */
    size_t num_var_binds;   /**< Number of variables in message. */
    tapi_snmp_varbind_t *vars;  /**< Pointer to array with variables.  */ 

    /**< Trap v1 only fields */
    tapi_snmp_oid_t   enterprise;
    long              gen_trap;
    long              spec_trap;
    uint8_t           agent_addr[4]; 
} tapi_snmp_message_t;


typedef struct {
    size_t   len;       /**< length of the octet string */
    uint8_t  data[0];   /**< pointer to the first octet */
} tapi_snmp_oct_string_t;

/**
 * Structure to contain snmp variable mib access
 */ 
typedef struct tapi_snmp_var_access {
    struct tapi_snmp_var_access  *next; /* Pointer to the next peer in sense of MIB structure */	
    char                          label[MAX_LABLE_LEN];
    tapi_snmp_oid_t               oid;
    tapi_snmp_mib_access          access;
} tapi_snmp_var_access;    

/**
 * Concatenate two object identifiers and put result into first one. 
 * Mainly intended for table indexes. 
 * 
 * @param base          base OID, will contain result (IN/OUT).
 * @param suffix        second OID, to be added to the base. 
 *
 * @return zero on success, otherwise error code.
 */
extern int tapi_snmp_cat_oid(tapi_snmp_oid_t *base, 
                             const tapi_snmp_oid_t *suffix);


/**
 * Boolean check is the first OID contained in second one.
 * 
 * @param tree          OID of MIB sub-tree.
 * @param node          OID of node to be checked on presence in sub-tree.
 *
 * @retval 1 node is in tree
 * @retval 0 otherwise
 */
extern int tapi_snmp_is_sub_oid(const tapi_snmp_oid_t *tree, 
                                const tapi_snmp_oid_t *node);


/** 
 * Callback function for the tapi_snmp_walk() routine, it is called
 * for each variable in a walk subtree.
 *
 * @param varbind       Variable binding, for which the callback function
 *                      is called.
 * @param userdata      Parameter, provided by the caller of tapi_snmp_walk().
 */
typedef int (*walk_callback)(const tapi_snmp_varbind_t *varbind, void *userdata);


/**
 * Free memory allocated in snmp varbind structure
 *
 * @param varbind  Varbind to be freed
 *
 * @return N/A
*/
extern void tapi_snmp_free_varbind(tapi_snmp_varbind_t *varbind);

/**
 * Free memory allocated in snmp message structure got from 
 * tapi_snmp_packet_to_plain function
 *
 * @param snmp_message  converted structure
 *
 * @return N/A
 */
extern void tapi_snmp_free_message(tapi_snmp_message_t *snmp_message);

/**
 * Convert SNMP-Message ASN.1 value to plain C structure.
 *
 * @param pkt           ASN.1 value of type SNMP-Message or Generic-PDU with 
 *                      choice "snmp". 
 * @param snmp_message  converted structure (OUT).
 *
 * @return zero on success or error code.
 */
extern int tapi_snmp_packet_to_plain(asn_value *pkt, 
                                     tapi_snmp_message_t *snmp_message);


/**
 * Creates usual SNMP CSAP on specified Test Agent and got its handle.
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF Session ID.
 * @param snmp_agent    Address of SNMP agent.
 * @param community     SNMP community.
 * @param snmp_version  SNMP version.
 * @param csap_id       identifier of an SNMP CSAP. (OUT)
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_csap_create(const char *ta, int sid, 
                                const char *snmp_agent, const char *community,
                                int snmp_version, int *csap_id);


/**
 * Creates generic SNMP CSAP on specified Test Agent and got its handle.
 * 
 * @param ta            Test Agent name.
 * @param sid           RCF Session ID.
 * @param snmp_agent    Address of SNMP agent.
 * @param community     SNMP community.
 * @param snmp_version  SNMP version.
 * @param rem_port      Remote UDP port.
 * @param local_port    Local UDP port.
 * @param timeout       Default timeout, measured in milliseconds. 
 * @param csap_id       identifier of an SNMP CSAP. (OUT)
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_gen_csap_create(const char *ta, int sid, 
                        const char *snmp_agent, const char *community, 
                        int snmp_version, uint16_t rem_port, uint16_t loc_port,
                        int timeout, int *csap_id);


/**
 * The function send SNMP-SET request for specified objects to the SNMP agent, 
 * associated with a particular SNMP CSAP. 
 * Note, that the function waits for SNMP agent response.
 * 
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param var_binds     Array with var-binds to be sent in request.
 * @param num_vars      Number of var-binds in array below.
 * @param errstat       Rhe value of error-status field in response
 *                      message (OUT), may be zero if not need.
 * @param errindex      Rhe value of error-index field in response
 *                      message (OUT), may be zero if not need.
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_set(const char *ta, int sid, int csap_id, 
                         const tapi_snmp_varbind_t *var_binds, 
                         size_t num_vars, int *errstat, int *errindex);


/**
 * The function send SNMP-SET request for specified objects to the SNMP agent, 
 * associated with a particular SNMP CSAP. 
 * Note, that the function waits for SNMP agent response.
 * 
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param errstat       Rhe value of error-status field in response
 *                      message (OUT), may be zero if not need.
 * @param errindex      Rhe value of error-index field in response
 *                      message (OUT), may be zero if not need.
 * @param common_index  Common index part of OID, which should be concatened
 *                      to all varbind's OIDs below; may be NULL. 
 * @param ...           Sequence of "varbind groups": label of MIB leaf
 *                      and value, which may be either integer, 
 *                      or pair <char *, int>, for OCTET_STRING types, 
 *                      or tapi_snmp_oid_t*;  ended by NULL.
 *                      Passed pointers are desired as 'const',
 *                      i.e. OID and data are not changed.
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_set_row(const char *ta, int sid, int csap_id, 
                             int *errstat, int *errindex,
                             const tapi_snmp_oid_t *common_index, ...);



/**
 * The function send SNMP-GET request for specified objects to the SNMP agent, 
 * associated with a particular SNMP CSAP. 
 * Note, that the function waits for SNMP agent response.
 * 
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param common_index  Common index part of OID, which should be concatened
 *                      to all varbind's OIDs below; may be NULL. 
 * @param ...           Sequence of "varbind groups": label of MIB leaf
 *                      and value, which may be either integer, 
 *                      or pair <char *, int>, for OCTET_STRING types, 
 *                      or tapi_snmp_oid_t*;  ended by NULL.
 *                      Passed pointers are desired as 'const',
 *                      i.e. OID and data are not changed.
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_get_row(const char *ta, int sid, int csap_id, 
                             const tapi_snmp_oid_t *common_index, ...);


/**
 * The function makes an attempt to set an integer value for an SNMP object
 * by means of SNMP Set request, which is sent to the SNMP agent, associated
 * with a particular SNMP CSAP. Note, that the function waits for SNMP agent
 * response.
 * 
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param oid           ID of an SNMP object the value is to be set.
 * @param value         integer value.
 * @param errstat       the value of error-status field in response
 *                      message (OUT), may be zero if not need.
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_set_integer(const char *ta, int sid, int csap_id, 
                                const tapi_snmp_oid_t *oid, 
                                int value, int *errstat);

/**
 * The function makes an attempt to set an octet string value for an SNMP
 * object by means of SNMP Set request, which is sent to the SNMP agent,
 * associated with an SNMP CSAP. Note, that the function waits for SNMP
 * agent response.
 * 
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param oid           ID of an SNMP object the value is to be set.
 * @param value         octet string value.
 * @param size          octet string size.
 * @param errstat       the value of error-status field in response
 *                      message (OUT), may be zero if not need.
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_set_octetstring(const char *ta, int sid, int csap_id, 
                                     const tapi_snmp_oid_t *oid,
                                     const unsigned char *value, 
                                     size_t size, int *errstat);

/**
 * The function makes an attempt to set a string value for an SNMP object
 * by means of SNMP Set request, which is sent to the SNMP agent, associated
 * with an SNMP CSAP. Note, that the argument string should contain only
 * printable characters. For a general octet string use
 * tapi_snmp_set_octetstring() function. Note, that the function waits for
 * SNMP agent response.
 * 
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param oid           ID of an SNMP object the value is to be set.
 * @param value         zero-terminated string.
 * @param errstat       the value of error-status field in response
 *                      message (OUT), may be zero if not need.
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_set_string(const char *ta, int sid, int csap_id, 
                                const tapi_snmp_oid_t *oid,
                                const char *value, int *errstat);


/**
 * The function can be used to send either SNMP Get request or SNMP GetNext
 * request to the SNMP agent, associated with an SNMP CSAP, and obtain
 * a variable binding agent have responded.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param oid           ID of an SNMP object.
 * @param next          the parameter defines which of the SNMP request should
 *                      be send (GetRequest or GetNextRequest).
 * @param varbind       location for variable binding, got SNMP agent for the
 *                      given OID. Pointers in structure (for complex long 
 *                      data) are allocated by this method and should be freed
 *                      by user. (OUT)
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_get(const char *ta, int sid, int csap_id, 
                         const tapi_snmp_oid_t *oid, 
                         tapi_snmp_get_type_t next,
                         tapi_snmp_varbind_t *varbind);


/**
 * The function can be used to send either SNMP Get-Bulk request 
 * to the SNMP agent, associated with an SNMP CSAP, and obtain
 * a variable binding agent have responded.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param v_oid         ID of an SNMP object.
 * @param num           number of elements in 'varbinds' (IN/OUT).
 * @param varbind       location for variable bindings, got SNMP agent for the
 *                      given OID. Pointers in structures (for complex long 
 *                      data) are allocated by this method and should be freed
 *                      by user. (OUT)
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_getbulk(const char *ta, int sid, int csap_id, 
                  const tapi_snmp_oid_t *v_oid, 
                  int *num, tapi_snmp_varbind_t *varbind);

/**
 * The function uses SNMP GetNext requests to query an agent for a tree of
 * information. An object identifier specifies which portion of the object
 * identifier space will be searched using GetNext requests. All variables
 * in the subtree below the given OID are queried, and for every such
 * variable a callback function is called.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       identifier of an SNMP CSAP.
 * @param oid           OID which defines a subtree to work with.
 * @param userdata      opaque data to be passed into the callback function.
 * @param callback      callback function, which is executed for each leaf.
 * 
 * @return zero on success or error code.
 */
extern int tapi_snmp_walk(const char *ta, int sid, int csap_id, 
                          const tapi_snmp_oid_t *oid, void *userdata, 
                          walk_callback callback);

/**
 * 
 */


/** The following functions are useful wrappers */

/**
 * Sends SNMP Get request on MIB object with type InetAddr to the SNMP agent,
 * associated with an SNMP CSAP and obtains a value the agent have responded.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       SNMP CSAP handle
 * @param oid           ID of an SNMP object
 * @param addr          Location for returned IPv4 address (OUT)
 *                      Buffer must be sizeof(struct in_addr) bytes long
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_ipaddr(const char *ta, int sid, int csap_id,
                                const tapi_snmp_oid_t *oid, void *addr);


/**
 * Sends SNMP Get request on MIB object with type DateAndTime 
 * to the SNMP agent, associated with an SNMP CSAP and obtains a value 
 * the agent have responded.
 *
 * @param ta               Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id          SNMP CSAP handle
 * @param oid              ID of an SNMP object
 * @param val              Location for returned time (OUT)
 * @param offset_from_utc  Location for signed offset from UTC octet
 *                         in minutes (OUT)
 */
extern int tapi_snmp_get_date_and_time(const char *ta, int sid, int csap_id, 
                                       const tapi_snmp_oid_t *oid, time_t *val,
                                       int *offset_from_utc);

/**
 * Sends SNMP Get request on MIB object with type Integer to the SNMP agent,
 * associated with an SNMP CSAP and obtains a value the agent have responded.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       SNMP CSAP handle
 * @param oid           ID of an SNMP object
 * @param val           Location for returned value (OUT)
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_integer(const char *ta, int sid, int csap_id,
                                 const tapi_snmp_oid_t *oid, int *val);

/**
 * Sends SNMP Get request on MIB object with type DisplayString to
 * the SNMP agent, associated with an SNMP CSAP and obtains a value 
 * the agent have responded.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       SNMP CSAP handle
 * @param oid           ID of an SNMP object
 * @param buf           Location for returned string (OUT)
 * @param buf_size      Number of bytes in 'buf'
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_string(const char *ta, int sid, int csap_id,
                                const tapi_snmp_oid_t *oid,
                                char *buf, size_t buf_size);

/**
 * Sends SNMP Get request on MIB object with type OctetString to
 * the SNMP agent, associated with an SNMP CSAP and obtains a value 
 * the agent have responded.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       SNMP CSAP handle
 * @param oid           ID of an SNMP object
 * @param buf           Location for returned value (OUT)
 * @param buf_size      Number of bytes in 'buf' on input and 
 *                      the number of bytes actually written on output (IN/OUT)
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_oct_string(const char *ta, int sid, int csap_id,
                                    const tapi_snmp_oid_t *oid,
                                    void *buf, size_t *buf_size);

/**
 * Sends SNMP Get request on MIB object with type ObjectIdentifier to
 * the SNMP agent, associated with an SNMP CSAP and obtains a value 
 * the agent have responded.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id.
 * @param csap_id       SNMP CSAP handle
 * @param oid           ID of an SNMP object
 * @param ret_oid       Location for returned value (OUT)
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_objid(const char *ta, int sid, int csap_id,
                               const tapi_snmp_oid_t *oid,
                               tapi_snmp_oid_t *ret_oid);


/**
 * Get SNMP table or one column from it, depends on passed OID.
 * 
 * @param ta            Test Agent name
 * @param sid           RCF Session id
 * @param csap_id       SNMP CSAP handle
 * @param table_oid     OID of SNMP table Entry object, or one leaf in this 
 *                      entry
 * @param num           Number of raws in table = height of matrix below (OUT)
 * @param result        Pointer to the allocated matrix with results, if only 
 *                      one column should be get, matrix width is 1, otherwise 
 *                      matrix width is greatest subid of Table entry (OUT)
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_table(const char *ta, int sid, int csap_id,
                               tapi_snmp_oid_t *table_oid,
                               int *num, void **result);

/**
 * Get SNMP table row, list of leafs will be got from parsed MIB tree.
 * Operation is performed as single SNMP-GET with multiple OIDs for all table
 * entries, and result is returned as SNMP message with varbind array.
 *
 * @param ta            Test Agent name
 * @param sid           RCF Session id
 * @param csap_id       SNMP CSAP handle
 * @param table_entry   OID of SNMP table Entry MIB node
 * @param num           number of suffixes
 * @param suffixes      Array with index suffixes of desired table rows
 * @param result        Pointer to the allocated matrix with results,
 *                      matrix width is greatest subid of Table entry (OUT)
 * 
 * @return zero on success or error code
 */
extern int tapi_snmp_get_table_rows(const char *ta, int sid, int csap_id,
                                    tapi_snmp_oid_t *table_entry,
                                    int num, tapi_snmp_oid_t *suffix,
                                    void **result);

/**
 * Get SNMP table dimension.
 * 
 * @param table_oid     OID of SNMP table Entry object, or one leaf in this 
 *                      entry
 * @param dimension     Table dimension (OUT)
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_table_dimension(tapi_snmp_oid_t *table_oid, int *dimension);


/**
 * Make table index.
 *
 * @param	tbl	Table OID
 * @param	index	Table index pointer
 * @param	...	Values to be inserted in index.id
 *
 * @return zero on success or error code
 * 
 */ 
extern int tapi_snmp_make_table_index(tapi_snmp_oid_t *tbl, tapi_snmp_oid_t *index, ...);
	

/**
 * Get SNMP table columns.
 * 
 * @param table_oid     OID of SNMP table Entry object, or one leaf in this 
 *                      entry
 * @param columns       Pointer to the allocated array of columns (OUT)
 *
 * @return zero on success or error code
 */
extern int tapi_snmp_get_table_columns(tapi_snmp_oid_t *table_oid, tapi_snmp_var_access **columns);



/**
 * Loads all text OIDs from MIB file
 *
 * @param dir_path  Path to directory with MIB files
 * @param mib_file  File name of the MIB file
 *
 * @return Status of the operation
 */
extern int tapi_snmp_load_mib_with_path(const char *dir_path,
                                        const char *mib_file);

/**
 * The macro simplify usage of tapi_snmp_load_mib function with using only
 * one argument instead of two.
 * In the caller context SUITE_SRCDIR macro MUST be defined.
 */
#define tapi_snmp_load_mib(mib_file_) \
    tapi_snmp_load_mib_with_path(SUITE_SRCDIR "/mibs", mib_file_)

/**
 * Load all mibs specified in configurator.conf file in "/snmp/mibs/load" list.
 *
 * @param dir_path  Path to directory where to search MIB files
 *
 * @return Status of the operation
 */
extern int tapi_snmp_load_cfg_mibs(const char *dir_path);

/**
 * Parses text representation of OID to TAPI SNMP OID data structure
 *
 * @param oid_str  OID string representation
 * @param oid      Location for parsed OID (OUT)
 *
 * @return  Status of the operation
 */
extern int tapi_snmp_make_oid(const char *oid_str, tapi_snmp_oid_t *oid);

/**
 * Get SNMP object syntax from MIB.
 *
 * @param	oid	OID of SNMP object
 * @param	syntax	syntax of SNMP object (OUT)
 *
 * @return	Status of the operation
 *
 */
extern int tapi_snmp_get_syntax(tapi_snmp_oid_t *oid,  tapi_snmp_vartypes_t *syntax);

/** 
 * Callback function for the catching of SNMP traps. 
 *
 * @param trap          SNMP message with trap.
 * @param userdata      Parameter, provided by the caller of tapi_snmp_walk().
 */
typedef int (*tapi_snmp_trap_callback)(const tapi_snmp_message_t *trap, void *userdata);



/**
 * Start receive process on specified SNMP CSAP. If process
 * was started correctly (rc is OK) it can be managed by common RCF
 * methods 'rcf_ta_trrecv_wait', 'rcf_ta_trrecv_get' and 'rcf_ta_trrecv_stop'.
 *
 * @param ta_name   - Test Agent name
 * @param sid       - RCF session
 * @param eth_csap  - CSAP handle 
 * @param pattern   - ASN value with receive pattern
 * @param cb        - Callback function which will be called for each 
 *                    received frame, may me NULL if frames are not need
 * @param cb_data   - pointer to be passed to user callback
 * @param timeout   - Timeout for receiving of packets, measured in 
 *                    milliseconds
 * @param num       - Number of packets caller wants to receive
 *
 * @return zero on success, otherwise standard or common TE error code.
 */
extern int tapi_snmp_trap_recv_start(const char *ta_name, int sid,
                                     int snmp_csap, const asn_value *pattern,
                                     tapi_snmp_trap_callback cb, void *cb_data,
                                     unsigned int timeout, int num);


/**
 * Parses text representation of OID to TAPI SNMP OID data structure then
 * adds instance indexes to the TAPI SNMP OID data structure.
 *
 * @param oid_str  OID string representation
 * @param oid  Location for parsed OID (OUT)
 * @param ...  Indexes of table field instance
 *
 * @return  Status of the operation
 */
int
tapi_snmp_make_instance(const char *oid_str, tapi_snmp_oid_t *oid, ...);


/**
 * Print SNMP OID struct to string and return pointer to this string. 
 * Note, that buffer with string is static and behaviour of this function 
 * in multithread usage may be unpredictable. 
 *
 * @param oid       - pointer to structure with OID to be printed. 
 *
 * @return pointer to buffer with printed OID. 
 */
extern const char* print_oid (const tapi_snmp_oid_t *oid);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_SNMP__H__ */
