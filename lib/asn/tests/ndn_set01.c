#include <stdio.h>

#include "asn_impl.h"
#include "ndn.h"


char buffer [1000];

#define COMPON_TEST 1

int 
main (void)
{ 
    asn_value_p gen_pdu  = asn_init_value(ndn_generic_pdu);
    asn_value_p file_pdu  = asn_init_value(ndn_file_message);

    int a = 1981;
    int rc; 
    int l;

    char nm[] = "file-test-name"; 
    char label[100];

    rc = asn_write_value_field(file_pdu, nm, sizeof(nm), "line.#plain");
    if (rc) { printf ("write val-field rc: %x\n", rc); return 0; }

    l = 1000;
    rc = asn_read_value_field(file_pdu, buffer, &l, "line");
    if (rc) { printf ("read val-field rc: %x\n", rc); return 0; } 

    printf ("len : %d, str: '%s'\n", l, buffer);

    rc = asn_write_component_value(gen_pdu, file_pdu, "");
    if (rc) { printf ("write comp rc: %x\n", rc); return 0; } 

    asn_sprint_value(gen_pdu, buffer, 1000, 0);
    printf ("gen_pdu: \n%s", buffer);
    asn_free_value(gen_pdu);

    gen_pdu = asn_init_value(ndn_snmp_var_bind);
    rc = asn_write_value_field(gen_pdu, nm, sizeof(nm),
                            "value.#plain.#simple.#string-value");
    if (rc) { printf ("write simple.string value rc: %x\n", rc); return 0; } 

    asn_sprint_value(gen_pdu, buffer, 1000, 0);
    printf ("gen_pdu: \n%s", buffer); 

    {
        asn_value_p subv;
        const char * name;
        rc = asn_read_component_value(gen_pdu, &subv, "value");
        if (rc) { printf ("read comp rc: %x\n", rc); return 0; } 

        name = asn_get_name(subv);
        if (name)
            printf ("name of val: %s\n", name);
        else
            printf ("No name of val returned\n");

    }
    
    rc = asn_get_choice(gen_pdu, "value", label, 100);
    if (rc) { printf ("get choice rc: %x\n", rc); return 0; } 

    printf ("choice: %s\n", label);

    return 0;
}
