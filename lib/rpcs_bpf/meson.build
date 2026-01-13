# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2023-2026 OKTET Labs Ltd. All rights reserved.

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

#
# AF_XDP / XSK support
#
# xsk.h depends on kernel UAPI headers and may not compile
# with older kernel header versions.
#
xsk_header = ''
if cc.has_header('xdp/xsk.h')
    xsk_header = 'xdp/xsk.h'
elif cc.has_header('bpf/xsk.h')
    xsk_header = 'bpf/xsk.h'
endif

have_af_xdp = false
if xsk_header != ''
    code_af_xdp_headers = '''
#include <linux/if_xdp.h>
#include <@0@>

int main(void) {
    return 0;
}
'''.format(xsk_header)

    have_af_xdp = cc.compiles(code_af_xdp_headers, args: te_cflags,
                              name: 'AF_XDP headers usable')
endif

if have_af_xdp
    conf_data.set('HAVE_AF_XDP', 1)
    if xsk_header == 'xdp/xsk.h'
        conf_data.set('HAVE_XDP_XSK_H', 1)
    else
        conf_data.set('HAVE_BPF_XSK_H', 1)
    endif
else
    if xsk_header != ''
        warning('xsk.h is incompatible with the kernel headers')
    else
        warning('xsk.h is missing')
    endif
    warning('XDP RPC calls will not be supported on this platform')
endif

configure_file(output: 'config.h',
               configuration: conf_data)
