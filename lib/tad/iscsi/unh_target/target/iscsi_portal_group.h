/*	target/iscsi_portal_group.h

	vi: set autoindent tabstop=8 shiftwidth=8 :

	header file for defining the IP addresses, ports and portal group tags
	on which the target will listen for connections from initiators.

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

#ifndef ISCSI_PORTAL_GROUP
#define ISCSI_PORTAL_GROUP

/* portal group tag used by default */
#define DEFAULT_TARGET_PORTAL_GROUP_TAG 1

/* following are string versions of defines in <linux/in.h>, <linux/in6.h> */
#define INADDR_ANY_STRING	"0.0.0.0"
#define IN6ADDR_ANY_STRING	"[::]"

#define INADDR_LOOPBACK_STRING	"127.0.0.1"
#define IN6ADDR_LOOPBACK_STRING	"[::1]"


struct portal_group
{
        char *ip_string;
        char *port_string;
        uint16_t tag;
	uint16_t in_use;
	int family;
	struct sockaddr *ip_address;
	int ip_length;
};

/* Table to store ip, port, and tag for different server listening sockets,
   Add more portals as you need in the same way as the examples, 
   Note: no duplicate portals allowed, and IP must be valid and active */
extern struct portal_group iscsi_portal_groups[MAX_PORTAL];

#endif
