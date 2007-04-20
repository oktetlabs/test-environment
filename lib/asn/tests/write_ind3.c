#include "te_config.h"

#include <stdio.h>
#include "test_types.h"

#include "ndn.h"
#include "ndn_ipstack.h"


int
main(void)
{
    char buffer[1000];
    asn_value *opt, *options;
    int rc, syms;
    char len;
    uint32_t opt_val;
    

    options = asn_init_value(ndn_tcp_options_seq);

#if 1
    opt = asn_init_value(ndn_tcp_option);


    len = 4;
    rc = asn_write_value_field(opt, &len, 1, "#mss.length.#plain");
    if (rc) 
    {
        fprintf(stderr, "put mss length failed: %x\n", rc);
        return 2;
    }
    opt_val = 12345;
    rc = asn_write_value_field(opt, &opt_val, 4, "#mss.mss.#plain");
    if (rc) 
    {
        fprintf(stderr, "put mss value failed: %x\n", rc);
        return 3;
    }

    rc = asn_insert_indexed(options, opt, -1, "");
    if (rc) 
    {
        fprintf(stderr, "insert option failed: %x\n", rc);
        return 3;
    }
#endif
    {
        opt = asn_init_value(ndn_tcp_option);
        len = 10;
        rc = asn_write_value_field(opt, &len, 1, "#timestamp.length.#plain");
        opt_val = 12345678;
        rc = asn_write_value_field(opt, &opt_val, 4,
                                   "#timestamp.value.#plain");
        if (rc) 
        {
            fprintf(stderr, "write timestamp failed: %x\n", rc);
            return 3;
        }
        opt_val = 0;
        rc = asn_write_value_field(opt, &opt_val, 4,
                               "#timestamp.echo-reply.#plain");

        rc = asn_insert_indexed(options, opt, -1, "");
        if (rc) 
        {
            fprintf(stderr, "insert option failed: %x\n", rc);
            return 3;
        }
    }
    opt = asn_init_value(ndn_tcp_option);
    rc = asn_insert_indexed(options, opt, -1, "");


    asn_sprint_value(options, buffer, sizeof(buffer), 0);
    printf("new value: <%s>\n", buffer);

    return 0;
}
