/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Dummy FILE protocol implementaion, stack-related callbacks.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * Author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#ifdef HAVE_CONFIG_H
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
#include <string.h>

#include "tad_file_impl.h"

#define LGR_USER   "TAD File"
#include "logger_ta.h"


/**
 * Callback for read data from media of 'file' CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
file_read_cb (csap_p csap_descr, int timeout, char *buf, int buf_len)
{
    int rc = 0; 

    file_csap_specific_data_p spec_data = 
        (file_csap_specific_data_p) csap_descr->layer_data[0]; 
        /* Maybe this zero should be changed to something more appr. */ 

    csap_descr->last_errno = 0;

    if(spec_data->fstream == NULL)
        return -1;
    
    if (fgets (buf, buf_len, spec_data->fstream) == NULL)
    {
        if (feof( spec_data->fstream))
            csap_descr->last_errno = ETADENDOFDATA;
        else
            csap_descr->last_errno = errno;
        rc = -1;
    }
    else
        rc = strlen(buf);

    return rc;
}

/**
 * Callback for write data to media of 'file' CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
int 
file_write_cb (csap_p csap_descr, char *buf, int buf_len)
{
    int rc; 

    file_csap_specific_data_p spec_data = 
        (file_csap_specific_data_p) csap_descr->layer_data[0]; 
        /* Maybe this zero should be changed to something more appr. */ 



    if(spec_data->fstream == NULL )
    {
        return -1;
    }
    rc = fwrite (buf, buf_len, 1, spec_data->fstream);
    printf ("write in file csap: %d bytes written\n", rc);

    return rc;
}

/**
 * Callback for write data to media of 'file' CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param w_buf         buffer with data to be written.
 * @param w_buf_len     length of data in w_buf.
 * @param r_buf         buffer for data to be read.
 * @param r_buf_len     available length r_buf.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
int 
file_write_read_cb (csap_p csap_descr, int timeout,
			char *w_buf, int w_buf_len,
			char *r_buf, int r_buf_len)
{
    return -1;
}

/**
 * Callback for init 'file' CSAP layer  if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
file_single_init_cb (int csap_id, const asn_value_p csap_nds, int layer)
{
    char * filename;
    char   mode [8];
    int    fn_len;
    int    rc; 

    asn_value_p                 file_csap_spec; 
    file_csap_specific_data_p   spec_data; 
    csap_p                      csap_descr;

    file_csap_spec = asn_read_indexed(csap_nds, layer, "");
    csap_descr = csap_find(csap_id);

    fn_len = asn_get_length(file_csap_spec, "filename");
    filename = malloc(fn_len + 1);
    rc = asn_read_value_field(file_csap_spec, filename, &fn_len, 
                "filename"); 
    if (rc) return rc;

    fn_len = sizeof(mode);
    rc = asn_read_value_field(file_csap_spec, mode, &fn_len, 
                "mode");
    if (rc == EASNINCOMPLVAL)
        strcpy(mode, "a+");
    else if (rc != 0)
    {
        free(filename);
        VERB("Init, get mode fail: ASN error %x", rc);
        return rc;
    }


    if ((spec_data = malloc(sizeof(file_csap_specific_data_t)) ) == NULL)
        return ENOMEM;


    csap_descr->timeout          = 50000; 
    csap_descr->layer_data[layer] = spec_data;
    spec_data->filename = filename;

    printf ("open file for CSAP file: %s with mode <%s>\n", filename, mode);

    if (( spec_data->fstream = fopen(filename, mode)) == NULL) 
    { 
        return errno; 
        /* return standard system errno which should be set by 'fopen'. */
    }
    printf ("file is opened\n");

    csap_descr->write_cb         = file_write_cb; 
    csap_descr->read_cb          = file_read_cb; 
    csap_descr->read_write_layer = layer;

    return 0;
}

/**
 * Callback for destroy 'file' CSAP layer  if single in stack.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
int 
file_single_destroy_cb (int csap_id, int layer)
{
    csap_p csap_descr = csap_find(csap_id);

    file_csap_specific_data_p spec_data = 
        (file_csap_specific_data_p) csap_descr->layer_data[layer]; 

    if(spec_data->fstream)
        fclose(spec_data->fstream);
    
    spec_data->fstream = NULL;

    if (spec_data->filename)
        free(spec_data->filename);
   
    spec_data->filename = NULL;

    return 0;
}


