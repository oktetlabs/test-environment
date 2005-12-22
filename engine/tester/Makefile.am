
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) 
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LFLAGS = --header-file=$*.h -o $(LEX_OUTPUT_ROOT).c
AM_YFLAGS = -d

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) 


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
                  -lipc -ltools @XML2_LIBS@ -lpopt -lfl -lpthread 
te_tester_DEPENDENCIES = @TE_DEPENDENCIES@ \
                         $(DESTDIR)/$(libdir)/libbsapi.a \
                         $(DESTDIR)/$(libdir)/libconfapi.a \
                         $(DESTDIR)/$(libdir)/libconf_oid.a \
                         $(DESTDIR)/$(libdir)/liblogger_ten.a \
                         $(DESTDIR)/$(libdir)/libipc.a \
                         $(DESTDIR)/$(libdir)/libtools.a
