# Process this file with configure to produce a Makefile

# Test Environment
#
# NUT libraries Makefile
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

NUTS=@TE_BS_NUTLIBS@

all:
	. ../builder.conf.processed ;
	if test -z "$${TE_INSTALL_NUT}" ; then \
	    TE_INSTALL_NUT=$${TE_INSTALL}/nuts ; \
	fi ; \
	mkdir -p $${TE_INSTALL_NUT} ; \
	for i in $(NUTS) ; do \
	    nut_sources=`eval echo '$$NUT_'$$i'_SOURCES'` ; \
	    nut_script=`eval echo '$$NUT_'$$i'_SCRIPT'` ; \
	    nut_parms=`eval echo '$$NUT_'$$i'_PARMS'` ; \
	    nut_st_location=`eval echo '$$NUT_'$$i'_ST_LOCATION'` ; \
	    nut_st_string=`eval echo '$$NUT_'$$i'_ST_STRING'` ; \
	    tce_fmt=`eval echo '$$NUT_'$$i'_TCE_FMT'` ; \
	    if test -n "$${tce_fmt}" ; then \
	        tce_type=`eval echo '$$NUT_'$$i'_TCE_TYPE'` ; \
	        tce_src=`eval echo '$$NUT_'$$i'_TCE_SOURCES'` ; \
	        tce_instrument_sources $${tce_type} -p $${nut_sources} \
	                               $${tce_type} $${tce_fmt} $${tce_src} \
	        || exit 1 ; \
	    fi ; \
	    TE_INSTALL=$${TE_INSTALL} TE_INSTALL_NUT=$${TE_INSTALL_NUT} \
	    $${nut_script} $${i} $${nut_parms} ; \
	    nut_script_rc=$$? ; \
	    if test -n "$${tce_fmt}" ; then \
	        tce_uninstrument_sources -p $${nut_sources} || exit 1 ; \
	    fi ; \
	    if test $${nut_script_rc} -ne 0 ; then \
	        exit 1 ; \
	    fi ; \
	done

