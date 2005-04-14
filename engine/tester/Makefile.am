
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS = @XML2_CFLAGS@
AM_LFLAGS = --header-file=$*.h -o $(LEX_OUTPUT_ROOT).c
AM_YFLAGS = -d

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS = internal.h reqs.h run_path.h tester_flags.h test_params.h \
                 iters.h


bin_PROGRAMS = te_tester 


te_tester_SOURCES = tester.c parse.c run.c run_path.c term_out.c iters.c \
                    reqs.c reqs_expr.y reqs_expr_lex.l

CLEANFILES = reqs_expr.h reqs_expr.c reqs_expr_lex.h reqs_expr_lex.c
BUILT_SOURCES = reqs_expr.h reqs_expr_lex.h
# reqs_expr.h is included in dist automatically
EXTRA_DIST = reqs_expr_lex.h

reqs_expr_lex.h: reqs_expr_lex.c


te_tester_LDADD = @TE_LDADD@ -lbsapi -lconfapi -lconf_oid -llogger_ten \
                  -lipc @XML2_LIBS@ -lpopt -lfl -lpthread
te_tester_DEPENDENCIES = @TE_DEPENDENCIES@ \
                         $(DESTDIR)/$(prefix)/lib/libbsapi.a \
                         $(DESTDIR)/$(prefix)/lib/libconfapi.a \
                         $(DESTDIR)/$(prefix)/lib/libconf_oid.a \
                         $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                         $(DESTDIR)/$(prefix)/lib/libipc.a


dist_bin_SCRIPTS = te_tester.sh
