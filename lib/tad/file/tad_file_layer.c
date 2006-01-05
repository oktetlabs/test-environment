/** @file
 * @brief TAD Dummy FILE Protocol
 *
 * Traffic Application Domain Command Handler.
 * Dummy FILE protocol implementaion, layer-related callbacks.
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "tad_file_impl.h"

#include "logger_api.h"


/* See description in tad_file_impl.h */
te_errno
tad_file_gen_bin_cb(csap_p csap, unsigned int layer,
                    const asn_value *tmpl_pdu, void *opaque,
                    const tad_tmpl_arg_t *args, size_t arg_num, 
                    tad_pkts *sdus, tad_pkts *pdus)
{
#if 0
    int rc;

    size_t line_len; 
    char  *line;

    UNUSED(csap);
    UNUSED(layer);
    UNUSED(args);
    UNUSED(arg_num);
    UNUSED(up_payload); 

    line_len = asn_get_length(tmpl_pdu, "line");
    if (line_len <= 0)
    {
        return 1;
    }

    if ((line = malloc (line_len)) == NULL)
        return TE_ENOMEM;
    rc = asn_read_value_field(tmpl_pdu, line, &line_len, "line");

    pkts->data = line;
    pkts->len  = line_len;
    pkts->next = NULL;

    pkts->free_data_cb = free;

    while (line_len)
    {
        if (!(*line)) *line = '\n';
        line++, line_len--;
    }

    return 0;
#endif
}


/* See description in tad_file_impl.h */
te_errno
tad_file_match_bin_cb(csap_p csap, unsigned int layer,
                      const asn_value *pattern_pdu,
                      const csap_pkts *pkt, csap_pkts *payload, 
                      asn_value_p parsed_packet)
{
    char buf[10000];

    char *line = (char*) pkt->data;
    int line_len = pkt->len;
    int rc;

    UNUSED(csap);
    UNUSED(layer);
    UNUSED(pattern_pdu);
    UNUSED(payload);

    printf ("file_match. len: %d, line: %s\n", line_len, line);

    rc = asn_write_value_field(parsed_packet, line, line_len, 
                                "#file.line.#plain");

    if (rc) return rc;

    asn_sprint_value(parsed_packet, buf, sizeof(buf), 0);
    printf ("file_match. parsed packet:\n%s\n--\n", buf); 

    return 0;
}
