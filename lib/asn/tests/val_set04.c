#include <stdio.h>

#include "asn_impl.h"



asn_type my_sequence_of = {
    "MySeqOf",
    {APPLICATION, 7},
    SEQUENCE_OF,
    2,
    {&asn_base_integer}
};



asn_named_entry_t my_entry_array [] = {
    { "name", &asn_base_charstring },
    { "array", &my_sequence_of }
};



asn_type named_array = {
    "NamedArray",
    {APPLICATION, 6},
    SEQUENCE,
    2,
    {&my_entry_array}
};





char buffer [1000];

#define COMPON_TEST 1

int 
main (void)
{ 
    asn_value_p seq_val  = asn_init_value(&my_sequence_of);
    asn_value_p for_ins  = asn_init_value(&asn_base_integer);
    asn_value_p n_array  = asn_init_value(&named_array);

    int a = 1981;
    int r; 
    int l;

    char nm[] = "my great array!";

    r = asn_write_value_field(for_ins, &a, sizeof(a), "");
    if (r) { printf ("write_field error code: %6x\n", r); return r; }

    r = asn_insert_indexed (seq_val, for_ins, 0, "");
    if (r) { printf ("insert error code: %6x\n", r); return r; }

    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("after insert:\n--\n%s\n--\n", buffer); 

    a = 1;
    asn_write_value_field(for_ins, &a, sizeof(a), "");
    r = asn_insert_indexed (seq_val, for_ins, 0, "");
    if (r) { printf ("insert error code: %6x\n", r); return r; }

    a = 2;
    asn_write_value_field(for_ins, &a, sizeof(a), "");
    r = asn_insert_indexed (seq_val, for_ins, -1, "");
    if (r) { printf ("insert error code: %6x\n", r); return r; }

    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("after all inserts:\n--\n%s\n--\n", buffer); 

    l = asn_get_length (seq_val, "");
    printf ("length:%d\n", l); 

    r = asn_write_component_value(n_array, seq_val, "array");
    if (r) { printf ("insert error code: %6x\n", r); return r; }

    asn_sprint_value(n_array, buffer, 1000, 0);
    printf ("complex::\n--\n%s\n--\n", buffer); 

    asn_remove_indexed(seq_val, 1, "");

    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("after first remove:\n--\n%s\n--\n", buffer); 

    r = asn_remove_indexed(seq_val, 2, "");
    if (r) { printf ("shoud be wrong index error code: %6x\n", r); }
    asn_remove_indexed(seq_val, -1, "");
    asn_remove_indexed(seq_val, 0, "");

    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("at the end:\n--\n%s\n--\n", buffer); 

    asn_free_value(seq_val);

    asn_sprint_value(n_array, buffer, 1000, 0);
    printf ("complex::\n--\n%s\n--\n", buffer); 

    asn_remove_indexed(n_array, 0, "array");

    a = 55;
    asn_write_value_field(for_ins, &a, sizeof(a), "");
    r = asn_insert_indexed (n_array, for_ins, 1, "array");

    asn_free_value(for_ins);
    asn_write_value_field(n_array, nm, sizeof(nm), "name");

    asn_sprint_value(n_array, buffer, 1000, 0);
    printf ("complex::\n--\n%s\n--\n", buffer); 


    return 0;
}
