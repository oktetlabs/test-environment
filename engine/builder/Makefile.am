
AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS = 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS = te_stdouterr te_tce_dump

te_stdouterr_SOURCES = te_stdouterr.c
te_stdouterr_LDADD = -llogger_ten -lipc @TE_LDADD@ -lpthread
te_stdouterr_DEPENDENCIES = @TE_DEPENDENCIES@ \
                            $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                            $(DESTDIR)/$(prefix)/lib/libipc.a

te_tce_dump_SOURCES = te_tce_dump.c
te_tce_dump_LDADD = -lrcfapi -llogger_ten -lipc @TE_LDADD@ -lpthread
te_tce_dump_DEPENDENCIES = @TE_DEPENDENCIES@ \
                           $(DESTDIR)/$(prefix)/lib/librcfapi.a \
                           $(DESTDIR)/$(prefix)/lib/liblogger_ten.a \
                           $(DESTDIR)/$(prefix)/lib/libipc.a

# Scripts to be installed by 'install-data' (required for TE build)
# Test Suites are build on distcheck, therefore 'te_build_suite' and
# 'te_tests_info.sh' should be installed by 'install-data'.
mybindir=$(prefix)/bin
dist_mybin_SCRIPTS = \
    te_builder_log \
    te_builder_opts \
    te_create_inst_dirs \
    te_discover_host \
    te_configure_package \
    te_cross_build \
    te_cross_make \
    te_generate_symtbl \
    te_generate_rpctbl \
    te_rpc_supported \
    te_build_suite \
    te_tests_info.sh

# Scripts to be installed by 'install'
dist_bin_SCRIPTS = \
    te_build_nuts \
    te_tce_dump.sh \
    te_tce_process

EXTRA_DIST = builder.m4

mydatadir=$(datadir)/@PACKAGE_NAME@
dist_mydata_DATA = nut.m4
