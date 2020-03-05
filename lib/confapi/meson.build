# Copyright (C) 2018 OKTET Labs. All rights reserved.

deps += [ dep_lib_conf_oid ]

te_lib_conf_types = library('conf_types', 'conf_types.c', install: install_lib,
                            dependencies: deps)
dep_lib_conf_types = declare_dependency(link_with: te_lib_conf_types,
                                       include_directories: includes,
                                       dependencies: deps)

te_lib_conf_ipc = static_library('confipc', files('conf_ipc.c'), install: install_lib,
                                 include_directories: includes)
dep_lib_static_conf_ipc = declare_dependency(link_with: te_lib_conf_ipc,
                                             include_directories: includes)

headers += files('conf_api.h', 'conf_types.h', 'conf_messages.h')
sources += files('conf_api.c', 'conf_types.c')
deps += [
    dep_lib_ipc,
    dep_lib_rcfapi,
    dep_lib_static_conf_ipc,
]
