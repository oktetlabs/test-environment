
ACLOCAL_AMFLAGS = -I $(TE_M4)

AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include $(TE_CPPFLAGS) \
              -include te_config.h -include config.h \
              -I$(srcdir)/.. -I$(srcdir)/util -I$(srcdir)/route \
              -I$(srcdir)/daemons -I. @EXTRA_CPPFLAGS@
              
AM_CFLAGS = -fno-strict-aliasing $(TE_CFLAGS)

AM_YFLAGS = -d

tadir=$(prefix)/@with_name@

ta_DATA=daemons/ldap/slapd.conf

ta_SCRIPTS=daemons/ldap/slapd_run.sh

EXTRA_DIST=daemons/ldap/slapd.conf daemons/ldap/slapd_run.sh

noinst_HEADERS = \
	route/conf_route.h \
	util/conf_dlpi.h 
        
noinst_LIBRARIES=libconf.a

libconf_a_SOURCES = \
	base/conf.c \
	base/conf_stats.c \
	base/conf_sys.c \
	base/conf_phy.c \
	route/conf_route.c \
	route/conf_route_netlink.c \
	route/conf_route_libnetconf.c \
	route/conf_route_socket.c \
	util/conf_getmsg.c \
	util/conf_dlpi.c 

CLEANFILES=

if ISCSI_TARGET_USE

AM_CPPFLAGS += -I$(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/include/iscsi_unh_target 
libconf_a_SOURCES += conf_iscsi_target.c 

endif

if CFG_VCM
libconf_a_SOURCES += base/conf_vcm.c 
endif

if CFG_TR069
libconf_a_SOURCES += base/conf_acse.c 
endif

if CFG_UNIX_WIFI
libconf_a_SOURCES += base/conf_wifi.c
endif

if CFG_UNIX_8021X
libconf_a_SOURCES += daemons/8021x/conf_8021x.c
endif

if CFG_UNIX_DAEMONS

AM_CPPFLAGS += @DAEMONS_CFLAGS@

libconf_a_SOURCES += daemons/conf_daemons.c daemons/ldap/ldap.c
noinst_HEADERS += daemons/conf_daemons.h 

if CFG_UNIX_DAEMONS_ISC_DHCP_SERVER

libconf_a_SOURCES += daemons/dhcp/dhcp_server.c 
noinst_HEADERS += daemons/dhcp/dhcp_server.h 

endif

if CFG_UNIX_DAEMONS_PPPOE_SERVER

libconf_a_SOURCES += daemons/pppoe/pppoe_server.c 
noinst_HEADERS += daemons/pppoe/pppoe_server.h 

endif

if CFG_UNIX_DAEMONS_NAMED

libconf_a_SOURCES += daemons/dns/dns_server.c \
                     daemons/dns/named.conf.y \
                     daemons/dns/named.lex.l

CLEANFILES+=named.conf.c named.lex.c named.conf.h

endif

if CFG_UNIX_DAEMONS_RADIUS
libconf_a_SOURCES += daemons/radius/radius.c
endif


if CFG_UNIX_DAEMONS_VTUND
libconf_a_SOURCES += daemons/vtund/conf_vtund.c
endif

AM_CPPFLAGS += @EXTRA_CPPFLAGS@


if ISCSI_TARGET_USE
libconf_a_DEPENDENCIES = \
    $(DESTDIR)/$(prefix)/../$(TE_PLATFORM)/lib/libiscsi_initiator_conf.a
endif

endif
