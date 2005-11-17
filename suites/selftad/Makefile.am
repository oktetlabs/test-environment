
#EXTRA_DIST = mainpage.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

tetest_PROGRAMS = prologue

SUBDIRS = eth ipstack dhcp snmp cli iscsi

MYDEPS = @SELFTAD_TE_DEPS@

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

MY_LDADD=-ltapi_rpc -ltapi_env -ltapi -lrcfrpc -lconfapi -lconf_oid \
         -lrcfapi -lrpcxdr -llogger_ten -lipc -ltools -lexpat -lpthread -lfl

prologue_SOURCES = prologue.c 
prologue_LDADD = $(MY_LDADD)
prologue_DEPENDENCIES = $(MY_DEPS)

