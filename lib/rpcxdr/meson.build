# Copyright (C) 2018 OKTET Labs. All rights reserved.

conf = configuration_data()

rpcdefs = files('tarpc_base.x.m4')
rpcdefs += files(get_option('rpcdefs'))

if cc.has_header('expat.h')
    have = 'HAVE_EXPAT_H'
    conf.set(have, 1)
    c_args += [ '-D' + have ]
endif

if cc.has_header('rpc/types.h')
    have = 'HAVE_RPC_TYPES_H'
    conf.set(have, 1)
    c_args += [ '-D' + have ]
endif
if cc.has_header('rpc/xdr.h')
    have = 'HAVE_RPC_XDR_H'
    conf.set(have, 1)
    c_args += [ '-D' + have ]
endif

#
# First we need to define which library we should use in our
# environment. This macro updates LIBS variable, which is used
# in the next checkings against in AC_CHECK_MEMBER() macro.
# - Solaris2 (SunOS 5.11) -lnsl
# - Modern Linux distros, such as Fedora 28, do not have RPC support
#   in glibc; libtirpc can be used instead.
#
if not cc.has_function('xdr_bytes', args: te_cflags)
    dep_rpc = cc.find_library('rpc', required: false)
    if dep_rpc.found() and cc.has_function('xdr_bytes', args: te_cflags,
                                           dependencies: dep_rpc)
        deps += [ dep_rpc ]
    else
        dep_nsl = cc.find_library('nsl', required: false)
        if dep_nsl.found() and
           cc.has_function('xdr_bytes', dependencies: dep_nsl)
            deps += [ dep_nsl ]
        else
            dep_tirpc = cc.find_library('tirpc', required: false)
            if dep_tirpc.found() and cc.has_function('xdr_bytes',
                                                     args: te_cflags,
                                                     dependencies: dep_tirpc)
                deps += [ dep_tirpc ]
            else
                error('No RPC library found')
            endif
        endif
    endif
endif

rpc_headers = '''
#if HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif
#if HAVE_RPC_XDR_H
#include <rpc/xdr.h>
#endif
'''
if cc.has_member('struct xdr_ops', 'x_putint32', args: c_args,
                 prefix: rpc_headers)
    conf.set('x_int32_arg_t', 'int32_t')
else
    conf.set('x_putint32', 'x_putlong')
    conf.set('x_getint32', 'x_getlong')
    conf.set('x_int32_arg_t', 'long')
endif


if not cc.has_function('xdr_int8_t', args: te_cflags, dependencies: deps)
    if cc.has_function('xdr_char', args: te_cflags, dependencies: deps)
        conf.set('xdr_int8_t', 'xdr_char')
    else
        error('There is no xdr_uint8_t or xdr_char!')
    endif
endif

if not cc.has_function('xdr_uint8_t', args: te_cflags, dependencies: deps)
    if cc.has_function('xdr_u_char', args: te_cflags, dependencies: deps)
        conf.set('xdr_uint8_t', 'xdr_u_char')
    elif cc.has_function('xdr_u_int8_t', args: te_cflags, dependencies: deps)
        conf.set('xdr_uint8_t', 'xdr_u_int8_t')
    else
        error('There is no xdr_uint8_t or xdr_u_char!')
    endif
endif

if not cc.has_function('xdr_int16_t', args: te_cflags, dependencies: deps)
    if cc.has_function('xdr_short', args: te_cflags, dependencies: deps)
        conf.set('xdr_int16_t', 'xdr_short')
    else
        error('There is no xdr_int16_t or xdr_short!')
    endif
endif

if not cc.has_function('xdr_uint16_t', args: te_cflags, dependencies: deps)
    if cc.has_function('xdr_u_int16_t', args: te_cflags, dependencies: deps)
        conf.set('xdr_uint16_t', 'xdr_u_int16_t')
    elif cc.has_function('xdr_u_short', args: te_cflags, dependencies: deps)
        conf.set('xdr_uint16_t', 'xdr_u_short')
    else
        error('There is no xdr_uint16_t, xdr_u_int16_t or xdr_u_short!')
    endif
endif

if not cc.has_function('xdr_int32_t', args: te_cflags, dependencies: deps)
    if cc.has_function('xdr_int', args: te_cflags, dependencies: deps)
        conf.set('xdr_int32_t', 'xdr_int')
    else
        error('There is no xdr_int32_t or xdr_int!')
    endif
endif

if not cc.has_function('xdr_uint32_t', args: te_cflags, dependencies: deps)
    if cc.has_function('xdr_u_int32_t', args: te_cflags, dependencies: deps)
        conf.set('xdr_uint32_t', 'xdr_u_int32_t')
    elif cc.has_function('xdr_u_int', args: te_cflags, dependencies: deps)
        conf.set('xdr_uint32_t', 'xdr_u_int')
    else
        error('There is no xdr_uint32_t, xdr_u_int32_t or xdr_u_int!')
    endif
endif

if not cc.has_function('xdr_uint64_t', args: te_cflags, dependencies: deps)
    if cc.has_function('xdr_u_int64_t', args: te_cflags, dependencies: deps)
        conf.set('xdr_uint64_t', 'xdr_u_int64_t')
    else
        error('There is no xdr_uint64_t or xdr_u_int64_t!')
    endif
endif

dep_expat = dependency('expat', required: false)
if dep_expat.found() and cc.has_function('XML_ParserCreate', args: te_cflags,
                                         dependencies: dep_expat)
    conf.set('HAVE_XML_PARSERCREATE', 1)
endif

configure_file(input: 'config.h.meson',
               output: 'config.h',
               configuration: conf)
c_args += [ '-DHAVE_CONFIG_H' ]

rpc_x = custom_target('gen-tarpc', input: rpcdefs,
                      capture: true, output: [ 'tarpc.x' ],
                      install: install_lib,
                      install_dir: join_paths(get_option('datadir'), 'rpcxdr'),
                      command: [ m4, '-P', rpcgen_m4, '@INPUT@' ])

rpc_h = custom_target('gen-tarpc-h', input: rpc_x,
                      output: [ '@BASENAME@.h' ],
                      install: install_lib,
                      install_dir: get_option('includedir'),
                      command: [ te_rpcgen, '-h', '@OUTPUT@', '@INPUT@' ])
rpc_xdr = custom_target('gen-tarpc-xdr', input: rpc_x,
                        output: [ '@BASENAME@_xdr.c' ],
                        command: [ te_rpcgen, '-x', '@OUTPUT@', '@INPUT@' ])
rpc_tbl = custom_target('gen-tarpc-rpctbl', input: rpc_x,
                        output: [ '@BASENAME@_rpctbl.c' ],
                        command: [ te_rpcgen, '-t', '@OUTPUT@', '@INPUT@' ])
rpc_stub = custom_target('gen-tarpc-stub', input: rpc_x,
                         output: [ '@BASENAME@_stub.c' ],
                         command: [ te_rpcgen, '-s', '@OUTPUT@', '@INPUT@' ])

rpc_h_dep = declare_dependency(sources: rpc_h)

headers += files('rpc_xdr.h')

sources += files('rpc_xdr.c', 'xml_xdr.c')
sources += rpc_xdr
sources += rpc_tbl

c_args += [
    '-Wno-unused-parameter',
    '-Wno-unused-variable',
    '-fno-strict-aliasing',
]
deps += [ rpc_h_dep ]

# Build server-side rpcxdr library here
te_lib_rpcxdrta = library('rpcxdrta', sources, rpc_stub, install: install_lib,
                          include_directories: includes,
                          c_args: [ '-DTE_DUMMY', c_args ],
                          dependencies: deps)
dep_lib_rpcxdrta = declare_dependency(link_with: te_lib_rpcxdrta,
                                      include_directories: includes,
                                      dependencies: deps)

# Add flags required for client-side rpcxdr library build
c_args += [ '-DTE_RPC_CLIENT' ]
