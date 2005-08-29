/*	common/tcp_utilities.c
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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string.h>

#include "../userland_lib/my_memory.h"
#include "debug.h"
#include "tcp_utilities.h"

/*	Called to turn off the Nagle Algorithm on this socket */
void
tcp_nagle_off(int sock)
{
    int optval = 1;
    int rc;

    rc = setsockopt(sock, SOL_TCP, TCP_NODELAY, &optval, sizeof(int));
}

/*	Called to set option that allows this socket's port to be reused quickly */
void
tcp_reuse_port(int sock)
{
    (void)sock;
}

/* convert binary sockaddr structure
 * to printable dotted-decimal or colon-hex value in ip_string and
 * printable numeric port value in port_string.
 * Returns AF_INET if ip_address is ipv4,
 *	   AF_INET6 if ip_address is ipv6,
 *	   <0 otherwise
 */
int
cnv_inet_to_string(struct sockaddr *ip_address, char *ip_string,
		   char *port_string)
{
	uint8_t *ptr;
	uint32_t k;
	int i, c;

	if (ip_address == NULL) {
		i = -EINVAL;
	} else if (ip_address->sa_family == AF_INET) {
		ptr = (uint8_t *)&((struct sockaddr_in *)ip_address)
							->sin_addr.s_addr;
		for (i = 0; i < 3; i++) {
			ip_string += sprintf(ip_string, "%u.", *ptr++);
		}
		sprintf(ip_string, "%u", *ptr);
		sprintf(port_string, "%u",
			ntohs(((struct sockaddr_in *)ip_address)->sin_port));
		i = AF_INET;
	} else if (ip_address->sa_family == AF_INET6) {
		ptr = (uint8_t *)((struct sockaddr_in6 *)ip_address)
			->sin6_addr.s6_addr;
		c = '[';
		for (i = 0; i < 8; i++ ) {
			k = *ptr++ << 8;
			k += *ptr++;
			ip_string += sprintf(ip_string, "%c%x", c, k);
			c = ':';
		}
		sprintf(ip_string, "]");
		sprintf(port_string, "%u",
			ntohs(((struct sockaddr_in6 *)ip_address)->sin6_port));
		i = AF_INET6;
	} else {
		strcpy(ip_string, "Unknown protocol family");
		i = -EPFNOSUPPORT;
	}
	return i;
}

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
		   struct sockaddr **ip_address, int *length)
{
	char *ptr, *ptr2;
	int i, k, span;
	uint32_t uj;
	struct sockaddr *ip;
	uint16_t ipv6_array[8];

	TRACE(TRACE_ENTER_LEAVE, "Enter cnv_string_to_inet, ip %s, port %s\n",
				ip_string, port_string);

	ip = (struct sockaddr *)malloc(sizeof(struct sockaddr));
	TRACE(TRACE_NET, "Alloc ip_address %p\n", ip);

	if (ip == NULL) {
		/* couldn't get memory for structure, leave with error */
		i = -ENOMEM;
		goto out;
	}

	memset(ip, 0, sizeof(struct sockaddr));

	ptr = strchr(ip_string, ':');	/* first colon in string, if any */
	ptr2 = strchr(ip_string, '.');	/* first dot in string, if any */

	if (ptr != NULL && (ptr2 == NULL || ptr2 > ptr)) {
		/* found a colon that precedes any dot in ip_string,
		   assume ipv6 address */
		ptr = ip_string;
		if (*ptr == '[')
			ptr++;		/* must have [ipv6] */
		if (*ptr == ':') {
			/* string starts with colon, next char must also be : */
			ptr++;
			if (*ptr != ':') {
				goto out2;
			}
		}
		span = -1;		/* initially no span found */
		i = 0;
		while (i < 8) {
			if (*ptr == ':') {
				/* hit end of a span, must be the only one */
				if (span >= 0) {
					/* already hit a span, can't have 2nd */
					goto out2;
				}
				span = i;	/* we hit span here */
				ptr++;		/* skip over : at end of span */
			} else if ((uj = strspn(ptr, "0123456789abcdefABCDEF"))
									== 0) {
				/* not a hex digit or a colon */
				if (*ptr == '\0' || strspn(ptr, " ]")) {
					/* hit end legitimate end of string */
					break;
				} else {
					/* illegal character in string */
					goto out2;
				}
			} else if (uj>4) {
				/* too many hex digits in field (max of 4) */
				goto out2;
			} else {
				/* must be a number, find out how it ends */
				if (*(ptr+uj) == '.') {
					/* it ends with dot, must be decimal */
					for (k = 0; k < 4; k++) {
						if (k & 1) {
							ipv6_array[i++] = htons(
								(uj << 8)
							       + strtoul(
								ptr, &ptr, 10));
						} else {
							uj = strtoul(
								ptr, &ptr, 10);
						}
						if (*ptr != '.' && k < 3) {
							/* illegal character */
							goto out2;
						}
						ptr++;	/* skip over . */
					}
					break;	/* dotted-decimal ends scan */
				} else {
					/* does not end with dot, must be hex */
					ipv6_array[i++] = htons(strtoul(
								ptr, &ptr, 16));
					/* skip ending colon, else loop done */
					if (*ptr != ':')
						break;
					ptr++;		/* skip over : */
				}
			}
		}
		if (span >= 0) {
			/* have to adjust a span to get full 8 chars */
			k = 7;
			i = i-1;
			while (i >= span) {
				ipv6_array[k--] = ipv6_array[i--];
			}
			while (k >= span) {
				ipv6_array[k--] = 0;
			}
		} else if (i != 8) {
			goto out2;
		}

		/* finally, move ipv6 binary string into structure */
		memcpy(((struct sockaddr_in6 *)ip)->sin6_addr.s6_addr,
								ipv6_array, 16);
		((struct sockaddr_in6 *)ip)->sin6_family = i = AF_INET6;
		((struct sockaddr_in6 *)ip)->sin6_port =
				htons(strtoul(port_string, NULL, 0));
		/* seems to need a non-zero scope_id -- why 64?? */
		((struct sockaddr_in6 *)ip)->sin6_scope_id = 64;
		*ip_address = ip;
		*length = sizeof(struct sockaddr_in6);
	} else {
		if (ptr2 != NULL) {
			/* found a dot that precedes any colon in ip_string,
			   assume dotted-decimal ipv4 address */
			uj = 0;
			ptr = ip_string;
			for (i = 0;  i < 4;  i++) {
				uj = (uj << 8) + strtoul(ptr, &ptr, 10);
				if (*ptr != '.' && i < 3) {
					TRACE_ERROR("Illegal ipv4 string %s\n",
							ip_string);
					i = -EINVAL;
					goto out1;
				}
				ptr++;
			}
			TRACE(TRACE_ISCSI, "binary ipv4 address 0x%08x\n", uj);
			((struct sockaddr_in *)ip)->sin_addr.s_addr = htonl(uj);
		} else {
			/* no colon or dot in ip_string,
			   assume numeric ipv4 addr*/
			((struct sockaddr_in *)ip)->sin_addr.s_addr =
				htonl(strtoul(ip_string, NULL, 0));
		}
		((struct sockaddr_in *)ip)->sin_family = i = AF_INET;
		((struct sockaddr_in *)ip)->sin_port =
				htons(strtoul(port_string, NULL, 0));
		*ip_address = ip;
		*length = sizeof(struct sockaddr_in);
	}
out:
	TRACE(TRACE_ENTER_LEAVE, "Leave cnv_string_to_inet, retval %d\n", i);
	return i;
out2:
	TRACE_ERROR("Illegal ipv6 string %s\n", ip_string);
	i = -EINVAL;
out1:
	TRACE(TRACE_NET, "Free ip_address %p\n", ip);
	my_free((void *) &ip);
	goto out;
}

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
		int *new_ip_length)
{
	int i;

	*new_ip_address = (struct sockaddr *)malloc(sizeof(struct sockaddr));
	TRACE(TRACE_NET, "Alloc ip_address %p\n", *new_ip_address);

	if (*new_ip_address == NULL) {
		/* couldn't get memory for structure, leave with error */
		i = -ENOMEM;
	} else {
		memcpy(*new_ip_address,old_ip_address,sizeof(struct sockaddr));
		i = (*new_ip_address)->sa_family;
		if (i == AF_INET6)
			*new_ip_length = sizeof(struct sockaddr_in6);
		else if (i == AF_INET)
			*new_ip_length = sizeof(struct sockaddr_in);
		else {
			TRACE_ERROR("Unknown protocol family %d\n", i);
			my_free((void *)new_ip_address);
			i = -EPFNOSUPPORT;
		}
	}
	return i;
}
