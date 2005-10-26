#
# This file contains build for Test Environment features, used in 
# in regular self tests for agents on native (Linux) platform. 
#

#
# Host platform
#
TE_PLATFORM([], [], [], [], \
            [tools rpc_types ipc bsapi loggerten rpcxdr rcfapi conf_oid \
             confapi tapi comm_net_engine rcfunix loggerta comm_net_agent \
             rcfpch  rcfrpc tce asn ndn tapi_rpc tapi_env tapi_tad tapi_eth \
             tapi_bridge  tapi_arp tapi_ipstack tapi_dhcp tapi_snmp \
             tapi_cli tapi_iscsi tad ])

TE_APP_PARMS([rcf], [--enable-ltdl-install], [], [], [])

TE_TOOLS([rgt rpcgen trc tce millicom])

TE_LIB_PARMS([tad], [], [], \
             [--with-file --with-eth --with-bridge --with-iscsi\
              --with-ipstack --with-dhcp --with-snmp --with-cli], [])

TE_TA_TYPE([linux], [], [unix], [--with-rcf-rpc  --with-netlink --with-netlink-route ], [], [], \
            [ndn asn comm_net_agent])
