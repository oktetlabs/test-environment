#
# This file contains build for Test Environment features, used in 
# in regular self tests for agents on native (Linux) platform. 
#

#
# Host platform
#
TE_PLATFORM([], [], [], [], \
            [ipc bsapi loggerten rcfapi conf_oid confapi tapi \
             comm_net_engine rcfunix loggerta comm_net_agent rcfpch \
             rcfrpc tapi_rpcsock tapi_tad tapi_eth tapi_bridge tapi_arp \
             tapi_ipstack tapi_dhcp tapi_snmp tapi_cli \
             asn ndn tad \
            ])

TE_APP_PARMS([rcf], [--enable-ltdl-install], [], [], [])

TE_LIB_PARMS([tad], [], [], \
             [--with-file --with-eth --with-bridge \
              --with-ipstack --with-dhcp --with-snmp --with-cli], [])

TE_TA_TYPE([linux], [], [linux], [--with-rcf-rpc], [], \
           [-lexpect -lnetsnmp -lcrypto], [ndn asn comm_net_agent])
