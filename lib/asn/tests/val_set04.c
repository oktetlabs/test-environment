#include <stdio.h>

#include "asn_impl.h"
#include "logger_api.h"

DEFINE_LGR_ENTITY("val_set04");


asn_type my_sequence_of = {
    "MySeqOf",
    {APPLICATION, 7},
    SEQUENCE_OF,
    2,
    {subtype:&asn_base_integer_s}
};



asn_named_entry_t my_entry_array [] = {
    { "name",  &asn_base_charstring_s, {PRIVATE, 0} },
    { "array", &my_sequence_of, {PRIVATE, 1} }
};



asn_type named_array = {
    "NamedArray",
    {APPLICATION, 6},
    SEQUENCE,
    2,
    {my_entry_array}
};





char buffer [1000];

#define COMPON_TEST 1
#define DEBUG 0

int 
main(void)
{ 
    asn_value *seq_val  = asn_init_value(&my_sequence_of);
    asn_value *for_ins  = asn_init_value(asn_base_integer);
    asn_value *n_array  = asn_init_value(&named_array);

    int a = 1981;
    int r; 
    int l;

    char nm[] = "my great array!";

    r = asn_write_value_field(for_ins, &a, sizeof(a), "");
    if (r) { fprintf(stderr, "write_field error code: %6x\n", r); return r; }

    r = asn_insert_indexed(seq_val, asn_copy_value(for_ins), 0, "");
    if (r) { fprintf(stderr, "insert error code: %6x\n", r); return r; }

    l = asn_sprint_value(seq_val, buffer, 1000, 0);
    if (l < 0) {fprintf(stderr, "error on sprint value\n"); return 1; }
#if DEBUG
    printf("after insert:\n--\n%s\n--\n", buffer); 
#endif

    a = 1;
    asn_write_primitive(for_ins, &a, sizeof(a));
    r = asn_insert_indexed(seq_val, asn_copy_value(for_ins), 0, "");
    if (r) { fprintf(stderr, "insert error code: %6x\n", r); return r; }

    a = 20;
    asn_write_value_field(for_ins, &a, sizeof(a), "");
    r = asn_insert_indexed(seq_val, asn_copy_value(for_ins), -1, "");
    if (r) { fprintf(stderr, "insert error code: %6x\n", r); return r; }

#if DEBUG
    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("after all inserts:\n--\n%s\n--\n", buffer); 
#endif

    l = asn_get_length(seq_val, "");
    printf("length:%d\n", l); 

    r = asn_write_component_value(n_array, seq_val, "array");
    if (r) { fprintf(stderr, "insert error code: %6x\n", r); return r; }

#if DEBUG
    asn_sprint_value(n_array, buffer, 1000, 0);
    printf("complex::\n--\n%s\n--\n", buffer); 
#endif

#define DEBUG 1
    r = asn_remove_indexed(seq_val, 1, "");
    if (r) { fprintf(stderr, "remove error code: %6x\n", r); return r; }

#if DEBUG
    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("after first remove:\n--\n%s\n--\n", buffer); 
#endif

    r = asn_remove_indexed(seq_val, 2, "");
    if (r) { fprintf(stderr, "rc : %6x, should be TE_EASNWRONGLABEL: %6x\n",
                     r, TE_EASNWRONGLABEL); }
    asn_remove_indexed(seq_val, -1, "");
    asn_remove_indexed(seq_val, 0, "");

#if DEBUG
    asn_sprint_value(seq_val, buffer, 1000, 0);
    printf("at the end:\n--\n%s\n--\n", buffer); 
#endif

    asn_free_value(seq_val);

#if DEBUG
    asn_sprint_value(n_array, buffer, 1000, 0);
    printf("complex::\n--\n%s\n--\n", buffer); 
#endif

    asn_remove_indexed(n_array, 0, "array");

    a = 55;
    asn_write_primitive(for_ins, &a, sizeof(a));
    r = asn_insert_indexed(n_array, asn_copy_value(for_ins), 1, "array");

    asn_free_value(for_ins);
    asn_write_value_field(n_array, nm, sizeof(nm), "name");

#if DEBUG
    asn_sprint_value(n_array, buffer, 1000, 0);
    printf("complex::\n--\n%s\n--\n", buffer); 
#endif


    return 0;
}
