#! /bin/sh

TE_RUN_DIR=`pwd`

DISPATCHER=`which $0` 

# which somewhere may give only relative, but not absolute path to the 
# dispatcher directory, therefore the following more safe and portable way
# is chosen.
pushd `dirname $DISPATCHER` >/dev/null
DISPATCHER_DIR=`pwd`
popd >/dev/null

usage()
{
cat <<EOF
Usage: dispatcher.sh [<generic options>] [[<test options> tests ]...
Generic options:
  -q                            Suppress part of output messages

  --conf-dir=<directory>        specify configuration file directory
                                (${TE_BASE}/conf or . by default)

    In configuration files options below <filename> is full name of the
    configuration file or name of the file in the configuration directory.

  --conf-builder=<filename>     Builder config file (builder.conf by default)
  --conf-cs=<filename>          Configurator config file (cs.conf by default)
  --conf-logger=<filename>      Logger config file (logger.conf by default)
  --conf-rcf=<filename>         RCF config file (rcf.conf by default)
  --conf-rgt=<filename>         RGT config file (rgt.conf by default)
  --conf-tester=<filename>      Tester config file (tester.conf by default)

  --env=<filename>              Name of the file to get shell variables

  --live-log                    Run RGT in live mode

  --lock-dir=<directory>        lock file directory (/tmp/te/lock by default)

  --no-builder                  Do not build TE
  --no-tester                   Do not run Tester
  --no-cs                       Do not run Configurator
  --no-rcf                      Do not run RCF
  --no-run                      Do not run Logger, RCF, Configurator and Tester

  --opts=<filename>             Get additional command-line options from file

  --script-tester               Use script Tester with text configuration files
  --vg-tests                    Run tests under valgrind (without by default)
                                May be used with script Tester only

  --cs-print-trees              Print configurator trees.
  --cs-print-diff               Log backup diff unconditionally.

  --build-cs                    Build configurator.
  --build-logger                Build logger.
  --build-rcf                   Build RCF.
  --build-tester                Build tester.
  --build-lib-xxx               Build host library xxx.
  --build-ta-xxx                Build Test Agent xxx.
  --build-log-xxx               Build package with log level 0xFFFF.
  --build-nolog-xxx             Build package with undefined log level.

  --tester-fake                 Do not run any test scripts, just emulate
                                Usefull for configuration debugging
  --tester-nobuild              Do not build Test Suite sources
  --tester-no-cs                Do not interact with Configurator
  --tester-nocfgtrack           Do not track configuration changes
  --tester-nologues             Disable prologues and epilogues globally
  --tester-suite=[NAME:PATH]    Specify location of Test Sutie sources
  --tester-req=[REQ|!REQ]       Requirement to be tested (or excluded,
                                if its first symbol is !)
  --tester-quietskip            Quietly skip tests which do not meet
                                specified requirements
  --tester-run=[PATH]           Run a test item defined by PATH
  --tester-vg=[PATH]            Run test scripts in specified path under
                                valgrind
  --tester-gdb=[PATH]           Run test scripts in specified path under gdb

  --trc-db=<filename>           TRC database to be used
  --trc-tag=<TAG>               Tag to get specific expected results
  --trc-html=<filename>         Name of the file for HTML report
  --trc-brief-html=<filename>   Name of the file for brief HTML report
  --trc-txt=<filename>          Name of the file for text report
                                (by default, it is generated to stdout)
  --trc-quiet                   Do not output total statistics to stdout
  --trc-update                  Update TRC database
  --trc-init                    Initialize TRC database (be careful,
                                TRC database file will be rewritten)

  --vg-engine                   Run RCF, Configurator, Logger and Tester under
                                valgrind (without by default)
  --vg-cs                       Run Configurator under valgrind
  --vg-logger                   Run Logger under valgrind (without by default)
  --vg-rcf                      Run RCF under valgrind (without by default)
                                (without by default)
  --vg-tester                   Run Tester under valgrind (without by default)
  
EOF
#    echo -e '  '--storage='<string>'\\t\\tconfiguration string for the storage
#    echo -e \\t\\t\\t\\twith data to be updated by Dispatcher
#    echo -e '  '--update-files='<list>'\\t\\tupdate files and directories specified
#    echo -e \\t\\t\\t\\tin the '<list>' from the storage
#    echo
#    echo -e '  '--log-storage\\t\\t\\tconfiguration string for the storage
#    echo -e \\t\\t\\t\\twhere raw log should be submitted
#    echo -e '  '--log-storage-dir='<directory>'\\traw log directory in the storage
#    echo -e '  '--log-dir='<directory>'\\t\\t'local directory for raw log (. by default)'
#    echo -e '  '--log-online\\t\\t\\tconvert and print log on stdout during work
#    echo

#    echo -e '  --tester-norandom'\\t\\t'Force to run all tests in defined order as well'
#    echo -e \\t\\t\\t\\t'as to get values of all arguments in defined'
#    echo -e \\t\\t\\t\\t'order.  Usefull for debugging."'
#    echo -e '  --tester-nosimultaneous'\\t'Force to run all tests in series.'
#    echo -e \\t\\t\\t\\t'Usefull for debugging.'
}

exit_with_log()
{
    te_log_archive --storage="${LOG_STORAGE}" \
                   --storage-dir="${LOG_STORAGE_DIR}" ;
    rm -f ${LOCK_DIR}/ds
    rm -rf ${TE_TMP}
    popd >/dev/null
    exit 1
}


# Parse options

EXT_OPTS_PROCESSED=

QUIET=

# No additional Tester options by default
TESTER_OPTS=
# No additional TRV options by default
TRC_OPTS=
# Configurator options
CS_OPTS=
# Building options
BUILDER_OPTS=

LIVE_LOG=

VG_OPTIONS="--tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=32"
VG_TESTS=
VG_RCF=
VG_CS=
VG_LGR=
VG_TESTER=

# Subsystems to be initialized
BUILDER=yes
TESTER=yes
RCF=yes
CONFIGURATOR=yes
LOGGER=yes

# Tester executable extension (set to .sh for script tester)
TESTER_EXT=

# Whether Test Suite should be built
BUILD_TS=yes

# Tests to be started if Tester is not initialized
TESTS=

# Configuration directory
CONF_DIR=

# Directory for raw log file
TE_LOG_DIR=`pwd`

# Configuration files
CONF_BUILDER=builder.conf
CONF_LOGGER=logger.conf
CONF_TESTER=tester.conf
CONF_CONFIGURATOR=configurator.conf
CONF_RCF=rcf.conf
CONF_RGT=

# Storage initialization string for files/directories to be updated
STORAGE=

# Storage files/directories to be updated from the storage
UPDATE_FILES=

# Storage initialization string for log
LOG_STORAGE=

# Location of the log in the storage
LOG_STORAGE_DIR=

# If yes, generate on-line log in the logging directory
LOG_ONLINE=

# Name of the file with test logs to be genetated
RGT_LOG_TXT=log.txt
# Name of the file with plain HTML logs to be genetated
RGT_LOG_HTML_PLAIN=
# Name of the directory for structured HTML logs to be genetated
RGT_LOG_HTML=

# Directory for lock file
#LOCK_DIR=/var/lock/te
# RedHat does not allow to write usual users in /var/lock directory
LOCK_DIR=/tmp/te_lock

#
# Process Dispatcher options.
#
process_opts()
{
    while test -n "$1" ; do
        case $1 in 
            --help ) usage ; exit 0 ;;

            --opts=* )
                EXT_OPTS_PROCESSED=yes
                OPTS="${1#--opts=}" ;
                if test "${OPTS:0:1}" != "/" ; then 
                    OPTS="${CONF_DIR}/${OPTS}" ;
                fi ;
                if test -f ${OPTS} ; then
                    process_opts `cat ${OPTS}` ;
                else
                    echo "File with options ${OPTS} not found" >&2 ;
                    exit 1 ;
                fi
                ;;

            --env=* )
                ENV_FILE="${1#--env=}" ;
                if test "${ENV_FILE:0:1}" != "/" ; then 
                    ENV_FILE="${CONF_DIR}/${ENV_FILE}" ;
                fi ;
                if test -f ${ENV_FILE} ; then
                    . ${ENV_FILE} ;
                else
                    echo "File with environment variables ${ENV_FILE} not found" >&2 ;
                    exit 1 ;
                fi
                ;;

            -q) QUIET=yes ;;
            --live-log)  LIVE_LOG=yes ; TESTER_OPTS="-q ${TESTER_OPTS}" ;;

            --log-txt=*)        RGT_LOG_TXT="${1#--log-txt=}" ;;
            --log-html=*)       RGT_LOG_HTML="${1#--log-html=}" ;;
            --log-plain-html=*) RGT_LOG_HTML_PLAIN="${1#--log-plain-html=}" ;;

            --vg-tests)  VG_TESTS=yes ;;
            --vg-rcf)    VG_RCF=yes ;;
            --vg-cs)     VG_CS=yes ;;
            --vg-logger) VG_LGR=yes ;;
            --vg-tester) VG_TESTER=yes ;;
            --vg-engine) VG_RCF=yes; VG_CS=yes; VG_LGR=yes ; VG_TESTER=yes ;;    
        
            --no-builder) BUILDER= ;;
            --no-tester) TESTER= ;;
            --no-cs) CONFIGURATOR= ;;
            --no-rcf) RCF= ;;
            --no-run) RCF= ; CONFIGURATOR= ; TESTER= ; LOGGER= ;;
            
            --conf-dir=*) CONF_DIR="${1#--conf-dir=}" ;;
            
            --conf-builder=*) CONF_BUILDER="${1#--conf-builder=}" ;;
            --conf-logger=*) CONF_LOGGER="${1#--conf-logger=}" ;;
            --conf-tester=*) CONF_TESTER="${1#--conf-tester=}" ;;
            --conf-cs=*) CONF_CONFIGURATOR="${1#--conf-cs=}" ;;
            --conf-rcf=*) CONF_RCF=${1#--conf-rcf=} ;;
            --conf-rgt=*) CONF_RGT=${1#--conf-rgt=} ;;
            
            --storage=*) STORAGE="${1#--storage=}" ;;
            --update-files=*) UPDATE_FILES="${1#--update-files=}" ;;
            
            --log-storage=*) LOG_STORAGE="${1#--log-storage=}" ; ;;
            --log-storage-dir=*) LOG_STORAGE_DIR="${1#--log-storage-dir=}" ;;
            --log-dir=*) TE_LOG_DIR="${1#--log-dir=}" ;;
            --log-online) LOG_ONLINE=yes ;;
            
            --lock-dir=*) LOCK_DIR="${1#--lock-dir=}" ;;

            --no-ts-build) BUILD_TS= ; TESTER_OPTS="${TESTER_OPTS} --nobuild" ;;

            --script-tester) TESTER_EXT=".sh" ;;

            --tester-*) TESTER_OPTS="${TESTER_OPTS} --${1#--tester-}" ;;

            --trc-db=*) 
                TRC_DB="${1#--trc-db=}" ;
                if test "${TRC_DB:0:1}" != "/" ; then 
                    TRC_DB="${CONF_DIR}/${TRC_DB}" ;
                fi ;
                TRC_OPTS="${TRC_OPTS} --db=${TRC_DB}" ;
                ;;
            --trc-*) TRC_OPTS="${TRC_OPTS} --${1#--trc-}" ;;
    
            --cs-*) CS_OPTS="${CS_OPTS} --${1#--cs-}" ;;

            --build-log=*) 
                BUILDER_OPTS="${BUILDER_OPTS} --pathlog=${1#--build-log=}"
                BUILDER=
                ;;

            --build-nolog=*) 
                BUILDER_OPTS="${BUILDER_OPTS} --pathnolog=${1#--build-nolog=}"
                BUILDER=
                ;;
                
            --build-*) 
                BUILDER_OPTS="${BUILDER_OPTS} --${1#--build-}" 
                BUILDER=
                ;;

            --build=*) 
                BUILDER_OPTS="${BUILDER_OPTS} --path=${1#--build=}"
                BUILDER=
                ;;

            *)  echo "Unknown option: $1" >&2;
                usage ;
                exit 1 ;;
        esac
        shift 1 ;
    done
}


# Export TE_BASE
if test -z "${TE_BASE}" ; then
    if test -e ${DISPATCHER_DIR}/configure.ac ; then
        echo "TE source directory is ${DISPATCHER_DIR} - exporting TE_BASE." ;
        export TE_BASE=${DISPATCHER_DIR} ;
    fi
fi

if test -z "$CONF_DIR" ; then
    if test -n "${TE_BASE}" ; then
        CONF_DIR=${TE_BASE}/conf ;
    else
        CONF_DIR=`pwd` ;
    fi
fi


# Process command-line options
process_opts "$@"


if test -e ${LOCK_DIR}/ds ; then
    echo "Cannot obtain lock" >&2
    exit 2
fi

# Lock directory must be world readable/writable with sticky tag
mkdir -p -m 1777 ${LOCK_DIR}
# Lock file should be writable for creator only
touch ${LOCK_DIR}/ds
if test $? -ne 0 ; then
    echo "Cannot obtain lock" >&2
    exit 2
fi


if test -z "${LOG_STORAGE_DIR}" ; then
    LOG_STORAGE=
fi

if test -z "${LOG_STORAGE}" ; then
    LOG_STORAGE_DIR=
fi

for i in BUILDER LOGGER TESTER CONFIGURATOR RCF RGT ; do
    CONF_FILE=`eval echo '$CONF_'$i` ;
    if test -n "${CONF_FILE}" -a "${CONF_FILE:0:1}" != "/" ; then
        eval CONF_$i="${CONF_DIR}/${CONF_FILE}" ;
    fi
done

if test -z "$TE_BASE" -a -n "$BUILDER" ; then
    echo "Cannot find TE sources for building - exiting." >&2
    rm -f ${LOCK_DIR}/ds
    exit 1
fi


if test -n "$BUILDER" ; then
    # Verifying build directory
    if test -e dispatcher.sh -a -e configure.ac ; then
        mkdir -p build
        TE_BUILD=`pwd`/build
    else
        TE_BUILD=`pwd`
    fi
else
    if test -e build/builder.conf.processed ; then
        TE_BUILD=`pwd`/build
    fi
    if test -z "${TE_BUILD}" ; then
        TE_BUILD=`pwd`
        if test -z "${QUIET}" ; then
            echo "Guessing TE_BUILD=${TE_BUILD}"
        fi
    fi
fi
export TE_BUILD
pushd $TE_BUILD >/dev/null

# Create directory for temporary files
if test -z "$TE_TMP" ; then
    mkdir -p tmp
    export TE_TMP=`pwd`/tmp
fi

export TE_LOG_DIR=${TE_LOG_DIR}
mkdir -p ${TE_LOG_DIR}

# Export TE_INSTALL
TE_PATH=
if test -z "$TE_INSTALL" ; then
    if test -e $DISPATCHER_DIR/configure.ac ; then
        if test -n "${TE_BUILD}" ; then
            TE_INSTALL=${TE_BUILD}/inst
        else
            TE_INSTALL=$DISPATCHER_DIR/build/inst
        fi
    else
        TE_PATH=`dirname ${DISPATCHER_DIR}`
        TE_INSTALL=`dirname ${TE_PATH}`
    fi
    if test -z "${QUIET}" ; then
        echo "Exporting TE installation directory as TE_INSTALL:"
        echo '    '$TE_INSTALL
    fi
    export TE_INSTALL ;
fi

if test -z "${TE_PATH}" ; then
    TMP=`find ${TE_INSTALL} -name dispatcher.sh 2>/dev/null`
    if test -n "${TMP}" ; then
        TMP=`dirname ${TMP}`
        TE_PATH=${TE_PATH/%\/bin/}
        if test x${TE_PATH} = x${TMP} ; then
            echo "dispatcher.sh is found in unexpected path." >&2
            exit_with_log
        fi
    fi
fi    

if test -z "${TE_PATH}" ; then
    if test -e ${CONF_BUILDER} ; then
        TMP=`cat ${CONF_BUILDER} | grep TE_HOST`
        TMP=`echo 'changequote([,]) define([TE_HOST], [host=$1])' "${TMP}" | m4 | grep 'host=' | tail -n 1`
        eval $TMP
    fi
    if test -z "${host}" ; then
        host=`${TE_BASE}/engine/builder/te_discover_host` 
    fi        
    if test -z "$host" ; then
        echo 'Cannot determine host platform.' >&2
        exit_with_log
    fi
    TE_PATH=${TE_INSTALL}/${host}
fi

# Export PATH
if test -z "${QUIET}" ; then
    echo "Exporting path to host executables:"
    echo "    ${TE_PATH}/bin"
fi
export PATH=$PATH:${TE_PATH}/bin ;
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${TE_PATH}/lib


if test -z "$TE_INSTALL_NUT" ; then
    export TE_INSTALL_NUT=${TE_INSTALL}/nuts
fi

if test -z "$TE_INSTALL_SUITE" ; then
    export TE_INSTALL_SUITE=${TE_INSTALL}/suites
fi

TMP=`which te_log_init 2>/dev/null` ;
if test -z "$TMP" ; then
    if test -z "$BUILDER" ; then
        echo "TE core executables are not installed - exiting." >&2
        exit_with_log
    fi
    #Export path to logging, building and storage scripts, which are not installed yet
    export PATH=$PATH:${TE_BASE}/engine/logger:${TE_BASE}/engine/builder:${TE_BASE}/lib/storage
fi

# TODO: update directories from storage

# Intitialize log
te_log_init

# Build Test Environment
TE_BUILD_LOG=${TE_RUN_DIR}/build.log
if test -n "$BUILDER" ; then
    for i in `find $TE_BASE -name configure.ac` ; do 
        pushd `dirname $i` >/dev/null
        if test ! -e configure ; then
            if test -n "${QUIET}" ; then
                echo "Calling aclocal/autoconf/automake in `pwd`" \
                    >>${TE_BUILD_LOG}
            else
                echo "Calling aclocal/autoheader/autoconf/automake in `pwd`"
            fi
            aclocal -I ${TE_BASE}/auxdir || exit_with_log
            if cat configure.ac | grep -q AC_CONFIG_HEADER ; then
                autoheader || exit_with_log
            fi
            autoconf || exit_with_log
            automake || exit_with_log
        fi
        popd >/dev/null
    done    
    cd ${TE_BUILD}
    # FINAL ${TE_BASE}/configure --prefix=${TE_INSTALL} --with-config=${CONF_BUILDER} 2>&1 | te_builder_log
    if test -n "${QUIET}" ; then
        ${TE_BASE}/configure -q --prefix=${TE_INSTALL} \
            --with-config=${CONF_BUILDER} >${TE_BUILD_LOG} || \
            exit_with_log ;
        make te >>${TE_BUILD_LOG} || exit_with_log ;
    else
        ${TE_BASE}/configure -q --prefix=${TE_INSTALL} \
            --with-config=${CONF_BUILDER} || exit_with_log ;
        make te || exit_with_log ;
    fi
fi

te_builder_opts $BUILDER_OPTS || exit_with_log

# Goto the directory where the script was called
popd >/dev/null


if test -n "${SUITE_SOURCES}" -a -n "${BUILD_TS}" ; then
    te_build_suite `basename ${SUITE_SOURCES}` $SUITE_SOURCES || exit_with_log
fi    

rm -f valgrind.* vg.*

# Ignore Ctrl-C
trap "" SIGINT

myecho() {
    if test -z "$LIVE_LOG" ; then
        echo $1
    fi
}

# Run RGT in live mode in background
if test -n "$LIVE_LOG" ; then
    rgt-conv -m live -f tmp_raw_log &
    LIVE_LOG_PID=$!
fi

te_log_message Engine Dispatcher "Starting TEN applications"

if test -n "$RCF" ; then
    te_log_message Engine Dispatcher "Start RCF"
    myecho "--->>> Start RCF"
    if test -n "$VG_RCF" ; then
        valgrind $VG_OPTIONS te_rcf "${CONF_RCF}" 2>valgrind.rcf &
    else
        te_rcf "${CONF_RCF}" &
    fi
fi

if test -n "$LOGGER" ; then
    te_log_message Engine Dispatcher "Start Logger"
    myecho "--->>> Start Logger"
    if test -n "$VG_LGR" ; then
        valgrind $VG_OPTIONS te_logger "${CONF_LOGGER}" 2>valgrind.logger &
    else
        te_logger "${CONF_LOGGER}" &
    fi
fi

if test -n "$CONFIGURATOR" ; then
    te_log_message Engine Dispatcher "Start Configurator"
    myecho "--->>> Start Configurator"
    if test -n "$VG_CS" ; then
        valgrind $VG_OPTIONS te_cs "${CONF_CONFIGURATOR}" \
            ${CS_OPTS} 2>valgrind.cs &
    else
        te_cs "${CONF_CONFIGURATOR}" ${CS_OPTS} &
    fi
    CS_PID=$!
fi

if test -n "$TESTER" ; then
    myecho "--->>> Start Tester"
    if test -n "$VG_TESTER" ; then
        VG_TESTS=$VG_TESTS valgrind $VG_OPTIONS te_tester${TESTER_EXT} \
            ${TESTER_OPTS} "${CONF_TESTER}" 2>valgrind.tester
    else
        VG_TESTS=$VG_TESTS te_tester${TESTER_EXT} ${TESTER_OPTS} \
            "${CONF_TESTER}" 
    fi
fi

if test -n "$CONFIGURATOR" ; then
    te_log_message Engine Dispatcher "Shutdown Configurator"
    myecho "--->>> Shutdown Configurator"
    te_cs_shutdown || kill $CS_PID
fi

if test -n "$LOGGER" ; then
    te_log_message Engine Dispatcher "Flush log"
    myecho "--->>> Log FLUSH"
    te_log_flush
fi

if test -n "$RCF" ; then
    te_log_message Engine Dispatcher "Shutdown RCF"
    myecho "--->>> Shutdown RCF"
    te_rcf_shutdown 
fi

#TODO: TCE

if test -n "$LOGGER" ; then
    te_log_message Engine Dispatcher "Shutdown Logger"
    myecho "--->>> Shutdown Logger"
    te_log_shutdown
fi

#TODO: TCE

# Wait for RGT in live mode finish
if test -n "$LIVE_LOG" ; then
    wait ${LIVE_LOG_PID}
fi

#te_log_archive --storage="${LOG_STORAGE}" --storage-dir=="${LOG_STORAGE_DIR}" ;

# Create log file in text representation
if test -n "${CONF_RGT}" ; then
    CONF_RGT="-c ${CONF_RGT}"
fi
if test -n "${RGT_LOG_TXT}" -o -n "${RGT_LOG_HTML_PLAIN}" ; then
    # Generate XML log do not taking into account control messages
    rgt-conv -m postponed ${CONF_RGT} \
        -f ${TE_LOG_DIR}/tmp_raw_log -o tmp_raw_log.xml
    if test $? -eq 0 ; then
        if test -n "${RGT_LOG_TXT}" ; then
            rgt-xml2text -f tmp_raw_log.xml -o ${RGT_LOG_TXT}
        fi
        if test -n "${RGT_LOG_HTML_PLAIN}" ; then
            rgt-xml2html -f tmp_raw_log.xml -o ${RGT_LOG_HTML_PLAIN}
        fi
    fi
fi
if test -n "${RGT_LOG_HTML}" ; then
    # Generate XML log taking into account control messages
    rgt-conv --no-cntrl-msg -m postponed ${CONF_RGT} \
        -f ${TE_LOG_DIR}/tmp_raw_log -o tmp_raw_log2.xml
    if test $? -eq 0 ; then
        rgt-xml2html -m tmp_raw_log2.xml ${RGT_LOG_HTML}
    fi
fi

# Run TRC, if any its option is provided
if test -n "${TRC_OPTS}" ; then
    te_trc.sh ${TRC_OPTS} ${TE_LOG_DIR}/tmp_raw_log
fi

rm -f ${LOCK_DIR}/ds
rm -rf ${TE_TMP}

exit 0
