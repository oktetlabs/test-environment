# Copyright (C) 2018-2022 OKTET Labs. All rights reserved.

rgt_idx_tools = [
    'make',
    'apply',
    'sort-mem',
    'fake',
    'sort-vrfy'
]
foreach rgt_idx_tool: rgt_idx_tools
    executable(
        'rgt-idx-' + rgt_idx_tool,
        rgt_idx_tool + '.c',
        include_directories: inc,
        install: true,
    )
endforeach
