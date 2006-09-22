
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@

noinst_HEADERS = trc_diff.h trc_html.h

noinst_LIBRARIES = libtrc_diff.a

libtrc_diff_a_SOURCES = diff_tags.c diff.c diff_html.c html.c
