# Copyright (C) 2018 OKTET Labs. All rights reserved.

conf_data = configuration_data()

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

#
# Check ethtool-related things.
#

conf_data.set('TA_ETHTOOL_LINK_MODE_UNDEF', '0xffff',
              description: 'Link mode is not defined in ethtool.h')

link_modes = [
    'ETHTOOL_LINK_MODE_10baseT_Half_BIT',
    'ETHTOOL_LINK_MODE_10baseT_Full_BIT',
    'ETHTOOL_LINK_MODE_100baseT_Half_BIT',
    'ETHTOOL_LINK_MODE_100baseT_Full_BIT',
    'ETHTOOL_LINK_MODE_1000baseT_Half_BIT',
    'ETHTOOL_LINK_MODE_1000baseT_Full_BIT',
    'ETHTOOL_LINK_MODE_Autoneg_BIT',
    'ETHTOOL_LINK_MODE_TP_BIT',
    'ETHTOOL_LINK_MODE_AUI_BIT',
    'ETHTOOL_LINK_MODE_MII_BIT',
    'ETHTOOL_LINK_MODE_FIBRE_BIT',
    'ETHTOOL_LINK_MODE_BNC_BIT',
    'ETHTOOL_LINK_MODE_10000baseT_Full_BIT',
    'ETHTOOL_LINK_MODE_Pause_BIT',
    'ETHTOOL_LINK_MODE_Asym_Pause_BIT',
    'ETHTOOL_LINK_MODE_2500baseX_Full_BIT',
    'ETHTOOL_LINK_MODE_Backplane_BIT',
    'ETHTOOL_LINK_MODE_1000baseKX_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseKX4_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseKR_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseR_FEC_BIT',
    'ETHTOOL_LINK_MODE_20000baseMLD2_Full_BIT',
    'ETHTOOL_LINK_MODE_20000baseKR2_Full_BIT',
    'ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT',
    'ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT',
    'ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT',
    'ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT',
    'ETHTOOL_LINK_MODE_56000baseKR4_Full_BIT',
    'ETHTOOL_LINK_MODE_56000baseCR4_Full_BIT',
    'ETHTOOL_LINK_MODE_56000baseSR4_Full_BIT',
    'ETHTOOL_LINK_MODE_56000baseLR4_Full_BIT',
    'ETHTOOL_LINK_MODE_25000baseCR_Full_BIT',
    'ETHTOOL_LINK_MODE_25000baseKR_Full_BIT',
    'ETHTOOL_LINK_MODE_25000baseSR_Full_BIT',
    'ETHTOOL_LINK_MODE_50000baseCR2_Full_BIT',
    'ETHTOOL_LINK_MODE_50000baseKR2_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseSR4_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full_BIT',
    'ETHTOOL_LINK_MODE_50000baseSR2_Full_BIT',
    'ETHTOOL_LINK_MODE_1000baseX_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseCR_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseSR_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseLR_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseLRM_Full_BIT',
    'ETHTOOL_LINK_MODE_10000baseER_Full_BIT',
    'ETHTOOL_LINK_MODE_2500baseT_Full_BIT',
    'ETHTOOL_LINK_MODE_5000baseT_Full_BIT',
    'ETHTOOL_LINK_MODE_FEC_NONE_BIT',
    'ETHTOOL_LINK_MODE_FEC_RS_BIT',
    'ETHTOOL_LINK_MODE_FEC_BASER_BIT',
    'ETHTOOL_LINK_MODE_50000baseKR_Full_BIT',
    'ETHTOOL_LINK_MODE_50000baseSR_Full_BIT',
    'ETHTOOL_LINK_MODE_50000baseCR_Full_BIT',
    'ETHTOOL_LINK_MODE_50000baseLR_ER_FR_Full_BIT',
    'ETHTOOL_LINK_MODE_50000baseDR_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseKR2_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseSR2_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseCR2_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseLR2_ER2_FR2_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseDR2_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseKR4_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseSR4_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseLR4_ER4_FR4_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseDR4_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseCR4_Full_BIT',
    'ETHTOOL_LINK_MODE_100baseT1_Full_BIT',
    'ETHTOOL_LINK_MODE_1000baseT1_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseKR8_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseSR8_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseLR8_ER8_FR8_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseDR8_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseCR8_Full_BIT',
    'ETHTOOL_LINK_MODE_FEC_LLRS_BIT',
    'ETHTOOL_LINK_MODE_100000baseKR_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseSR_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseLR_ER_FR_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseCR_Full_BIT',
    'ETHTOOL_LINK_MODE_100000baseDR_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseKR2_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseSR2_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseLR2_ER2_FR2_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseDR2_Full_BIT',
    'ETHTOOL_LINK_MODE_200000baseCR2_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseKR4_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseSR4_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseLR4_ER4_FR4_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseDR4_Full_BIT',
    'ETHTOOL_LINK_MODE_400000baseCR4_Full_BIT',
    'ETHTOOL_LINK_MODE_100baseFX_Half_BIT',
    'ETHTOOL_LINK_MODE_100baseFX_Full_BIT',
    'SUPPORTED_10baseT_Half',
    'SUPPORTED_10baseT_Full',
    'SUPPORTED_100baseT_Half',
    'SUPPORTED_100baseT_Full',
    'SUPPORTED_1000baseT_Half',
    'SUPPORTED_1000baseT_Full',
    'SUPPORTED_Autoneg',
    'SUPPORTED_TP',
    'SUPPORTED_AUI',
    'SUPPORTED_MII',
    'SUPPORTED_FIBRE',
    'SUPPORTED_BNC',
    'SUPPORTED_10000baseT_Full',
    'SUPPORTED_Pause',
    'SUPPORTED_Asym_Pause',
    'SUPPORTED_2500baseX_Full',
    'SUPPORTED_Backplane',
    'SUPPORTED_1000baseKX_Full',
    'SUPPORTED_10000baseKX4_Full',
    'SUPPORTED_10000baseKR_Full',
    'SUPPORTED_10000baseR_FEC',
    'SUPPORTED_20000baseMLD2_Full',
    'SUPPORTED_20000baseKR2_Full',
    'SUPPORTED_40000baseKR4_Full',
    'SUPPORTED_40000baseCR4_Full',
    'SUPPORTED_40000baseSR4_Full',
    'SUPPORTED_40000baseLR4_Full',
    'SUPPORTED_56000baseKR4_Full',
    'SUPPORTED_56000baseCR4_Full',
    'SUPPORTED_56000baseSR4_Full',
    'SUPPORTED_56000baseLR4_Full',
    'ADVERTISED_10baseT_Half',
    'ADVERTISED_10baseT_Full',
    'ADVERTISED_100baseT_Half',
    'ADVERTISED_100baseT_Full',
    'ADVERTISED_1000baseT_Half',
    'ADVERTISED_1000baseT_Full',
    'ADVERTISED_Autoneg',
    'ADVERTISED_TP',
    'ADVERTISED_AUI',
    'ADVERTISED_MII',
    'ADVERTISED_FIBRE',
    'ADVERTISED_BNC',
    'ADVERTISED_10000baseT_Full',
    'ADVERTISED_Pause',
    'ADVERTISED_Asym_Pause',
    'ADVERTISED_2500baseX_Full',
    'ADVERTISED_Backplane',
    'ADVERTISED_1000baseKX_Full',
    'ADVERTISED_10000baseKX4_Full',
    'ADVERTISED_10000baseKR_Full',
    'ADVERTISED_10000baseR_FEC',
    'ADVERTISED_20000baseMLD2_Full',
    'ADVERTISED_20000baseKR2_Full',
    'ADVERTISED_40000baseKR4_Full',
    'ADVERTISED_40000baseCR4_Full',
    'ADVERTISED_40000baseSR4_Full',
    'ADVERTISED_40000baseLR4_Full',
    'ADVERTISED_56000baseKR4_Full',
    'ADVERTISED_56000baseCR4_Full',
    'ADVERTISED_56000baseSR4_Full',
    'ADVERTISED_56000baseLR4_Full',
]

#
# This code will define any link mode not found in ethtool.h
# to incorrect value 0xffff, so that TA code will be able to
# check whether a given link mode is known without using #ifdef.
# It will make the code much simpler, because macros cannot
# include #ifdef.
#

foreach d : link_modes
    if not cc.has_header_symbol('linux/ethtool.h', d)
        conf_data.set(d, 'TA_ETHTOOL_LINK_MODE_UNDEF')
    endif
endforeach


string_sets_list = [
    'ETH_SS_TEST',
    'ETH_SS_STATS',
    'ETH_SS_PRIV_FLAGS',
    'ETH_SS_NTUPLE_FILTERS',
    'ETH_SS_FEATURES',
    'ETH_SS_RSS_HASH_FUNCS',
    'ETH_SS_TUNABLES',
    'ETH_SS_PHY_STATS',
    'ETH_SS_PHY_TUNABLES',
    'ETH_SS_LINK_MODES',
    'ETH_SS_MSG_CLASSES',
    'ETH_SS_WOL_MODES',
    'ETH_SS_SOF_TIMESTAMPING',
    'ETH_SS_TS_TX_TYPES',
    'ETH_SS_TS_RX_FILTERS',
    'ETH_SS_UDP_TUNNEL_TYPES',
]

foreach d : string_sets_list
    if cc.has_header_symbol('linux/ethtool.h', d)
        conf_data.set('HAVE_DECL_' + d.to_upper(), '1')
    else
        conf_data.set('HAVE_DECL_' + d.to_upper(), '0')
    endif
endforeach

#
# Without input template, meson will generate defines for all
# things from conf_data automatically.
#
configure_file(output: 'config.h',
               configuration: conf_data)

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
