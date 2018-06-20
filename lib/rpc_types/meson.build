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
    'te_rpc_signal.h',
    'te_rpc_sys_epoll.h',
    'te_rpc_sys_poll.h',
    'te_rpc_sys_resource.h',
    'te_rpc_sys_socket.h',
    'te_rpc_sys_stat.h',
    'te_rpc_sys_systeminfo.h',
    'te_rpc_sys_time.h',
    'te_rpc_sys_wait.h',
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
    'signal.c.rpch',
    'sys_epoll.c.rpch',
    'sys_poll.c.rpch',
    'sys_resource.c.rpch',
    'sys_socket.c.rpch',
    'sys_stat.c.rpch',
    'sys_systeminfo.c.rpch',
    'sys_wait.c.rpch',
    'sysconf_name.c.rpch',
    'wsa.c.rpch',
]

foreach rpch : rpch_sources
    sources += gen_c_rpch_c.process(rpch)
endforeach

deps += [
    dep_lib_tools,
    dep_lib_rpcxdr,
]
