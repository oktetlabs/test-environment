
#EXTRA_DIST = package.dox

dist_tetest_DATA = package.xml


AM_CFLAGS = -I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = udp2

LDADD= -lrcfapi -llogger_ten -lipc -lpthread -lndn -ltapi_ipstack -lndn_ipstack -lndn -lasn 

