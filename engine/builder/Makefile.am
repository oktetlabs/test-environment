
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

mybindir=$(prefix)/bin

mybin_SCRIPTS=te_builder_log te_generate_symtbl te_generate_symtbl_cygwin \
              te_build_suites tce_report tce_instrument_sources \
              tce_uninstrument_sources

EXTRA_DIST=builder.m4 te_builder_log te_generate_symtbl \
           te_generate_symtbl_cygwin te_build_suites tce_report \
           tce_instrument_sources tce_uninstrument_sources
