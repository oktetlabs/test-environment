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

AC_CHECK_MEMBERS([struct rte_flow_ops.isolate],
[], [],
[[
#include "rte_config.h"
#include "rte_flow.h"
#include "rte_flow_driver.h"
]])

AC_COMPILE_IFELSE([
AC_LANG_PROGRAM(
[[
#include "rte_config.h"
#include "rte_flow.h"

enum rte_flow_item_type type;
]],
[[type = RTE_FLOW_ITEM_TYPE_GENEVE;]]
)], [AC_DEFINE([HAVE_RTE_FLOW_GENEVE], 1, [ ])], [])


AC_CHECK_MEMBERS([struct rte_eth_dev_info.rx_queue_offload_capa],
[], [],
[[
#include "rte_config.h"
#include "rte_ethdev.h"
]])

AC_CHECK_MEMBERS([struct rte_eth_dev_info.tx_queue_offload_capa],
[], [],
[[
#include "rte_config.h"
#include "rte_ethdev.h"
]])

AC_CHECK_MEMBERS([struct rte_eth_dev_info.dev_capa],
[], [],
[[
#include "rte_config.h"
#include "rte_ethdev.h"
]])

AC_CHECK_MEMBERS([struct rte_flow_action_rss.rss_conf],
[], [],
[[
#include "rte_config.h"
#include "rte_flow.h"
]])

AC_CHECK_MEMBERS([struct rte_flow_action_rss.num],
[], [],
[[
#include "rte_config.h"
#include "rte_flow.h"
]])

AC_CHECK_MEMBERS([struct rte_flow_item_vlan.tpid],
[], [],
[[
#include "rte_config.h"
#include "rte_flow.h"
]])

LIBS="-lnuma -lpthread -ldl $LIBS"
LIBS="-lrte_mbuf -lrte_net -lrte_mempool -lrte_ring -lrte_eal -lrte_kvargs $LIBS"
AC_CHECK_FUNCS([rte_pktmbuf_pool_create_by_ops])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
