#
# This file contains build for Test Environment features, used in 
# in regular self tests for agents on native (Linux) platform. 
#

#
# Host platform
#
TE_PLATFORM([], [], [], [], [], \
            [tools logic_expr ipc bsapi loggerten rpcxdr rpc_types \
             rcfapi conf_oid confapi tapi comm_net_engine rcfunix \
             loggerta comm_net_agent rcfpch rcfrpc tce asn ndn \
             tapi_rpc tapi_env tapi_tad tad \
             iscsi_unh_target iscsi_initiator_conf])

TE_APP_PARMS([rcf], [--enable-ltdl-install], [], [], [], [])

TE_TOOLS([rgt rpcgen trc tce millicom])

TE_LIB_PARMS([tad], [], [], \
             [-with-pcap --with-eth --with-bridge \
              --with-arp --with-ipstack --with-dhcp --with-iscsi \
              --with-snmp --with-cli], [], [], [])

TE_TA_TYPE([linux], [], [unix], [--with-rcf-rpc --with-iscsi], \
           [], [], [], [ndn asn comm_net_agent])
