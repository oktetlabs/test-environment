#! /bin/bash

# Script to clean TE traces from all hosts in the testing configuration
#
# Configuration file should contain --conf-rcf option (like run.conf.dori).
#

SSH="ssh -qxTn"
SSH_TERM="ssh -xt"

usage()
{
cat >&2 <<EUSAGE
USAGE: $0 <option> ...
where options may be:
    -c <config file>    file with TE run options, containing --conf-rcf
    -r <rcf config>     RCF config file 
    -e <environment>    Environment settings for the configuration
    -l <log file>       name of file where all messages will be stored
    -h <TE engine host> name of host where TE engine should be stopped, 
                        if found. 
Exactly one of '-c' and '-r' option must present in command line.
EUSAGE
}

log_msg()
{
    if test -n "$LOG_FILE"; then
        moment=`date "+%Y.%m.%e %R:%S"`
        echo "$moment:  " "$@" >> $LOG_FILE
    fi
}

find_pid()
{
    local awk_cmd="awk '{if (\$5 ~ /$1/) {print \$1;}}'"
    local SH
    if test -n "$2"; then 
        SH="${SSH} $2 "
    else
        SH="$TE_SH"
    fi
    $SH ps ax | eval $awk_cmd 
}


kill_accurate()
{
    if test -z "$2"; then return; fi

    if test -n "$3"; then
        LOC_SH="${SSH} $3"
    else
        LOC_SH="$TE_SH"
    fi

    ps_line=`eval $LOC_SH ps h --pid $2`
    if test -z "$ps_line"; then return; fi

    clean_need=yes;
    log_msg "ERROR: need to kill $1: $ps_line" 

    $LOC_SH sudo /bin/kill $2 || 
        log_msg "ERROR: kill of $2 failed"
    sleep 1
    if $LOC_SH ps --pid $2 >/dev/null; then
        log_msg "ERROR: need kill -9 $1: `eval $LOC_SH ps h --pid $2`" 
        $LOC_SH sudo /bin/kill -9 $2 || 
            log_msg "ERROR: kill -9 of $2 failed"
    fi
}

if test $# -lt 2; then
    usage
    exit 1
fi

LOG_FILE=
TE_HOST=localhost

while test -n "$1"; do
    case $1 in
        -c ) 
            CFG_FILE=$2;;
        -r ) 
            RCF_CFG_FILE=$2;;
        -e ) 
            ENV_CFG_FILE=$2;;
        -l ) 
            LOG_FILE=$2;;
        -h )
            TE_HOST=$2;;
    esac
    if test $# -lt 2; then
        usage
        exit 1;
    else
        shift 2;
    fi
done 

if test -n "$CFG_FILE"; then 
    if test -n "$RCF_CFG_FILE"; then
        usage
        exit 1
    fi
    rcf_cfg_line=`grep rcf.conf ${CFG_FILE}`
    RCF_CFG_FILE=${rcf_cfg_line#--conf-rcf=}
fi

if test -z "$RCF_CFG_FILE" -a -z "$ENV_CFG_FILE" ; then
    usage
    exit 1
fi
if test -n "$LOG_FILE"; then 
    echo "">$LOG_FILE
fi

log_msg "RCF cfg file: ${RCF_CFG_FILE}"
log_msg "Env cfg file: ${ENV_CFG_FILE}"

# Find hosts where agents may be started
if test -r "${ENV_CFG_FILE}" ; then
    . "${ENV_CFG_FILE}"
    TA_HOSTS="$TE_IUT $TE_TST1 $TE_TST2"
elif test -r "${RCF_CFG_FILE}" ; then
    TA_HOSTS=`cat ${RCF_CFG_FILE} | awk '
                  /confstr/ { i = index($0, "confstr") + 9; \
                              n = index(substr($0, i), ":") - 1 ; \
                              print substr($0, i, n) ; }'`
else
    echo "Neither RCF nor Env configuration file is readable" >&2
    exit 1
fi

log_msg "All TA hosts: ${TA_HOSTS}"

TA_HOSTS_ALL="${TA_HOSTS}"
TA_HOSTS=
for host in "" ${TA_HOSTS_ALL} ; do
    test -n "$host" || continue
    os="$(${SSH} "$host" "uname -s")"
    if test "x$os" = "xLinux" ; then
        TA_HOSTS="${TA_HOSTS} $host"
    else
        log_msg "Unable to clean up on the host $host"
    fi
done
log_msg "TA hosts to be processed: ${TA_HOSTS}"

clean_need=
# 
# Store all interesting status information before cleaning. 
#
if test -n "$LOG_FILE"; then 
    TEMP_FILE=`tempfile`
    echo "">$TEMP_FILE
    for h in $TE_HOST $TA_HOSTS; do
        if test $h = "localhost"; then 
            SH=
        else
            SH="${SSH} $h "
        fi
        echo "---- processes on $h: ---- "  >>$TEMP_FILE 
        $SH ps aux >>$TEMP_FILE 2>&1 || 
            log_msg "ps aux on $h failed"
        echo "---- content of /tmp/ $h: ---- " >>$TEMP_FILE
        $SH ls -l /tmp/ >>$TEMP_FILE 2>&1 || 
            log_msg "ls -l /tmp/ on $h failed"
        echo "---- interfaces on $h: ---- " >>$TEMP_FILE
        $SH /sbin/ifconfig >>$TEMP_FILE 2>&1 || 
            log_msg "ifconfig on $h failed"
        echo "---- ip addresses on $h: ---- " >>$TEMP_FILE
        $SH /sbin/ip addr list >>$TEMP_FILE 2>&1 || 
            log_msg "ip addr list on $h failed"
        echo "---- route table on $h: ---- " >>$TEMP_FILE
        $SH /sbin/route -n >>$TEMP_FILE 2>&1 ||
            log_msg "route on $h failed" 
        log_msg "status on host $h stored into tmp file"
    done
fi

if test $TE_HOST = "localhost"; then
    TE_SH=
else
    TE_SH="${SSH} $TE_HOST "
fi

# Clean TA hosts. 
for ta_host in ${TA_HOSTS}; do 
    log_msg "---- Try clear TA host $ta_host"

    user_str=`${SSH} $ta_host grep te10000 /etc/passwd`

    if test -n "$user_str"; then
        log_msg "ERROR: user: $user_str on $ta_host, del it, rm home"
        ${SSH_TERM} $ta_host sudo /usr/sbin/userdel ${user_str/%:*/}
#        ${SSH_TERM} $ta_host sudo rm -rf /home/${user_str/%:*/} 
    fi

    group_str=`${SSH} $ta_host grep te10000 /etc/group`
    if test -n "$group_str"; then
        log_msg "ERROR: group: $group_str on $ta_host, delete it"
        ${SSH_TERM} $ta_host sudo /usr/sbin/groupdel ${group_str/%:*/}
    fi

    xvfb_pids=`find_pid Xvfb $ta_host`
    for p in $xvfb_pids; do
        if ${SSH} $ta_host ps h --pid $p | grep ":50" >/dev/null; then
            kill_accurate "Xvfb on $ta_host" $p $ta_host
        fi
    done

done 

if test -n "$LOG_FILE" && test -n "$clean_need"; then
    log_msg "++++ Some cleaning was made, priliminary status was: +++"
    cat $TEMP_FILE >> $LOG_FILE
fi

rm $TEMP_FILE

