# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2012-2022 OKTET Labs Ltd. All rights reserved.

#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

tetest_DATA = tests-info.xml

tests-info.xml: $(srcdir)/*.c
	@TE_PATH@/te_tests_info.sh $(srcdir) > tests-info.xml

CLEANFILES = tests-info.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD=-ltapi -ltapi_rpc -lrpc_types -ltapi_env -ltapi -lasn -lndn -lrcfrpc \
         -lconfapi -lconf_oid -lrcfapi -lrpcxdr -llogger_ten -lipc \
         -ltools -llogger_core -lexpat -lpthread -lm

tetest_PROGRAMS = clientserver

clientserver_SOURCES = clientserver.c
clientserver_LDADD = $(MY_LDADD)
clientserver_DEPENDENCIES = \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/librcfapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/liblogger_core.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/liblogger_ten.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libipc.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libtapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconfapi.a \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libconf_types.a


