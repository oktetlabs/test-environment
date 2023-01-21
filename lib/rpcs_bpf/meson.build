# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2023 OKTET Labs Ltd. All rights reserved.

conf_data = configuration_data()

sources += files(
    'bpf.c',
)

te_libs += [
    'rpcserver',
]

if cc.has_header('bpf/bpf.h')
    conf_data.set('HAVE_BPF_BPF_H', '1')
else
    warning('bpf.h is missing, BPF and XDP RPC calls will not be supported')
endif

if cc.has_header('xdp/xsk.h')
    conf_data.set('HAVE_XDP_XSK_H', '1')
elif cc.has_header('bpf/xsk.h')
    conf_data.set('HAVE_BPF_XSK_H', '1')
else
    warning('xsk.h is missing, XDP RPC calls will not be supported')
endif

configure_file(output: 'config.h',
               configuration: conf_data)
