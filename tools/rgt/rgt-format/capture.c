/** @file
 * @brief Test Environment: Callback functions of capture packets processing.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov  <Andrey.Dmitrov@oktetlabs.ru>
 *
 */

#include "xml2gen.h"

#include "capture.h"

static int packet_num = 0;

/* See description in capture.h */
capture_tmpls_output_t capture_tmpls_out_cb = NULL;

/**
 * Call rgt_tmpls_output() or callback which should call it to
 * output an RGT template related to captured packets.
 *
 * @param _gctx     Pointer to rgt_gen_ctx_t.
 * @param _dctx     Pointer to rgt_depth_ctx_t.
 * @param _fd       FILE pointer.
 * @param _tmpl     Template to output.
 * @param _attrs    Template's attributes.
 */
#define OUTPUT_CAPTURE_TMPL(_gctx, _dctx, _fd, _tmpl, _attrs) \
    do {                                                          \
        if (capture_tmpls_out_cb != NULL)                         \
            capture_tmpls_out_cb(_gctx, _dctx, _tmpl, _attrs);    \
        else                                                      \
            rgt_tmpls_output(_fd, _tmpl, _attrs);                 \
    } while (0)

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

    /*
     * fd == NULL when we do not want to fill the file,
     * so it should be simply ignored.
     */
    if (fd == NULL)
        return;

    RGT_GET_TMPL_IDX(LOG_PACKET_START, "log_packet_start");

    packet_num++;
    attrs = rgt_tmpls_attrs_new(xml_attrs);
    rgt_tmpls_attrs_add_fstr(attrs, "packet_num", "%d", packet_num);
    OUTPUT_CAPTURE_TMPL(ctx, depth_ctx, fd,
                        &xml2fmt_tmpls[LOG_PACKET_START], attrs);
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

    /*
     * fd == NULL when we do not want to fill the file,
     * so it should be simply ignored.
     */
    if (fd == NULL)
        return;

    RGT_GET_TMPL_IDX(LOG_PACKET_PROTO_START, "log_packet_proto_start");

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    if (rgt_tmpls_xml_attrs_get(xml_attrs, "showname") == NULL &&
       (show = rgt_tmpls_xml_attrs_get(xml_attrs, "name")) != NULL)
    {
        rgt_tmpls_attrs_add_fstr(attrs, "showname", "%s", show);
    }
    rgt_tmpls_attrs_add_fstr(attrs, "packet_num", "%d", packet_num);
    OUTPUT_CAPTURE_TMPL(ctx, depth_ctx, fd,
                        &xml2fmt_tmpls[LOG_PACKET_PROTO_START],
                        attrs);
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

    RGT_FUNC_UNUSED_PRMS();

    if (!detailed_packets)
        return;

    if (depth_ctx->user_data == NULL)
        fd = (FILE *)(*((void **)(ctx->user_data)));
    else
        fd = (FILE *)(*((void **)(depth_ctx->user_data)));

    /*
     * fd == NULL when we do not want to fill the file,
     * so it should be simply ignored.
     */
    if (fd == NULL)
        return;

    RGT_GET_TMPL_IDX(LOG_PACKET_FIELD_START, "log_packet_field_start");
    RGT_GET_TMPL_IDX(LOG_PACKET_FIELD_DATA, "log_packet_field_data");

    attrs = rgt_tmpls_attrs_new(xml_attrs);
    if ((show = rgt_tmpls_xml_attrs_get(xml_attrs, "showname")) == NULL &&
        (show = rgt_tmpls_xml_attrs_get(xml_attrs, "show")) != NULL)
    {
        rgt_tmpls_attrs_add_fstr(attrs, "showname", "%s", show);
    }

    if (show != NULL)
    {
        OUTPUT_CAPTURE_TMPL(ctx, depth_ctx, fd,
                            &xml2fmt_tmpls[LOG_PACKET_FIELD_START],
                            attrs);
    }

    if ((name = rgt_tmpls_xml_attrs_get(xml_attrs, "name")) != NULL &&
        strstr(name, "data") != NULL && strcmp(name, "data.len") != 0 &&
        (show = rgt_tmpls_xml_attrs_get(xml_attrs, "show")) != NULL)
    {
        OUTPUT_CAPTURE_TMPL(ctx, depth_ctx, fd,
                            &xml2fmt_tmpls[LOG_PACKET_FIELD_DATA],
                            attrs);
    }

    rgt_tmpls_attrs_free(attrs);
}
