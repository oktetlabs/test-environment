# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.

#EXTRA_DIST = mainpage.dox

SUBDIRS = . eth ipstack dhcp snmp cli iscsi

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

MY_LDADD=-ltapi_rpc -lrpc_types -ltapi_env -ltapi -lasn -lndn -lrcfrpc \
         -lconfapi -lconf_oid -lrcfapi -lrpcxdr -llogger_ten -lipc \
         -ltools -llogger_core -lexpat -lpthread

MYDEPS = @SELFTAD_TE_DEPS@


tetest_PROGRAMS = prologue


prologue_SOURCES = prologue.c
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPS)

