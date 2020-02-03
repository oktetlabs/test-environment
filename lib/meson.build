# Copyright (C) 2018 OKTET Labs. All rights reserved.

tad_protocols = [
    'arp',
    'atm',
    'bridge',
    'cli',
    'dhcp',
    'eth',
    'flow',
    'forw',
    'geneve',
    'gre',
    'igmp',
    'ipstack',
    'iscsi',
    'pcap',
    'ppp',
    'rte_mbuf',
    'snmp',
    'socket',
    'vxlan',
]

#
# Lists of libraries in build/dependency order
#

# Common libraries
common_libs = [
    'logger_core',
    'tools',
    'conf_oid',
    'asn',
    'ndn',
    'rpcxdr',
    'rpc_dpdk',
    'rpc_types',
]

# Test Engine libraries
engine_libs = [
    'bsapi',
    'logic_expr',
    'ipc',
    'loggerten',
    'comm_net_engine',
    'rcfapi',
    'rcfunix',
    'confapi',
    'rcfrpc',
    'tapi',
    'tapi_rpc',
    'tapi_bpf',
    'tapi_env',
    'tapi_fio',
    'tapi_job',
    'tapi_serial',
    'tapi_nvme',
    'tapi_tad',
    'tapi_tcp_states',
    'tapi_packetdrill',
    'tapi_performance',
    'tapi_tool',
    'tce',
    'trc',
    'rpcc_dpdk',
    'tapi_dpdk',
    'tapi_gtest',
    'tapi_upnp',
]

# Test Agent libraries
agent_libs = [
    'comm_net_agent',
    'loggerta',
    'netconf',
    'rpctransport',
    'agentlib',
    'rpcserver',
    'rcfpch',
    'tad',
    'rpcs_dpdk',
    'rpcs_job',
    'rpcs_serial',
    'upnp_cp',
]

all_libs = common_libs + engine_libs + agent_libs

libs = get_option('libs').split()

# Process list of libraries to build in order to:
#  - add external libraries to all_libs
foreach l : libs
    # Add external library at the end of the list, no TE libraries can
    # depend on it
    if not all_libs.contains(l)
        all_libs += [ l ]
    endif
endforeach

# Static version of common libraries is required for agents only
build_agents = get_option('agents').split().length() != 0

# Iterate over all libraries in order to follow build order
foreach l : all_libs
    if libs.contains(l)
        if engine_libs.contains(l)
            build_lib_shared = true
            build_lib_static = false
        elif common_libs.contains(l)
            build_lib_shared = true
            build_lib_static = build_agents
        elif agent_libs.contains(l)
            build_lib_shared = false
            build_lib_static = true
        else
            # External libraries are agent side only now
            build_lib_shared = false
            build_lib_static = true
        endif
        if common_libs.contains(l) or engine_libs.contains(l)
            install_lib = install_dev
        else
            install_lib = get_option('install-dev')
        endif
        # Library name to build, may be overridden in subdir
        libname = l
        # List of headers to install
        headers = []
        # List of sources to build library
        sources = []
        # List of include directories required to build
        includes = [ te_include ]
        # C compiler arguments to be used
        c_args = []
        # List of dependencies to add include directories and libraries
        deps = []
        # List of dependencies in TE library name format
        te_libs = []
        # logger_core is required dependency for everything else
        if l != 'logger_core'
            te_libs += [ 'logger_core' ]
        endif

        subdir(l)

        # If there are headers to be installed, do it
        if install_lib and headers.length() != 0
            install_headers(headers)
        endif
        # If there are sources to be compiled as library, do it
        if sources.length() != 0
            # Automatically add the library itself to search for headers
            includes += include_directories(l)
            if build_lib_static
                deps_static = deps
                foreach te_lib : te_libs
                    deps_static += [ get_variable('dep_lib_static_' + te_lib) ]
                endforeach
                lib = static_library(libname, sources, install: install_lib,
                                     include_directories: includes,
                                     implicit_include_directories: false,
                                     c_args: c_args, dependencies: deps_static)
                dep = declare_dependency(link_with: lib,
                                         include_directories: includes,
                                         dependencies: deps_static)
                set_variable('dep_lib_static_' + libname, dep)
            endif
            if build_lib_shared
                deps_shared = deps
                foreach te_lib : te_libs
                    deps_shared += [ get_variable('dep_lib_' + te_lib) ]
                endforeach
                lib = shared_library(libname, sources, install: install_lib,
                                     include_directories: includes,
                                     implicit_include_directories: false,
                                     c_args: c_args, dependencies: deps_shared)
                dep = declare_dependency(link_with: lib,
                                         include_directories: includes,
                                         dependencies: deps_shared)
                set_variable('dep_lib_' + libname, dep)
            endif
        endif
    endif
endforeach
