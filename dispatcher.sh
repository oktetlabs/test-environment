#! /bin/sh

STARTING_DIR=`pwd`
DISPATCHER=`which $0` 

# which somewhere may give only relative, but not absolute path to the 
# dispatcher directory, therefore the following more safe and portable way
# is chosen.
DISP_DIRNAME=`dirname $DISPATCHER`
pushd $DISP_DIRNAME >/dev/null
DISPATCHER_DIR=`pwd`
popd >/dev/null

usage()
{
    echo Usage: dispatcher.sh ['<'generic options'>'] [['<'test options'>' tests ]...
    echo -e Generic options:
    echo -e '  '-q\\t\\t\\t\\tSuppress part of output messages
    echo -e '  '--live-log\\t\\t\\tRun RGT in live mode
    echo
    echo -e '  '--no-builder\\t\\t\\t'Do not build TE'
    echo -e '  '--no-tester\\t\\t\\t'Do not run Tester'
    echo -e '  '--no-cs\\t\\t\\t'Do not run Configurator'
    echo -e '  '--no-rcf\\t\\t\\t'Do not run RCF'
    echo -e '  '--no-run\\t\\t\\t'Do not run RCF, Configurator and Tester'
    echo
    echo -e '  '--conf-dir='<directory>'\\t'specify configuration file directory'
    echo -e \\t\\t\\t\\t'(${TE_BASE}/storage/conf or '.' by default)'
    echo
    echo -e '    In configuration files options below <filename> is full name of the'
    echo -e '    configuration file or name of the file in the configuration directory.'
    echo
    echo -e '  '--conf-builder='<filename>'\\t'Builder config file (builder.conf by default)'
    echo -e '  '--conf-logger='<filename>'\\t'Logger config file (logger.conf by default)'
    echo -e '  '--conf-tester='<filename>'\\t'Tester config file (tester.conf by default)'
    echo -e '  '--conf-cs='<filename>'\\t\\t'Configurator config file (cs.conf by default)'
    echo -e '  '--conf-rcf='<filename>'\\t\\t'RCF config file (rcf.conf by default)'
    echo -e '  '--conf-rgt='<filename>'\\t\\t'RGT config file (rgt.conf by default)'
    echo
    echo -e '  '--storage='<string>'\\t\\tconfiguration string for the storage
    echo -e \\t\\t\\t\\twith data to be updated by Dispatcher
    echo -e '  '--update-files='<list>'\\t\\tupdate files and directories specified
    echo -e \\t\\t\\t\\tin the '<list>' from the storage
    echo
    echo -e '  '--log-storage\\t\\t\\tconfiguration string for the storage
    echo -e \\t\\t\\t\\twhere raw log should be submitted
    echo -e '  '--log-storage-dir='<directory>'\\traw log directory in the storage 
    echo -e '  '--log-dir='<directory>'\\t\\t'local directory for raw log (. by default)'
    echo -e '  '--log-online\\t\\t\\tconvert and print log on stdout during work
    echo    
    echo -e '  '--lock-dir='<directory>'\\t'lock file directory (/tmp/te/lock by default)'
    echo
    echo -e '  '--vg-rcf\\t\\t\\t'Run RCF under valgrind (without by default)'
    echo -e '  '--vg-cs\\t\\t\\t'Run Configurator under valgrind'
    echo -e \\t\\t\\t\\t'(without by default)'
    echo -e '  '--vg-logger\\t\\t\\t'Run Logger under valgrind (without by default)'
    echo -e '  '--vg-tester\\t\\t\\t'Run Tester under valgrind (without by default)'
    echo -e '  '--vg-engine\\t\\t\\t'Run RCF, Configurator, Logger and Tester under'
    echo -e \\t\\t\\t\\t'valgrind (without by default)'
    echo
    echo -e '  '--script-tester\\t\\t'Use script Tester with text configuration files'
    echo -e '  '--vg-tests\\t\\t\\t'Run tests under valgrind (without by default).'
    echo -e \\t\\t\\t\\t'May be used with script Tester only.'
    echo
    echo -e '  --tester-fake'\\t\\t\\t'Do not run any test scripts, just emulate test'
    echo -e '  --tester-nobuild'\\t\\t'Do not build Test Suite sources'
    echo -e '  --tester-no-cs'\\t\\t'Do not interact with Configurator.'
    echo -e '  --tester-nocfgtrack'\\t\\t'Do not track configuration changes.'
    echo -e '  --tester-nologues'\\t\\t'Disable prologues and epilogues globally.'
#    echo -e '  --tester-norandom'\\t\\t'Force to run all tests in defined order as well'
#    echo -e \\t\\t\\t\\t'as to get values of all arguments in defined'
#    echo -e \\t\\t\\t\\t'order.  Usefull for debugging."'
#    echo -e '  --tester-nosimultaneous'\\t'Force to run all tests in series.'
#    echo -e \\t\\t\\t\\t'Usefull for debugging.'
    echo -e '  --tester-suite=[NAME:PATH]'\\t'Specify location of Test Sutie sources.'
    echo -e '  --tester-req=[REQ|!REQ]'\\t'Requirement to be tested (or excluded,'
    echo -e '  --tester-run=[PATH]'\\t\\t'Run a test item defined by PATH.'
    echo -e \\t\\t\\t\\t'if its first symbol is !).'
    echo -e \\t\\t\\t\\t'failure. Usefull for configuration debugging.'
    echo -e '  --tester-vg=[PATH]'\\t\\t'Run test scripts in specified path under'
    echo -e \\t\\t\\t\\t'valgrind.'
    echo -e '  --tester-gdb=[PATH]'\\t\\t'Run test scripts in specified path under gdb.'

    echo
}

exit_with_log()
{
    te_log_archive --storage="${LOG_STORAGE}" \
                   --storage-dir="${LOG_STORAGE_DIR}" ;
    rm -f ${LOCK_DIR}/ds
    rm -rf ${TE_TMP}
    exit 1 ;
}

# Parse options

QUIET=

# No additional Tester options by default
TESTER_OPTS=

LIVE_LOG=

VG_OPTIONS="--tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=8"
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
CONF_RGT=rgt.conf

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

# Directory for lock file
#LOCK_DIR=/var/lock/te
# RedHat does not allow to write usual users in /var/lock directory
LOCK_DIR=/tmp/te_lock

while test -n "$1" ; do
    case $1 in 
        --help ) usage ; exit 0 ;;

        -q) QUIET=yes ;;
        --live-log)  LIVE_LOG=yes ; TESTER_OPTS="-q ${TESTER_OPTS}" ;;

        --vg-tests)  VG_TESTS=yes ;;
        --vg-rcf)    VG_RCF=yes ;;
        --vg-cs)     VG_CS=yes ;;
        --vg-logger) VG_LGR=yes ;;
        --vg-tester) VG_TESTER=yes ;;
        --vg-engine) VG_RCF=yes; VG_CS=yes; VG_LGR=yes ;;    
    
        --no-builder) BUILDER= ;;
        --no-tester) TESTER= ;;
        --no-cs) CONFIGURATOR= ;;
        --no-rcf) RCF= ;;
        --no-run) RCF= ; CONFIGURATOR= ; TESTER= ;;
        
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

        *)  echo "Unknown option: $1" ;
            usage ;
            exit 1 ;;
    esac
    shift 1 ;
done


if test -e ${LOCK_DIR}/ds ; then
    echo "Cannot obtain lock"
    exit 2
fi

# Lock directory must be world readable/writable with sticky tag
mkdir -p -m 1777 ${LOCK_DIR}
# Lock file should be writable for creator only
touch ${LOCK_DIR}/ds
if test $? -ne 0 ; then
    echo "Cannot obtain lock"
    exit 2
fi


if test -z "${LOG_STORAGE_DIR}" ; then
    LOG_STORAGE=
fi

if test -z "${LOG_STORAGE}" ; then
    LOG_STORAGE_DIR=
fi

# Export TE_BASE
if test -z "${TE_BASE}" ; then
    if test -e ${DISPATCHER_DIR}/configure.ac ; then
        echo "TE source directory is ${DISPATCHER_DIR} - exporting TE_BASE." ;
        export TE_BASE=${DISPATCHER_DIR} ;
    fi
fi

if test -z "$CONF_DIR" ; then
    if test -n "${TE_BASE}" ; then
        CONF_DIR=${TE_BASE}/storage/conf ;
    else
        CONF_DIR=`pwd` ;
    fi
fi

for i in BUILDER LOGGER TESTER CONFIGURATOR RCF RGT ; do
    CONF_FILE=`eval echo '$CONF_'$i` ;
    if test "${CONF_FILE:0:1}" != "/" ; then
        eval CONF_$i="${CONF_DIR}/${CONF_FILE}" ;
    fi
done

if test -z "$TE_BASE" -a -n "$BUILDER" ; then
    echo "Cannot find TE sources for building - exiting." ;
    rm -f ${LOCK_DIR}/ds
    exit 1 ;
fi


if test -n "$BUILDER" ; then
    # Verifiing build directory
    if test -e dispatcher.sh -a -e configure.ac ; then
        mkdir -p build ;
        cd build ;
    fi
    export TE_BUILD=`pwd`
else
    if test -e build/builder.conf.processed ; then
        export TE_BUILD=`pwd`/build
    fi
    if test -z "${TE_BUILD}" ; then
        export TE_BUILD=`pwd`
        if test -z "${QUIET}" ; then
            echo "Guessing TE_BUILD=${TE_BUILD}"
        fi
    fi
fi

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
            echo "dispatcher.sh is found in unexpected path."
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
        CONFIG_GUESS=`find /usr/share/automake* -name config.guess | tail -n 1` 
        host=`$CONFIG_GUESS` 
    fi        
    if test -z "$host" ; then
        echo 'Cannot determine host platform.'
        rm -f ${LOCK_DIR}/ds
        rm -rf ${TE_TMP}
        exit 1
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
        echo "TE core executables are not installed - exiting." ;
        rm -f ${LOCK_DIR}/ds
        rm -rf ${TE_TMP}
        exit 1 ;
    fi
    #Export path to logging, building and storage scripts, which are not installed yet
    export PATH=$PATH:${TE_BASE}/engine/logger:${TE_BASE}/engine/builder:${TE_BASE}/lib/storage
fi

# TODO: update directories from storage

# Intitialize log
te_log_init

# Build Test Environment
if test -n "$BUILDER" ; then
    cd $TE_BASE    
    for i in `find . -name configure.ac` ; do 
        DIRNAME=`dirname $i`
        CURRDIR=`pwd`
        cd $DIRNAME
        if test ! -e configure ; then
            echo Calling aclocal/autoconf/automake in `pwd`
            aclocal -I ${TE_BASE}/aux || exit_with_log
            autoconf || exit_with_log
            automake || exit_with_log
        fi
        cd $CURRDIR
    done    
    cd ${TE_BUILD}
    # FINAL ${TE_BASE}/configure --prefix=${TE_INSTALL} --with-config=${CONF_BUILDER} 2>&1 | te_builder_log
    if test -n "${QUIET}" ; then
        ${TE_BASE}/configure -q --prefix=${TE_INSTALL} \
            --with-config=${CONF_BUILDER} >build.log || exit_with_log ;
        make te >>build.log || exit_with_log ;
    else
        ${TE_BASE}/configure -q --prefix=${TE_INSTALL} \
            --with-config=${CONF_BUILDER} || exit_with_log ;
        make te || exit_with_log ;
    fi
fi


if test -n "${SUITE_SOURCES}" -a -n "${BUILD_TS}" ; then
    te_build_suite `basename ${SUITE_SOURCES}` $SUITE_SOURCES || exit_with_log
fi    

cd ${STARTING_DIR}

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
        valgrind $VG_OPTIONS rcf "${CONF_RCF}" 2>valgrind.rcf &
    else
        rcf "${CONF_RCF}" &
    fi
fi

te_log_message Engine Dispatcher "Start Logger"
myecho "--->>> Start Logger"
if test -n "$VG_LGR" ; then
    valgrind $VG_OPTIONS te_logger "${CONF_LOGGER}" 2>valgrind.logger &
else
    te_logger "${CONF_LOGGER}" &
fi

if test -n "$CONFIGURATOR" ; then
    te_log_message Engine Dispatcher "Start Configurator"
    myecho "--->>> Start Configurator"
    if test -n "$VG_CS" ; then
        valgrind $VG_OPTIONS configurator "${CONF_CONFIGURATOR}" \
            2>valgrind.cfg &
    else
        configurator "${CONF_CONFIGURATOR}" &
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
    conf_shutdown || kill $CS_PID
fi

te_log_message Engine Dispatcher "Flush log"
myecho "--->>> Log FLUSH"
te_log_flush

if test -n "$RCF" ; then
    te_log_message Engine Dispatcher "Shutdown RCF"
    myecho "--->>> Shutdown RCF"
    rcf_shutdown 
fi

#TODO: TCE

te_log_message Engine Dispatcher "Shutdown Logger"
myecho "--->>> Shutdown Logger"
te_log_shutdown

#TODO: TCE

# Wait for RGT in live mode finish
if test -n "$LIVE_LOG" ; then
    wait ${LIVE_LOG_PID}
fi

#te_log_archive --storage="${LOG_STORAGE}" --storage-dir=="${LOG_STORAGE_DIR}" ;

# Create log file in text representation
rgt-conv -m postponed -f tmp_raw_log -o tmp_raw_log.xml
rgt-xml2text -f tmp_raw_log.xml -o tmp_raw_log.txt
rgt-xml2html -f tmp_raw_log.xml -o tmp_raw_log.html
rm -f tmp_raw_log.xml

rm -f ${LOCK_DIR}/ds
rm -rf ${TE_TMP}

exit 0
