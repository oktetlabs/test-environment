
SUBDIRS = .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) @SOAP_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ -DWITH_OPENSSL

AM_LDFLAGS = @TE_LDFLAGS@


lib_LTLIBRARIES = libacse_cwmp.la libcwmp_data.la

# Library for auto-generated and external sources
noinst_LTLIBRARIES = libgsoap_cwmp.la

# bin_PROGRAMS = acse_cli

include_HEADERS = acse_epc.h cwmp_data.h acse_user.h cwmp_utils.h

noinst_HEADERS = cli_utils.h

# acse_single_SOURCES = acse_dummy.c

libgsoap_cwmp_la_SOURCES = \
                    acse_soapC.c \
                    acse_soapH.h \
                    acse_soapServer.c \
                    httpda.c \
                    httpda.h \
                    md5evp.c \
                    md5evp.h

libacse_cwmp_la_SOURCES = acse.c \
                    acse_conn.c \
                    acse_creq.c \
                    acse_cwmp.c \
                    acse_db.c \
                    acse_epc.c \
                    acse_epc_disp.c \
                    acse_internal.h \
                    acse_mem.c \
                    acse_mem.h \
                    acse_rpc.c \
                    acse_user.c
libacse_cwmp_la_LIBADD = libgsoap_cwmp.la

libcwmp_data_la_SOURCES = cwmp_data.c cwmp_utils.c

# acse_cli_SOURCES = acse_cli.c cli_utils.c

#libacse_a_LIBADD = /usr/lib/libssl.a /usr/lib/libcrypto.a /usr/lib/libz.a


# acse_dummy_LDFLAGS = -lgsoap
# acse_single_LDADD = -lacse_epc -lacse_cwmp -lgsoapssl -lz \
#             -llogger_file -lpthread -ltools -lcrypto -lrt -lssl

#acse_cli_LDADD = -lcwmp_data  -lacse_cwmp -lgsoapssl -llogger_file \
#            -lpthread -ltools -lrt -lssl -lcrypto -lz -lpopt -ldl
# acse_cli_LDADD = -lacse_epc -llogger_file -lpthread -ltools -lrt -lpopt

install-exec-hook:
	touch -c -r libacse_cwmp.* $(DESTDIR)/$(libdir)/libacse_cwmp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
