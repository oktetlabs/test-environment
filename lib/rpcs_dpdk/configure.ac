#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rpcs_dpdk], [0.1], [Andrew.Rybcehnko@oktetlabs.ru])

AC_CONFIG_SRCDIR([rpcs_dpdk.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.11.1 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_RANLIB
AC_PROG_INSTALL

RANLIB=touch

AC_SUBST([TE_BASE])
AC_ARG_VAR([TE_BASE])

TE_LIB_SET

AC_CONFIG_HEADERS([package.h])

TA=unix
AC_SUBST(TA)

AC_COMPILE_IFELSE([
AC_LANG_PROGRAM(
[[
#include "rte_config.h"
#include "rte_ring.h"
]],
[[(void)rte_ring_enqueue_bulk(NULL, NULL, 0, NULL);]]
)], [AC_DEFINE([HAVE_RTE_RING_ENQUEUE_BULK_ARG_FREE_SPACE], 1, [ ])], [])

AC_COMPILE_IFELSE([
AC_LANG_PROGRAM(
[[
#include "rte_config.h"
#include "rte_ring.h"
]],
[[(void)rte_ring_dequeue_bulk(NULL, NULL, 0, NULL);]]
)], [AC_DEFINE([HAVE_RTE_RING_DEQUEUE_BULK_ARG_AVAILABLE], 1, [ ])], [])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
