#include <stdio.h>
#include "tad.h"

int 
main(int argc, char *argv[])
{
    tad_int_expr_t *expression = NULL;
    int syms = 0;
    int rc;
    int64_t res;
    tad_template_arg_t args[2] = 
    {
        {ARG_INT, 4, 10}, 
        {ARG_INT, 4, 15}, 
    };

    uint8_t b[] = {0, 1, 2, 3};

    if (argc < 2)
    {
        fprintf (stderr, "expression expected in command line\n");
        return 1;
    }
    rc = tad_int_expr_parse(argv[1], &expression, &syms);

    printf ("rc %x, syms %d, expr %p\n", rc, syms, expression);
    if (rc == 0 && expression != NULL)
    { 
        printf ("type: %d, d_len %d\n", expression->n_type, expression->d_len);
        switch (expression->n_type)
        {
            case CONSTANT:
                printf ("int const: %d\n", expression->val_i32);
                break;
            case EXP_ADD:
                printf ("summa\n");
                break;
            case ARG_LINK:
                printf ("argument %d\n", expression->arg_num);
        }

        rc = tad_int_expr_calculate(expression, args, &res);
        printf ("rc: %x, result: %lld, %llx\n", rc, res, res);
    } 
    tad_int_expr_free(expression);

    res = 0x233445;
    printf ("ntoh for 64 bit test: %016llx, %016llx\n", res, tad_ntohll(res));

    expression = tad_int_expr_constant_arr(b, sizeof(b));
    rc = tad_int_expr_calculate(expression, args, &res);
    printf ("rc: %x, result: %016llx\n", rc, res);

    return 0; 
}
