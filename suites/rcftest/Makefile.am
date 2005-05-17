
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

#NDS_DIR=${TE_BASE}/storage/nds
#tetest_DATA = ${NDS_DIR}/file-csap.asn ${NDS_DIR}/file-tmpl01.asn


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = rcftest tareboot


rcftest_SOURCES = rcftest.c
rcftest_LDADD = -lrcfapi -llogger_ten -lipc -lpthread
rcftest_DEPENDENCIES = ${TE_INSTALL}/@host@/lib/librcfapi.a \
                       ${TE_INSTALL}/@host@/lib/librpcxdr.a \
                       ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
                       ${TE_INSTALL}/@host@/lib/libipc.a

tareboot_SOURCES = tareboot.c
tareboot_LDADD = -lrcfapi -lrpcxdr -llogger_ten -lipc -lpthread
tareboot_DEPENDENCIES = ${TE_INSTALL}/@host@/lib/librcfapi.a \
                       ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
                       ${TE_INSTALL}/@host@/lib/libipc.a
