
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = @TE_LDFLAGS@

include_HEADERS = \
	tapi_mem.h \
	tapi_proc.h \
	tapi_test.h \
	tapi_test_log.h \
	tapi_test_behaviour.h \
	tapi_jmp.h \
	tapi_test_run_status.h \
	tapi_cache.h \
	tapi_cfg.h \
	tapi_cfg_base.h \
	tapi_cfg_phy.h \
	tapi_cfg_net.h \
	tapi_cfg_arl.h \
	tapi_cfg_dhcp.h \
	tapi_cfg_pppoe.h \
	tapi_cfg_vm.h \
	tapi_cfg_vtund.h \
	tapi_cfg_ip6.h \
	tapi_cfg_stats.h \
	tapi_cfg_xen.h \
	tapi_cfg_iptables.h \
	tapi_cfg_eth.h \
	tapi_file.h \
	tapi_reqs.h \
	tapi_gw.h \
	tapi_route_gw.h \
	tapi_sh_env.h \
	tapi_cfg_acse.h \
	tapi_sniffer.h \
	tapi_serial_parse.h \
	tapi_ntpd.h \
	tapi_sfptpd.h \
	tapi_cfg_aggr.h \
	tapi_cfg_cmd_monitor.h \
	tapi_cfg_ip_rule.h \
	tapi_cfg_openvpn.h \
	tapi_cfg_l2tp.h \
	tapi_cfg_socks.h \
	tapi_namespaces.h \
	tapi_cfg_sys.h \
	tapi_cfg_local.h \
	tapi_host_ns.h \
	tapi_cfg_modules.h \
	tapi_cfg_qdisc.h \
	tapi_cfg_netem.h \
	tapi_cfg_tbf.h \
	tapi_cfg_cpu.h \
	tapi_cfg_nginx.h

lib_LTLIBRARIES = libtapi.la

libtapi_la_SOURCES = \
	tapi_mem.c \
	tapi_proc.c \
	test_params.c \
	tapi_jmp.c \
	tapi_test_run_status.c \
	tapi_test_behaviour.c \
	tapi_cache.c \
	tapi_cfg.c \
	tapi_cfg_local_to_ta.c \
	tapi_cfg_base.c \
	tapi_cfg_phy.c \
	tapi_cfg_net.c \
	tapi_cfg_arl.c \
	tapi_cfg_dhcp.c \
	tapi_cfg_pppoe.c \
	tapi_cfg_vm.c \
	tapi_cfg_vtund.c \
	tapi_cfg_ip6.c \
	tapi_cfg_stats.c \
	tapi_cfg_xen.c \
	tapi_cfg_iptables.c \
	tapi_cfg_eth.c \
	tapi_file.c \
	tapi_tester_msg.c \
	tapi_test_fail_state.c \
	tapi_reqs.c \
	tapi_cfg_acse.c \
	tapi_sniffer.c \
	tapi_serial_parse.c \
	tapi_cfg_aggr.c \
	tapi_cfg_ip_rule.c \
	tapi_cfg_openvpn.c \
	tapi_cfg_l2tp.c \
	tapi_cfg_socks.c \
	tapi_route_gw.c \
	tapi_netns.c \
	tapi_cfg_sys.c \
	tapi_host_ns.c \
	tapi_cfg_modules.c \
	tapi_cfg_qdisc.c \
	tapi_cfg_netem.c \
	tapi_cfg_tbf.c \
	tapi_cfg_cpu.c \
	tapi_cfg_nginx.c

install-exec-hook:
	touch -c -r libtapi.la $(DESTDIR)/$(libdir)/libtapi.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
		touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
