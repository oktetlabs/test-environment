# Copyright (C) 2019 OKTET Labs. All rights reserved.

headers += files(
    'conf_upnp_cp.h',
    'te_upnp_cp.h',
)

sources += files(
    'conf_upnp_cp.c',
    'tarpc_upnp_cp.c',
    'te_upnp_cp.c',
)

te_libs += [
    'tools',
    'rpcxdrta',
    'rcfpch',
    'rpcserver',
]

upnp_deps_with_opts = [
    ['gobject-2.0'],
    ['gssdp-1.2', 'gssdp-1.0'],
    ['gupnp-1.2', 'gupnp-1.0'],
    ['jansson'],
]

foreach dep_with_opts : upnp_deps_with_opts
    foreach dep_name : dep_with_opts
        dep = dependency(dep_name, required : false)
        if dep.found()
            deps += dep
            break
        endif
    endforeach
    if not dep.found()
        if dep_with_opts.length() > 1
            error('Neither ' + ' nor '.join(dep_with_opts) + ' found')
        else
            error('No ' + ''.join(dep_with_opts) + ' found')
        endif
    endif
endforeach
