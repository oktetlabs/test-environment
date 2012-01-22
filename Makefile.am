# Process this file with configure to produce a Makefile

# Test Environment
#
# Root Makefile
#
# Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
# root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA  02111-1307  USA
#
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
	pushd $(DESTDIR)/$(prefix) >/dev/null ; \
	ln -sf $(TE_HOST)/bin/dispatcher.sh dispatcher.sh ; \
	popd >/dev/null ; \
	$(srcdir)/engine/builder/te_cross_build $(prefix)

uninstall-hook:
	rm -f $(DESTDIR)/$(prefix)/dispatcher.sh
	$(srcdir)/engine/builder/te_cross_make uninstall

clean-local:
	$(srcdir)/engine/builder/te_cross_make clean

distclean-local:
	rm -rf `cat @TE_EXT_LIBS_FILE@`
	rm -rf @TE_EXT_LIBS_FILE@
	$(srcdir)/engine/builder/te_cross_make distclean
