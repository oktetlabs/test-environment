#include <stdio.h>

#include "tad_ch_impl.h"

int 
main()
{
    int id, id2;
    csap_p cp;

    csap_db_init(); 
    id = csap_create("first.low"); 
    if (id == 0)
    {
        fprintf (stderr, "CSAP was not created!\n");
        return 1;
    }
    printf("first csap id: %d\n", id);
    id = csap_create("second.low"); 
    printf("second csap id: %d\n", id);
    id2 = csap_create("third.low"); 
    printf("third csap id: %d\n", id2);

    cp = csap_find(id);
    printf ("ID: %d, up proto: %s, low proto: %s, depth: %d\n", 
            cp->id, cp->proto[0], cp->proto[1], cp->depth);
    csap_destroy (id);
    cp = csap_find(id);
    if (cp != NULL)
    {
        fprintf (stderr, "just destroyed CSAP is found in DB!\n");
        return 1;
    }
    if (csap_destroy(id)==0) /* destroy was successful?? */
    {
        fprintf (stderr, "just destroyed CSAP is destroyed again!\n");
        return 1;
    }


    id2 = csap_create("some.low");

/* This test checks implementation specific hint only! */
    if (id2 != id) 
    {
        fprintf (stderr, "WARNING: wrong numeration of CSAPs\n");
    }
    return 0;
}
