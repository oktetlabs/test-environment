
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include \
              -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS = internal.h reqs.h run_path.h tester_flags.h test_params.h \
                 iters.h


bin_PROGRAMS = te_tester


te_tester_SOURCES = tester.c parse.c run.c run_path.c reqs.c term_out.c iters.c

te_tester_LDADD = @TE_LDADD@ -lbsapi -lconfapi -lconf_oid -llogger_ten \
                  -lipc -lpopt -lpthread
te_tester_DEPENDENCIES = @TE_DEPENDENCIES@ \
                         $(DESTDIR)/$(prefix)/lib/libbsapi.a \
                         $(DESTDIR)/$(prefix)/lib/libconfapi.a \
                         $(DESTDIR)/$(prefix)/lib/libconf_oid.a \
                         $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                         $(DESTDIR)/$(prefix)/lib/libipc.a


dist_bin_SCRIPTS = te_tester.sh
