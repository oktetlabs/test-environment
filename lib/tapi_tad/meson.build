# Copyright (C) 2018-2022 OKTET Labs. All rights reserved.

headers += files(
    'tapi_ndn.h',
    'tapi_tad.h',
    'tapi_tad_internal.h',
)

build_protocols = []
req_protocols = get_option('tad-protocols').split()

foreach protocol : tad_protocols
    if req_protocols.length() == 0 or req_protocols.contains(protocol)
        build_protocols += [ protocol ]
    endif
endforeach

if build_protocols.contains('iscsi') and not build_protocols.contains('ipstack')
    build_protocols += [ 'ipstack' ]
endif
if build_protocols.contains('ipstack') and not build_protocols.contains('arp')
    build_protocols += [ 'arp' ]
    # It will pull Ethernet automatically below
endif
if (build_protocols.contains('arp') or build_protocols.contains('bridge')) and not build_protocols.contains('eth')
    build_protocols += [ 'eth' ]
endif

if build_protocols.contains('snmp')
    # FIXME Add SNMP-related checks here
endif

sources += files(
    'tapi_ndn.c',
    'tapi_ndn_subst.c',
    'tapi_tad.c'
)

tapi_tad_subdirs = [
    'arp',
    'atm',
    'bridge',
    'cli',
    'dhcp',
    'eth',
    'flow',
    'forw',
    'igmp',
    'ipstack',
    'iscsi',
    'pcap',
    'ppp',
    'rte_mbuf',
    'snmp',
]

foreach build_protocol : build_protocols
    if tapi_tad_subdirs.contains(build_protocol)
        subdir(build_protocol)
    endif
endforeach

includes += include_directories('eth')
includes += include_directories('arp')
includes += include_directories('ipstack')

te_libs += [
    'tools',
    'asn',
    'ndn',
    'tapi',
]

# FIXME It should be possible to build without tapi_env
c_args += [ '-DHAVE_TAPI_ENV_H' ]
te_libs += [ 'tapi_env' ]
