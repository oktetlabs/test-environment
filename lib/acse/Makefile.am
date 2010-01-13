
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
                    acse_cwmp.c

#                    soapClient.c # Why ?? 

#                    stdsoap2.c \
#                    soapC.c \
#                    soapServer.c 

#libacse_a_LIBADD = /usr/lib/libssl.a /usr/lib/libcrypto.a /usr/lib/libz.a

bin_PROGRAMS = acse_dummy

acse_dummy_SOURCES = acse_dummy.c \
                    soapC.c \
                    stdsoap2.c \
                    soapServer.c 

# acse_dummy_LDFLAGS = -lgsoap
acse_dummy_LDFLAGS = -lz

BUILT_SOURCES = cwmp_acs.h cwmp_cpe.h cwmp.h cwmp_decls.h cwmp.nsmap \
                soapC.c soapClient.c soapServer.c soapH.h soapStub.h
CLEANFILES    = cwmp_acs.h cwmp_cpe.h cwmp.h cwmp_decls.h cwmp.nsmap \
                soapC.c soapClient.c soapServer.c soapH.h soapStub.h

# Copy stdsoap2.c to the build directory (since configure substitution in _SOURCES variable is not allowed)
stdsoap2.c: @SOAPC@
	cp @SOAPC@ $@

# Generate ACS SOAP sources
soapServer.c: cwmp_acs.h
	$(SOAPCPP2) $(SOAPCPP2_ACS_FLAGS) cwmp_acs.h
	rm -f soapC.c soapH.h soapStub.h cwmp.nsmap

# Generate CPE SOAP sources
soapClient.c: cwmp_cpe.h
	$(SOAPCPP2) $(SOAPCPP2_CPE_FLAGS) cwmp_cpe.h
	rm -f soapC.c soapH.h soapStub.h cwmp.nsmap

# Generate common SOAP sources
soapC.c soapH.h soapStub.h cwmp.nsmap: cwmp.h soapServer.c soapClient.c
	$(SOAPCPP2) $(SOAPCPP2_FLAGS) cwmp.h

# Add specifications of the methods to the generated declarations
cwmp_acs.h: cwmp_decls.h $(srcdir)/cwmp_acs_methods.dat $(srcdir)/cwmp_common_methods.dat
	cp cwmp_decls.h $@
	awk '{print "int __cwmp__"$$1"(_cwmp__"$$1" *cwmp__"$$1", _cwmp__"$$1"Response *cwmp__"$$1"Response);"}' $(srcdir)/cwmp_common_methods.dat >> $@
	awk '{print "int __cwmp__"$$1"(_cwmp__"$$1" *cwmp__"$$1", _cwmp__"$$1"Response *cwmp__"$$1"Response);"}' $(srcdir)/cwmp_acs_methods.dat >> $@

# Add specifications of the methods to the generated declarations
cwmp_cpe.h: cwmp_decls.h $(srcdir)/cwmp_cpe_methods.dat $(srcdir)/cwmp_common_methods.dat
	cp cwmp_decls.h $@
	awk '{print "int __cwmp__"$$1"(_cwmp__"$$1" *cwmp__"$$1", _cwmp__"$$1"Response *cwmp__"$$1"Response);"}' $(srcdir)/cwmp_common_methods.dat >> $@
	awk '{print "int __cwmp__"$$1"(_cwmp__"$$1" *cwmp__"$$1", _cwmp__"$$1"Response *cwmp__"$$1"Response);"}' $(srcdir)/cwmp_cpe_methods.dat >> $@

# Add specifications of the methods to the generated declarations
cwmp.h: cwmp_decls.h $(srcdir)/cwmp_acs_methods.dat $(srcdir)/cwmp_cpe_methods.dat $(srcdir)/cwmp_common_methods.dat
	cp cwmp_decls.h $@
	awk '{print "int __cwmp__"$$1"(_cwmp__"$$1" *cwmp__"$$1", _cwmp__"$$1"Response *cwmp__"$$1"Response);"}' $(srcdir)/cwmp_common_methods.dat >> $@
	awk '{print "int __cwmp__"$$1"(_cwmp__"$$1" *cwmp__"$$1", _cwmp__"$$1"Response *cwmp__"$$1"Response);"}' $(srcdir)/cwmp_acs_methods.dat >> $@
	awk '{print "int __cwmp__"$$1"(_cwmp__"$$1" *cwmp__"$$1", _cwmp__"$$1"Response *cwmp__"$$1"Response);"}' $(srcdir)/cwmp_cpe_methods.dat >> $@

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
