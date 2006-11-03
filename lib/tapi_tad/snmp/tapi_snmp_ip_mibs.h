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
 *
 * $Id$
 */
#ifndef __TE__TAPI_SNMP_IP_MIBS_H__
#define __TE__TAPI_SNMP_IP_MIBS_H__ 

/** Rows in an instance of an IP-MIB::ipAddressTable */
typedef struct tapi_snmp_ip_address_table_row_s {
    tapi_snmp_oid_t *index_suffix;   /**< index suffix of OID */

    int                    *ipAddressAddrType;     /* InetAddressType */
    tapi_snmp_oct_string_t *ipAddressAddr;         /* InetAddress */
    int                    *ipAddressIfIndex;      /* InterfaceIndex */
    int                    *ipAddressType;         /* INTEGER */
    tapi_snmp_oid_t        *ipAddressPrefix;       /* RowPointer */
    int                    *ipAddressOrigin;       /* IpAddressOriginTC */
    int                    *ipAddressStatus;       /* IpAddressStatusTC */
    uint32_t               *ipAddressCreated;      /* TimeStamp */
    uint32_t               *ipAddressLastChanged;  /* TimeStamp */
    int                    *ipAddressRowStatus;    /* RowStatus */
    int                    *ipAddressStorageType;  /* StorageType */ 
} tapi_snmp_ip_address_table_row_t;

/** Rows in an instance of an IP-MIB::ipNetToMediaTable */
typedef struct tapi_snmp_ip_net_to_media_table_row_s {
    tapi_snmp_oid_t *index_suffix;   /**< index suffix of OID */

    int                    *ipNetToMediaIfIndex;      /* INTEGER */
    tapi_snmp_oct_string_t *ipNetToMediaPhysAddress;  /* PhysAddress */
    struct in_addr         *ipNetToMediaNetAddress;   /* IpAddress */
    int                    *ipNetToMediaType;         /* INTEGER */

} tapi_snmp_ip_net_to_media_table_row_t;

/** Rows in an instance of an IP-FORWARD-MIB::ipCidrRouteTable */
typedef struct tapi_snmp_ip_cidr_route_table_row_s {
    tapi_snmp_oid_t *index_suffix;   /**< index suffix of OID */

    struct in_addr      *ipCidrRouteDest;       /* IpAddress */
    struct in_addr      *ipCidrRouteMask;       /* IpAddress */
    int                 *ipCidrRouteTos;        /* Integer32 */
    struct in_addr      *ipCidrRouteNextHop;    /* IpAddress */
    int                 *ipCidrRouteIfIndex;    /* Integer32 */
    int                 *ipCidrRouteType;       /* INTEGER */
    int                 *ipCidrRouteProto;      /* INTEGER */
    int                 *ipCidrRouteAge;        /* Integer32 */
    tapi_snmp_oid_t     *ipCidrRouteInfo;       /* OBJECT IDENTIFIER */
    int                 *ipCidrRouteNextHopAS;  /* Integer32 */
    int                 *ipCidrRouteMetric1;    /* Integer32 */
    int                 *ipCidrRouteMetric2;    /* Integer32 */
    int                 *ipCidrRouteMetric3;    /* Integer32 */
    int                 *ipCidrRouteMetric4;    /* Integer32 */
    int                 *ipCidrRouteMetric5;    /* Integer32 */
    int                 *ipCidrRouteStatus;     /* RowStatus  */
} tapi_snmp_ip_cidr_route_table_row_t;

#endif /* __TE__TAPI_SNMP_IFTABLE_H__ */
