
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

bin_PROGRAMS = te_stdouterr te_tce_dump

te_stdouterr_SOURCES = te_stdouterr.c
te_stdouterr_LDADD = -llogger_ten -lipc -ltools @TE_LDADD@ -lpthread
te_stdouterr_DEPENDENCIES = @TE_DEPENDENCIES@ \
                            $(DESTDIR)/$(libdir)/liblogger_ten.a \
                            $(DESTDIR)/$(libdir)/libipc.a \
                            $(DESTDIR)/$(libdir)/libtools.a

te_tce_dump_SOURCES = te_tce_dump.c
te_tce_dump_LDADD = -lrcfapi -llogger_ten -lipc -ltools @TE_LDADD@ \
                    -lpthread
te_tce_dump_DEPENDENCIES = @TE_DEPENDENCIES@ \
                           $(DESTDIR)/$(libdir)/librcfapi.a \
                           $(DESTDIR)/$(libdir)/liblogger_ten.a \
                           $(DESTDIR)/$(libdir)/libipc.a\
                           $(DESTDIR)/$(libdir)/libtools.a

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
    te_rpc_unsupported \
    te_build_suite \
    te_build_win_lib \
    te_build_win_app \
    te_tests_info.sh

# Scripts to be installed by 'install'
dist_bin_SCRIPTS = \
    te_build_nuts \
    te_tce_dump.sh \
    te_tce_process

EXTRA_DIST = builder.m4

mydatadir=$(datadir)/@PACKAGE_NAME@
dist_mydata_DATA = nut.m4
