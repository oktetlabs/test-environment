
AUTOMAKE_OPTIONS = foreign 



SUBDIRS= @TAD_SUBDIRS@

#SUBDIRS = tests


AM_CFLAGS=@TAD_CH_DEFS@ @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                                    -I$(DESTDIR)/$(prefix)/../include 


AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS=tad.h

noinst_LIBRARIES=libtad_core.a

libtad_core_a_SOURCES=tad_ch.c csap_db.c csap_spt_db.c tad_recv.c tad_send.c tad_utils.c



lib_LIBRARIES=libtad.a

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

# Tests (incorrect)
#
#TESTS=cdb01 cdb02
#check_PROGRAMS=expr01 cdb01 cdb02
#VPATH = @srcdir@:@srcdir@/tests

#extra_DIST=tests/cdb01.c tests/cdb02.c tests/debug_commands tests/expr01.c


#LDADD= -ltad -lasn -llogger_ta -lpthread


all: install

