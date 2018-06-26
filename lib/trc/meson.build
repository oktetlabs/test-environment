# Copyright (C) 2018 OKTET Labs. All rights reserved.

if cc.has_type('xmlError', prefix: '#include <libxml/xmlerror.h>',
               dependencies: dep_libxml2)
    c_args += [ '-DHAVE_XMLERROR' ]
endif

headers += files(
    'te_trc.h',
    'trc_db.h',
)
sources += files(
    'compare.c',
    'db.c',
    'db_io.c',
    'db_walker.c'
)
deps += [ dep_lib_tools ]
deps += [ dep_lib_logic_expr ]
deps += [ dep_libxml2 ]