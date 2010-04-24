
SUBDIRS = .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) @SOAP_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ -DWITH_OPENSSL

AM_LDFLAGS = @TE_LDFLAGS@


lib_LTLIBRARIES = libacse_epc.la libacse_cwmp.la libcwmp_data.la

bin_PROGRAMS = acse_cli

include_HEADERS = acse_epc.h cwmp_data.h

# acse_single_SOURCES = acse_dummy.c

libacse_cwmp_la_SOURCES = acse.c \
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

libacse_epc_la_SOURCES = acse_epc.c cwmp_data.c

libcwmp_data_la_SOURCES = cwmp_data.c
                    
                    
acse_cli_SOURCES = acse_cli.c

#libacse_a_LIBADD = /usr/lib/libssl.a /usr/lib/libcrypto.a /usr/lib/libz.a


# acse_dummy_LDFLAGS = -lgsoap
# acse_single_LDADD = -lacse_epc -lacse_cwmp -lgsoapssl -lz \
#             -llogger_file -ltools -lcrypto -lrt -lssl

acse_cli_LDADD = -lacse_epc  -lacse_cwmp -lgsoapssl -lz -llogger_file \
            -ltools -lcrypto -lrt -lssl -lpopt
# acse_cli_LDADD = -lacse_epc -llogger_file -ltools -lrt -lpopt

install-exec-hook:
	touch -c -r libacse_epc.* $(DESTDIR)/$(libdir)/libacse_epc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
