
SUBDIRS = .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) @SOAP_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ 

AM_LDFLAGS = @TE_LDFLAGS@

# WSDL/XSD to C declarations generation/transformation tool and its flags
WSDL2H = @WSDL2H@
WSDL2H_FLAGS = @WSDL2H_FLAGS@

# SOAP implementation generation tool and its flags
SOAPCPP2 = @SOAPCPP2@
SOAPCPP2_FLAGS = @SOAPCPP2_FLAGS@

lib_LTLIBRARIES = libacse_epc.la

bin_PROGRAMS = acse

include_HEADERS = acse_epc.h acse_soapStub.h cwmp_data.h

acse_SOURCES = acse.c acse_dummy.c \
                    acse_conn.c \
                    acse_creq.c \
                    acse_cwmp.c \
                    acse_epc_disp.c \
                    acse_db.c \
                    acse_soapC.c \
                    httpda.c \
                    md5evp.c \
                    stdsoap2.c \
                    acse_soapServer.c 

libacse_epc_la_SOURCES = acse_epc.c \
                    cwmp_data.c
                    

#libacse_a_LIBADD = /usr/lib/libssl.a /usr/lib/libcrypto.a /usr/lib/libz.a


# acse_dummy_LDFLAGS = -lgsoap
acse_LDADD = -lacse_epc -lz -llogger_file -ltools -lcrypto

install-exec-hook:
	touch -c -r libacse_epc.* $(DESTDIR)/$(libdir)/libacse_epc.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
