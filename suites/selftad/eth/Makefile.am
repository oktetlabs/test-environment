

AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

bin_PROGRAMS=get_any eth1 eth2 eth3 eth_serial1 eth_serial2
bindir=${prefix}/bin/eth/

LDADD= -lrcfapi -lipc -lpthread -ltapi -ltapi_eth -lndn -lasn -llogger_ten
#eth2_LDADD= -lrcfapi -llogger_ten -lipc -lpthread -lndn -lndn_eth -ltapi_eth -lndn -lasn 


TESTS_DEPS = ${TE_INSTALL}/i686-pc-linux-gnu/lib/libconfapi.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/librcfapi.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/liblogger_ten.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/libipc.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/libtapi_eth.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/libtapi.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/libndn.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/libndn_eth.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/libndn_bridge.a \
             ${TE_INSTALL}/i686-pc-linux-gnu/lib/libasn.a 

