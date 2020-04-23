# Copyright (C) 2018-2019 OKTET Labs. All rights reserved.

is_multi_build_library = true

req_protocols = get_variable('opt-tad-protocols'.underscorify()).split()

if req_protocols.length() == 0
    c_args += [ '-DTAD_DUMMY' ]
    sources += files('tad_ch.c')
    # FIXME subdir_done() is a new feature of meson 0.47 and it is
    # exactly what we need here. Right now there is no usecases for
    # TAD dummy, if we really have, we should require the meson
    # version or do in a different way
    #subdir_done()
endif

build_subdirs = []
foreach protocol : tad_protocols
    if req_protocols.contains(protocol)
        build_subdirs += [ protocol ]
    endif
endforeach

if build_subdirs.contains('atm') or build_subdirs.contains('eth')
    build_subdirs += [ 'llc' ]
endif

if build_subdirs.contains('cli')
    if cc.has_header('expect.h')
        c_args += '-DHAVE_EXPECT_H'
    elif cc.has_header('tcl8.4/expect.h')
        c_args += '-DHAVE_TCL8_4_EXPECT_H'
    elif cc.has_header('tcl8.5/expect.h')
        c_args += '-DHAVE_TCL8_5_EXPECT_H'
        deps += [ cc.find_library('tcl8.5') ]
    elif cc.has_header('tcl8.6/expect.h')
        c_args += '-DHAVE_TCL8_6_EXPECT_H'
        deps += [ cc.find_library('tcl8.6') ]
    endif
    # FIXME In practice expect is sufficient for me, but it is tested
    # on a couple of OS versions only. So, keep below deps commented
    # out just as a reminder
    deps += [ cc.find_library('expect') ]
    #deps += [ cc.find_library('util') ]
    #deps += [ cc.find_library('tcl') ]
    #deps += [ cc.find_library('m') ]
endif
if build_subdirs.contains('snmp')
    error('TAD SNMI is not supported by meson build')
endif

headers += files(
    'tad_api.h',
    'tad_csap_inst.h',
    'tad_csap_support.h',
    'tad_pkt.h',
    'tad_poll.h',
    'tad_recv.h',
    'tad_recv_pkt.h',
    'tad_reply.h',
    'tad_send.h',
    'tad_send_recv.h',
    'tad_types.h',
    'tad_utils.h',
)

sources += files(
    'agent_csap.c',
    'csap_id.c',
    'csap_inst.c',
    'csap_spt_db.c',
    'tad_bps.c',
    'tad_ch.c',
    'tad_eth_sap.c',
    'tad_eth_sap_dlpi.c',
    'tad_pkt.c',
    'tad_poll.c',
    'tad_recv.c',
    'tad_recv_pkt.c',
    'tad_reply_rcf.c',
    'tad_send.c',
    'tad_utils.c',
)

if build_subdirs.contains('geneve') or build_subdirs.contains('gre')
    headers += files('tad_overlay_tools.h')
    sources += files('tad_overlay_tools.c')
endif
if build_subdirs.contains('rte_mbuf')
    sources += files('tad_rte_mbuf_sap.c')
endif

foreach build_subdir : build_subdirs
    subdir(build_subdir)
    includes += include_directories(build_subdir)
    c_args += [ '-DWITH_' + build_subdir.to_upper() ]
endforeach

if build_subdirs.contains('eth') or build_subdirs.contains('pcap')
    deps += [ cc.find_library('pcap') ]
endif

if get_variable('opt-tad-cs'.underscorify())
    c_args += [ '-DWITH_CS' ]
endif

if get_variable('opt-tad-packet_mmap_rx_ring'.underscorify())
    if not cc.has_header('poll.h')
        error('Cannot find poll.h header')
    endif
    if not cc.has_header('sys/mman.h')
        error('Cannot find sys/mman.h header')
    endif
    c_args += [ '-DWITH_PACKET_MMAP_RX_RING' ]
endif

te_libs += [
    'loggerta',
    'asn',
    'ndn',
    'rcfpch',
]
