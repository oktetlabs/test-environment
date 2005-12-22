
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

#NDS_DIR=${TE_BASE}/storage/nds
#tetest_DATA = ${NDS_DIR}/file-csap.asn ${NDS_DIR}/file-tmpl01.asn


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = rcftest tareboot


rcftest_SOURCES = rcftest.c
rcftest_LDADD = -lrcfapi -llogger_ten -lipc -ltools -lpthread
rcftest_DEPENDENCIES = ${TE_INSTALL}/@host@/lib/librcfapi.a \
                       ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
                       ${TE_INSTALL}/@host@/lib/libipc.a \
                       ${TE_INSTALL}/@host@/lib/libtools.a

tareboot_SOURCES = tareboot.c
tareboot_LDADD = -lrcfapi -llogger_ten -lipc -ltools -lpthread
tareboot_DEPENDENCIES = ${TE_INSTALL}/@host@/lib/librcfapi.a \
                        ${TE_INSTALL}/@host@/lib/liblogger_ten.a \
                        ${TE_INSTALL}/@host@/lib/libipc.a \
                        ${TE_INSTALL}/@host@/lib/libtools.a
