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

TE_APP_SET

AC_CHECK_HEADER([stdsoap2.h], [],
    [AC_MSG_ERROR([failed to find stdsoap2.h, is gSOAP installed?])
])

SOAP_CPPFLAGS="-DWITH_COOKIES -DWITH_UDP \
               -DWITH_GZIP -DWITH_OPENSSL -DWITH_NOEMPTYSTRUCT"
AC_SUBST([SOAP_CPPFLAGS])

# Save the list of LIBS
MY_LIBS_SAVED=$LIBS

# Try to find all the libs that are necessary for linking with TA
AC_SEARCH_LIBS([dlopen], [dl])
AC_CHECK_LIB([z], [deflate], [],
             [AC_MSG_ERROR([failed to find z library])])
AC_CHECK_LIB([crypto], [X509_sign], [],
             [AC_MSG_ERROR([failed to find crypto library])])
AC_CHECK_LIB([ssl], [SSL_new], [],
             [AC_MSG_ERROR([failed to find ssl library])])
ACSE_LIBS="$LIBS"
AC_SUBST([ACSE_LIBS])
# Restore the list of LIBS
LIBS=$MY_LIBS_SAVED

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
