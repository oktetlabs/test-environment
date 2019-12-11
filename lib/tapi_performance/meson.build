# Copyright (C) 2019 OKTET Labs. All rights reserved.

headers += files(
    'tapi_performance.h',
)
sources += files(
    'iperf.c',
    'iperf3.c',
    'performance_internal.c',
    'tapi_performance.c',
)
deps += [
    dep_lib_tapi,
    dep_lib_tapi_rpc,
    dep_lib_tapi_job,
    dependency('jansson'),
]
