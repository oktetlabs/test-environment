
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS = @XML2_CFLAGS@

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS = internal.h reqs.h run_path.h tester_flags.h test_params.h \
                 iters.h


bin_PROGRAMS = te_tester

te_tester_LDADD = @TE_LDADD@ -lbsapi -lconfapi -lconf_oid -llogger_ten \
                  -lipc @XML2_LIBS@ -lpopt -lpthread
te_tester_DEPENDENCIES = @TE_DEPENDENCIES@ \
                         $(DESTDIR)/$(prefix)/lib/libbsapi.a \
                         $(DESTDIR)/$(prefix)/lib/libconfapi.a \
                         $(DESTDIR)/$(prefix)/lib/libconf_oid.a \
                         $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                         $(DESTDIR)/$(prefix)/lib/libipc.a

te_tester_SOURCES = tester.c parse.c run.c run_path.c reqs.c term_out.c iters.c


bin_SCRIPTS = te_tester.sh


EXTRA_DIST = tester_config.xsd test_package.xsd tester_defs.xsd \
             $(bin_SCRIPTS)
