
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


bin_PROGRAMS=te_stdouterr

te_stdouterr_SOURCES=te_stdouterr.c
te_stdouterr_LDADD=-llogger_ten -lipc @TE_LDADD@ -lpthread
te_stdouterr_DEPENDENCIES=@TE_DEPENDENCIES@ \
                          $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                          $(DESTDIR)/$(prefix)/lib/libipc.a

mybindir=$(prefix)/bin
dist_mybin_SCRIPTS=te_builder_log te_create_inst_dirs te_discover_host \
                   te_generate_symtbl te_rpc_supported \
                   tce_instrument_sources tce_uninstrument_sources \
                   tce_report \
                   te_configure_package te_cross_build \
                   te_build_nuts te_build_nut te_build_suite \
                   te_tests_info.sh

EXTRA_DIST=builder.m4
