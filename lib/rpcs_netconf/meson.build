# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2024 OKTET Labs Ltd. All rights reserved.

sources += files(
    'netconf.c',
)

te_libs += [
    'rpcserver',
]

dep_libnetconf = dependency('libnetconf2', required: true)
dep_libyang = dependency('libyang', required: true)
deps += [ dep_libnetconf, dep_libyang ]
