

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS= create send0 recv0 recv1
bindir=${prefix}/bin/bridge/

LDADD= -lrcfapi -llogger_ten -lipc -lpthread -ltapi_bridge -ltapi_eth -lndn -lasn 


