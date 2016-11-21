
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


# Scripts to be installed by 'install-data' (required for TE build)
# Test Suites are build on distcheck, therefore 'te_build_suite' and
# 'te_tests_info.sh' should be installed by 'install-data'.
mybindir=$(prefix)/bin
dist_mybin_SCRIPTS = \
    te_builder_log \
    te_builder_opts \
    te_create_inst_dirs \
    te_configure_package \
    te_cross_build \
    te_cross_make \
    te_build_ext \
    te_fixlink \
    te_generate_symtbl \
    te_build_suite \
    te_build_win_platform \
    te_build_win_lib \
    te_build_win_app \
    te_tests_info.sh \
    te_struct_layout \
    te_functions \
    te_rcf_consistency_checks \
    static_libs_check.pl

# Scripts to be installed by 'install'
dist_bin_SCRIPTS = \
    te_build_nuts \
    te_tce_process

EXTRA_DIST = builder.m4

dist_pkgdata_DATA = nut.m4
