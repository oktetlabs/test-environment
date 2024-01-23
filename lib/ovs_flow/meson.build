# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.

lib_ovs_flow = static_library('ovs_flow', 'ovs_flow_rule.c',
                              install: install_lib,
                              include_directories: includes,
                              dependencies: [dep_lib_logger_core, dep_lib_tools])
dep_lib_ovs_flow = declare_dependency(link_with: lib_ovs_flow,
                                      dependencies: [dep_lib_logger_core, dep_lib_tools],
                                      include_directories: includes)
dep_lib_static_ovs_flow = dep_lib_ovs_flow

headers += files(
    'ovs_flow_rule.h',
)
sources += files(
    'ovs_flow_rule.c',
)
te_libs += [
    'tools',
]
