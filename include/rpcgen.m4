m4_dnl Copyright (C) 2016-2022 OKTET Labs Ltd. All rights reserved.
m4_changecom(`/*',`*/')m4_dnl
m4_define(`_rpc_counter', `1')m4_dnl
m4_dnl RPC entry point numbers normally start with 1 for each RPC program
m4_define(`program', ``program'm4_define(`_rpc_counter', `1')')m4_dnl
m4_define(`RPC_DEF', ``tarpc_$1_out _$1(tarpc_$1_in *)' = _rpc_counter;m4_define(`_rpc_counter', m4_incr(_rpc_counter))')m4_dnl
