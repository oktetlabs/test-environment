/** @file
 * @brief Test Environment
 *
 * Simple RCF test
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "rcf_api.h"
#include "logger_api.h"

DEFINE_LGR_ENTITY("rcftest");

int
main()
{
    char ta[32];
    int  len = sizeof(ta);
    int  sid;
    
    printf("Starting test\n");
    if (rcf_get_ta_list(ta, &len) != 0)
    {
        printf("rcf_get_ta_list failed\n");
        return 1;
    }
    printf("Agent: %s\n", ta);
    
    /* Type test */
    {
        char type[16];
        if (rcf_ta_name2type(ta, type) != 0)
        {
            printf("rcf_ta_name2type failed\n");
            return 1;
        }
        printf("TA type: %s\n", type); 
    }
    
    /* Session */
    {
        if (rcf_ta_create_session(ta, &sid) != 0)
        {
            printf("rcf_ta_create_session failed\n");
            return 1;
        }
        printf("Created session: %d\n", sid); 
    }

#if 0
    {
        int sid1;
        if (rcf_ta_create_session("loki1", &sid1) != 0)
        {
            printf("rcf_ta_create_session failed\n");
            return 1;
        }
        printf("Created session: %d\n", sid1); 
    }
#endif    
    
#if 0
    /* Variable tests */
    {
        int test_a;
        
        if (rcf_ta_get_var(ta, sid, "test_a", RCF_INT32, 4, &test_a) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        printf("test_a = %d\n", test_a); 
        test_a = 25;
        if (rcf_ta_set_var(ta, sid, "test_a", RCF_INT32, &test_a) != 0)
        {
            printf("rcf_ta_set_var failed\n");
            return 1;
        }
        test_a = 0;
        if (rcf_ta_get_var(ta, sid, "test_a", RCF_INT32, 4, &test_a) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        if (test_a != 25)
        {
            printf("test_a = %d\n", test_a);
            return 1;
        }
    }
#endif

#if 0   
    {
        unsigned short test_b;
        
        if (rcf_ta_get_var(ta, sid, "test_b", RCF_UINT16, 2, &test_b) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        printf("test_b = %u\n", test_b); 
        test_b = 20;
        if (rcf_ta_set_var(ta, sid, "test_b", RCF_UINT16, &test_b) != 0)
        {
            printf("rcf_ta_set_var failed\n");
            return 1;
        }
        test_b = 0;
        if (rcf_ta_get_var(ta, sid, "test_b", RCF_UINT16, 2, &test_b) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        if (test_b != 20)
        {
            printf("test_b = %d\n", test_b);
            return 1;
        }
    }
#endif
#if 0
    {
        unsigned long long test_c;
        
        if (rcf_ta_get_var(ta, sid, "test_c", RCF_UINT64, 8, &test_c) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        printf("test_c = %llu\n", test_c); 
        test_c = 1111111111;
        if (rcf_ta_set_var(ta, sid, "test_c", RCF_UINT64, &test_c) != 0)
        {
            printf("rcf_ta_set_var failed\n");
            return 1;
        }
        test_c = 0;
        if (rcf_ta_get_var(ta, sid, "test_c", RCF_UINT64, 8, &test_c) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        if (test_c != 1111111111)
        {
            printf("test_c = %d\n", test_c);
            return 1;
        }
    }
#endif
#if 0
    {
        char test_d[40];
        
        if (rcf_ta_get_var(ta, sid, "test_d", RCF_STRING, 40, test_d) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        printf("test_d = %s\n", test_d); 
        strcpy(test_d, "he-he");
        if (rcf_ta_set_var(ta, sid, "test_d", RCF_STRING, test_d) != 0)
        {
            printf("rcf_ta_set_var failed\n");
            return 1;
        }
        memset(test_d, 0, sizeof(test_d));
        if (rcf_ta_get_var(ta, sid, "test_d", RCF_STRING, 40, test_d) != 0)
        {
            printf("rcf_ta_get_var failed\n");
            return 1;
        }
        if (strcmp(test_d, "he-he") != 0)
        {
            printf("test_d = %s\n", test_d);
            return 1;
        }
    }
#endif

#if 0
    /* Routine tests */
    {
        int rc;
        
        if (rcf_ta_call(ta, sid, "test_e", &rc, 2, 0, RCF_INT32, 5, 
                        RCF_STRING, "hi-hi-hi") != 0)
        {
            printf("rcf_ta_call failed\n");
            return 1;
        }
        printf("rc = %d\n", rc);

        if (rcf_ta_call(ta, sid, "test_f", &rc, 4, 1, "vvv", "lvr", 
                        "helen", "arybchik") != 0)
        {
            printf("rcf_ta_call failed\n");
            return 1;
        }
        printf("rc = %d\n", rc);
    }
#endif

#if 0
    {
        int pid1, pid2;
        
        int sid2;
        
        if (rcf_ta_create_session(ta, &sid2) != 0)
        {
            printf("rcf_ta_create_session failed\n");
            return 1;
        }
        
        if (rcf_ta_start_task(ta, sid2, 0, "test_g", &pid1, 3, 0,
                              RCF_INT32, 10, 
                              RCF_INT32, 20,
                              RCF_STRING, "helen") != 0)
        {
            printf("rcf_ta_start_task failed\n");
            return 1;
        }
        printf("pid = %d\n", pid1);

        if (rcf_ta_start_task(ta, sid, 0, "test_g", &pid2, 3, 0,
                              RCF_INT32, 15, 
                              RCF_INT32, 15,
                              RCF_STRING, "vvv") != 0)
        {
            printf("rcf_ta_start_task failed\n");
            return 1;
        }
        printf("pid = %d\n", pid2);
        sleep(40);
        
        if (rcf_ta_kill_task(ta, sid2, pid2) != 0)
        {
            printf("rcf_ta_kill_task failed\n");
            return 1;
        }
        
        sleep(30);

        if (rcf_ta_kill_task(ta, sid, pid1) != 0)
        {
            printf("rcf_ta_kill_task failed\n");
            return 1;
        }
    }
#endif
#if 1
    /* CSAP tests */
    do {
        char path[1000];
        int  path_prefix;

        char val[64];
        int handle = 20;
        int num;
        int timeout = 30;
        int rc;
        char *te_suites = getenv("TE_INSTALL_SUITE");

        if (te_suites)
            printf ("te_suites: %s\n", te_suites);
        else 
            break;

        strcpy (path, te_suites);
        strcat (path, "/rcftest/nds/");
        path_prefix = strlen(path);

        strcat (path, "file-csap.asn"); 
        rc =   rcf_ta_csap_create(ta, sid, "file", path, &handle);
        printf("csap_create rc: %d\n", rc);

        if (rc) break;

        strcpy(path + path_prefix, "file-tmpl01.asn");
        printf ("send template full path: %s\n", path);

        rc = rcf_ta_trsend_start(ta, sid, handle, path, 1);
        printf("trsend_start: 0x%x\n", rc);
        if (rc) break;

        rc = rcf_ta_trsend_stop(ta, sid, handle, &num);
        printf("trsend_stop: 0x%x, num: %d\n", rc, num);
        if (rc) break;

        printf("csap_destroy: %d\n", 
               rcf_ta_csap_destroy(ta, sid, handle)); 

#if 0
        printf("csap_create: %d\n", 
               rcf_ta_csap_create(ta, sid, 
               "dhcp.udp.ip.eth", "/tmp/csap.param", &handle)); 

        printf("csap_destroy: %d\n", 
               rcf_ta_csap_destroy(ta, sid, handle)); 

        printf("csap_param: %d\n", 
               rcf_ta_csap_param(ta, sid, handle, "source_port", 64, val)); 

        printf("trsend_start: %d\n", 
               rcf_ta_trsend_start(ta, sid, handle, "/tmp/template", 1));

        printf("trsend_start: %d\n", 
               rcf_ta_trsend_start(ta, sid, handle, "/tmp/template", 0));

        printf("trsend_stop: %d\n", 
               rcf_ta_trsend_stop(ta, sid, handle, &num));

        printf("trsend_recv: %d\n", 
               rcf_ta_trsend_recv(ta, sid, handle, "/tmp/template", 0, 0,
                                  &timeout, &num));

        printf("trrecv_start: %d\n", 
               rcf_ta_trrecv_start(ta, sid, handle, "/tmp/pattern", 0,
                                   (void *)1, 0, 1));

        printf("trrecv_start: %d\n", 
               rcf_ta_trrecv_start(ta, sid, handle, "/tmp/pattern", 0,
                                   (void *)1, 0, 0));

        printf("trrecv_start: %d\n", 
               rcf_ta_trrecv_start(ta, sid, handle, "/tmp/pattern", 0,
                                   0, 0, 1));

        printf("trrecv_start: %d\n", 
               rcf_ta_trrecv_start(ta, sid, handle, "/tmp/pattern", 0,
                                   0, 0, 0));

        printf("trrecv_get: %d\n", 
               rcf_ta_trrecv_get(ta, handle, &num));

        printf("trrecv_stop: %d\n", 
               rcf_ta_trrecv_stop(ta, handle, &num));
#endif
    } while(0);
#endif
#if 0
    /* File tests */
    {
        unlink("/tmp/localrfc");
        unlink("/tmp/remoterfc");
        unlink("/tmp/localmem");
        unlink("/tmp/localmem2");
        
        if (rcf_ta_get_file(ta, sid, "/tmp/rfc2428.txt",
                            "/tmp/localrfc") != 0)
        {
            printf("rcf_ta_get_file failed\n");
        }
        if (rcf_ta_put_file(ta, sid, "/tmp/localrfc",
                            "/tmp/remoterfc") != 0)
        {
            printf("rcf_ta_put_file failed\n");
        } 
        if (rcf_ta_get_file(ta, sid, "/memory/test_d:12",
                            "/tmp/localmem") != 0)
        {
            printf("rcf_ta_get_file failed for memory\n");
        } 
        if (rcf_ta_put_file(ta, sid, "/tmp/mem", "/memory/test_d") != 0)
        {
            printf("rcf_ta_put_file failed for memory\n");
        } 
        if (rcf_ta_get_file(ta, sid, "/memory/test_d:12",
                            "/tmp/localmem2") != 0)
        {
            printf("rcf_ta_get_file failed for memory\n");
        } 
    }
#endif        

#if 0
    /* Configuration tests */
    {
        char val[2048];

        printf("configure_get: %d\n", 
               rcf_ta_cfg_get(ta, sid, "*", val, sizeof(val)));
        printf("<%s>\n", val);
        
        printf("configure_get: %d\n", 
               rcf_ta_cfg_get(ta, sid, "*:*", val, sizeof(val)));
        printf("<%s>\n", val); 
  
        printf("configure_add: %d\n", 
               rcf_ta_cfg_add(ta, sid, "/agent:vcube/interface:eth0:3",
                              ""));
        printf("configure_add: %d\n", 
               rcf_ta_cfg_add(ta, sid, 
               "/agent:vcube/interface:eth0:3/net_addr:1.2.3.4", ""));
        printf("configure_set: %d\n", 
               rcf_ta_cfg_set(ta, sid,
                              "/agent:vcube/interface:eth0:3/status:", 
                              "1"));
        printf("configure_get: %d\n", 
               rcf_ta_cfg_get(ta, sid,
                              "/agent:vcube/interface:eth0:3/link_addr:", 
                              val, sizeof(val)));
        printf("HW address: %s\n", val);
        system("/sbin/ifconfig"); 
        printf("configure_del: %d\n", 
               rcf_ta_cfg_del(ta, sid, "/agent:vcube/interface:eth0:3")); 
    }
#endif
    return 0;
}
