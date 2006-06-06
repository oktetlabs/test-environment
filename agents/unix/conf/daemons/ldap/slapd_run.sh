#! /bin/bash

# Script to create SLAPD database and run slapd 
# Intended to be used from the Test Agent.
# Parameters:
#    port to listen
#    name of the file with LDIF (stored file ldif in slapd directory)

PORT=$1
LDIF=$2

SLAPD_DIR=/tmp/te_ldap_$PORT

failure()
{
    rm -rf $SLAPD_DIR
    exit 1
}

rm -rf $SLAPD_DIR || exit 1
mkdir $SLAPD_DIR || exit 1
cat `dirname $0`/slapd.conf | sed 's/te_ldap/'te_ldap_$PORT'/g' > \
    $SLAPD_DIR/slapd.conf || failure
echo $2 > $SLAPD_DIR/ldif || failure
/usr/sbin/slapadd -f $SLAPD_DIR/slapd.conf -l $LDIF || failure
/usr/sbin/slapd -4 -f $SLAPD_DIR/slapd.conf -h ldap://0.0.0.0:$PORT || failure
exit 0
