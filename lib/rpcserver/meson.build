# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

is_multi_build_library = true

conf = configuration_data()

rpcserver_agentlib = get_variable('opt-rpcserver-agentlib'.underscorify())

if get_variable('opt-rpcserver-idle-plugins'.underscorify())
    conf.set('ENABLE_RPC_PLUGINS', 1)
endif

if get_variable('opt-rpcserver-smart-mem'.underscorify())
    conf.set('ENABLE_RPC_MEM', 1)
endif

if get_variable('opt-static-rpc-lookup'.underscorify())
    conf.set('DEFAULT_STATIC_RPC_LOOKUP', 1)
endif

if get_variable('opt-typesafe-rpc'.underscorify())
    code_typeof = '''
    int testfunc(void *ptr);
    int testfunc2(void *ptr);
    __typeof(testfunc) *funcptr = testfunc2;
    '''
    if cc.compiles(code_typeof, name: 'type-safe RPC implementation',
                   args: te_cflags)
        conf.set('ENABLE_TYPESAFE_RPC', 1)
    else
        error('__typeof is not supported')
    endif
endif

t = 'struct mmsghdr'
if cc.has_type(t, prefix: '#include <sys/socket.h>', args: te_cflags)
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

t = 'struct ip_mreqn'
if cc.has_type(t, prefix: '#include <netinet/in.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

t = 'struct group_req'
if cc.has_type(t, prefix: '#include <netinet/in.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

t = 'struct tcp_info'
if cc.has_type(t, prefix: '#include <netinet/tcp.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

t = 'struct tcp_info'
m = 'tcpi_rcv_rtt'
if cc.has_member(t, m, prefix: '#include <netinet/in.h>')
    conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
             m.to_upper().underscorify(), 1)
endif

t = 'struct ipv6_mreq'
m = 'ipv6mr_interface'
if cc.has_member(t, m, prefix: '#include <netinet/in.h>')
    conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
             m.to_upper().underscorify(), 1)
endif
m = 'ipv6mr_ifindex'
if cc.has_member(t, m, prefix: '#include <netinet/in.h>')
    conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
             m.to_upper().underscorify(), 1)
endif

code_prefix_net_if_h = '''
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
'''

t = 'struct lifreq'
if cc.has_type(t, prefix: '#include <net/if.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
    struct_ifreq_members = [ 'ifr_mtu', 'lifr_mtu' ]
    foreach m : struct_ifreq_members
        if cc.has_member(t, m, prefix: code_prefix_net_if_h)
            conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
                     m.to_upper().underscorify(), 1)
        endif
    endforeach
endif

t = 'struct ifreq'
struct_ifreq_members = [ 'ifr_mtu', 'ifr_hwaddr', 'ifr_enaddr' ]
foreach m : struct_ifreq_members
    if cc.has_member(t, m, prefix: code_prefix_net_if_h)
        conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
                 m.to_upper().underscorify(), 1)
    endif
endforeach

code_prefix_signal_h = '''
#include <signal.h>
'''

t = 'struct sigaction'
m = 'sa_restorer'
if cc.has_member(t, m, prefix: code_prefix_signal_h)
    conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
             m.to_upper().underscorify(), 1)
endif

t = 'siginfo_t'
struct_siginfo_t_members = [
    'si_trapno',
    'si_addr_lsb',
    'si_utime',
    'si_stime',
    'si_int',
    'si_ptr',
    'si_overrun',
    'si_band',
    'si_timerid',
    'si_fd',
]
foreach m : struct_siginfo_t_members
    if cc.has_member(t, m, prefix: code_prefix_signal_h)
        conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
                 m.to_upper().underscorify(), 1)
    endif
endforeach

t = 'union sigval'
union_sigval_members = [
    'sival_int',
    'sigval_int',
    'sival_ptr',
    'sigval_ptr',
]
foreach m : union_sigval_members
    if cc.has_member(t, m, prefix: code_prefix_signal_h)
        conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
                 m.to_upper().underscorify(), 1)
    endif
endforeach


code_prefix_dirent_h = '''
#include <dirent.h>
'''

t = 'struct dirent'
struct_dirent_members = [
    'd_type',
    'd_off',
    'd_offset',
    'd_namelen',
]
foreach m : struct_dirent_members
    if cc.has_member(t, m, prefix: code_prefix_dirent_h)
        conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
                 m.to_upper().underscorify(), 1)
    endif
endforeach

t = 'struct passwd'
m = 'pw_gecos'
if cc.has_member(t, m, prefix: '#include <pwd.h>')
    conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
             m.to_upper().underscorify(), 1)
endif

t = 'struct arpreq'
m = 'arp_dev'
if cc.has_member(t, m, prefix: '#include <net/if_arp.h>')
    conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
             m.to_upper().underscorify(), 1)
endif

#
# Dynamic linker functions are provided by:
# - -ldl in Linux;
# - -lc in FreeBSD;
# - N/A for NetBSD, since automatically included in every dynamically
#   linked program.
#
f = 'dlopen'
have_dlopen = false
if cc.has_function(f, args: te_cflags)
    have_dlopen = true
else
    dep_dl = cc.find_library('dl', required: false)
    if cc.has_function(f, args: te_cflags, dependencies: dep_dl)
        have_dlopen = true
        deps += [ dep_dl ]
    else
        warning('No dynamic linker API found')
    endif
endif
if have_dlopen
    conf.set('HAVE_' + f.to_upper().underscorify(), 1)
endif

#
# Check if RTLD_NODELETE constant is defined
# (it could be defined either as a macro or
# as an element of enumeration).
#
code_dlopen_rtld_nodelete = '''
#include <dlfcn.h>
int main(void) {
    dlopen("", RTLD_NODELETE);
    return 0;
}
'''
if cc.compiles(code_dlopen_rtld_nodelete, name: 'check RTLD_NODELETE constant',
               args: te_cflags)
    conf.set('HAVE_RTLD_NODELETE', 1)
endif

check_headers = [
    'sys/systeminfo.h',
    'sys/klog.h',
    'linux/if_vlan.h',
    'sys/statvfs.h',
    'termios.h',
    'linux/ppdev.h',
]
foreach h : check_headers
    if cc.has_header(h)
        conf.set('HAVE_' + h.to_upper().underscorify(), 1)
    endif
endforeach

#
# We do not need epoll really, since we use it via RPC and dlopen,
# but we do need the header for struct epoll_event.
#
t = 'struct epoll_event'
if cc.has_type(t, prefix: '#include <sys/epoll.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

t = 'struct if_nameindex'
if cc.has_type(t, prefix: '#include <net/if.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

#
# Check for 'struct sg_io_hdr'
#
sg_io_headers = [
    'scsi/sg.h',
    'cam/scsi/scsi_sg.h',
]
foreach h : sg_io_headers
    if cc.has_header(h, args: c_args)
        have = 'HAVE_' + h.to_upper().underscorify()
        conf.set(have, 1)
        c_args += [ '-D' + have ]
        t = 'struct sg_io_hdr'
        if cc.has_type(t, prefix: '#include <' + h + '>')
           conf.set('HAVE_' + t.to_upper().underscorify(), 1)
        endif
   endif
endforeach

#
# Check for 'struct ptp_clock_caps' and some of its members
#
t = 'struct ptp_clock_caps'
if cc.has_type(t, prefix: '#include <linux/ptp_clock.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)

    members = [
        'n_pins',
        'cross_timestamping',
        'adjust_phase',
    ]
    foreach m : members
        if cc.has_member(t, m, prefix: '#include <linux/ptp_clock.h>')
            conf.set('HAVE_' + t.to_upper().underscorify() + '_' +
                     m.to_upper().underscorify(), 1)
        endif
    endforeach
endif

t = 'struct ptp_sys_offset'
if cc.has_type(t, prefix: '#include <linux/ptp_clock.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

t = 'struct ptp_sys_offset_extended'
if cc.has_type(t, prefix: '#include <linux/ptp_clock.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

t = 'struct ptp_sys_offset_precise'
if cc.has_type(t, prefix: '#include <linux/ptp_clock.h>')
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

#
# Asynchronous I/O:
# - FreeBSD has in -lc;
# - Linux has in -lnsl -lrt;
# - Solaris2 (SunOS 5.11) in -lrt;
# - NetBSD does not have.
#
f = 'aio_read'
have_aio_read = false
if cc.has_function(f, args: te_cflags)
    have_aio_read = true
else
    dep_rt = cc.find_library('rt', required: false)
    if cc.has_function(f, args: te_cflags, dependencies: dep_rt)
        have_aio_read = true
        deps += [ dep_rt ]
    else
        dep_nsl = cc.find_library('nsl', required: false)
        if cc.has_function(f, args: te_cflags,
                           dependencies: [ dep_nsl, dep_rt ])
            have_aio_read = true
            deps += [ dep_nsl, dep_rt ]
        else
            warning('Asynchronous I/O not supported')
        endif
    endif
endif
if have_aio_read
    conf.set('HAVE_' + f.to_upper().underscorify(), 1)
endif

unistd_funcs = [ 'pwritev', 'preadv' ]
foreach f : unistd_funcs
    if cc.has_function(f, args: te_cflags)
        conf.set('HAVE_' + f.to_upper().underscorify(), 1)
    endif
endforeach

#
# FreeBSD 4.x has no svc_exit().
# NetBSD has no svcunix_create().
#
svc_funcs = [ 'svcunix_create', 'svc_exit' ]
foreach f : svc_funcs
    if cc.has_function(f, args: te_cflags)
        conf.set('HAVE_' + f.to_upper().underscorify(), 1)
    endif
endforeach

deps += [ dep_rpcs ]

#
# First we need to define which library we should use in our
# environment. This macro updates LIBS variable, which is used
# in the next checkings against in AC_CHECK_MEMBER() macro.
# - Solaris2 (SunOS 5.11) -lnsl
# - Modern Linux distros, such as Fedora 28, do not have RPC support
#   in glibc; libtirpc can be used instead.
#
if not cc.has_function('xdr_bytes', args: te_cflags, dependencies: deps)
    dep_rpc = cc.find_library('rpc', required: false)
    if dep_rpc.found() and cc.has_function('xdr_bytes', args: te_cflags,
                                           dependencies: dep_rpc)
        deps += [ dep_rpc ]
    else
        dep_nsl = cc.find_library('nsl', required: false)
        if dep_nsl.found() and
           cc.has_function('xdr_bytes', args: te_cflags, dependencies: dep_nsl)
            deps += [ dep_nsl ]
        else
            error('No RPC library found')
        endif
    endif
endif

# This symbol must be checked with a full dependeny set
t = 'SVCXPRT'
if cc.has_header_symbol('rpc/rpc.h', t, dependencies: deps)
    conf.set('HAVE_' + t.to_upper().underscorify(), 1)
endif

rpc_headers = '''
#include "te_config.h"
#if HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif
#if HAVE_RPC_XDR_H
#include <rpc/xdr.h>
#endif
'''
if cc.has_member('struct xdr_ops', 'x_putint32', args: c_args,
                 include_directories: includes,
                 prefix: rpc_headers, dependencies: deps)
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
    elif cc.has_function('xdr_u_int', dependencies: deps)
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

rpc_server_config_h = configure_file(input: 'rpc_server_config.h.meson',
                                     output: 'rpc_server_config.h',
                                     configuration: conf)
headers += [ rpc_server_config_h ]
headers += files(
    'iomux.h',
    'rcf_pch_mem.h',
    'rpc_server.h',
)
sources += files(
    'iomux.c',
    'rcf_pch_mem.c',
    'rpc_server.c',
    'rpcs_conv.c',
    'rpcs_msghdr.c',
    'tarpc_imp.c',
)

te_libs += [
    'loggerta',
    'rpc_types',
    rpcserver_agentlib,
    'rpctransport',
]
