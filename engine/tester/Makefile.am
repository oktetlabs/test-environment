
SUBDIRS = . tests

ACLOCAL_AMFLAGS = -I @TE_M4@

if WITH_TRC
TRC_CFLAGS = -DWITH_TRC
TRC_LDADD = -ltrc
TRC_DEPS = $(DESTDIR)/$(libdir)/libtrc.a
else
TRC_CFLAGS=
TRC_LDADD=
TRC_DEPS=
endif

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@ $(TE_CFLAGS_VAR) $(TRC_CFLAGS)
AM_LFLAGS = --header-file=$(ABS_BUILDDIR)/$*.h -o $(LEX_OUTPUT_ROOT).c
AM_YFLAGS = -d -Wno-yacc -Wno-deprecated

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)
MD5_LDADD = -lcrypto

noinst_HEADERS = \
	type_lib.h \
	test_path.h \
	tester_build.h \
	tester_conf.h \
	tester_defs.h \
	tester_flags.h \
	tester_reqs.h \
	tester_result.h \
	tester_run.h \
	tester_term.h \
	tester_interactive.h \
	tester.h \
	tester_serial_thread.h \
	tester_cmd_monitor.h


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
	test_path_lex.l \
	test_path_gram.y \
	test_path.c \
	mix.c \
	run.c \
	scenario.c \
	test_msg.c \
	tester.c \
	tester_interactive.c \
	tester_serial_thread.c \
	tester_cmd_monitor.c

CLEANFILES = \
	test_path_lex.h \
	test_path_lex.c \
	test_path_gram.h \
	test_path_gram.c

BUILT_SOURCES = \
	test_path_lex.h \
	test_path_gram.h

# test_path_gram.h are included in dist automatically
EXTRA_DIST = test_path_lex.h

#.l.c:
#	$(SHELL) $(YLWRAP) $< $(LEX_OUTPUT_ROOT).c $@ $*.h $*.h -- $(LEXCOMPILE)

test_path_lex.h: test_path_lex.c

te_tester_LDADD = @TE_LDADD@ -lbsapi -lconfapi -lconf_oid $(TRC_LDADD) \
                  -llogger_ten -lipc -lipcserver -llogic_expr \
                  -llog_bufs -ltools -llogger_core $(MD5_LDADD) \
                  @XML2_LIBS@ -lpopt -lpthread 

te_tester_DEPENDENCIES = @TE_DEPENDENCIES@ $(TRC_DEPS) \
	$(DESTDIR)/$(libdir)/libbsapi.a \
	$(DESTDIR)/$(libdir)/libconfapi.a \
	$(DESTDIR)/$(libdir)/libconf_oid.a \
	$(DESTDIR)/$(libdir)/liblogger_core.a \
	$(DESTDIR)/$(libdir)/liblogger_ten.a \
	$(DESTDIR)/$(libdir)/libipc.a \
	$(DESTDIR)/$(libdir)/libipcserver.a \
	$(DESTDIR)/$(libdir)/liblogic_expr.a \
	$(DESTDIR)/$(libdir)/libtools.a
