# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

xml2text_tmpls = files(
    'br.tmpl',
    'document_end.tmpl',
    'document_start.tmpl',
    'log_msg_file_end.tmpl',
    'log_msg_file_start.tmpl',
    'log_msg_start.tmpl',
    'log_msg_start_line_prefix.tmpl',
    'log_packet_field_data.tmpl',
    'log_packet_field_start.tmpl',
    'log_packet_proto_start.tmpl',
    'log_packet_start.tmpl',
    'mem_dump_elem_empty.tmpl',
    'mem_dump_elem_end.tmpl',
    'mem_dump_elem_start.tmpl',
    'mem_dump_end.tmpl',
    'mem_dump_row_end.tmpl',
    'mem_dump_row_start.tmpl',
    'mem_dump_start.tmpl',
)

install_data(
    xml2text_tmpls,
    install_dir: join_paths(
        get_option('datadir'), 'rgt-format', 'xml2text', 'tmpls',
    ),
)
