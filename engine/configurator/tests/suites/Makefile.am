
AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS =@TE_CFLAGS@
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) -L./../db -L./../rcf_emul

TESTS = test

check_PROGRAMS = $(TESTS)

noinst_HEADERS = test.h

test_SOURCES = test.c

EXTRA_DIST = config.db test.conf conf_tester.common logger.conf

test_LDADD =  -lrcf_emul -lconfapi -lconf_oid -lpthread

test_DEPENDENCIES = @TE_DEPENDENCIES@ \
                    $(DESTDIR)/$(libdir)/libconfapi.a \
                    $(DESTDIR)/$(libdir)/libconf_oid.a \
                    $(DESTDIR)/$(libdir)/liblogger_ten.a \
                    $(DESTDIR)/$(libdir)/librcfapi.a \
                    $(DESTDIR)/$(libdir)/librpcxdr.a \
                    $(DESTDIR)/$(libdir)/libipcserver.a \
                    $(DESTDIR)/$(libdir)/libipc.a \
                    ./../rcf_emul/.libs/librcf_emul.a \
                    ./../db/.libs/libdb.a
