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
    'tools',
    'conf_oid',
    'asn',
    'ndn',
    'rpcxdr',
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
    'tapi_rpc',
    'tapi',
    'tapi_env',
    'tapi_tad',
]

# Test Agent libraries
agent_libs = [
    'comm_net_agent',
    'loggerta',
    'netconf',
    'rpctransport',
    'agentlib',
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

# Iterate over all libraries in order to follow build order
foreach l : all_libs
    if libs.contains(l)
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

        subdir(l)

        # If there are headers to be installed, do it
        if install_lib and headers.length() != 0
            install_headers(headers)
        endif
        # If there are sources to be compiled as library, do it
        if sources.length() != 0
            # Automatically add the library itself to search for headers
            includes += include_directories(l)
            foreach te_lib : te_libs
                deps += [ get_variable('dep_lib_' + te_lib) ]
            endforeach

            lib = library(libname, sources, install: install_lib,
                          include_directories: includes,
                          implicit_include_directories: false,
                          c_args: c_args, dependencies: deps)
            # Populate meson variable with the library build target object
            set_variable('te_lib_' + libname, lib)

            dep = declare_dependency(link_with: lib,
                                     include_directories: includes,
                                     dependencies: deps)
            # Populate meson variable with the library as dependency object
            set_variable('dep_lib_' + libname, dep)
        endif
    endif
endforeach
