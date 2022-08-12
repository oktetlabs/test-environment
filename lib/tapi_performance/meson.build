# Copyright (C) 2019-2022 OKTET Labs. All rights reserved.

headers += files(
    'tapi_performance.h',
)
sources += files(
    'iperf.c',
    'iperf3.c',
    'performance_internal.c',
    'tapi_performance.c',
)
te_libs += [
    'tapi',
    'tapi_rpc',
    'tapi_job',
]
deps += [
    dependency('jansson'),
]
