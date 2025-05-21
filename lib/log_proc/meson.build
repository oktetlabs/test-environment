# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.

headers += files('log_msg_view.h', 'log_msg_filter.h', 'log_flow_filters.h',
                 'log_filters_xml.h', 'log_filters_yaml.h')
sources += files('log_msg_view.c', 'log_msg_filter.c', 'log_flow_filters.c',
                 'log_filters_xml.c', 'log_filters_yaml.c')
te_libs += [ 'tools' ]

dep_pcre = dependency('libpcre2-8', required: false)
required_deps += 'libpcre2-8'
if not dep_pcre.found()
    missed_deps += 'libpcre2-8'
endif

dep_yaml = dependency('yaml-0.1', required: false)
required_deps += 'yaml-0.1'
if not dep_yaml.found()
    missed_deps += 'yaml-0.1'
endif

deps += [dep_pcre, dep_libxml2, dep_yaml]
