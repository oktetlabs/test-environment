# Process this file with configure to produce a Makefile

# Test Environment
#
# Root Makefile
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>

SUBDIRS = include doc lib tools engine agents suites

mybindir=$(prefix)/$(TE_HOST)/bin
dist_mybin_SCRIPTS = dispatcher.sh common_vars.sh

DISTCLEANFILES = builder.conf builder.conf.processed 

EXTRA_DIST = Doxyfile auxdir/acinclude.m4 auxdir/pkg.m4

if SUITE_SCRIPTS_PROVIDED

EXTRA_DIST+=@SUITE_SCRIPTS_EXTRA_DIST@

@SUITE_SCRIPTS_EXTRA_DIST@:
	for i in @SUITE_SCRIPTS@ ; do   \
		cp $$i . ;              \
	done

endif

# Workaround for bug in latest libtool: libtool defines ECHO, but uses $echo
export echo=echo

all: configure.ac install-data

dist-hook:
	if test -n "@TE_CONF_DIR@" ; then \
            cp -pR @TE_CONF_DIR@ $(distdir) ; \
            rm -rf `find $(distdir)/conf -name .svn` \
                   `find $(distdir)/conf -name CVS` ; \
        fi
	if test -e $(distdir)/conf/builder.conf ; then \
            chmod u+w $(distdir)/conf ; \
            rm -f $(distdir)/conf/builder.conf ; \
        fi ;
	if test -n "@TE_BUILDER_CONF@" ; then \
            cp @TE_BUILDER_CONF@ $(distdir)/conf/builder.conf ; \
        fi

install-exec-hook:
	export TE_EXT_LIBS_FILE="@TE_EXT_LIBS_FILE@" ; \
	export TE_BASE="@TE_BASE@" ; \
	"@TE_BASE@/engine/builder/te_build_ext_helper" --prefix="$(prefix)" \
                                                   --tatype="$(TE_HOST)" \
                                                   --mode=app || exit 1; \
	pushd $(DESTDIR)/$(prefix) >/dev/null ; \
	ln -sf $(TE_HOST)/bin/dispatcher.sh dispatcher.sh ; \
	ln -sf $(TE_HOST)/bin/common_vars.sh common_vars.sh ; \
	popd >/dev/null ;

uninstall-hook:
	pushd $(DESTDIR)/$(prefix) >/dev/null ; \
	rm -rf `cat @TE_EXT_LIBS_FILE@` ; \
	popd >/dev/null 
	rm -f $(DESTDIR)/$(prefix)/dispatcher.sh
	rm -f $(DESTDIR)/$(prefix)/common_vars.sh
	$(srcdir)/engine/builder/te_cross_make uninstall

clean-local:
	$(srcdir)/engine/builder/te_cross_make clean

distclean-local:
	pushd ./platforms >/dev/null ; \
	rm -rf `cat @TE_EXT_LIBS_FILE@` ; \
	popd >/dev/null ; \
	rm -rf @TE_EXT_LIBS_FILE@
	$(srcdir)/engine/builder/te_cross_make distclean
