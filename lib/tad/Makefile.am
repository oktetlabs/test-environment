
SUBDIRS= @TAD_SUBDIRS@ .


AM_CPPFLAGS=@TAD_CH_DEFS@ -I$(DESTDIR)/$(prefix)/include -I$(top_srcdir)
AM_LDFLAGS=@TE_LDFLAGS@ 

include_HEADERS=tad_csap_inst.h
noinst_HEADERS=tad_csap_support.h tad_utils.h pkt_socket.h

noinst_LIBRARIES=libtad_core.a

if DUMMY_TAD
AM_CPPFLAGS+= -DDUMMY_TAD
libtad_core_a_SOURCES=tad_ch.c
else 
libtad_core_a_SOURCES=tad_ch.c csap_db.c csap_spt_db.c tad_recv.c \
                      tad_send.c tad_utils.c pkt_socket.c
endif 



TAD_FULL_LIBLIST=$(noinst_LIBRARIES)@TAD_LIBLIST@

lib_LIBRARIES=lib@with_name@.a

lib@with_name@_a_SOURCES=
lib@with_name@_a_DEPENDENCIES=$(TAD_FULL_LIBLIST)

lib@with_name@.a: $(lib@with_name@_a_DEPENDENCIES)
	AR_SCRIPT="CREATE $@\n" ; \
	for i in $(TAD_FULL_LIBLIST) ; do \
		AR_SCRIPT="$${AR_SCRIPT}ADDLIB $$i\n" ; \
	done ; \
	AR_SCRIPT="$${AR_SCRIPT}SAVE\nEND\n" ; \
	echo -e $${AR_SCRIPT} | $(AR) -M
	$(RANLIB) $@


mypkgdir=$(libdir)/pkgconfig
mypkg_DATA = te-@with_name@.pc
CLEANFILES = te-@with_name@.pc

te-@with_name@.pc:
	echo -n -e "\
	Name: te-$(with_name)\n\
	Description: Traffic Application Domain for TE\n\
	Version: $(PACKAGE_VERSION)\n\
	Requires:\n\
	Libs: $(TA_TAD_LIBS)\n\
	Cflags: \n" > $@

install-exec-hook:
	touch -c -r lib@with_name@.a $(DESTDIR)/$(prefix)/lib/lib@with_name@.a

install-data-hook:
	touch -c -r te-@with_name@.pc $(mybindir)/te-@with_name@.pc ; \
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
