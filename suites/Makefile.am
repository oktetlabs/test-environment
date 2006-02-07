# Process this file with configure to produce a Makefile

# Test Environment
#
# Makefile for building of suites configured in the builder.conf
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


 include Makefile.suites
 
DIST_SUBDIRS=$(TE_BS_SUITES) 

EXTRA_DIST=Makefile.suites

DISTCLEANFILES=Makefile.suites

all:
	. ../builder.conf.processed ; \
	cd $${TE_BUILD} ; \
	for i in $${TE_BS_SUITES} $(TE_BS_SUITES) ; do \
	$${TE_INSTALL}/$(host)/bin/te_build_suite $$i \
		$${TE_BASE}/suites/$$i yes || exit 1 ; \
	done

uninstall-hook:
	. ../builder.conf.processed ; rm -rf $${TE_INSTALL_SUITE}

