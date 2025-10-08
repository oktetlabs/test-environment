# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.

headers += files(
    'tapi_net.h',
    'tapi_net_yaml.h',
    'tapi_net_env.h',
)
sources += files(
    'tapi_net.c',
    'tapi_net_yaml.c',
    'tapi_net_env.c',
)

te_libs += [
    'tools',
    'tapi',
    'tapi_env',
]

dep_yaml = dependency('yaml-0.1', required: false)
required_deps += 'yaml-0.1'
if not dep_yaml.found()
    missed_deps += 'yaml-0.1'
endif

deps += [dep_yaml]
