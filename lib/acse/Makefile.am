
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

lib_LTLIBRARIES = libacse.la

include_HEADERS = acse.h

libacse_la_SOURCES = acse.c \
                    acse_conn.c \
                    acse_epc.c \
                    acse_sreq.c \
                    acse_cwmp.c \
                    acse_db.c \
                    acse_soapC.c \
                    stdsoap2.c \
                    httpda.c \
                    md5evp.c \
                    acse_soapServer.c 


#libacse_a_LIBADD = /usr/lib/libssl.a /usr/lib/libcrypto.a /usr/lib/libz.a

bin_PROGRAMS = acse_dummy

acse_dummy_SOURCES = acse_dummy.c

# acse_dummy_LDFLAGS = -lgsoap
acse_dummy_LDADD = -lacse -lz -llogger_file -ltools -lcrypto

install-exec-hook:
	touch -c -r libacse.* $(DESTDIR)/$(libdir)/libacse.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
