# Copyright (C) 2018 OKTET Labs. All rights reserved.

name = get_option('agent-unix-name')
ta_platform = get_option('agent-unix-platform')
tadir = join_paths(get_option('agentsdir'), name)

includes = []

c_args = [ '-fno-strict-aliasing' ]
c_args += [ '-DRCF_RPC' ]
c_args += [ '-DTE_AGT_PLATFORM="' + ta_platform + '"' ]

link_args = []

deps = [ dep_threads, cc.find_library('m') ]

sources = files(
    'main.c',
    'log_remote.c',
)

subdir('conf')

ta_symtbl_libs = [
    'tools',
    'rcfpch',
    'rpcserver',
    'agentlib',
    'tad',
    'ta_job',
]

ta_non_symtbl_libs = [
    'rpcxdrta',
    'rpc_types',
    'conf_oid',
    'loggerta',
    'logger_file',
    'logger_core',
]

foreach extra_lib : get_option('agent-unix-libs')
    if not ta_symtbl_libs.contains(extra_lib)
        ta_symtbl_libs += [ extra_lib ]
    endif
endforeach

if get_option('libs').split().contains('netconf')
    c_args += [ '-DUSE_LIBNETCONF' ]
    ta_non_symtbl_libs += [ 'netconf' ]
endif

check_headers = [
    'linux/if_vlan.h',
    'search.h',
    'sys/klog.h',
]

foreach h : check_headers
    if cc.has_header(h)
        c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
    endif
endforeach

code_prefix_net_route_h = '''
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
'''

t = 'struct rtentry'
if cc.has_type(t, prefix: code_prefix_net_route_h)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif
t = 'struct ortentry'
if cc.has_type(t, prefix: code_prefix_net_route_h)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif

code_prefix_net_if_h = '''
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
'''

t = 'struct lifreq'
if cc.has_type(t, prefix: code_prefix_net_if_h)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
    struct_ifreq_members = [ 'ifr_mtu', 'lifr_mtu' ]
    foreach m : struct_ifreq_members
        if cc.has_member(t, m, prefix: code_prefix_net_if_h)
            c_args += [ '-DHAVE_' + t.to_upper().underscorify() + '_' +
                        m.to_upper().underscorify() ]
        endif
    endforeach
endif

t = 'struct ifreq'
struct_ifreq_members = [ 'ifr_mtu', 'ifr_hwaddr', 'ifr_enaddr' ]
foreach m : struct_ifreq_members
    if cc.has_member(t, m, prefix: code_prefix_net_if_h)
        c_args += [ '-DHAVE_' + t.to_upper().underscorify() + '_' +
                    m.to_upper().underscorify() ]
    endif
endforeach

t = 'struct arpreq'
m = 'arp_dev'
if cc.has_member(t, m, prefix: '#include <net/if_arp.h>')
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() + '_' +
                m.to_upper().underscorify() ]
endif

#
# Berkeley Socket API is provided by:
#  - -lc on Linux, FreeBSD (5, 6), NetBSD (1.6, 2.0);
#  - -lsocket on Solaris2 (SunOS 5.11).
#
if not cc.has_function('socket', args: te_cflags)
    dep_socket = cc.find_library('socket', required: false)
    if dep_socket.found() and
       cc.has_function('socket', args: te_cflags, dependencies: dep_socket)
        deps += [ dep_socket ]
    endif
endif

#
# sendfile() is provided by:
#  - -lc on Linux;
#  - -lsendfile on Solaris2 (SunOS 5.11).
#
if not cc.has_function('sendfile', args: te_cflags)
    dep_sendfile = cc.find_library('sendfile', required: false)
    if dep_sendfile.found() and
       cc.has_function('sendfile', args: te_cflags, dependencies: dep_sendfile)
        deps += [ dep_sendfile ]
    endif
endif

#
# libdlpi is provided on Solaris
#
dep_dlpi = cc.find_library('dlpi', required: false)
if dep_dlpi.found() and cc.has_function('dlpi_open', args: te_cflags,
                                        dependencies: dep_dlpi)
    deps += [ dep_dlpi ]
    h = 'libdlpi.h'
    if cc.has_header(h)
        c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
    endif
endif

#
# Dynamic linker functions are provided by:
#  - -ldl in Linux;
#  - -lc in FreeBSD;
#  - N/A for NetBSD, since automatically included in every dynamically
#    linked program.
#
if not cc.has_function('dlopen', args: te_cflags)
    dep_dl = cc.find_library('dl', required: false)
    if dep_dl.found() and cc.has_function('dlopen', args: te_cflags,
                                          dependencies: dep_dl)
        deps += [ dep_dl ]
    endif
endif

f = 'setns'
if cc.has_function(f, args: te_cflags)
    c_args += [ '-DHAVE_' + f.to_upper() ]
endif

f = 'mkdtemp'
if cc.has_function(f, args: te_cflags)
    c_args += [ '-DHAVE_' + f.to_upper() ]
endif

#
# Asynchronous I/O:
#  - FreeBSD has in -lc;
#  - Linux has in -lnsl -lrt;
#  - Solaris2 (SunOS 5.11) in -lrt;
#  - NetBSD does not have.
#
if not cc.has_function('aio_read', args: te_cflags)
    aio_read_found = false
    dep_rt = cc.find_library('rt', required: false)
    if dep_rt.found()
        if cc.has_function('aio_read', args: te_cflags, dependencies: dep_rt)
            aio_read_found = true
            deps += [ dep_rt ]
        else
            dep_nsl = cc.find_library('nsl', required: false)
            if dep_nsl.found() and
               cc.has_function('aio_read', args: te_cflags,
                               dependencies: [ dep_nsl, dep_rt ] )
                aio_read_found = true
                deps += [ dep_nsl, dep_rt ]
            endif
        endif
    endif
    if not aio_read_found
        warning('Asynchronous I/O not supported')
    endif
endif


if get_option('tr069')
    ta_symtbl_libs += [ 'acse_cwmp' ]
endif

foreach l : ta_non_symtbl_libs + ta_symtbl_libs
    deps += [ get_variable('dep_lib_static_' + l) ]
endforeach

generated_sources = []

if get_option('agent-unix-symtbl')
    # FIXME add check for: nm --format=sysv
    nm = find_program('nm')
    ta_library = static_library('ta', sources,
                                include_directories: includes,
                                c_args: c_args,
                                dependencies: deps)
    symtbl_src = [ ta_library ]
    foreach l : ta_symtbl_libs
        symtbl_src += [ get_variable('te_lib_' + l) ]
    endforeach
    c_args += [ '-DENABLE_GENERATED_SYMTBL' ]
    symtbl = custom_target('gen-symtbl', output: 'symtbl.c', capture: true,
                           input: [ symtbl_src ],
                           command: [ te_generate_symtbl, nm, '@INPUT@' ])
    generated_sources += [ symtbl ]
endif

rcf_consistency_checks = custom_target('gen-rcf-consistency-check',
    output: 'rcf_consistency_checks.c',
    capture: true, build_always: true,
    command: [ te_rcf_consistency_checks, '--generate-c' ])
generated_sources += [ rcf_consistency_checks ]

executable('ta', sources, generated_sources,
           install: true, install_dir: tadir,
           include_directories: includes,
           c_args: c_args,
           link_args: link_args,
           dependencies: deps)

#
# Build ta_rcf_listener
#
rcf_listener_sources = files(
    'ta_rcf_listener.c',
)

# Just use the ta dependencies: this seems to be enough
executable('ta_rcf_listener', rcf_listener_sources,
           install: true, install_dir: tadir,
           include_directories: includes,
           c_args: c_args,
           link_args: link_args,
           dependencies: deps)
