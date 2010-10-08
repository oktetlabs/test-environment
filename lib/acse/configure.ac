#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([acse], [0.02], [Konstantin.Abramenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([acse.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.9.6 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_PATH_TOOL([WSDL2H], [wsdl2h])
if test -z $WSDL2H; then
    AC_MSG_ERROR([failed to find wsdl2h tool, is gsoap toolkit installed?])
fi

AC_PATH_TOOL([SOAPCPP2], [soapcpp2])
if test -z $SOAPCPP2; then
    AC_MSG_ERROR([failed to find soapcpp2 tool, is gsoap toolkit installed?])
fi

TE_APP_SET

SOAP_PATH=[`echo 'changequote([,])builtin([patsubst],['"$SOAPCPP2"'],[soapcpp2$],[])' | m4`]
SOAP_PATH=[`echo 'changequote([,])builtin([patsubst],['"$SOAP_PATH"'],[/$],[])' | m4`]
AC_SUBST([SOAP_PATH])

SOAPH="$SOAP_PATH"'/../include/stdsoap2.h'
AC_SUBST([SOAPH])
AC_CHECK_HEADER([$SOAPH], [],
    [AC_MSG_ERROR([failed to find stdsoap2.h on path '$SOAP_PATH/../include', is gsoap toolkit installed properly?])])

#SOAPC="$SOAP_PATH"'/../include/gsoap/stdsoap2.c'
#AC_SUBST([SOAPC])
#AC_MSG_CHECKING([for $SOAPC])
#if test -r "$SOAPC"; then
#    AC_MSG_RESULT([yes])
#else
#    AC_MSG_RESULT([no])
#    AC_MSG_ERROR([failed to find stdsoap2.c on path '$SOAP_PATH/../include/gsoap', is gsoap toolkit installed properly?])
#fi

WSDL2H_FLAGS='-cy'
AC_SUBST([WSDL2H_FLAGS])

SOAPCPP2_ACS_FLAGS='-1cLSxwse'
AC_SUBST([SOAPCPP2_ACS_FLAGS])

SOAPCPP2_CPE_FLAGS='-1cLCxwse'
AC_SUBST([SOAPCPP2_CPE_FLAGS])

SOAPCPP2_FLAGS='-1cLCSxwse'
AC_SUBST([SOAPCPP2_FLAGS])

SOAP_CPPFLAGS='-I'"$SOAP_PATH"'/../include -DWITH_COOKIES -DWITH_GZIP -DWITH_OPENSSL -DWITH_NOEMPTYSTRUCT -DWITH_DOM'
AC_SUBST([SOAP_CPPFLAGS])

# Save the list of LIBS
MY_LIBS_SAVED=$LIBS

# Try to find all the libs that are necessary for linking with TA
AC_CHECK_LIB([z], [deflate], [], [AC_MSG_ERROR([failed to find z library])])
AC_CHECK_LIB([crypto], [X509_sign], [], [AC_MSG_ERROR([failed to find crypto library])], [-lz -ldl])
AC_CHECK_LIB([ssl], [SSL_new], [], [AC_MSG_ERROR([failed to find ssl library])], [-lz -lcrypto -ldl]) 
# Restore the list of LIBS
LIBS=$MY_LIBS_SAVED

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
