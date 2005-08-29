/*	common/tcp_utilities.h
 * 
 *	These are some special routines to manipulate TCP settings
 *
 *	vi: set autoindent tabstop=8 shiftwidth=4 :
 *
 *	This file contains auxilliary functions for iscsi initiator 
 *	code that are responsible for dealing with error recovery.
 *
 *	Copyright (C) 2001-2003 InterOperability Lab (IOL)
 *	University of New Hampshier (UNH)
 *	Durham, NH 03824
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2, or (at your option)
 *	any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *	USA.
 *
 *	The name of IOL and/or UNH may not be used to endorse or promote 
 *	products derived from this software without specific prior 
 * 	written permission.
*/

#ifndef	_TCP_UTILITIES_H
#define	_TCP_UTILITIES_H

/*	include files not latched correctly -- this is in <netinet/in.h> */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#include <sys/socket.h>

/*	Called to turn off the Nagle Algorithm on this socket */
void tcp_nagle_off(int sock);

/*	Called to set option that allows this socket's port to be reused quickly */
void tcp_reuse_port(int sock);

/* convert binary sockaddr structure
 * to printable dotted-decimal value in ip_string and
 * printable numeric port value in port_string.
 * Returns AF_INET if ip_address is ipv4,
 *	   AF_INET6 if ip_address is ipv6,
 *	   <0 otherwise
 */
int
cnv_inet_to_string(struct sockaddr *ip_address, char *ip_string,
		   char *port_string);

/* convert printable ip_string and port_string
 * to newly-allocated binary sockaddr structure.
 * Sets length to number of bytes in sockaddr that are actually used.
 * Returns AF_INET if ip_string is valid ipv4 dotted-decimal value,
 *		   or is binary ip address
 *	   AF_INET6 if ip_string is valid ipv6 colon-separated value,
 *	   <0 otherwise (sockaddr is not allocated, length is not set)
 */
int
cnv_string_to_inet(char *ip_string, char *port_string,
		   struct sockaddr **ip_address, int *length);

/* copy an existing ip sockaddr structure
 * to a newly-allocated binary sockaddr structure.
 * and set new_ip_length to number of bytes in sockaddr actually used.
 * Returns AF_INET if structure is ipv4,
 *	   AF_INET6 if structure is ipv6,
 *	   <0 otherwise (new structure is not allocated, new length not set)
 */
int
dup_inet_struct(struct sockaddr *old_ip_address,
		struct sockaddr **new_ip_address,
		int *new_ip_length);

#endif
