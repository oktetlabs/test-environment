
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CFLAGS = -I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 



tetest_PROGRAMS = get_any eth1 eth2 eth3 eth_serial1 eth_serial2

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

