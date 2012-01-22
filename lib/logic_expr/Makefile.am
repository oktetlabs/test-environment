
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ 
AM_LDFLAGS = @TE_LDFLAGS@ 
AM_LFLAGS = --header-file=$(ABS_BUILDDIR)/$*.h -o $(LEX_OUTPUT_ROOT).c
AM_YFLAGS = -d


include_HEADERS = \
	logic_expr.h

lib_LTLIBRARIES = liblogic_expr.la

liblogic_expr_la_SOURCES = \
	logic_expr.c \
	logic_expr_gram.y \
	logic_expr_lex.l


CLEANFILES = logic_expr_gram.h logic_expr_gram.c \
             logic_expr_lex.h logic_expr_lex.c
BUILT_SOURCES = logic_expr_gram.h logic_expr_lex.h
EXTRA_DIST = logic_expr_gram.h logic_expr_lex.h

logic_expr_lex.h: logic_expr_lex.c

install-exec-hook:
	touch -c -r liblogic_expr.la $(DESTDIR)/$(libdir)/liblogic_expr.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
