/** @file
 * @brief Power Distribution Unit Proxy Test Agent
 *
 * Net-SNMP library wrapper functions.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 * 
 * $Id$
 */

#include "te_config.h"
#include "config.h"

#include "te_stdint.h"
#include "te_errno.h"

#ifdef HAVE_NET_SNMP_SESSION_API_H
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/utilities.h>
#include <net-snmp/session_api.h>
#include <net-snmp/pdu_api.h>
#else
#error Cannot compile without Net-SNMP library headers.
#endif

/* SNMP session information */
typedef netsnmp_session ta_snmp_session;

/* Component of OID */
typedef oid ta_snmp_oid;

/* Data types for OID values */
typedef enum {
    TA_SNMP_NULL    = ASN_NULL,
    TA_SNMP_INTEGER = ASN_INTEGER,
    TA_SNMP_STRING  = ASN_OCTET_STR,
} ta_snmp_type_t;

/** Initialize Net-SNMP library */
void ta_snmp_init();

/**
 * Open new SNMP session for getting/setting values at the specified agent
 *
 * @param net_addr   Address of SNMP agent
 * @param version    SNMP version to use
 * @param community  SNMP v1/2c community name to use
 *
 * @return Pointer to new SNMP session structure, or NULL if failed.
 */
ta_snmp_session *ta_snmp_open_session(const struct sockaddr *net_addr,
                                      long int version,
                                      const char *community);

/**
 * Close opened SNMP session
 *
 * @param session  Close opened SNMP session
 */
void ta_snmp_close_session(ta_snmp_session *session);

/**
 * Set value of single SNMP object
 *
 * @param session    Opened SNMP session
 * @param oid        OID of the object to set value
 * @param oid_len    Length of OID (in OID components)
 * @param type       Value data type
 * @param value      Pointer to value data
 * @param value_len  Length of value data (in bytes)
 *
 * @return Status code.
 */
te_errno ta_snmp_set(ta_snmp_session *session, ta_snmp_oid *oid,
                     size_t oid_len, ta_snmp_type_t type,
                     const uint8_t *value, size_t value_len);

/**
 * Get the value of single SNMP object (generic function)
 *
 * @param session  Opened SNMP session
 * @param oid      OID of the object to get value from
 * @param oid_len  Length of OID (in OID components)
 * @param type     Value data type
 * @param buf      Buffer for value data
 * @param buf_len  Length of buffer (in bytes)
 *
 * @return Status code.
 */
te_errno ta_snmp_get(ta_snmp_session *session, ta_snmp_oid *oid,
                     size_t oid_len, ta_snmp_type_t *type,
                     uint8_t *buf, size_t *buf_len);

/**
 * Get integer SNMP value of single SNMP object
 *
 * @param session  Opened SNMP session
 * @param oid      OID of the object to get value from
 * @param oid_len  Length of OID (in OID components)
 * @param value    Place for value data
 *
 * @return Status code.
 */
te_errno ta_snmp_get_int(ta_snmp_session *session, ta_snmp_oid *oid,
                         size_t oid_len, long int *value);

/**
 * Get octet string SNMP value of single SNMP object
 * (value will be terminated by adding trailing zero byte)
 *
 * @param session  Opened SNMP session
 * @param oid      OID of the object to get value from
 * @param oid_len  Length of OID (in OID components)
 * @param buf      Buffer for value data
 * @param buf_len  Length of buffer (IN), actual length of value (OUT)
 *                 (in bytes)
 *
 * @return Status code.
 */
te_errno ta_snmp_get_string(ta_snmp_session *session, ta_snmp_oid *oid,
                            size_t oid_len, char *buf, size_t *buf_len);

