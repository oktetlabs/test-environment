# Copyright (C) 2018 OKTET Labs. All rights reserved.

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
    subdir(app)
endforeach
