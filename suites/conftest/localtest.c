#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "te_stdint.h"
#include "te_errno.h"
#include "conf_api.h"
#include "rcf_api.h"

typedef struct conftest_user_data {
} conftest_user_data;    

/* Locals */
static FILE * output;
static FILE * outerr;

static int obj_num;
static int inst_num;

static cfg_handle *objects;
static cfg_handle *instances;

static conftest_user_data user_data;
static int process_object(cfg_handle handle);
static int process_instance(cfg_handle handle);
static int process_value(cfg_handle handle);
static int process_oids(cfg_handle handle);
static int process_family(cfg_handle handle);
static int get_descr_by_instance(cfg_handle handle, cfg_obj_descr *descr);
static void delete_all();

static int new_object = 0;

static int
process_object(cfg_handle handle)
{
    char *str;

    int   ret_val;
    
    ret_val = cfg_get_oid_str(handle, &str);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_object: cfg_get_oid_str() failed\n");
        return ret_val;
    }

    /* Register a new object */
    {   
        char   oid_str[CFG_OID_MAX];
        char  *subid;
        char   object_subid[CFG_OID_MAX];
    
        cfg_oid      *oid;
        cfg_obj_descr descr;
        cfg_handle    object;
        cfg_handle    tmp;

        ret_val = cfg_get_subid(handle, &subid);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_object: cfg_get_subid() failed\n");
            return ret_val;
        }
            
        ret_val = cfg_get_object_descr(handle, &descr);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_object: cfg_get_obj_descr() failed\n");
            return ret_val;
        }

        strncpy(oid_str, str, strlen(str) - strlen(subid));
        oid_str[strlen(str) - strlen(subid)] = '\0';
        sprintf(object_subid, "new_object_%d\n", new_object);
        new_object++;
        strcat(oid_str, object_subid);

        oid = cfg_convert_oid_str(oid_str);
        if (oid == NULL)
        {
            fprintf(outerr, "process_object: zero oid\n");
            return 1;
        }
        
        fprintf(output, "Register new object %s\n", oid_str);
        
        ret_val = cfg_register_object(oid, &descr, &object);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_object: cfg_register_object() failed\n");
            return ret_val;
        } 

        ret_val = cfg_find(oid, &tmp);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_object: cfg_find() failed\n");
            return ret_val;
        }    
        
        if (tmp != object)
        {
            fprintf(outerr, "process_object: Compare two handles, comparison failed\n");
            return 1;
        }
    }    
    return 0;
}

static int
get_descr_by_instance(cfg_handle handle, cfg_obj_descr *descr)
{
    int ret_val;
    cfg_handle object;
    cfg_obj_descr tmp;
    
    ret_val = cfg_find_object_by_instance(handle, &object);
    if (ret_val != 0)
    {
        fprintf(outerr, "get_descr_by_instance: cfg_find_object_by() failed\n");
        return ret_val;
    }
    ret_val = cfg_get_object_descr(object, &tmp);
    if (ret_val != 0)
    {
        fprintf(outerr, "get_descr_by_instance: cfg_get_descr() failed\n");
        return ret_val;
    }
    *descr = tmp;
    return 0;
}

/* create backup, add instance, create backup_instance, process_value, 
 * delete instance, verify backup. Handle must be an instance handle */
static int
process_instance(cfg_handle handle)
{
    char *str;
    
    char *backup;
    int   backup_len;
    
    int ret_val;
    
    cfg_obj_descr descr;
    
    if (handle == CFG_HANDLE_INVALID)
        return EINVAL;
        
    ret_val = get_descr_by_instance(handle, &descr);    
    if (ret_val != 0)
    {
        fprintf(outerr, "process_instance: get_desct_by_instance() failed\n");
        return ret_val;
    }
    
    ret_val = cfg_create_backup(&backup, &backup_len);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_instance: cfg_create_backup() failed\n");
        return ret_val;
    }    
    
    ret_val = cfg_get_oid_str(handle, &str);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_instance() : cfg_get_oid_str() failed\n");
        return ret_val;
    }
    
    if (strcmp(str, "/:") == 0)
    {
        return 0;
    }
    if (descr.access != CFG_READ_CREATE)
        return 0;

#if 1    
    /* Add a new instance, process it*/
    do {
        cfg_handle    instance;
        cfg_val_type  type;
        char         *inst_name;
        char          oid_str[CFG_OID_MAX];
        void         *value;
        cfg_oid      *oid;

        ret_val = cfg_get_oid_str(handle, &str);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance() : cfg_get_oid_str() failed\n");
            return ret_val;
        }
        
        ret_val = cfg_get_inst_name(handle, &inst_name);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: cfg_get_inst_name() failed\n");
            return ret_val;
        }    

        ret_val = cfg_get_instance(handle, &type, &value);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance() : cfg_get_instance() failed for handle %d\n", handle);
            return ret_val;
        }    
 
        if (!((type == CVT_INTEGER) || (type == CVT_STRING) || (type == CVT_ADDRESS) ||
            (type == CVT_NONE)))
        {
            fprintf(outerr, "process_instance: cfg_get_instance() get unknown type for handle 0x%x\n", handle);
            return 1;
        }
    
        strncpy(oid_str, str, strlen(str) - strlen(inst_name));
        oid_str[strlen(str) - strlen(inst_name)] = '\0';
        strcat(oid_str, "new_instance");

        fprintf(output, "Add a new instance %s\n", oid_str);
        
        oid = cfg_convert_oid_str(oid_str);
        if (oid == NULL)
        {
            fprintf(outerr, "process_instance: cfg_convert_oid_str() failed\n");
            return 1;
        }

        ret_val = cfg_add_instance(oid, &instance, type, value);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: cfg_add_instance() failed %s, ret_val %x\n", oid_str, ret_val);
            break;;
        }
        fprintf(output, "Get a new instance handle %x\n", instance);
        
        ret_val = process_value(instance);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: process_value() failed for added value %x\n", instance);
            return ret_val;        
        }
    
        ret_val = cfg_del_instance(instance, FALSE);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: cfg_del_instance() failed for oid %s, handle %x\n", oid_str, instance);
            return ret_val;        
        }

        ret_val = cfg_verify_backup(backup);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: cfg_verify_backup() failed\n");
            return ret_val;        
        }
    } while (0);
#endif        
#if 1
    /* Delete an instance with children and restore */    
    do {
        char *nostr;
        
        ret_val = cfg_del_instance(handle, FALSE);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: cfg_del_instance() failed for handle 0x%x oid %s\n", handle, str);
            break;
        }

        ret_val = cfg_get_oid_str(handle, &nostr);
        if (!(ret_val == EINVAL || ret_val == ENOENT))
        {
            fprintf(outerr, "process_instance: cfg_get_oid_str() must return EINVAL or ENOENT\n");
            return 1;
        }

        ret_val = cfg_restore_backup(backup);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: cfg_restore_backup() failed\n");
            return ret_val;
        }
        ret_val = cfg_find_str(str, &handle);
        if (ret_val != 0 || handle == CFG_HANDLE_INVALID)
        {
            fprintf(outerr, "process_instance: cfg_find_str() failed\n");
            return ret_val;
        }
    } while (0);
#endif
    return 0;
}

/* Get value, set new value, get value, check, and set old value */
static int
process_value(cfg_handle handle)
{
    int ret_val;
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
    char *str;
    cfg_obj_descr descr;
    
    void *value;
        
    cfg_val_type type;
    
    memset(&addr4, 0, sizeof(addr4));
    memset(&addr6, 0, sizeof(addr6));
    
    addr4.sin_family = AF_INET;
    addr6.sin6_family = AF_INET6;
    
    ret_val = inet_pton(AF_INET, "255.255.255.255", &(addr4.sin_addr));
    if (ret_val <= 0)
    {
        fprintf(outerr, "process_value: inet_pton() for AF_INET failed %d\n", ret_val);
        return 1;
    }
    
    ret_val = inet_pton(AF_INET6, "ff:ff::f0", &(addr6.sin6_addr));
    if (ret_val <= 0)
    {
        fprintf(outerr, "process_value: inet_pton() for AF_INET6 failed\n");
        return 1;
    }
    
    ret_val = cfg_get_oid_str(handle, &str);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_value: cfg_get_oid_str() failed, %x\n", handle);
        return 1;
    }
    
    ret_val = cfg_get_object_descr(handle, &descr);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_value: cfg_get_object_descr() failed for handle %x\n", handle);
        return 1;
    }
    
    type = descr.type;
    
    switch (type)
    {
         case CVT_INTEGER:
         {
             int int_val;
             int tmp;

             ret_val = cfg_get_instance(handle, &type, &int_val);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_get_instance() failed for integer\n");
                 return ret_val;
             }
             
             fprintf(output, "Get %s value %d\n", str, int_val);
             if (descr.access == CFG_READ_ONLY)
                 return 0;

             ret_val = cfg_set_instance(handle, type, int_val + 2);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_set_value() failed, %s\n", str);
                 return 1;
             }
             fprintf(output, "Set %s value %d\n", str, int_val + 2);
             
             ret_val = cfg_get_instance(handle, &type, &tmp);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_get_value() failed\n");
                 return 1;
             }
             if (tmp != int_val + 2)
             {
                 fprintf(outerr, "process_value: Comparison failed: %d %d\n", 
                        *(int *)value, int_val + 2);
                 return 1;       
             }
             ret_val = cfg_set_instance(handle, type, int_val);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_set_value() failed 2 %s\n", str);
                 return 1;
             }
             fprintf(output, "Set %s value %d\n", str, int_val);
             break;
         }
         case CVT_STRING:
         {
             char *tmp = "renata";
             char *str_val;

             ret_val = cfg_get_instance(handle, &type, &str_val);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_get_instance() failed for string\n");
                 return ret_val;
             }
             fprintf(output, "Get %s value %s\n", str, str_val);

             if (descr.access == CFG_READ_ONLY)
                 return 0;
             
             ret_val = cfg_set_instance(handle, type, tmp);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_set_value() failed %s\n", str);
                 return 1;
             }
             fprintf(output, "Set %s value %s\n", str, tmp);

             ret_val = cfg_get_instance(handle, &type, &tmp);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_get_value() failed\n");
                 return 1;
             }
             if (strcmp(tmp, "renata") != 0)
             {
                 fprintf(outerr, "process_value: Comparison failed: %s %s\n", 
                        tmp, "renata");
                 return 1;       
             }
             ret_val = cfg_set_instance(handle, type, str_val);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_set_value() 2 failed %s\n", str);
                 return 1;
             }
             fprintf(output, "Set %s value %s\n", str, str_val);

             break;
         }    
         case CVT_ADDRESS:
         {
             struct sockaddr *addr;
             
             ret_val = cfg_get_instance(handle, &type, &addr);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_get_instance() failed for addr\n");
                 return ret_val;
             }

             if (descr.access == CFG_READ_ONLY)
                 return 0;

             switch (addr->sa_family)
             {
                 case AF_INET:
                 {
                     char addr_str[INET_ADDRSTRLEN];

                     struct sockaddr_in *addr_value = 
                         (struct sockaddr_in *)addr;
                     struct sockaddr_in *tmp = &addr4;
                     
                     if (inet_ntop(AF_INET, &(addr_value->sin_addr), 
                                   addr_str, INET_ADDRSTRLEN) == NULL)
                     {
                         fprintf(outerr, "process_value: inet_ntop() failed 1\n");
                         perror("Ah-ah-ah!\n");
                         return 1;
                     }
                     fprintf(output, "Get %s value %s\n", str, addr_str);
                     
                     ret_val = cfg_set_instance(handle, type, tmp);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_set_value() failed %s\n", str);
                         return 1;
                     }
                     fprintf(output, "Set %s value %s\n", str, "255.255.255.255");

                     ret_val = cfg_get_instance(handle, &type, &tmp);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_get_value() failed\n");
                         return 1;
                     }

                     if (inet_ntop(AF_INET, &(tmp->sin_addr.s_addr), 
                                   addr_str, INET_ADDRSTRLEN) == NULL)
                     {
                         fprintf(outerr, "process_value: inet_ntop() failed 2\n");
                         return 1;
                     }
                     fprintf(output, "Get %s value %s\n", str, addr_str);
                     
                     if (tmp->sin_addr.s_addr != addr4.sin_addr.s_addr)
                     {
                         fprintf(outerr, "process_value: Comparison failed for addresses:\n");
                         return 1;       
                     }
                     ret_val = cfg_set_instance(handle, type, addr_value);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_set_value() 2 failed %s\n", str);
                         return 1;
                     } 

                     if (inet_ntop(AF_INET, &(addr_value->sin_addr.s_addr), 
                         addr_str, INET_ADDRSTRLEN) == NULL)
                     {
                         fprintf(outerr, "process_value: inet_ntop() failed 3\n");
                         return 1;
                     }
                     fprintf(output, "Get %s value %s\n", str, addr_str);
                     
                     break;
                 }
                 case AF_INET6:
                 {
                     struct sockaddr_in6 *addr_value = 
                         (struct sockaddr_in6 *)addr;
                     struct sockaddr_in6 *tmp = &addr6;
                     
                     ret_val = cfg_set_instance(handle, type, tmp);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_set_value() failed %s\n", str);
                         return 1;
                     }
                     ret_val = cfg_get_instance(handle, &type, &tmp);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_get_value() failed\n");
                         return 1;
                     }
                     if (memcmp(&(tmp->sin6_addr), &(addr6.sin6_addr), 
                         sizeof(struct in6_addr)) != 0)
                     {
                         fprintf(outerr, "process_value: Comparison failed for addresses:\n");
                         return 1;       
                     }
                     ret_val = cfg_set_instance(handle, type, addr_value);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_set_value() 2 failed %s\n", str);
                         return 1;
                     } 
                     break;
                 }
             }
             break;
         }
         case CVT_NONE:
         {
             break;
         }
         default:
         {
             fprintf(outerr, "process_value: get unknown type\n");
             break;
         }
    }
    return 0;    
}

/* Main main :) */
#if 1
int
main()
{
    int ret_val;
    int i = 0;
    cfg_handle *current;
    char *history_name = "/tmp/history";
    char *str;
    
    output = fopen("/tmp/conf_api_output.txt", "w");
    if (output == NULL)
    {
        fprintf(outerr, "Couldn't open file\n");
        return 1;
    }    

    outerr = fopen("/tmp/conf_api_outerr.txt", "w");
    if (outerr == NULL)
    {
        fprintf(outerr, "Couldn't open file\n");
        return 1;
    }    
        
    printf("Start to test Configurator API\n");

    ret_val = cfg_find_pattern("*", &obj_num, &objects);
    if (ret_val != 0)
    {
        fprintf(outerr, "cfg_find_pattern(*) failed\n");
        return 1;
    }

    ret_val = cfg_find_pattern("*:*", &inst_num, &instances);
    if (ret_val != 0)
    {
        fprintf(outerr, "cfg_find_pattern(*:*) failed\n");
        return 1;
    }

    fprintf(stderr, "Found %d instances\n", inst_num);
    current = objects;
    
    while (i < obj_num)
    {
        ret_val = cfg_get_oid_str(*current, &str);
        if (ret_val != 0)
        {
            fprintf(outerr, "!!!main: cfg_get_oid_str() failed\n");
            return ret_val;
        }    
        fprintf(stderr, "%d: Working handle %x %s\n", i + 1, *current, str);
        i++;
        current++;
    }
    
    i= 0;
    current = instances;
    
    while (i < inst_num)
    {
        ret_val = cfg_get_oid_str(*current, &str);
        if (ret_val != 0)
        {
            fprintf(outerr, "!!!main: cfg_get_oid_str() failed\n");
            return ret_val;
        }
        fprintf(stderr, "%d: Working handle %x %s\n", i + 1, *current, str);
        i++;
        current++;
    }
    
    i = 0;
    current = objects;
    
    while (i < obj_num)
    {
        fprintf(stderr, "Working handle %x\n", *current);
        ret_val = process_object(*current);
        if (ret_val != 0)
        {
            fprintf(outerr, "main: process_object() failed\n");
            return ret_val;
        }    
        i++;
        current++;
    }
    i = 0;
    current = instances;

    while (i < inst_num)
    {
        cfg_handle handle = *current;

        fprintf(stderr, "Working handle %x\n", handle);
        ret_val = process_instance(handle);
        if (ret_val != 0)
        {
            printf("main: process_instance() failed for handle %x\n", handle);
            return 1;
        }
        i++;
        current++;
    }
    ret_val = cfg_create_config(history_name, TRUE);
    if (ret_val != 0)
    {
        fprintf(outerr, "cfg_create_config() failed\n");
        return ret_val;
    }
    
    printf("main - normal exit\n");
    return 0;
}
#endif


/* Interface lo */
#if 0
int
main()
{
    int ret_val, inst_num, i = 0;
    cfg_handle net_addr, netmask, *instances, *current;

    ret_val = cfg_find_pattern("*:*", &inst_num, &instances);
    if (ret_val != 0)
    {
        fprintf(outerr, "cfg_find_pattern(*:*) failed\n");
        return 1;
    }

    fprintf(stderr, "Found %d instances\n", inst_num);
    current = instances;
    
    while (i < inst_num)
    {
        char *str;
        ret_val = cfg_get_oid_str(*current, &str);
        if (ret_val != 0)
        {
            fprintf(outerr, "!!!main: cfg_get_oid_str() failed\n");
            return ret_val;
        }
        fprintf(stderr, "%d: Working handle %x %s\n", i + 1, *current, str);
        i++;
        current++;
    }

#if 0    
    ret_val = cfg_add_instance_str("/agent:kai/interface:lo/net_addr:new_instance", &net_addr, CVT_NONE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_add_instance_str() failed, net_addr\n");
    }
    
    ret_val = cfg_find_str("/agent:kai/interface:lo/net_addr:127.0.0.1", &net_addr);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_find_str() failed, net_addr\n");
        return 1;
    }
    
    ret_val = cfg_del_instance(net_addr, FALSE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_del_instance() failed\n");
    }
#endif
    ret_val = cfg_find_str("/agent:kai/interface:lo/net_addr:127.0.0.1/netmask:", &netmask);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_find_str() failed, netmask\n");
    }
    return 0;
}
#endif


/* Interfaces */
#if 0
int
main()
{
    int ret_val;
    cfg_handle interface, net_addr, link_addr, status;
    struct sockaddr *local;
    unsigned char buffer[18] = {0,};
    unsigned char *str;

    ret_val = cfg_add_instance_str("/agent:kai/interface:eth0:3", &interface, CVT_NONE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_add_instance_str() failed, interface\n");
        return 1;
    }

    ret_val = cfg_add_instance_str("/agent:kai/interface:eth0:3/net_addr:1.2.3.4", &net_addr, CVT_NONE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_add_instance_str() failed, net_addr\n");
        return 1;
    }
    
    ret_val = cfg_synchronize("/agent:kai", TRUE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_synch() failed\n");
        return 1;
    }
    
    ret_val = cfg_find_str("/agent:kai/interface:eth0:3/status:", &status);
    if (ret_val != 0 || status == CFG_HANDLE_INVALID)
    {
        fprintf(stderr, "cfg_find_str() failed, status\n");
        return 1;
    }
#if 1    
    ret_val = cfg_set_instance(status, CVT_INTEGER, 1);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_set_instance_str() failed, status\n");
        return 1;
    }
#endif
    ret_val = cfg_find_str("/agent:kai/interface:eth0:3/link_addr:", &link_addr);
    if (ret_val != 0 || link_addr == CFG_HANDLE_INVALID)
    {
        fprintf(stderr, "cfg_find_str() failed, link_addr\n");
        return 1;
    }

    ret_val = cfg_get_instance(link_addr, NULL, &local);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_set_instance_str() failed, status\n");
        return 1;
    }
    
    str = (unsigned char *)(local->sa_data);
    
    sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", 
            str[0], str[1], str[2], str[3], str[4], str[5]);
    buffer[17] = '\0';
            
    fprintf(stdout, "link_addr is %s\n", buffer);           
    
    system("/sbin/ifconfig");

#if 1    
    ret_val = cfg_set_instance(status, CVT_INTEGER, 0);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_set_instance_str() failed, status\n");
        return 1;
    }
#endif    
    ret_val = cfg_del_instance(interface, TRUE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_del_instance() failed, ret_val %x\n", ret_val);
        return 1;
    }
    return 0;
}
#endif







/* history problem */
#if 0
int
main()
{
    int ret_val;
    cfg_handle interface;
    char *history = "/tmp/history";
    
    output = fopen("/tmp/conf_api_output.txt", "w");
    if (output == NULL)
    {
        fprintf(outerr, "Couldn't open file\n");
        return 1;
    }    

    outerr = fopen("/tmp/conf_api_outerr.txt", "w");
    if (outerr == NULL)
    {
        fprintf(outerr, "Couldn't open file\n");
        return 1;
    }    

    ret_val = cfg_find_str("/agent:kai/interface:lo", &interface);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_find_str() failed\n");
        return 1;
    }
    
    ret_val = process_instance(interface);
    if (ret_val != 0)
    {
        fprintf(stderr, "process_instance() failed\n");
        return 1;
    }
    
    ret_val = cfg_create_config(history, TRUE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_create_config() failed\n");
        return 1;
    }
    return 0;
}
#endif




/* history problem */
#if 0
int
main()
{
    int ret_val;
    char *history = "/tmp/history";
    cfg_handle interface;
    char *backup;
    int backup_len;
    
    ret_val = cfg_create_backup(&backup, &backup_len);
    if (ret_val != 0)
    {
        fprintf(stderr, "create_backup() failed\n");
        return 1;
    }
    
    ret_val = cfg_add_instance_str("/agent:kai/interface:new_instance", &interface, CVT_NONE);
    if (ret_val != 0)
    {
        fprintf(stderr, "add_instance() failed\n");
        return 1;
    }
    ret_val = process_value(interface);
    if (ret_val != 0)
    {
        fprintf(stderr, "process_value() failed\n");
        return 1;
    }
    
    ret_val = cfg_del_instance(interface, FALSE);
    if (ret_val != 0)
    {
        fprintf(stderr, "del_instance() failed\n");
        return 1;
    }
    ret_val = cfg_verify_backup(backup);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_verify_backup() failed\n");
        return 1;
    }
    ret_val = cfg_create_config(history, TRUE);
    if (ret_val != 0)
    {
        fprintf(stderr, "cfg_create_config() failed\n");
        return 1;
    }
    return 0;
}
#endif

static void
delete_all()
{
    cfg_handle handle;
    int ret_val;
    char *str = "/:";
    
    ret_val = cfg_find_str(str, &handle);
    if (ret_val != 0)
    {
        fprintf(stderr, "Can't find handle for '/:'\n");
        return;
    }
    ret_val = cfg_del_instance(handle, TRUE);
    return;
}    
