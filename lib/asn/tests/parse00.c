#include <stdio.h>

#include "asn_usr.h"
#include "ndn.h"


char buffer [1000];

int 
main (int argc, char *argv[])
{ 
    int rc;
    int s_parsed;
    asn_value_p new_val; 
    if (argc < 2)
        return 0;

    rc = asn_parse_dvalue_in_file(argv[1],
                            ndn_traffic_template, &new_val, &s_parsed);
    printf ("ret code from parse sequence: %6x, syms: %d\n", rc, s_parsed);
    asn_sprint_value(new_val, buffer, 1000, 0);
    printf ("\nparsed value: \n--\n%s\n--\n", buffer); 
    asn_free_value (new_val); 

    return 0;
}
