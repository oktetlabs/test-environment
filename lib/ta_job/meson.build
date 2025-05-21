# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved.

headers += files(
    'ta_job.h',
)

sources += files(
    'ta_job.c',
)

te_libs += [
    'agentlib',
    'rpc_types',
    'tools',
]

dep_pcre = dependency('libpcre2-8', required: false)
required_deps += 'libpcre2-8'
if not dep_pcre.found()
    missed_deps += 'libpcre2-8'
endif

deps += [
    dep_pcre,
]
