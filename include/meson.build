# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

te_include = include_directories('')

# By default meson do not use project CFLAGS in compiler checks
# Take CFLAGS into account to detect sizes correctly in the case of -m32
c_args = te_cflags

conf = configuration_data()

conf.set('STDC_HEADERS', 1)

#
# Check headers, start from those which used by others
#
check_headers = [
    'sys/types.h',
    'sys/queue.h',
    'sys/param.h',
    'sys/socket.h',
    'net/if.h',
    'netinet/in.h',
    'aio.h',
    'arpa/inet.h',
    'assert.h',
    'ctype.h',
    'dirent.h',
    'dlfcn.h',
    'errno.h',
    'fcntl.h',
    'fnmatch.h',
    'ftw.h',
    'glob.h',
    'inet/mib2.h',
    'inet/nd.h',
    'inttypes.h',
    'libgen.h',
    'limits.h',
    'linux/if_ether.h',
    'linux/if_packet.h',
    'linux/net_tstamp.h',
    'linux/ptp_clock.h',
    'linux/udp.h',
    'math.h',
    'net/ethernet.h',
    'net/if_arp.h',
    'net/if_dl.h',
    'net/if_ether.h',
    'net/route.h',
    'netdb.h',
    'netinet/ether.h',
    'netinet/icmp6.h',
    'netinet/if_ether.h',
    'netinet/in_systm.h',
    'netinet/in.h',
    'netinet/ip.h',
    'netinet/ip_icmp.h',
    'netinet/udp.h',
    'netinet/tcp.h',
    'netpacket/packet.h',
    'obstack.h',
    'pcap.h',
    'pcre.h',
    'poll.h',
    'popt.h',
    'pthread.h',
    'pwd.h',
    'regex.h',
    'scsi/sg.h',
    'search.h',
    'semaphore.h',
    'setjmp.h',
    'signal.h',
    'stdarg.h',
    'stddef.h',
    'stdint.h',
    'stdio.h',
    'stdlib.h',
    'string.h',
    'strings.h',
    'stropts.h',
    'sys/cdefs.h',
    'sys/dlpi.h',
    'sys/errno.h',
    'sys/ethernet.h',
    'sys/filio.h',
    'sys/ioctl.h',
    'sys/mman.h',
    'sys/mount.h',
    'sys/poll.h',
    'sys/resource.h',
    'sys/select.h',
    'sys/sendfile.h',
    'sys/sockio.h',
    'sys/stat.h',
    'sys/sysctl.h',
    'sys/tihdr.h',
    'sys/time.h',
    'sys/timex.h',
    'sys/uio.h',
    'sys/un.h',
    'sys/utsname.h',
    'sys/wait.h',
    'sys/syscall.h',
    'sys/sysmacros.h',
    'termio.h',
    'time.h',
    'ucd-snmp/snmp_impl.h',
    'unistd.h'
]

check_headers_prefix = '''
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#if HAVE_SYS_PARAM_H
/* NetBSD: sys/sysctl.h */
#include <sys/param.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETINET_IN_H
/* NetBSD: netinet/ip.h, netinet/ip_icmp.h  */
#include <netinet/in.h>
#endif
#if HAVE_NETINET_IN_SYSTM_H
/* NetBSD: netinet/ip.h, netinet/ip_icmp.h */
#include <netinet/in_systm.h>
#endif
#if HAVE_NETINET_IP_H
/* NetBSD: netinet/ip_icmp.h */
#include <netinet/ip.h>
#endif
'''

foreach h : check_headers
    if cc.has_header(h, args: c_args, prefix: check_headers_prefix)
        have = 'HAVE_' + h.to_upper().underscorify()
        conf.set(have, 1)
        c_args += [ '-D' + have ]
    endif
endforeach

#
# Check RPC headers, start from those which used by others
#
check_rpc_headers = [
    'rpc/types.h',
    'rpc/xdr.h',
    'rpc/auth.h',
    'rpc/clnt.h',
    'rpc/pmap_clnt.h',
]

check_rpc_headers_prefix = '''
#if HAVE_RPC_TYPES_H
/* NetBSD: rpc/pmap_clnt.h */
#include <rpc/types.h>
#endif
#if HAVE_RPC_XDR_H
/* NetBSD: rpc/pmap_clnt.h */
#include <rpc/xdr.h>
#endif
#if HAVE_RPC_AUTH_H
/* NetBSD: rpc/clnt.h */
#include <rpc/auth.h>
#endif
#if HAVE_RPC_CLNT_H
/* NetBSD: rpc/pmap_clnt.h */
#include <rpc/clnt.h>
#endif
'''

# tirpc headers lie in a different subfolder. The problem with it is
# that dependencies are deeply broken (mostly due to expectation that
# cc.find_library() returns entirely self-containing dependency, while it only
# contains linker flags) and real header location is never added to target
# include directories.
# dependency() uses pkg-config at the back end and is therefore self-containing.
#
# The line below would produce empty dependencies on systems having no tirpc, so
# it is not a problem
dep_rpcs = dependency('libtirpc', required: false)

# rpc/ headers here require tirpc on some distros, so we add
# the corresponding dependency
foreach h : check_rpc_headers
    if cc.has_header(h, args: c_args, prefix: check_rpc_headers_prefix,
                     dependencies: dep_rpcs)
        have = 'HAVE_' + h.to_upper().underscorify()
        conf.set(have, 1)
        c_args += [ '-D' + have ]
    endif
endforeach

#
# Check if in_port_t data type is defined
#
in_port_t_prefix = '''
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
'''
if cc.has_type('in_port_t', args: c_args, prefix: in_port_t_prefix)
    conf.set('HAVE_IN_PORT_T', 1)
else
    conf.set('in_port_t', 'unsigned short int')
endif

#
# Check for Linux kernel interfaces
#
ethtool_prefix = '''
/* linux/ethtool.h: see te_ethtool.h for comments */
#include <asm/types.h>
#include <sys/types.h>
typedef unsigned long long u64;
typedef __uint32_t u32;
typedef __uint16_t u16;
typedef __uint8_t  u8;
'''
h = 'linux/ethtool.h'
if cc.has_header(h, args: c_args, prefix: ethtool_prefix)
    conf.set('HAVE_' + h.to_upper().underscorify(), 1)
endif

h = 'linux/sockios.h'
if cc.has_header(h, args: c_args)
    conf.set('HAVE_' + h.to_upper().underscorify(), 1)
endif

#
# Check for Net-SNMP headers
#
if cc.has_header('net-snmp/net-snmp-config.h')
    conf.set('HAVE_NET_SNMP_NET_SNMP_CONFIG_H', 1)
    net_snmp_headers = [ 'net-snmp/definitions.h', 'net-snmp/session_api.h' ]
    foreach h : net_snmp_headers
        if cc.has_header(h, args: c_args,
                         prefix: '#include <net-snmp/net-snmp-config.h>')
            conf.set('HAVE_' + h.to_upper().underscorify(), 1)
        endif
    endforeach
endif

#
# Check for 'sa_len'-like members in 'struct sockaddr'-like structures.
#
sockaddr_sa_len_prefix = '''
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
'''
if cc.has_member('struct sockaddr', 'sa_len', args: c_args,
                 prefix: sockaddr_sa_len_prefix)
    conf.set('HAVE_STRUCT_SOCKADDR_SA_LEN', 1)
endif

#
# Generate defines for some data types
#
types = [
    'void *',
    'char',
    'short',
    'int',
    'long',
    'long long',
    'size_t',
    'off_t',
    'socklen_t'
]
sizeof_prefix = '''
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
'''
foreach t : types
    if t == 'void *'
        type_name = 'VOID_P'
    else
        type_name = t.to_upper().underscorify()
    endif
    conf.set('SIZEOF_' + type_name,
             cc.sizeof(t, args: c_args, prefix: sizeof_prefix))
endforeach

#
# Generate defines for TE raw log fields size
#
te_log_types = [
    'te_log_nfl',
    'te_log_version',
    'te_log_ts_sec',
    'te_log_ts_usec',
    'te_log_level',
    'te_log_id',
    'te_log_seqno'
]
foreach t : te_log_types
    conf.set('SIZEOF_' + t.to_upper().underscorify(),
             cc.sizeof(t, args: c_args, prefix: '#include "te_raw_log.h"',
                       include_directories: te_include))
endforeach

#
# Thread-local storage specifier
#
thread_specifiers = [
    '_Thread_local',
    '__thread',
]
foreach s : thread_specifiers
    code = s + ' int x; int main() { return 0; }'
    if cc.compiles(code, name: s + ' implementation', args: c_args)
        conf.set('TE_THREAD_LOCAL', s)
    endif
endforeach

te_config_internal = configure_file(input: 'te_config_internal.h.meson',
                                    output: 'te_config_internal.h',
                                    configuration: conf)
headers = [ te_config_internal ]

headers += [
    'comm_agent.h',
    'cs_common.h',
    'cwmp_soapStub.h',
    'logger_api.h',
    'logger_defs.h',
    'logger_int.h',
    'rcf_common.h',
    'rcf_internal.h',
    'rcf_methods.h',
    'rcf_rpc_defs.h',
    'ta_common.h',
    'tad_common.h',
    'te_bpf_common.h',
    'te_config.h',
    'te_cwmp.h',
    'te_defs.h',
    'te_errno.h',
    'te_ethernet.h',
    'te_ethernet_phy.h',
    'te_ethtool.h',
    'te_param.h',
    'te_power_sw.h',
    'te_printf.h',
    'te_proto.h',
    'te_queue.h',
    'te_raw_log.h',
    'te_sniffers.h',
    'te_stdint.h',
    'te_test_result.h',
    'te_upnp.h',
    'te_win_defs.h',
    'tester_msg.h',
]

if install_dev
    install_headers(headers)
endif
