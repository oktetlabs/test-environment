# Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

headers += files('log_msg_view.h', 'log_msg_filter.h', 'log_flow_filters.h',
                 'log_filters_xml.h', 'log_filters_yaml.h')
sources += files('log_msg_view.c', 'log_msg_filter.c', 'log_flow_filters.c',
                 'log_filters_xml.c', 'log_filters_yaml.c')
te_libs += [ 'tools' ]
deps += [dependency('libpcre'), dep_libxml2, dependency('yaml-0.1')]
