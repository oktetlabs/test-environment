# Copyright (C) 2018 OKTET Labs. All rights reserved.

headers += files(
    'te_rpc_aio.h',
    'te_rpc_defs.h',
    'te_rpc_dirent.h',
    'te_rpc_dlfcn.h',
    'te_rpc_errno.h',
    'te_rpc_fcntl.h',
    'te_rpc_linux_net_tstamp.h',
    'te_rpc_net_if.h',
    'te_rpc_net_if_arp.h',
    'te_rpc_netdb.h',
    'te_rpc_pthread.h',
    'te_rpc_signal.h',
    'te_rpc_sys_epoll.h',
    'te_rpc_sys_poll.h',
    'te_rpc_sys_resource.h',
    'te_rpc_sys_socket.h',
    'te_rpc_sys_stat.h',
    'te_rpc_sys_systeminfo.h',
    'te_rpc_sys_time.h',
    'te_rpc_sys_wait.h',
    'te_rpc_sys_mman.h',
    'te_rpc_sysconf.h',
    'te_rpc_types.h',
    'te_rpc_wsa.h',
)

sources += files('defs.c')

rpch_sources = [
    'aio.c.rpch',
    'dirent.c.rpch',
    'dlfcn.c.rpch',
    'fcntl.c.rpch',
    'linux_net_tstamp.c.rpch',
    'net_if.c.rpch',
    'net_if_arp.c.rpch',
    'netdb.c.rpch',
    'pthread.c.rpch',
    'signal.c.rpch',
    'sys_epoll.c.rpch',
    'sys_poll.c.rpch',
    'sys_resource.c.rpch',
    'sys_socket.c.rpch',
    'sys_stat.c.rpch',
    'sys_systeminfo.c.rpch',
    'sys_wait.c.rpch',
    'sys_mman.c.rpch',
    'sysconf_name.c.rpch',
    'wsa.c.rpch',
]

foreach rpch : rpch_sources
    sources += gen_c_rpch_c.process(rpch)
endforeach

te_libs += [
    'tools',
    'rpcxdr',
]

#
# Check headers and define appropriate macro
#
check_headers = [
    'linux/errqueue.h',
# Solaris-style sysinfo()
    'sys/systeminfo.h',
    'sys/epoll.h',
    'dirent.h',
]

foreach h : check_headers
    if cc.has_header(h)
        c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
    endif
endforeach

t = 'struct sock_extended_err'
if cc.has_header_symbol('linux/errqueue.h', t)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif

t = 'struct in_pktinfo'
if cc.has_header_symbol('netinet/in.h', t)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif

t = 'struct in6_pktinfo'
if cc.has_header_symbol('netinet/in.h', t)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif

t = 'struct in6_pktinfo'
if cc.has_header_symbol('netinet/in.h', t)
    c_args += [ '-DHAVE_' + t.to_upper().underscorify() ]
endif

t = 'struct dirent'
m = 'd_type'
check_decls_headers = '''
#include <dirent.h>
'''
if cc.has_member(t, m, prefix: check_decls_headers)
    c_args += [ '-DHAVE_' + (t + '_' + m).to_upper().underscorify() ]
endif

t = 'struct sigaction'
m = 'sa_restorer'
check_decls_headers = '''
#include <signal.h>
'''
if cc.has_member(t, m, prefix: check_decls_headers)
    c_args += [ '-DHAVE_' + (t + '_' + m).to_upper().underscorify() ]
endif

t = 'struct sockaddr_in6'
m = '__sin6_src_id'
check_decls_headers = '''
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
'''
if cc.has_member(t, m, prefix: check_decls_headers)
    c_args += [ '-DHAVE_' + (t + '_' + m).to_upper().underscorify() ]
endif

#
# Check enum members from the following list and define
# appropiate macro (if found).
#
check_decls_list = [
    'TCP_ESTABLISHED',
    'TCP_SYN_SENT',
    'TCP_SYN_RECV',
    'TCP_FIN_WAIT1',
    'TCP_FIN_WAIT2',
    'TCP_TIME_WAIT',
    'TCP_CLOSE',
    'TCP_CLOSE_WAIT',
    'TCP_LAST_ACK',
    'TCP_LISTEN',
    'TCP_CLOSING',
    'TCP_CA_Open',
    'TCP_CA_Disorder',
    'TCP_CA_CWR',
    'TCP_CA_Recovery',
    'TCP_CA_Loss',
]

check_decls_headers = '''
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
'''

foreach d : check_decls_list
    if cc.has_type(d, prefix: check_decls_headers)
        c_args += '-D' + d.to_upper() + '=' + d
    endif
endforeach

