#include <stdio.h>

#include "tad_ch_impl.h"

int 
main()
{
    char my_type[] = "a.b";
    int id;
    csap_p cp = NULL;

    csap_id_init(); 
    if (csap_create(my_type, &cp) != 0)
    {
        fprintf (stderr, "Failed to create a new CSAP!\n");
        return 1;
    }
    id = cp->id;

    cp = csap_find(id);
    if (cp == NULL)
    {
        fprintf (stderr, "just created CSAP not found!\n");
        return 1;
    }
    printf ("ID: %d, up proto: %s, low proto: %s, depth: %d\n", 
            cp->id, cp->proto[0], cp->proto[1], cp->depth);
    csap_destroy (id);
    cp = csap_find(id);
    if (cp != NULL)
    {
        fprintf (stderr, "just destroyed CSAP is found in DB!\n");
        return 1;
    }
    return 0;
}
