#! /bin/bash

# Script to create SLAPD database and run slapd 
# Intended to be used from the Test Agent.
# Parameters:
#    port to listen
#    name of the file with LDIF (stored file ldif in slapd directory)

PORT=$1
LDIF=$2

SLAPD_DIR=/tmp/te_ldap_$PORT

if [ -d /etc/ldap/schema ]; then
    # Debian case
    SCHEMA_DIR=/etc/ldap/schema
elif [ -d /etc/openldap/schema ]; then
    # RedHat case
    SCHEMA_DIR=/etc/openldap/schema
    REMOVE_MODULES=1
else
    echo "Failed to find directory with LDAP schemas."
    exit 1
fi

failure()
{
    echo "$@"
#    rm -rf $SLAPD_DIR
    exit 1
}

rm -rf $SLAPD_DIR || exit 1
mkdir $SLAPD_DIR || exit 1
sed -e "s/te_ldap/te_ldap_$PORT/g" -e "s?%SCHEMA_DIR%?$SCHEMA_DIR?" \
    <`dirname $0`/slapd.conf > $SLAPD_DIR/slapd.conf \
    || failure "Failed to edit slapd.conf"
if [ ! -z $REMOVE_MODULES ]; then
    echo sed -i -e 's/moduleload.*//' $SLAPD_DIR/slapd.conf
    sed -i -e 's/moduleload.*//' $SLAPD_DIR/slapd.conf \
        || failure "Failed to remove module lines from slapd.conf"
fi
echo $2 > $SLAPD_DIR/ldif || failure "Failed to create ldif file"
/usr/sbin/slapadd -f $SLAPD_DIR/slapd.conf -l $LDIF || \
    failure "slapadd failed"
/usr/sbin/slapd -4 -f $SLAPD_DIR/slapd.conf -h ldap://0.0.0.0:$PORT || \
    failure "slapd start failed"
exit 0
