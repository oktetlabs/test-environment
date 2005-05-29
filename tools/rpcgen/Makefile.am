
CFLAGS=-D_RPC_THREAD_SAFE_

AM_CPPFLAGS = @TE_CFLAGS@

noinst_HEADERS=proto.h rpc_util.h rpc_parse.h rpc_scan.h

bin_PROGRAMS = rpcgen

dist_data_DATA = compound_header.in compound_trailer.in

rpcgen_SOURCES = rpc_clntout.c rpc_cout.c rpc_hout.c rpc_main.c rpc_parse.c \
                 rpc_scan.c rpc_svcout.c rpc_util.c rpc_sample.c rpc_tblout.c

