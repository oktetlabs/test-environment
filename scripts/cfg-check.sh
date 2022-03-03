#!/bin/bash

RUN_DIR="${PWD}"

CFG_NAME=$1
shift

help () {
    echo "cfg-check.sh <cfg_name>"
    exit 0
}

( [ -z "$CFG_NAME]" ] || [ "$CFG_NAME" = "-h" ] || [ "$CFG_NAME" = "--help" ] ) && help

CFG_FILE=${CONFDIR}/env/$CFG_NAME

hosts=$(cat $CFG_FILE | egrep "(TE_IUT=|TE_TST[0-9]*=|TE_TST_(LAN|WIFI|SERIAL|WAN).*)" | sed "s/.*=//")

for the_host in ${hosts}; do
    echo -ne "Checking ${the_host}... \t"
    if ssh $the_host sudo uname -a >/dev/null 2>&1; then
        echo -ne "\tOK\n"
    else
        echo -ne "FAILED\n"
    fi
done
