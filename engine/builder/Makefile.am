
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include
AM_CFLAGS=
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

mybindir=$(prefix)/bin

mybin_PROGRAMS=te_stdouterr

te_stdouterr_SOURCES=te_stdouterr.c
te_stdouterr_LDADD=-llogger_ten -lipc @TE_LDADD@
te_stdouterr_DEPENDENCIES=@TE_DEPENDENCIES@ \
                          $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                          $(DESTDIR)/$(prefix)/lib/libipc.a

mybin_SCRIPTS=te_builder_log te_generate_symtbl te_generate_symtbl_cygwin \
              te_build_suite tce_report tce_instrument_sources \
              tce_uninstrument_sources te_configure_package te_cross_build \
              te_build_nut te_build_nuts

EXTRA_DIST=builder.m4 te_builder_log te_generate_symtbl \
           te_generate_symtbl_cygwin te_build_suite tce_report \
           tce_instrument_sources tce_uninstrument_sources \
           te_configure_package te_cross_build \
           te_build_nut te_build_nuts
