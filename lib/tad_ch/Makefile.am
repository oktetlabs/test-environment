
AUTOMAKE_OPTIONS = foreign 



#SUBDIRS = tests


AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include 

if USE_SNMP
AM_CFLAGS+=-DWITH_SNMP
endif

if USE_FILE
AM_CFLAGS+=-DWITH_FILE
endif

if USE_CLI
AM_CFLAGS+=-DWITH_CLI
endif

if USE_ETH
AM_CFLAGS+=-DWITH_ETH
endif

if USE_IPSTACK
AM_CFLAGS+=-DWITH_IPSTACK
endif

if USE_DHCP
AM_CFLAGS+=-DWITH_DHCP
endif

if USE_BRIDGE 
AM_CFLAGS+= -DWITH_BRIDGE
endif

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

include_HEADERS=tad.h


lib_LIBRARIES=libtad_ch.a

libtad_ch_a_SOURCES=tad_ch.c csap_db.c csap_spt_db.c tad_recv.c tad_send.c tad_utils.c

# libtad_ch_la_LIBS=-lasn

install-exec-hook:
	touch -c -r libtad_ch.a $(DESTDIR)/$(prefix)/lib/libtad_ch.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

# Tests (incorrect)
#
#TESTS=cdb01 cdb02
#check_PROGRAMS=expr01 cdb01 cdb02
#VPATH = @srcdir@:@srcdir@/tests

extra_DIST=tests/cdb01.c tests/cdb02.c tests/debug_commands tests/expr01.c


LDADD= -ltad_ch -lasn -llogger_ta -lpthread


all: install

