
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) 
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LFLAGS = --header-file=$*.h -o $(LEX_OUTPUT_ROOT).c
AM_YFLAGS = -d

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) 


noinst_HEADERS = \
	type_lib.h \
	test_path.h \
	tester_build.h \
	tester_conf.h \
	tester_defs.h \
	tester_flags.h \
	tester_reqs.h \
	tester_run.h \
	tester_term.h


bin_PROGRAMS = te_tester 


te_tester_SOURCES = \
	build.c \
	enumerate.c \
	term_out.c \
	type_lib.c \
	config_parse.c \
	config_walk.c \
	config_prepare.c \
	reqs.c \
	reqs_expr_lex.l \
	reqs_expr_gram.y \
	test_path_lex.l \
	test_path_gram.y \
	test_path.c \
	mix.c \
	run.c \
	scenario.c \
	verdicts.c \
	tester.c

CLEANFILES = \
	reqs_expr_lex.h \
	reqs_expr_lex.c \
	reqs_expr_gram.h \
	reqs_expr_gram.c \
	test_path_lex.h \
	test_path_lex.c \
	test_path_gram.h \
	test_path_gram.c

BUILT_SOURCES = \
	reqs_expr_lex.h \
	reqs_expr_gram.h \
	test_path_lex.h \
	test_path_gram.h

# reqs_expr_gram.h and test_path_gram.h are included in dist automatically
EXTRA_DIST = reqs_expr_lex.h test_path_lex.h

.l.c:
	$(SHELL) $(YLWRAP) $< $(LEX_OUTPUT_ROOT).c $@ $*.h $*.h -- $(LEXCOMPILE)

reqs_expr_lex.h: reqs_expr_lex.c

test_path_lex.h: test_path_lex.c


te_tester_LDADD = @TE_LDADD@ -lbsapi -lconfapi -lconf_oid -llogger_ten \
                  -lipc -lipcserver -ltools \
                  @XML2_LIBS@ -lpopt -lfl -lpthread 

te_tester_DEPENDENCIES = @TE_DEPENDENCIES@ \
	$(DESTDIR)/$(libdir)/libbsapi.a \
	$(DESTDIR)/$(libdir)/libconfapi.a \
	$(DESTDIR)/$(libdir)/libconf_oid.a \
	$(DESTDIR)/$(libdir)/liblogger_ten.a \
	$(DESTDIR)/$(libdir)/libipc.a \
	$(DESTDIR)/$(libdir)/libipcserver.a \
	$(DESTDIR)/$(libdir)/libtools.a
