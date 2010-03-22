
SUBDIRS = .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) @SOAP_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ -DWITH_OPENSSL

AM_LDFLAGS = @TE_LDFLAGS@


lib_LTLIBRARIES = libacse_epc.la

bin_PROGRAMS = acse acse_cli

include_HEADERS = acse_epc.h acse_soapStub.h cwmp_data.h

acse_SOURCES = acse.c acse_dummy.c \
                    acse_conn.c \
                    acse_creq.c \
                    acse_cwmp.c \
                    acse_epc_disp.c \
                    acse_db.c \
                    acse_mem.c \
                    acse_soapC.c \
                    httpda.c \
                    md5evp.c \
                    acse_soapServer.c

libacse_epc_la_SOURCES = acse_epc.c \
                    cwmp_data.c
                    
acse_cli_SOURCES = acse_cli.c

#libacse_a_LIBADD = /usr/lib/libssl.a /usr/lib/libcrypto.a /usr/lib/libz.a


# acse_dummy_LDFLAGS = -lgsoap
acse_LDADD = -lacse_epc -lgsoapssl -lz -llogger_file -ltools \
             -lcrypto -lrt -lssl
#acse_LDADD = -lacse_epc -lz -llogger_file -ltools -lcrypto -lrt

acse_cli_LDADD = -lacse_epc -llogger_file -ltools -lrt -lpopt

install-exec-hook:
	touch -c -r libacse_epc.* $(DESTDIR)/$(libdir)/libacse_epc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
