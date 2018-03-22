# Process this file with configure to produce a Makefile

# Test Environment
#
# lib/ Makefile
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>

AM_MAKEFLAGS="prefix=@PREFIX_HOST@"

SUBDIRS=. @LIB_SUBDIRS@
DIST_SUBDIRS=@LIB_DIST_SUBDIRS@

install-exec-local:
	for i in $(SUBDIRS) ; do \
	    pushd $$i >/dev/null ; \
	    if test -e Makefile ; then \
	        $(MAKE) $(AM_MAKEFLAGS) install-data ; \
	    fi ; \
	    popd >/dev/null ; \
	done

distclean-local:
	for i in $(SUBDIRS) ; do \
	    pushd $$i >/dev/null ; \
	    if test -e Makefile ; then \
	        $(MAKE) $(AM_MAKEFLAGS) distclean ; \
	    fi ; \
	    popd >/dev/null ; \
	done
