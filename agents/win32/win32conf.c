/** @file
 * @brief Windows Test Agent
 *
 * Windows TA configuring support
 *
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id: win32conf.c 3612 2004-07-19 10:13:31Z arybchik $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <w32api/winsock2.h>

#include "te_errno.h"
#include "te_defs.h"
#include "logger_ta.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#define ARP_TABLE_SIZE   400
#include "windows.h"
#include "iprtrmib.h"
#include "w32api/iphlpapi.h"



#define TE_LGR_USER     "Windows Conf"
#include "logger_api.h"

/* TA name pointer */
extern char *ta_name;

RCF_PCH_CFG_NODE_AGENT(node_agent, NULL);

/**
 * Get root of the tree of supported objects.
 *
 * @return root pointer
 */
rcf_pch_cfg_object *
rcf_ch_conf_root()
{
    return &node_agent;
}

/**
 * Get Test Agent name.
 *
 * @return name pointer
 */
const char *
rcf_ch_conf_agent()
{
    return ta_name;
}

/**
 * Release resources allocated for configuration support.
 */
void
rcf_ch_conf_release()
{
}



/*
void 
get_arp_table()
{
ULONG nSize = ARP_TABLE_SIZE;
    
    PMIB_IPNETTABLE pMib = (PMIB_IPNETTABLE)malloc(sizeof(MIB_IPNETTABLE) +
                            sizeof(MIB_IPNETROW)*nSize);

    DWORD dwRet = GetIpNetTable(pMib,&nSize,TRUE);     

    if (nSize>ARP_TABLE_SIZE) 
    {
        printf("[Warning] Insufficient Memory(allocated %d needed %d)\n",
               ARP_TABLE_SIZE, nSize);
          
        nSize = ARP_TABLE_SIZE;
    } else 
    {
        nSize = (unsigned long)pMib->dwNumEntries ;
    }
    printf("ARP Table ( %d Entries) \n", nSize);
    printf("--------------------------------------------------------\n");
    printf("Internet Address      Physical Address         Type\n");
    
    for (int i = 0; i < nSize; i++) 
    {
        char ipaddr[20], macaddr[20];
        sprintf(ipaddr,"%d.%d.%d.%d", (pMib->table[i].dwAddr&0x0000ff), 
                ((pMib->table[i].dwAddr&0xff00)>>8),
                ((pMib->table[i].dwAddr&0xff0000)>>16),
		(pMib->table[i].dwAddr>>24));

        sprintf(macaddr, "%02x-%02x-%02x-%02x-%02x-%02x",
                pMib->table[i].bPhysAddr[0],pMib->table[i].bPhysAddr[1],
		pMib->table[i].bPhysAddr[2],pMib->table[i].bPhysAddr[3],
		pMib->table[i].bPhysAddr[4],pMib->table[i].bPhysAddr[5]);

        printf("%-20s  %-25s",ipaddr,macaddr);
        if (pMib->table[i].dwType == 3) 
	    printf("Dynamic\n");
        else if (pMib->table[i].dwType == 4) 
	    printf("Static\n");
    }
         
    return 0;




}
*/





