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
static int process_oids(cfg_handle handle);
static int process_family(cfg_handle handle);
static void delete_all();

static int new_object = 0;

static int
process_object(cfg_handle handle)
{
    char *str;

    int   ret_val;
    
    ret_val = process_oids(handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_object: process_oids() failed for handle %d\n", handle);
        return ret_val;
    }

    ret_val = process_family(handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_object: process_family() failed\n");
        return ret_val;
    }

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


/* create backup, add instance, create backup_instance, process_value, 
 * delete instance, verify backup. Handle must be an instance handle */
static int
process_instance(cfg_handle handle)
{
    char *str;
    
    char *backup;
    int   backup_len;
    
    int ret_val;
    
    if (handle == CFG_HANDLE_INVALID)
        return EINVAL;
        
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
#if 0    
    /* Add a new instance, process it*/
    {
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
            fprintf(outerr, "process_instance: cfg_add_instance() failed\n");
            return ret_val;
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
    }
#endif        
#if 1
    /* Delete an instance with children and restore */    
    do {
        char *nostr;
        
        ret_val = cfg_del_instance(handle, FALSE);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_instance: cfg_del_instance() failed for handle 0x%x\n", handle);
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
    
    void *value;
        
    cfg_val_type type;
    
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
    
    ret_val = cfg_get_instance(handle, &type, &value);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_value: cfg_get_instance() failed for handle %x\n", handle);
        return 1;
    }
    
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
             
             ret_val = cfg_set_instance(handle, type, int_val + 2);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_set_value() failed\n");
                 return 1;
             }
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
                 fprintf(outerr, "process_value: cfg_set_value() failed\n");
                 return 1;
             }
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
             
             ret_val = cfg_set_instance(handle, type, tmp);
             if (ret_val != 0)
             {
                 fprintf(outerr, "process_value: cfg_set_value() failed\n");
                 return 1;
             }
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
                 fprintf(outerr, "process_value: cfg_set_value() failed\n");
                 return 1;
             }
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

             switch (addr->sa_family)
             {
                 case AF_INET:
                 {
                     struct sockaddr_in *addr_value = 
                         (struct sockaddr_in *)addr;
                     struct sockaddr_in *tmp = &addr4;
                     
                     ret_val = cfg_set_instance(handle, type, tmp);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_set_value() failed\n");
                         return 1;
                     }
                     ret_val = cfg_get_instance(handle, &type, &tmp);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_get_value() failed\n");
                         return 1;
                     }
                     if (memcmp(tmp, &addr4, sizeof(struct sockaddr_in)) != 0)
                     {
                         fprintf(outerr, "process_value: Comparison failed for addresses:\n");
                         return 1;       
                     }
                     ret_val = cfg_set_instance(handle, type, addr_value);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_set_value() failed\n");
                         return 1;
                     } 
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
                         fprintf(outerr, "process_value: cfg_set_value() failed\n");
                         return 1;
                     }
                     ret_val = cfg_get_instance(handle, &type, &tmp);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_get_value() failed\n");
                         return 1;
                     }
                     if (memcmp(tmp, &addr6, sizeof(struct sockaddr_in6)) != 0)
                     {
                         fprintf(outerr, "process_value: Comparison failed for addresses:\n");
                         return 1;       
                     }
                     ret_val = cfg_set_instance(handle, type, addr_value);
                     if (ret_val != 0)
                     {
                         fprintf(outerr, "process_value: cfg_set_value() failed\n");
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

static int
process_family_member(cfg_handle handle)
{
    int ret_val;
    char *oid;
    
    ret_val = cfg_get_oid_str(handle, &oid);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_family_member: cfg_get_oid_str() failed for handle 0x%x\n");
        return 0;
    }
    ret_val = fprintf(output, "%s\n", oid);
    if (ret_val <= 0)
    {
        fprintf(outerr, "process_family_member: fprintf() failed\n");
        return 1;
    }
    free(oid);
    return 0;
}


static int
process_family(cfg_handle handle)
{
    cfg_handle father;
    cfg_handle brother;
    cfg_handle son;

    int ret_val;
    
    ret_val = cfg_get_father(handle, &father);    
    if (ret_val != 0)
    {
        fprintf(outerr, "process_family: cfg_get_father() failed\n");
        return 1;
    }
    if (father != CFG_HANDLE_INVALID)
    {
        fprintf(output, "\n      father: ");
        ret_val = process_family_member(father);
        if (ret_val != 0)
        {
           fprintf(outerr, "process_family: process father failed\n");
            return 1;
        }
    }

    ret_val = cfg_get_brother(handle, &brother);    
    if (ret_val != 0)
    {
        fprintf(outerr, "process_family: cfg_find_brother() failed\n");
        return 1;
    }
    if (brother != CFG_HANDLE_INVALID)
    {
        fprintf(output, "\n      brother: ");
        ret_val = process_family_member(brother);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_family: process brother failed\n");
            return 1;
        }
    }
    ret_val = cfg_get_son(handle, &son);    
    if (ret_val != 0)
    {
        fprintf(outerr, "process_family: cfg_find_son() failed\n");
        return 1;
    }
    if (son != CFG_HANDLE_INVALID)
    {
        fprintf(output, "\n      son: ");
        ret_val = process_family_member(son);
        if (ret_val != 0)
        {
            fprintf(outerr, "process_family: process son failed\n");
            return 1;
        }
    }
    return 0;
}

static int
process_oids(cfg_handle handle)
{
    int ret_val;

    char *oid_str;
    cfg_oid *oid;
    cfg_handle oid_handle;
    
    /* Get oids */
    ret_val = cfg_get_oid_str(handle, &oid_str);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_oids: cfg_get_oid_str() failed for handle %d\n", handle);
        return 1;
    }
    
    oid = cfg_convert_oid_str(oid_str);
    oid_str = cfg_convert_oid(oid);
    
    ret_val = cfg_find_str(oid_str, &oid_handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_oids: cfg_find_str() failed\n");
        return 1;
    }
    
    ret_val = cfg_get_oid(handle, &oid);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_oids: cfg_get_oid() failed for handle %d\n", handle);
        return 1;
    }
    
    /* Get handle */
    ret_val = cfg_find_str(oid_str, &oid_handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_oids: cfg_find_str() failed for handle %d\n", handle);
        return 1;
    }
    
    if (handle != oid_handle)
    {
        fprintf(outerr, "process_oids: Compare two handles: comparison failed\n");
        return 1;
    }
    
    ret_val = cfg_find(oid, &oid_handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "process_oids: cfg_find() failed\n");
        return 1;
    }
    if (handle != oid_handle)
    {
        fprintf(outerr, "process_oids: Compare two handles: comparison failed\n");
        return 1;
    }
    
    /* Write to the file */
    ret_val = fprintf(output, "\nhandle is 0x%x ; OID is %s  :", handle, oid_str);
    if (ret_val <= 0)
    {
        fprintf(outerr, "process_oids: fprintf() failed\n");
        return 1;
    }
    
    free(oid_str);
    cfg_free_oid(oid);
    
    return 0;
}


static int
callback(cfg_handle handle, void *data)
{
    int index = 0;
    int ret_val;
    cfg_handle *current = instances;
    
    /* Search in instances */
    while (index < inst_num)
    {
        if (handle == *current)
            break; 
        index++;
        current++;    
    }
    if (index == inst_num)
    {
        fprintf(outerr, "callback: search in instances failed\n");
        return 1;
    }
#if 0
    ret_val = process_oids(handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "callback: process_oids() failed\n");
        return 1;
    }
    ret_val = process_family(handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "callback: process_family() failed\n");
        return 1;
    }
#endif    
    ret_val = process_value(handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "callback: process_value() failed\n");
        return 1; 
    }
    ret_val = process_instance(handle);
    if (ret_val != 0)
    {
        fprintf(outerr, "callback: process_instance() failed\n");
        return ret_val;
    }
    return 0;
    UNUSED(data);
}

#if 1
int
main()
{
    int ret_val;
    char *backup;
    int backup_len;
    char *config_name = "/tmp/config.cfg";
    char *history_name = "/tmp/history.cfg";
    
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

    ret_val = cfg_create_backup(&backup, &backup_len);
    if (ret_val != 0)
    {
        fprintf(outerr, "main: cfg_create_backup() failed\n");
        return ret_val;
    }
    
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
    
    {
        int i;
        
        cfg_handle *handle = objects;
        for (i = 0; i < obj_num; i++)
        {
            ret_val = process_object(*handle);
            if (ret_val != 0)
            {
                fprintf(outerr, "main: process_object() failed for handle %d\n", *handle);
                return 1;
            }
            ret_val = cfg_enumerate(*handle, callback, &user_data);
            if (ret_val != 0)
            {
                fprintf(outerr, "cfg_enumerate() failed\n");
                return 1;
            }
            handle++;
        }    
    }
    
    ret_val = cfg_create_config(config_name, FALSE);
    if (ret_val != 0)
    {
        fprintf(outerr, "main: cfg_create_config() failed\n");
        return ret_val;
    }
    
    delete_all();
    
    ret_val = cfg_restore_backup(backup);
    if (ret_val != 0)
    {
        fprintf(outerr, "main: cfg_restore_backup() failed\n");
        return ret_val;
    }
    ret_val = cfg_create_config(history_name, TRUE);
    if (ret_val != 0)
    {
        fprintf(outerr, "main: cfg_create_config() failed\n");
        return ret_val; 
    }
    return 0;
}
#endif

#if 0
int
main()
{
    int ret_val;
    int i = 0;
    cfg_handle *current;
    
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

    ret_val = cfg_find_pattern("*:*", &inst_num, &instances);
    if (ret_val != 0)
    {
        fprintf(outerr, "cfg_find_pattern(*:*) failed\n");
        return 1;
    }

    fprintf(output, "Found %d instances\n", inst_num);
    current = instances;

    while (i < inst_num)
    {
        cfg_handle handle = *current;

        ret_val = process_value(handle);
        if (ret_val != 0)
        {
            printf("main: process_value() failed for handle %x\n", handle);
            return 1;
        }
        ret_val = process_instance(handle);
        if (ret_val != 0)
        {
            printf("main: process_instance() failed for handle %x\n", handle);
            return 1;
        }
        i++;
        current++;
    }
    printf("main - normal exit\n");
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
