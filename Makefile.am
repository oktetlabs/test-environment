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

AUTOMAKE_OPTIONS = foreign 

host=@host@
TA_REMOTE_SUBDIRS=@TA_REMOTE_SUBDIRS@

SUBDIRS=include doc lib tools engine nutlib agents nuts
DIST_SUBDIRS=$(SUBDIRS) @SUITE_SUBDIR@

mybindir=$(prefix)/$(host)/bin

mybin_SCRIPTS=dispatcher.sh host_name.sh

EXTRA_DIST=dispatcher.sh conf

if SUITE_SCRIPTS_PROVIDED

EXTRA_DIST+=@SUITE_SCRIPTS_EXTRA_DIST@

@SUITE_SCRIPTS_EXTRA_DIST@:
	for i in @SUITE_SCRIPTS@ ; do   \
            cp $$i . ;                   \
        done

endif

host_name.sh:
	echo 'echo ' @host@ > host_name.sh
	chmod a+x host_name.sh

DISTCLEANFILES=builder.conf.processed host_name.sh

te: install-data
	cd include ; make install 
	cd lib ; make install 
	cd nutlib ; make 
	cd tools ; make install 
	cd engine ; make install
	cd agents ; make install
	te_cross_build $(host) $(prefix)
	cd nuts ; make 

te-nuts:
	cd nuts; \
	make TE_INSTALL=$${TE_INSTALL} TE_BASE=$${TE_BASE}

tce-report:
	@echo Creating TCE report
	for i in $${TE_BS_NUTS}  $${TE_BS_NUTLIBS}; do \
	    tce_taname=`eval echo '$$NUT_'$$i'_TCE_TANAME'` ; \
	    if test -n "$${tce_taname}" ; then \
	        tce_report $${tce_taname} || exit 1 ; \
	    fi ; \
	done

suite:
	@echo Building Test Suite 
	if test -z "$${TE_INSTALL}" ; then \
	    TE_INSTALL=$(prefix) ; \
	fi ; \
	if test -z "$${TE_INSTALL_SUITE}" ; then \
	    TE_INSTALL_SUITE=$${TE_INSTALL}/suites ; \
	fi ; \
	if test -n "$(SUITE_SOURCES)" ; then \
	    SOURCES=$(SUITE_SOURCES) ; \
	    if test "$${SOURCES:0:1}" != "/" ; then \
	        SOURCES=$(srcdir)/$${SOURCES} ; \
	    fi ; \
	    mkdir -p suites ; \
	    cd suites ; \
	    if test ! -e Makefile ; then \
	        TE_INSTALL=$${TE_INSTALL} \
	        TE_BASE=$${TE_BASE} \
	        TE_CFLAGS="-Wall -W -O0 -ggdb -I$(srcdir)/include \
	                   -I$${TE_INSTALL}/$(host)/include \
	        TE_LDFLAGS="-L$${TE_INSTALL}/$(host)/lib" \
	        eval $${SOURCES}/configure '-q --prefix=$${TE_INSTALL_SUITE}' \
	        || exit 1 ; \
	    fi ; \
	    make install || exit 1 ; \
	fi

