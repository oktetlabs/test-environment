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

 include Makefile.conf

SUBDIRS=include doc lib engine tools agents 
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

DISTCLEANFILES=Makefile.conf builder.conf.processed host_name.sh

define build_library
    mkdir -p $${lib} ; \
    cd $${lib} ; \
    echo Building library $${lib} for platform $${p} ; \
    if test ! -e Makefile ; then \
        LIB_PARMS=`eval echo '$${TE_BS_LIB_'$${lib}'_PARMS}'` ; \
        ADD_CFLAGS=`eval echo '$${TE_BS_LIB_'$${lib}'_CFLAGS}'` ; \
        ADD_LDFLAGS=`eval echo '$${TE_BS_LIB_'$${lib}'_LDFLAGS}'` ; \
        echo $(srcdir)/lib/$${lib}/configure -q --prefix=$(prefix)/$${p} \
             --build=$(host) --host=$${p} $${PL_PARMS} $${LIB_PARMS} ; \
        TE_BASE=$${TE_BASE} \
        TE_CFLAGS="-Wall $${ADD_CFLAGS} $${PL_CFLAGS}"  \
        TE_LDFLAGS="$${ADD_LDFLAGS} $${PL_LDFLAGS}"  \
        eval $(srcdir)/lib/$${lib}/configure '-q --prefix=$(prefix)/$${p} \
                                              --build=$(host) --host=$${p}' \
                                              $${LIB_PARMS} $${PL_PARMS} \
        || exit 1 ; \
    fi ; \
    make install || exit 1 ; \
    cd .. ; 
endef

te: install-data
	@\
	$(import_variables) \
	\
	CURRDIR=`pwd` ; \
	cd lib ; \
	echo Building lib ; \
	make install || exit 1 ; \
	cd $${CURRDIR} ; \
	\
	echo Building NUT libraries ; \
	$(MAKE) $(AM_MAKEFLAGS) te-nuts NUTS="$${TE_BS_NUTLIBS}" ; \
	\
	for i in engine agents $(TA_REMOTE_SUBDIRS) ; do \
	     CURRDIR=`pwd` ; \
	     cd $${i} ; \
	     echo Building $${i} ; \
	     make install || exit 1 ; \
	     cd $${CURRDIR} ; \
	done ; \
	echo Building tools ; \
	cd tools ; \
	for i in $${TE_BS_TOOLS} ; do \
	    echo Tool $${i} ; \
	    if test -d $${i} ; then \
	        cd $${i} ; \
	        make install || exit 1 ; \
	        cd .. ; \
	    fi ; \
	done ; \
	cd .. ; \
	\
	for p in $${TE_BS_PLATFORMS} ; do \
	    mkdir -p $(prefix)/$${p} ; \
	    ta= ; \
	    for k in $${TE_BS_TA} ; do \
	        platform=`eval echo '$$TE_BS_TA_'$${k}'_PLATFORM'` ; \
	        if test "$${platform}" = "$${p}" ; \
	        then \
	            ta=$${k} ; \
	            break 1 ; \
	        fi ; \
	    done ; \
	    if test -z "$${ta}" ; then continue ; fi ; \
	\
	    mkdir -p $${p} ; \
	    cd $${p} ; \
	    pname=`echo $${p} | sed s/-/_/g` ; \
	    PL_PARMS=`eval echo '$${'$${pname}'_PARMS}'` ; \
	    PL_CFLAGS=`eval echo '$${'$${pname}'_CFLAGS}'` ; \
	    PL_LDFLAGS=`eval echo '$${'$${pname}'_LDFLAGS}'` ; \
	\
	    mkdir -p lib ; \
	    cd lib ; \
	    for lib in $${TE_BS_LIB} ; do \
	        option=`eval echo '$${TE_BS_'$${lib}'_TA}'` ; \
	        if test "$${option}" = "yes" ; then \
	            $(build_library) \
	        fi ; \
	    done ; \
	\
	    ta_el=`eval echo '$${TE_BS_TA_'$${ta}'_EL}'` ; \
	    for lib in $${ta_el} ; do \
	        $(build_library) \
	    done ; \
	    cd .. ; \
	\
	    TASRC=`eval echo '$${TE_BS_TA_'$${ta}'_SOURCES}'` ; \
	    mkdir -p agents ; \
	    cd agents ; \
	    CURRDIR=`pwd` ; \
	    mkdir -p $${CURRDIR}/$${TASRC} ; \
	    cd $${CURRDIR}/$${TASRC} ; \
	    echo Building Test Agent $${ta} for platform $${p} ; \
	    if test ! -e Makefile ; then \
	        TAPARMS=`eval echo '$${TE_BS_TA_'$${ta}'_PARMS}'` ; \
	        ADD_CFLAGS=`eval echo '$${TE_BS_TA_'$${ta}'_CFLAGS}'` ; \
	        ADD_LDFLAGS=`eval echo '$${TE_BS_TA_'$${ta}'_LDFLAGS}'` ; \
	        echo $(srcdir)/agents/$${TASRC}/configure \
	             -q --build=$(host) --host=$${p} --prefix=$(prefix)/agents \
	             $${TAPARMS} $${PL_PARMS} ; \
	        TE_BASE=$${TE_BASE}  \
	        TE_CFLAGS="-Wall $${ADD_CFLAGS} $${PL_CFLAGS}"  \
	        TE_LDFLAGS="$${ADD_LDFLAGS}  $${PL_LDFLAGS}"  \
	        TE_LDADD=`eval echo '$${TE_BS_TA_'$${ta}'_LDADD}'`  \
	        TE_DEPENDENCIES=`eval echo '$${TE_BS_TA_'$${ta}'_DEPENDENCIES}'`  \
	        eval $(srcdir)/agents/$${TASRC}/configure \
	             '-q --build=$(host) --host=$${p} --prefix=$(prefix)/agents' \
	             $${TAPARMS} $${PL_PARMS} || exit 1 ; \
	    fi ; \
	    make install || exit 1 ; \
	    cd $${CURRDIR}/.. ; \
	done ;
	$(MAKE) $(AM_MAKEFLAGS) te-nuts

        
te-tools:
	cd tools ; \
	make install ;

# TODO: submit to the storage
te-nuts:
	@echo Building NUT images
	@$(import_variables) \
	if test -z "$${TE_INSTALL}" ; then \
	    TE_INSTALL=$(prefix) ; \
	fi ; \
	if test -z "$${TE_INSTALL_NUT}" ; then \
	    TE_INSTALL_NUT=$${TE_INSTALL}/nuts ; \
	fi ; \
	mkdir -p $${TE_INSTALL_NUT} ; \
	if test -n "$(NUTS)" ; then TE_BS_NUTS=$(NUTS) ; fi ; \
	mkdir -p nuts/bin ; \
	for i in $${TE_BS_NUTS} ; do \
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

tce-report:
	@echo Creating TCE report
	@$(import_variables) \
	for i in $${TE_BS_NUTS} ; do \
	    tce_taname=`eval echo '$$NUT_'$$i'_TCE_TANAME'` ; \
	    if test -n "$${tce_taname}" ; then \
	        tce_report $${tce_taname} || exit 1 ; \
	    fi ; \
	done

suite:
	@echo Building Test Suite 
	$(import_variables) \
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
	                   $${TE_BS_HOST_CFLAGS}" \
	        TE_LDFLAGS="-L$${TE_INSTALL}/$(host)/lib $${TE_BS_HOST_LDFLAGS}" \
	        eval $${SOURCES}/configure '-q \
	             --prefix=$${TE_INSTALL_SUITE}' $${TE_BS_HOST_PARMS} \
	        || exit 1 ; \
	    fi ; \
	    make install || exit 1 ; \
	fi

