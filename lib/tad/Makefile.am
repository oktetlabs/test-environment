
SUBDIRS= @TAD_SUBDIRS@ .


AM_CPPFLAGS=@TAD_CH_DEFS@ -I$(DESTDIR)/$(prefix)/include \
                          -I$(DESTDIR)/$(prefix)/../include 

AM_CFLAGS=

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


include_HEADERS=tad.h

noinst_LIBRARIES=libtad_core.a

libtad_core_a_SOURCES=tad_ch.c csap_db.c csap_spt_db.c tad_recv.c \
                      tad_send.c tad_utils.c


lib_LIBRARIES=libtad.a

libtad_a_SOURCES=

AR_SCRIPT=CREATE $@\nADDLIB libtad_core.a\n@TAD_LIBADD@\nSAVE\nEND\n

libtad.a: libtad_core.a @TAD_LIBLIST@
	echo -e "$(AR_SCRIPT)" | ar -M
	ranlib $@

install-exec-hook:
	touch -c -r libtad.a $(DESTDIR)/$(prefix)/lib/libtad.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
