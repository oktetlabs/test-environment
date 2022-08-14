# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

headers += files('tapi_fio.h')
sources += files(
    'tapi_fio.c',
    'fio.c',
    'fio_internal.c',
)
te_libs += [
    'tapi',
    'tapi_rpc',
    'tapi_job',
]
deps += [
    dependency('jansson'),
]
