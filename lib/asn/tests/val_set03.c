#include <stdio.h>

#include "asn_impl.h"



asn_named_entry_t my_entry_array [] = {
    { "number", &asn_base_integer },
    { "string", &asn_base_charstring }
};


asn_named_entry_t str_ea [] = {
    { "string", &asn_base_charstring }
};

asn_named_entry_t num_ea [] = {
    { "number", &asn_base_integer }
};

asn_type my_sequence = {
    "MySequence",
    {APPLICATION, 1},
    SEQUENCE,
    2,
    {&my_entry_array}
};


asn_type my_str_sequence = {
    "MyStrSeq",
    {APPLICATION, 5},
    SEQUENCE,
    1,
    {&str_ea}
};


asn_type my_num_sequence = {
    "MyStrSeq",
    {APPLICATION, 5},
    SEQUENCE,
    1,
    {&num_ea}
};


asn_named_entry_t compl_entry_array [] = {
    { "nested", &my_sequence },
    { "name", &asn_base_charstring }
};

asn_type complex_sequence = {
    "ComplexSequence",
    {APPLICATION, 2},
    SEQUENCE,
    2,
    {&compl_entry_array}
};





char buffer [1000];

#define COMPON_TEST 1

#define BUF_TO_READ 100
char buf_to_read[BUF_TO_READ];

int 
main (void)
{ 
    asn_value_p seq_val   = asn_init_value(&my_sequence);
    asn_value_p compl_seq = asn_init_value(&complex_sequence);
    asn_value_p for_read;

    int a = 1981, b = 1234; 
    char str[] = "uura..";
    char str2[] = "sss.... lslsls";
    int r; 
    int read_len = BUF_TO_READ;

    r = asn_write_value_field(seq_val, &a, sizeof(a), "number");
    if (r) { printf ("error code returned: %d\n", r); return r; }

    printf ("str: '%s', strlen: %d, sizeof: %d\n", 
            str, strlen(str), sizeof(str));

    r = asn_write_value_field(seq_val, str, sizeof(str), "string");
    if (r) { printf ("error code returned: %d\n", r); return r; }

    r = asn_sprint_value(seq_val, buffer, 1000, 0);
    printf ("seq_val after write values: \n\"%s\"\n", buffer); 


    r = asn_write_value_field(compl_seq, str2, sizeof(str2), "nested.string");
    if (r) { printf ("error code returned: %6x\n", r); return r; }
    r = asn_write_value_field(compl_seq, &b, sizeof(b), "nested.number");
    if (r) { printf ("error code returned: %6x\n", r); return r; } 

    r = asn_sprint_value(compl_seq, buffer, 1000, 0);
    printf ("\n\ncompl_seq after write values: \n\"%s\"\n", buffer); 

    r = asn_read_component_value(compl_seq, &for_read, "nested");
    if (r) { printf ("error code returned from read comp val: %6x\n", r); return r; } 
    asn_sprint_value(for_read, buffer, 1000, 0);
    printf ("read component 'nested': \n--\n%s\n--\n", buffer); 

#if COMPON_TEST
    r = asn_write_component_value(compl_seq, seq_val, "nested");
    if (r) { printf ("write comp, error code returned: %6x\n", r); return r; }
#endif 

    r = asn_sprint_value(compl_seq, buffer, 1000, 0);
    printf ("\n\ncompl_seq after set component: \n\"%s\"\n", buffer); 
    return 0;
}
