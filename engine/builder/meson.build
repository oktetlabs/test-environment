# Copyright (C) 2020-2022 OKTET Labs. All rights reserved.

engine_scripts = [
    'te_build_suite',
    'te_functions',
    'te_ta_ssh_helper',
    'te_tce_process',
    'te_tests_info.awk',
    'te_tests_info.sh',
]

platform_scripts = [
    'te_clean_gcov.sh',
    'te_generate_symtbl',
    'te_generate_symtbl.sh',
    'te_meson_m32_exports',
    'te_process_gcov.sh',
]

if get_option('engine')
    install_data(engine_scripts, platform_scripts, install_dir: 'bin')
else
    install_data(platform_scripts, install_dir: 'bin')
endif
