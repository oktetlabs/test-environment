
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

DEF_CONTEXT_S=
if DEF_CONTEXT
DEF_CONTEXT_S+=ndn_context.c
endif
DEF_CONTEXT_H=

if USE_SNMP 
AM_CFLAGS+=-DWITH_SNMP -DCMU_COMPATIBLE -DUCD_COMPATIBLE
DEF_CONTEXT_S+=ndn_snmp.c
DEF_CONTEXT_H+=ndn_snmp.h
endif

if USE_FILE 
AM_CFLAGS+= -DWITH_FILE
DEF_CONTEXT_S+=ndn_file.c
DEF_CONTEXT_H+=ndn_file.h
endif

if USE_DHCP 
AM_CFLAGS+= -DWITH_DHCP
DEF_CONTEXT_S+=ndn_dhcp.c
DEF_CONTEXT_H+=ndn_dhcp.h
endif

if USE_ETH 
AM_CFLAGS+= -DWITH_ETH
DEF_CONTEXT_S+=ndn_eth.c
DEF_CONTEXT_H+=ndn_eth.h
endif 

if USE_IPSTACK 
AM_CFLAGS+= -DWITH_IPSTACK
DEF_CONTEXT_S+=ndn_ipstack.c
DEF_CONTEXT_H+=ndn_ipstack.h
endif

if USE_CLI 
AM_CFLAGS+= -DWITH_CLI
DEF_CONTEXT_S+=ndn_cli.c
DEF_CONTEXT_H+=ndn_cli.h
endif

if USE_BRIDGE 
AM_CFLAGS+= -DWITH_BRIDGE
DEF_CONTEXT_S+=ndn_bridge.c
DEF_CONTEXT_H+=ndn_bridge.h
endif


lib_LIBRARIES=libndn.a

include_HEADERS=ndn.h ndn_internal.h ${DEF_CONTEXT_H}

libndn_a_SOURCES= ndn_generic.c ${DEF_CONTEXT_S}

CFLAGS=@CFLAGS@ 

install-exec-hook:
	touch -c -r libndn.a $(DESTDIR)/$(prefix)/lib/libndn.a 

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

