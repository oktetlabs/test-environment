# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

apps = [
    'builder',
    'logger',
    'rcf',
    'configurator',
    'tester',
]

foreach app : get_option('engine-apps').split()
    if not apps.contains(app)
        error('Unknown engine application ' + app + ' to build')
    endif
    # builder should be processed in both cases: engine and platform
    if get_option('engine') or app == 'builder'
        subdir(app)
    endif
endforeach
