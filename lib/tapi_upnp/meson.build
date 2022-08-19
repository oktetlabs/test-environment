# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2019-2022 OKTET Labs Ltd. All rights reserved.

headers += files(
    'tapi_upnp.h',
    'tapi_upnp_content_directory.h',
    'tapi_upnp_cp.h',
    'tapi_upnp_device_info.h',
    'tapi_upnp_resources.h',
    'tapi_upnp_service_info.h',
)
sources += files(
    'tapi_upnp.c',
    'tapi_upnp_content_directory.c',
    'tapi_upnp_cp.c',
    'tapi_upnp_device_info.c',
    'tapi_upnp_resources.c',
    'tapi_upnp_service_info.c',
)

te_libs += [
    'rcfrpc',
    'tapi_rpc',
]

upnp_deps_with_opts = [
    ['glib-2.0'],
    ['gssdp-1.2', 'gssdp-1.0'],
    ['gupnp-1.2', 'gupnp-1.0'],
    ['gupnp-av-1.0'],
    ['jansson'],
    ['libsoup-2.4'],
    ['libxml-2.0'],
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
