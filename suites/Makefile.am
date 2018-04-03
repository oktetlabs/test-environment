# Process this file with configure to produce a Makefile

# Test Environment
#
# Makefile for building of suites configured in the builder.conf
#
# Copyright (C) 2003-2018 OKTET Labs.
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
	$${TE_INSTALL}/$(TE_HOST)/bin/te_build_suite $$i \
		$${TE_BASE}/suites/$$i yes || exit 1 ; \
	done

uninstall-hook:
	. ../builder.conf.processed ; rm -rf $${TE_INSTALL_SUITE}

