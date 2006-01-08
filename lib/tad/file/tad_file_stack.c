/** @file
 * @brief TAD Dummy FILE Protocol
 *
 * Traffic Application Domain Command Handler.
 * Dummy FILE protocol implementaion, stack-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD File"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "tad_file_impl.h"

#include "logger_api.h"


/* See description tad_file_impl.h */
int 
tad_file_read_cb(csap_p csap, int timeout, char *buf, size_t buf_len)
{
    int rc = 0; 

    UNUSED(timeout);

    file_csap_specific_data_p spec_data = 
        (file_csap_specific_data_p) csap->layers[0].specific_data; 
        /* Maybe this zero should be changed to something more appr. */ 

    csap->last_errno = 0;

    if(spec_data->fstream == NULL)
        return -1;
    
    if (fgets (buf, buf_len, spec_data->fstream) == NULL)
    {
        if (feof( spec_data->fstream))
            csap->last_errno = TE_ETADENDOFDATA;
        else
            csap->last_errno = errno;
        rc = -1;
    }
    else
        rc = strlen(buf);

    return rc;
}


/* See description tad_file_impl.h */
te_errno
tad_file_write_cb(csap_p csap, const tad_pkt *pkt)
{
#if 1
    const void *buf;
    size_t      buf_len;

    if (pkt == NULL || tad_pkt_seg_num(pkt) != 1)
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    buf     = pkt->segs.cqh_first->data_ptr;
    buf_len = pkt->segs.cqh_first->data_len;
#endif
    int rc; 

    file_csap_specific_data_p spec_data = 
        (file_csap_specific_data_p) csap->layers[0].specific_data; 
        /* Maybe this zero should be changed to something more appr. */ 



    if(spec_data->fstream == NULL )
    {
        return -1;
    }
    rc = fwrite (buf, buf_len, 1, spec_data->fstream);
    printf ("write in file csap: %d bytes written\n", rc);

    return rc;
}


/* See description tad_file_impl.h */
te_errno
tad_file_single_init_cb(csap_p csap, unsigned int layer,
                        const asn_value *csap_nds)
{
    char  *filename;
    char   mode [8];
    size_t fn_len;
    int    rc; 

    asn_value_p                 file_csap_spec; 
    file_csap_specific_data_p   spec_data; 

    file_csap_spec = asn_read_indexed(csap_nds, layer, "");

    fn_len = asn_get_length(file_csap_spec, "filename");
    filename = malloc(fn_len + 1);
    rc = asn_read_value_field(file_csap_spec, filename, &fn_len, 
                "filename"); 
    if (rc) return rc;

    fn_len = sizeof(mode);
    rc = asn_read_value_field(file_csap_spec, mode, &fn_len, 
                "mode");
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        strcpy(mode, "a+");
    else if (rc != 0)
    {
        free(filename);
        VERB("Init, get mode fail: ASN error %x", rc);
        return rc;
    }


    if ((spec_data = malloc(sizeof(file_csap_specific_data_t)) ) == NULL)
        return TE_ENOMEM;


    csap->timeout          = 50000; 
    csap_set_proto_spec_data(csap, layer, spec_data);
    spec_data->filename = filename;

    printf ("open file for CSAP file: %s with mode <%s>\n", filename, mode);

    if (( spec_data->fstream = fopen(filename, mode)) == NULL) 
    { 
        return errno; 
        /* return standard system errno which should be set by 'fopen'. */
    }
    printf ("file is opened\n");

    csap->read_write_layer = layer;

    return 0;
}


/* See description tad_file_impl.h */
te_errno
tad_file_single_destroy_cb(csap_p csap, unsigned int layer)
{
    file_csap_specific_data_p spec_data = 
        (file_csap_specific_data_p) csap_get_proto_spec_data(csap, layer); 

    if(spec_data->fstream)
        fclose(spec_data->fstream);
    
    spec_data->fstream = NULL;

    if (spec_data->filename)
        free(spec_data->filename);
   
    spec_data->filename = NULL;

    return 0;
}
