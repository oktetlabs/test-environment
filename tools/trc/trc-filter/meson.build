# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2025 OKTET Labs Ltd. All rights reserved.

te_trc_filter_main = executable(
    'te-trc-filter-main',
    'main.c',
    dependencies: [dep_common],
    c_args: c_args,
    install: true,
)

# Install scripts
install_data(
    [
        'te-trc-filter',
    ],
    install_dir: get_option('bindir'),
)
