# Copyright (C) 2019 OKTET Labs. All rights reserved.

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

deps += [
    dep_lib_rcfrpc,
    dep_lib_tapi_rpc,
    dep_libxml2,
    dependency('glib-2.0'),
    dependency('gssdp-1.0'),
    dependency('gupnp-1.0'),
    dependency('gupnp-av-1.0'),
    dependency('jansson'),
    dependency('libsoup-2.4'),
    dependency('libxml-2.0'),
]
