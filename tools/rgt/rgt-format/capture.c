/** @file
 * @brief Test Environment: Callback functions of capture packets processing.
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
 *
 * @author Andrey Dmitrov  <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: 
 */

#include "xml2gen.h"

static int packet_num = 0;

/* Max attribute length in one line */
int  rgt_max_attribute_length = 98;
/* A tag to separate lines */
const char *rgt_line_separator = "<br>";

#define RGT_GET_TMPL_IDX(_idx, _shortname)                      \
    if (_idx == -1)                                             \
    {                                                           \
        _idx = rgt_xml2fmt_files_get_idx(_shortname);           \
        if (_idx == -1)                                         \
        {                                                       \
            fprintf(stderr, "Couldn't find %s\n", _shortname);  \
            return;                                             \
        }                                                       \
    }

RGT_DEF_FUNC(proc_log_packet_start)
{
    FILE        *fd;
    rgt_attrs_t *attrs;
    static int   LOG_PACKET_START = -1;
    RGT_FUNC_UNUSED_PRMS();

    if (depth_ctx->user_data == NULL)
        fd = (FILE *)(*((void **)(ctx->user_data)));
    else
        fd = (FILE *)(*((void **)(depth_ctx->user_data)));

    RGT_GET_TMPL_IDX(LOG_PACKET_START, "log_packet_start");

    packet_num++;
    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "packet_num", "%d", packet_num);
    rgt_tmpls_output(fd, &xml2fmt_tmpls[LOG_PACKET_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

RGT_DEF_FUNC(proc_log_packet_proto_start)
{
    FILE           *fd;
    rgt_attrs_t    *attrs;
    const char     *show;
    static int      LOG_PACKET_PROTO_START = -1;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_ctx->user_data == NULL)
        fd = (FILE *)(*((void **)(ctx->user_data)));
    else
        fd = (FILE *)(*((void **)(depth_ctx->user_data)));

    RGT_GET_TMPL_IDX(LOG_PACKET_PROTO_START, "log_packet_proto_start");

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    if (rgt_tmpls_xml_attrs_get(xml_attrs, "showname") == NULL &&
       (show = rgt_tmpls_xml_attrs_get(xml_attrs, "name")) != NULL)
    {
        rgt_tmpls_attrs_add_fstr(attrs, "showname", "%s", show);
    }
    rgt_tmpls_attrs_add_fstr(attrs, "packet_num", "%d", packet_num);
    rgt_tmpls_output(fd, &xml2fmt_tmpls[LOG_PACKET_PROTO_START], attrs);
    rgt_tmpls_attrs_free(attrs);
}

RGT_DEF_FUNC(proc_log_packet_field_start)
{
    FILE           *fd;
    rgt_attrs_t    *attrs;
    const char     *show;
    const char     *name;
    static int      LOG_PACKET_FIELD_START = -1;
    static int      LOG_PACKET_FIELD_DATA  = -1;
    char           *buf;

    RGT_FUNC_UNUSED_PRMS();

    if (depth_ctx->user_data == NULL)
        fd = (FILE *)(*((void **)(ctx->user_data)));
    else
        fd = (FILE *)(*((void **)(depth_ctx->user_data)));

    RGT_GET_TMPL_IDX(LOG_PACKET_FIELD_START, "log_packet_field_start");
    RGT_GET_TMPL_IDX(LOG_PACKET_FIELD_DATA, "log_packet_field_data");

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    if ((show = rgt_tmpls_xml_attrs_get(xml_attrs, "showname")) == NULL &&
        (show = rgt_tmpls_xml_attrs_get(xml_attrs, "show")) != NULL)
    {
        rgt_tmpls_attrs_add_fstr(attrs, "showname", "%s", show);
    }

    if (show != NULL)
        rgt_tmpls_output(fd, &xml2fmt_tmpls[LOG_PACKET_FIELD_START], attrs);

    if ((name = rgt_tmpls_xml_attrs_get(xml_attrs, "name")) != NULL &&
        strstr(name, "data") != NULL && strcmp(name, "data.len") != 0 &&
        (show = rgt_tmpls_xml_attrs_get(xml_attrs, "show")) != NULL)
    {
        rgt_tmpls_output(fd, &xml2fmt_tmpls[LOG_PACKET_FIELD_DATA], attrs);
    }

    rgt_tmpls_attrs_free(attrs);
}
