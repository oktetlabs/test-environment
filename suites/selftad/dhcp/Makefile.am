

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS=dhcptest
bindir=${prefix}/bin/dhcp/

LDADD= -lrcfapi -llogger_ten -lipc -lpthread -lndn -lndn_dhcp -ltapi_dhcp -lndn -lasn 


