
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LFLAGS = --header-file=$*.h -o $(LEX_OUTPUT_ROOT).c
AM_YFLAGS = -d
AM_LDFLAGS = -L. @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

noinst_HEADERS = trc_log.h trc_db.h trc_xml.h trc_tag.h


dist_bin_SCRIPTS = te_trc.sh

bin_PROGRAMS = te_trc te_trc_diff


noinst_LIBRARIES = liblogic_expr.a

liblogic_expr_a_SOURCES = logic_expr.c logic_expr_gram.y logic_expr_lex.l


te_trc_SOURCES = main.c trc_tag.c db_io.c report_html.c log_parse.c

te_trc_LDADD = -llogic_expr -lpopt -lfl @XML2_LIBS@ @TE_LDADD@

te_trc_DEPENDENCIES = @TE_DEPENDENCIES@


te_trc_diff_SOURCES = trc_diff.c trc_tag.c db_io.c diff_report.c

te_trc_diff_LDADD = -llogic_expr -lpopt -lfl @XML2_LIBS@ @TE_LDADD@

te_trc_diff_DEPENDENCIES = @TE_DEPENDENCIES@


CLEANFILES = logic_expr_gram.h logic_expr_gram.c \
             logic_expr_lex.h logic_expr_lex.c
BUILT_SOURCES = logic_expr_gram.h logic_expr_lex.h
# logic_expr_gram.h is included in dist automatically
EXTRA_DIST = logic_expr_lex.h

logic_expr_lex.h: logic_expr_lex.c
