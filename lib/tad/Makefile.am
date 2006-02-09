
SUBDIRS= @TAD_SUBDIRS@ .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ @TAD_CH_DEFS@ \
              -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@

include_HEADERS = \
	tad_csap_inst.h \
	tad_types.h \
	tad_pkt.h \
	tad_send_recv.h \
	tad_send.h \
	tad_recv.h \
	tad_recv_pkt.h

noinst_HEADERS = \
	csap_id.h \
	tad_csap_support.h \
	tad_utils.h \
	pkt_socket.h \
	tad_bps.h

noinst_LIBRARIES = libtad_core.a

if DUMMY_TAD
AM_CPPFLAGS += -DDUMMY_TAD
libtad_core_a_SOURCES = tad_ch.c
else 
libtad_core_a_SOURCES = \
	tad_pkt.c \
	pkt_socket.c \
	tad_utils.c \
	tad_ch.c \
	csap_id.c \
	csap_spt_db.c \
	csap_inst.c \
	tad_send.c \
	tad_recv_pkt.c \
	tad_recv.c \
	tad_bps.c
endif 



TAD_FULL_LIBLIST = $(noinst_LIBRARIES)@TAD_LIBLIST@

lib_LIBRARIES = lib@with_name@.a

lib@with_name@_a_SOURCES =
lib@with_name@_a_DEPENDENCIES = $(TAD_FULL_LIBLIST)

lib@with_name@.a: $(lib@with_name@_a_DEPENDENCIES)
	AR_SCRIPT="CREATE $@\n" ; \
	for i in $(TAD_FULL_LIBLIST) ; do \
		AR_SCRIPT="$${AR_SCRIPT}ADDLIB $$i\n" ; \
	done ; \
	AR_SCRIPT="$${AR_SCRIPT}SAVE\nEND\n" ; \
	echo -e $${AR_SCRIPT} | $(AR) -M
	$(RANLIB) $@


mypkgdir = $(libdir)/pkgconfig
mypkg_DATA = te-@with_name@.pc
CLEANFILES = te-@with_name@.pc

te-@with_name@.pc:
	echo -n -e "\
	Name: te-$(with_name)\n\
	Description: Traffic Application Domain for TE\n\
	Version: $(PACKAGE_VERSION)\n\
	Requires:\n\
	Libs: @TA_TAD_LIBS@\n\
	Cflags: \n" > $@

install-exec-hook:
	touch -c -r lib@with_name@.a $(DESTDIR)/$(libdir)/lib@with_name@.a

install-data-hook:
	touch -c -r te-@with_name@.pc $(mybindir)/te-@with_name@.pc ; \
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
