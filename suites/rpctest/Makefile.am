

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

mytestdir = $(bindir)/rpctest

mytest_PROGRAMS = rpctest

rpctest_SOURCES=rpctest.c 

rpctest_LDADD= -ltapi_rpcsock -lrcfrpc -lconfapi -lconf_oid -lrcfapi  -llogger_ten -lipc -lpthread

rpctest_DEPENDENCIES=${TE_INSTALL}/i686-pc-linux-gnu/lib/librcfapi.a \
                     ${TE_INSTALL}/i686-pc-linux-gnu/lib/librcfrpc.a \
                     ${TE_INSTALL}/i686-pc-linux-gnu/lib/libconfapi.a \
                     ${TE_INSTALL}/i686-pc-linux-gnu/lib/libtapi_rpcsock.a \
                     ${TE_INSTALL}/i686-pc-linux-gnu/lib/liblogger_ten.a

