/*	target/iscsi_portal_group.c

	vi: set autoindent tabstop=8 shiftwidth=8 :

 	This is the table of IP addresses, ports and portal group tags
	on which the iscsi target will listen for connections from initiators.

	Users should modify this table to define their target configuration.

*/
/*
	Copyright (C) 2001-2004 InterOperability Lab (IOL)
				University of New Hampshire (UNH)
				Durham, NH 03824

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
	USA.

	The name of IOL and/or UNH may not be used to endorse or promote
	products derived from this software without specific prior
	written permission.
*/

#include "iscsi_target.h"

/* Table to store port, ip and tag for different server listening sockets,
 * Add more portals as you need in the same way as the examples, 
 * Note: no duplicate portals allowed, and IP must be valid and active.
 *
 * Also Note: IPv6 addresses MUST be enclosed in brackets [], as required in
 * Draft 20 Section 12.8 TargetAddress
 * "TargetAddress=domainname[:port][,portal-group-tag]
 * The domainname can be specified as either a DNS host name, a dotted-decimal
 * IPv4 address, or a bracketed IPv6 address as specified in [RFC2732]."
 *
 * In this table the first entry is just the IPv4 or IPv6 address as a string
 * (we do NOT accept DNS host names in this table), the second entry is just
 * the port number as a string, and the third entry is just the portal group
 * tag as an integer.
 *
 * In response to a SendTargets= during discovery session, the target will
 * reply with the strings in this table, exactly as given here.
 */

struct portal_group iscsi_portal_groups[MAX_PORTAL] =
{
	/* Default IANA portal */	
	{INADDR_ANY_STRING, ISCSI_WKP_STRING, DEFAULT_TARGET_PORTAL_GROUP_TAG},

	/* Default IANA system portal */
	{INADDR_ANY_STRING, ISCSI_SYSTEM_PORT_STRING,
	 DEFAULT_TARGET_PORTAL_GROUP_TAG},

	/* example portals */

	/* {"132.177.117.67", "5000", 3}, */	/* tweety */
	/* {"192.168.10.17", "5001", 2}, */	/* tweety-gig */
	/* {"192.168.10.17", "5002", DEFAULT_TARGET_PORTAL_GROUP_TAG}, *//* tweety-gig */
	/* {"[fe80::207:e9ff:fe19:9c35]", "5001", 2}, */ /* tweety-gig */
	/* {"[fe80::2e0:29ff:fe6c:70dc]", "5000", 3}, */ /* tweety */

	/* {"132.177.118.45", ISCSI_WKP_STRING, 3},	oakenfold */
	/* {"[2001:468:603:c001:0:7ff:fee3:c72c]", ISCSI_WKP_STRING, 3}, */

	/* end of table with NULL string pointers and tag of 0 */
	{NULL, NULL, 0}
};
