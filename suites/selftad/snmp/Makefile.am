# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I. -I$(DESTDIR)/$(includedir) \
              -I$(top_srcdir)/lib
AM_CFLAGS = @TE_CFLAGS@ @NET_SNMP_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L. -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = snmp_gets snmp_get_if_table walk trap_any


MYLDADD = -ltapi_tad -lndn -lasn -ltapi -ltapi_rpc -lrpc_types -lrcfrpc \
          -lrcfapi -lrpcxdr -lconfapi -lconf_oid -llogger_ten -lipc \
          -ltools -llogger_core -lpthread

LDADD=$(MYLDADD) @NET_SNMP_LIBS@
