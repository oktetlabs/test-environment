

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS=udp2
bindir=${prefix}/bin/ipstack/

LDADD= -lrcfapi -llogger_ten -lipc -lpthread -lndn -ltapi_ipstack -lndn_ipstack -lndn -lasn 

#eth2_LDADD= -lrcfapi -llogger_ten -lipc -lpthread -lndn -lndn_eth -ltapi_eth -lndn -lasn 


