
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_NET_SNMP_DEFINITIONS_H
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/definitions.h> 
#include <net-snmp/mib_api.h> 
#endif

#include "asn_impl.h"
#include "ndn_general.h" 
#include "tapi_snmp.h"

int 
main(int argc, char *argv[])
{
    int rc;
    struct tree *entry_node; 
    tapi_snmp_oid_t table_oid = {9, {1, 3, 6, 1, 2, 1, 1, 9, 1}};

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s path_to_mibs mib_file\n", argv[0]);
        return 1;
    }
    rc = tapi_snmp_load_mib_with_path(argv[1], argv[2]);
    if (rc) 
    {
        fprintf(stderr, "load mibs failed: %d \n", rc);
        return 2;
    } 

    entry_node = get_tree(table_oid.id, table_oid.length, get_tree_head());

    if (entry_node == NULL)
    {
        printf ("get tree failed\n");
    }
    else
    {
        printf ("NODE: subid %d, type %d; label <%s>; status %d\n", 
                    entry_node->subid, entry_node->type, entry_node->label,
                    entry_node->status);
        if (entry_node->indexes)
        {
            struct index_list *il;
            printf("   INDEX list: ");
            for (il = entry_node->indexes; il; il = il->next)
                printf ("label: %s; simplied: %d; ", il->ilabel, (int)il->isimplied);
            printf("\n");
        }
    }


}
