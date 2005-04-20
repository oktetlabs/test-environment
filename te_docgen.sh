#!/bin/bash
#
#


if test -z ${TE_BASE}; then
    echo "You must set the TE_BASE environment variable " >&2
    exit 1
fi

DOC_DIR=${TE_BASE}/lib/doc

#if test -d ${DOC_DIR}/latex ; then
#    rm -rf ${DOC_DIR}/latex
#fi

#if test -d ${DOC_DIR}/html ; then
#    rm -rf ${DOC_DIR}/html
#fi

pushd `pwd` > /dev/null
cd ${TE_BASE}/lib
echo "Generating documation..."
doxygen > /dev/null
popd > /dev/null

cp -f refman.tex ${DOC_DIR}/latex/
pushd `pwd` > /dev/null
cd ${TE_BASE}/lib/doc/latex/

sed -i -e '2s/^\(\\section\).*/\1\{Configurator API Reference\}/' conf__api_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test Engine Logger API\}/' logger__ten_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Remote Control Facility API\}/' rcf__api_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{RCF RPC Subsystem API\}/' rcf__rpc_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Access Configuration Model\}/' tapi__cfg_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Buffers dealer API\}/' tapi__bufs_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test Api for Network Configuration\}/' tapi__cfg__net_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{ARL Table Configuration Model\}/' tapi__cfg__arl_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Basic Configuration Model\}/' tapi__cfg__base_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for DHCP Configuration\}/' tapi__cfg__dhcp_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Convenient Work with files on TEN\}/' tapi__file_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Thread-safe Stack Jump\}/' tapi__jmp_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Generic Network Address Manipulation\}/' tapi__sockaddr_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Definition of Macros used in Test sources\}/' tapi__test_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for ARP\}/' tapi__arp_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for STP Reference\}/' tapi__stp_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{CLI API Reference\}/' tapi__cli_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API Reference for Ethernet\}/' tapi__eth_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API Reference for IPStack CSAP\}/' tapi__ipstack_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Asynchronous I\/O Calls\}/' tapi__rpc__aio_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Interface name\/index Calls\}/' tapi__rpc__ifnameindex_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for name\/address Resolution Remote Calls\}/' tapi__rpc__netdb_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Signal and Signal Sets Remote Calls\}/' tapi__rpc__signal_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{TAPI for Socket API Remote Calls\}/' tapi__rpc__socket_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Standard I\/O Calls\}/' tapi__rpc__stdio_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for File Operation Calls\}/' tapi__rpc__unistd_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Macros for Socket API Remote Calls\}/' tapi__rpcsock__macros_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{TAPI for Miscellaneous Remote Calls\}/' tapi__rpc__misc_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Remote Call of Winsock2-specific Routines\}/' tapi__rpc__winsock2_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for SNMP Implementation\}/' tapi__snmp_8h.tex
sed -i -e '2s/^\(\\section\).*/\1\{Test API for Common Traffic Application Domain\}/' tapi__tad_8h.tex

echo "Creating PDF format of documentation..."
make refman.pdf > /dev/null

cp -u refman.pdf ${TE_BASE}/
make clean
popd > /dev/null




