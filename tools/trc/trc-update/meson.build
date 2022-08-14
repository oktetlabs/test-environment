# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

te_trc_update = executable(
    'te-trc-update',
    'main.c',
    dependencies: [dep_common, dep_perl],
    link_with: libtrc_report,
    c_args: c_args,
    install: true,
)
