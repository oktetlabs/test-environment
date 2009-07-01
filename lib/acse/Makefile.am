
SUBDIRS = .

ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) @SOAP_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ 

# WSDL/XSD to C declarations generation/transformation tool and its flags
WSDL2H = @WSDL2H@
WSDL2H_FLAGS = @WSDL2H_FLAGS@

# SOAP implementation generation tool and its flags
SOAPCPP2 = @SOAPCPP2@
SOAPCPP2_FLAGS = @SOAPCPP2_FLAGS@

lib_LTLIBRARIES = libacse.la

include_HEADERS = acse.h

noinst_HEADERS = acse_internal.h cwmp.h soapH.h soapStub.h

libacse_la_SOURCES = acse.c \
                    acse_lrpc.c \
                    acse_conn.c \
                    acse_sreq.c \
                    acse_cwmp.c \
                    stdsoap2.c \
                    soapC.c \
                    soapServer.c

#libacse_a_LIBADD = /usr/lib/libssl.a /usr/lib/libcrypto.a /usr/lib/libz.a

BUILT_SOURCES = cwmp.h cwmp_decls.h soapC.c soapServer.c soapH.h soapStub.h cwmp.nsmap
CLEANFILES    = cwmp.h cwmp_decls.h soapC.c soapServer.c soapH.h soapStub.h cwmp.nsmap

# Copy stdsoap2.c to the build directory (since configure substitution in _SOURCES variable is not allowed)
stdsoap2.c: @SOAPC@
	cp @SOAPC@ $@

# Generate SOAP sources
soapC.c soapServer.c soapH.h soapStub.h cwmp.nsmap: cwmp.h
	$(SOAPCPP2) $(SOAPCPP2_FLAGS) cwmp.h

# Add specifications of the methods to the generated declarations
cwmp.h: cwmp_decls.h $(srcdir)/cwmp_methods.h
	if cat cwmp_decls.h $(srcdir)/cwmp_methods.h > $@; then :; else rm cwmp.h; false; fi

# Generete CWMP XSD based C declarations
cwmp_decls.h: cwmp_typemap.dat cwmp.xsd
	$(WSDL2H) $(WSDL2H_FLAGS) -o $@ -t $(srcdir)/cwmp_typemap.dat $(srcdir)/cwmp.xsd

install-exec-hook:
	touch -c -r libacse.* $(DESTDIR)/$(libdir)/libacse.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
