# Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.

name = get_option('agent-powerctl-name')
ta_platform = get_option('agent-powerctl-platform')
tadir = join_paths(get_option('agentsdir'), name)

includes = []
link_args = []

c_args += [ '-DTE_AGT_PLATFORM="' + ta_platform + '"' ]

deps = [ dep_threads, cc.find_library('m') ]

sources = files(
    'main.c',
    'ta_shell_cold_reboot.c',
)

ta_symtbl_libs = [
    'tools',
    'rcfpch',
    'agentlib',
    'tad',
]

ta_non_symtbl_libs = [
    'conf_oid',
    'loggerta',
    'logger_file',
    'logger_core',
]

foreach extra_lib : get_option('agent-powerctl-libs')
    if not ta_symtbl_libs.contains(extra_lib)
        ta_symtbl_libs += [ extra_lib ]
    endif
endforeach


netsnmp_deps = dependency('netsnmp', required : false)
if netsnmp_deps.found()
    deps += netsnmp_deps
    check_headers = [
        'net-snmp/session_api.h',
        'net-snmp/net-snmp-config.h',
        'net-snmp/library/snmp_transport.h',
    ]

    foreach h : check_headers
        if cc.has_header(h)
            c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
        endif
    endforeach


    check_funcs = [
        'snmp_pdu_type'
    ]

    foreach h : check_funcs
        if cc.has_function(h, dependencies: deps)
            c_args += [ '-DHAVE_' + h.to_upper().underscorify() ]
        endif
    endforeach

    sources += files(
        'ta_snmp_cold_reboot.c',
        'ta_snmp.c'
    )

endif


foreach l : ta_non_symtbl_libs + ta_symtbl_libs
    deps += [ get_variable('dep_lib_static_' + l) ]
endforeach

generated_sources = []

if get_option('agent-powerctl-symtbl')
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

executable('ta', sources, generated_sources,
           install: true, install_dir: tadir,
           include_directories: includes,
           c_args: c_args,
           link_args: link_args,
           dependencies: deps)