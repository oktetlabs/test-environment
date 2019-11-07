# Copyright (C) 2018 OKTET Labs. All rights reserved.

headers += files('tapi_env.h')
sources += files('tapi_env.c')
sources += lex_gen.process('env_lex.l')
sources += gram_gen.process('env_gram.y')
deps += [
    dep_lib_tapi,
    dep_lib_tapi_rpc,
]
