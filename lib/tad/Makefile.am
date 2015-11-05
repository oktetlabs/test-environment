
SUBDIRS= @TAD_SUBDIRS@ .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @TAD_CH_DEFS@ \
              -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

include_HEADERS = \
	tad_csap_inst.h \
	tad_csap_support.h \
	tad_types.h \
	tad_pkt.h \
	tad_send_recv.h \
	tad_send.h \
	tad_recv.h \
	tad_recv_pkt.h \
	tad_poll.h

noinst_HEADERS = \
	csap_id.h \
	tad_agent_csap.h \
	tad_utils.h \
	tad_eth_sap.h \
	tad_bps.h

noinst_LIBRARIES = libtad_core.a

if TAD_DUMMY
AM_CPPFLAGS += -DTAD_DUMMY
libtad_core_a_SOURCES = tad_ch.c
else 
libtad_core_a_SOURCES = \
	tad_pkt.c \
	tad_eth_sap.c \
	tad_eth_sap_dlpi.c \
	tad_utils.c \
	tad_ch.c \
	csap_id.c \
	agent_csap.c \
	csap_spt_db.c \
	csap_inst.c \
	tad_send.c \
	tad_recv_pkt.c \
	tad_recv.c \
	tad_bps.c \
	tad_poll.c
endif 



TAD_FULL_LIBLIST = $(noinst_LIBRARIES)@TAD_LIBLIST@

lib_LIBRARIES = lib@with_name@.a

lib@with_name@_a_SOURCES =
lib@with_name@_a_DEPENDENCIES = $(TAD_FULL_LIBLIST)

lib@with_name@.a: $(lib@with_name@_a_DEPENDENCIES)
	{ \
	    echo "CREATE $@" ; \
	    for i in $(TAD_FULL_LIBLIST) ; do \
	        echo "ADDLIB $$i" ; \
	    done ; \
	    echo "SAVE" ; \
	    echo "END" ; \
	} | $(AR) -M
	$(RANLIB) $@


mypkgdir = $(libdir)/pkgconfig
mypkg_DATA = te-@with_name@.pc
CLEANFILES = te-@with_name@.pc

te-@with_name@.pc:
	{ \
	    echo "Name: te-$(with_name)" ; \
	    echo "Description: Traffic Application Domain for TE" ; \
	    echo "Version: $(PACKAGE_VERSION)" ; \
	    echo "Requires:" ; \
	    echo "Libs: @TA_TAD_LIBS@" ; \
	    echo "Cflags: " ; \
	} > $@

install-exec-hook:
	touch -c -r lib@with_name@.a $(DESTDIR)/$(libdir)/lib@with_name@.a

install-data-hook:
	touch -c -r te-@with_name@.pc $(mybindir)/te-@with_name@.pc ; \
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
