/** @file
 * @brief Windows Test Agent
 *
 * Dummy definitions for Windows agent
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id: win32_rpc.h 3672 2004-07-20 00:00:04Z helen $
 */
 
#ifndef __WIN32_DUMMY_H__
#define __WIN32_DUMMY_H__

#undef ERROR

#define ERESTART        1000
#define ESTRPIPE        1001
#define EUCLEAN         1002
#define ENOTNAM         1003
#define ENAVAIL         1004
#define EISNAM          1005
#define EREMOTEIO       1006
#define EMEDIUMTYPE     1007

#define AF_PACKET       17

#define PF_PACKET       AF_PACKET

#define MSG_DONTWAIT    -201
#define MSG_WAITALL     -202
#define MSG_TRUNC       -203
#define MSG_CTRUNC      -204
#define MSG_ERRQUEUE    -205

#define SO_BINDTODEVICE -301
#define SO_PRIORITY     -302

#define IP_PKTINFO      -401
#define IP_RECVERR      -402
#define IP_RECVOPTS     -403
#define IP_RECVTOS      -404
#define IP_RECVTTL      -405
#define IP_RECVPTS      -406


#define SIOCGSTAMP      -501
#define SIOCSPGRP       -502
#define SIOCGPGRP       -503
#define SIOCSIFFLAGS    -504
#define SIOCSIFADDR     -505
#define SIOCSIFNETMASK  -506
#define SIOCSIFBRDADDR  -507
#define SIOCGIFDSTADDR  -508
#define SIOCSIFDSTADDR  -509
#define SIOCSIFMTU      -510

#define SIGUNUSED       33

#define AI_PASSIVE      -601
#define AI_CANONNAME    -602
#define AI_NUMERICHOST  -603

#define EAI_BADFLAGS    -701
#define EAI_NONAME      -702
#define EAI_AGAIN       -703
#define EAI_FAIL        -704
#define EAI_NODATA      -705
#define EAI_FAMILY      -706
#define EAI_SOCKTYPE    -707
#define EAI_SERVICE     -708
#define EAI_ADDRFAMILY  -709
#define EAI_MEMORY      -710
#define EAI_SYSTEM      -711

#define IFF_DEBUG       -801
#define IFF_POINTOPOINT -802
#define IFF_NOARP       -803
#define IFF_ALLMULTI    -804
#define IFF_MASTER      -805
#define IFF_SLAVE       -806
#define IFF_PORTSEL     -807

#define IP_RETOPTS      -901

#endif /* __WIN32_DUMMY_H__ */
