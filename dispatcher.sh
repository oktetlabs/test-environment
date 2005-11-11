#! /bin/sh

TE_RUN_DIR="${PWD}"

DISPATCHER="$(which $0)"
while test -L "$DISPATCHER" ; do
    DISPATCHER="$(dirname "$DISPATCHER")/$(readlink "${DISPATCHER}")"
done
pushd "$(dirname $DISPATCHER)" >/dev/null
DISPATCHER_DIR="${PWD}"
popd >/dev/null

usage()
{
cat <<EOF
USAGE: dispatcher.sh [<generic options>] [[<test options> tests ]...
Generic options:
  -q                            Suppress part of output messages

  --daemon[=<PID>]              Run/use TE engine daemons
  --shutdown[=<PID>]            Shut down TE engine daemons on exit

  --conf-dir=<directory>        specify configuration file directory
                                (\${TE_BASE}/conf or . by default)

    In configuration files options below <filename> is full name of the
    configuration file or name of the file in the configuration directory.

  --conf-builder=<filename>     Builder config file (builder.conf by default)
  --conf-cs=<filename>          Configurator config file (cs.conf by default)
  --conf-logger=<filename>      Logger config file (logger.conf by default)
  --conf-rcf=<filename>         RCF config file (rcf.conf by default)
  --conf-rgt=<filename>         RGT config file (rgt.conf by default)
  --conf-tester=<filename>      Tester config file (tester.conf by default)
  --conf-nut=<filename>         NUT config file (nut.conf by default)

  --env=<filename>              Name of the file to get shell variables

  --live-log                    Run RGT in live mode

  --log-html=<dirname>          Name of the directory with structured HTML logs
                                to be generated (do not generate by default)
  --log-plain-html=<filename>   Name of the file with plain HTML logs
                                to be generated (do not generate by default)
  --log-txt=<filename>          Name of the file with logs in text format
                                to be generated (log.txt by default)

  --no-builder                  Do not build TE
  --no-nuts-build               Do not build NUTs
  --no-tester                   Do not run Tester
  --no-cs                       Do not run Configurator
  --no-rcf                      Do not run RCF
  --no-run                      Do not run Logger, RCF, Configurator and Tester
  --no-autotool                 Do not try to perform autoconf/automake after
                                package configure failure

  --opts=<filename>             Get additional command-line options from file

  --script-tester               Use script Tester with text configuration files
  --vg-tests                    Run tests under valgrind (without by default)
                                May be used with script Tester only

  --cs-print-trees              Print configurator trees.
  --cs-log-diff                 Log backup diff unconditionally.

  --build=path                  Build package specified in the path.
  --build-log=path              Build package with log level 0xFFFF.
  --build-nolog=path            Build package with undefined log level.
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
  --tester-suite=NAME:PATH      Specify location of Test Sutie sources
  --tester-req=[REQ|!REQ]       Requirement to be tested (or excluded,
                                if its first symbol is !)
  --tester-quietskip            Quietly skip tests which do not meet
                                specified requirements
  --tester-run=TEST_PATH        Run a test item defined by TEST_PATH:
                                  TEST_PATH   := <test_path>
                                  <test_path> := <test_spec>[/<test_path>]
                                  <test_spec> := <test_name>[:<args>]
                                  <args>      := <arg>[,<args>]
                                  <arg>       := <arg_name>=<arg_value>
  --tester-vg=TEST_PATH         Run test scripts in specified path (see 
                                --tester-run) under valgrind
  --tester-gdb=TEST_PATH        Run test scripts in specified path (see
                                --tester-run) under gdb

  --trc-db=<filename>           TRC database to be used
  --trc-tag=<TAG>               Tag to get specific expected results
  --trc-html=<filename>         Name of the file for HTML report
  --trc-brief-html=<filename>   Name of the file for brief HTML report
  --trc-html-header=<filename>  Name of the file with header for all HTML
                                reports.
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

  --tcer-ignore-dirs            Respect only base file names when doing TCE
                                report (useful when a given file is used in
                                different contexts, e. g. in a user-space
                                library and a kernel module, and you want total
                                statistics. Note, however, that this will lead
                                to unexpected results if there are actually
                                files with equal base names in different
                                directories.
  --tcer-exclude=<pattern>      Ignore files with names matching <pattern>
                                when doing TCE report generation.
                                <pattern> is an expr(1)-style pattern
  --tces-totals-to=<filename>   If this option is present, <filename> will be
                                generated containing totals in text format
  --tces-modules                List of modules to include into TCE summary
  --tces-conditionals           List of conditionals to exclude from 
                                TCE summary
  --tces-datadir=<dir>          Directory for TCE summary results splitted
                                into many files
  --tces-sort-by=<mode>         Sort TCE summary table by covered branch
                                percentage (mode=branches, the default),
                                covered line percentage (mode=lines),
                                or source file names (mode=sources)

EOF
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
    rm -rf ${TE_TMP}
    popd >/dev/null
    exit 1
}


# Parse options

EXT_OPTS_PROCESSED=

QUIET=
DAEMON=
SHUTDOWN=yes

# No additional Tester options by default
TESTER_OPTS=
# No additional TRC options by default
TRC_OPTS=
# No additional TCE report options by default
TCER_OPTS=
# No additional TCE summary options by default
TCES_OPTS=
# Configurator options
CS_OPTS=
# Building options
BUILDER_OPTS=

LIVE_LOG=

VG_OPTIONS="--tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=32"
VG_TESTS=
VG_RCF=
VG_CS=
VG_LOGGER=
VG_TESTER=

# Subsystems to be initialized
BUILDER=yes
TESTER=yes
RCF=yes
CS=yes
LOGGER=yes

# Tester executable extension (set to .sh for script tester)
TESTER_EXT=

# Whether Test Suite should be built
BUILD_TS=yes

# Configuration directory
CONF_DIR=

# Directory for raw log file
TE_LOG_DIR="${TE_RUN_DIR}"

# Configuration files
CONF_BUILDER=builder.conf
CONF_LOGGER=logger.conf
CONF_TESTER=tester.conf
CONF_CS=configurator.conf
CONF_RCF=rcf.conf
CONF_RGT=
CONF_NUT=nut.conf

# Whether NUTs processing is requested
DO_NUTS=
# Whether NUTs should be built
BUILD_NUTS=yes

# If yes, generate on-line log in the logging directory
LOG_ONLINE=

# Name of the file with test logs to be genetated
RGT_LOG_TXT=log.txt
# Name of the file with plain HTML logs to be genetated
RGT_LOG_HTML_PLAIN=
# Name of the directory for structured HTML logs to be genetated
RGT_LOG_HTML=

#
# Process Dispatcher options.
#
process_opts()
{
    while test -n "$1" ; do
        case $1 in 
            --help ) usage ; exit 0 ;;

            --daemon=* )    DAEMON="${1#--daemon=}" ; SHUTDOWN= ;;
            --daemon   )    SHUTDOWN= ;;
            --shutdown=* )  DAEMON="${1#--shutdown=}" ; SHUTDOWN=yes ;;
            --shutdown )    SHUTDOWN=yes ;;

            --opts=* )
                EXT_OPTS_PROCESSED=yes
                OPTS="${1#--opts=}" ;
                if test "${OPTS:0:1}" != "/" ; then 
                    OPTS="${CONF_DIR}/${OPTS}" ;
                fi ;
                if test -f ${OPTS} ; then
                    process_opts $(cat ${OPTS}) ;
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
            --vg-logger) VG_LOGGER=yes ;;
            --vg-tester) VG_TESTER=yes ;;
            --vg-engine) VG_RCF=yes ;
                         VG_CS=yes ;
                         VG_LOGGER=yes ;
                         VG_TESTER=yes ;;
        
            --tcer-*) TCER_OPTS="${TCER_OPTS} --${1#--tcer-}" ;;
            --tces-*) TCES_OPTS="${TCES_OPTS} --${1#--tces-}" ;;

            --no-builder) BUILDER= ;;
            --no-nuts-build) BUILD_NUTS= ;;
            --no-tester) TESTER= ;;
            --no-cs) CS= ;;
            --no-rcf) RCF= ;;
            --no-run) RCF= ; CS= ; TESTER= ; LOGGER= ;;
            --no-autotool) TE_NO_AUTOTOOL=yes ;;
            
            --conf-dir=*) CONF_DIR="${1#--conf-dir=}" ;;
            
            --conf-builder=*) CONF_BUILDER="${1#--conf-builder=}" ;;
            --conf-logger=*) CONF_LOGGER="${1#--conf-logger=}" ;;
            --conf-tester=*) CONF_TESTER="${1#--conf-tester=}" ;;
            --conf-cs=*) CONF_CS="${1#--conf-cs=}" ;;
            --conf-rcf=*) CONF_RCF=${1#--conf-rcf=} ;;
            --conf-rgt=*) CONF_RGT=${1#--conf-rgt=} ;;
            --conf-nut=*) DO_NUTS=yes ; CONF_NUT="${1#--conf-nut=}" ;;
            
            --log-dir=*) TE_LOG_DIR="${1#--log-dir=}" ;;
            --log-online) LOG_ONLINE=yes ;;
            
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
        CONF_DIR="${PWD}" ;
    fi
fi


# Process command-line options
CMD_LINE_OPTS="$@"
process_opts "$@"

if test -z "$TE_BASE" -a -n "$BUILDER" ; then
    echo "Cannot find TE sources for building - exiting." >&2
    exit 1
fi

export TE_NO_AUTOTOOL

for i in BUILDER LOGGER TESTER CS RCF RGT NUT ; do
    CONF_FILE=$(eval echo '$CONF_'$i) ;
    if test -n "${CONF_FILE}" -a "${CONF_FILE:0:1}" != "/" ; then
        eval CONF_$i="${CONF_DIR}/${CONF_FILE}" ;
    fi
done

# Create directory for temporary files
if test -z "$TE_TMP" ; then
    export TE_TMP="${TE_RUN_DIR}/te_tmp"
    mkdir -p "${TE_TMP}" || exit 1
fi
# May be it should be passed to TE Logger only...
export TE_LOGGER_PID_FILE="${TE_TMP}/te_logger.pid"

if test -z "$TE_BUILD" ; then
    if test -n "$BUILDER" ; then
        # Verifying build directory
        if test -e dispatcher.sh -a -e configure.ac ; then
            mkdir -p build
            TE_BUILD="${PWD}"/build
        else
            TE_BUILD="${PWD}"
        fi
    else
        if test -e build/builder.conf.processed ; then
            TE_BUILD="${PWD}"/build
        fi
        if test -z "${TE_BUILD}" ; then
            TE_BUILD="${PWD}"
            if test -z "${QUIET}" ; then
                echo "Guessing TE_BUILD=${TE_BUILD}"
            fi
        fi
    fi
fi
export TE_BUILD
pushd "${TE_BUILD}" >/dev/null

export TE_LOG_DIR="${TE_LOG_DIR}"
mkdir -p "${TE_LOG_DIR}"
export TE_LOG_RAW="${TE_LOG_RAW:-${TE_LOG_DIR}/tmp_raw_log}"

# Export TE_INSTALL
if test -z "$TE_INSTALL" ; then
    if test -e "$DISPATCHER_DIR/configure.ac" ; then
        if test -n "${TE_BUILD}" ; then
            TE_INSTALL="${TE_BUILD}/inst"
        else
            TE_INSTALL="$DISPATCHER_DIR/build/inst"
        fi
    else
        TE_PATH="$(dirname "${DISPATCHER_DIR}")"
        TE_INSTALL="$(dirname "${TE_PATH}")"
    fi
    if test -z "${QUIET}" ; then
        echo "Exporting TE installation directory as TE_INSTALL:"
        echo '    '$TE_INSTALL
    fi
    export TE_INSTALL
fi

if test -z "${TE_PATH}" ; then
    if test -e "${CONF_BUILDER}" ; then
        tmp=`cat "${CONF_BUILDER}" | grep TE_HOST`
        tmp=`echo 'changequote([,]) define([TE_HOST], [host=$1])' "${tmp}" | m4 | grep 'host=' | tail -n 1`
        eval $tmp
    fi
    if test -z "${host}" -a -n "${TE_BASE}" ; then
        host=$(${TE_BASE}/engine/builder/te_discover_host)
    fi
    if test -z "${host}" -a -x "${DISPATCHER_DIR}/te_discover_host" ; then
        host=$(${DISPATCHER_DIR}/te_discover_host)
    fi
    if test -z "$host" ; then
        echo 'Cannot determine host platform.' >&2
        exit_with_log
    fi
    TE_PATH="${TE_INSTALL}/${host}"
fi

# Export PATH
if test -z "${QUIET}" ; then
    echo "Exporting path to host executables:"
    echo "    ${TE_PATH}/bin"
fi
export TE_PATH
export PATH="${TE_PATH}/bin:$PATH"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${TE_PATH}/lib"


if test -z "$TE_INSTALL_NUT" ; then
    export TE_INSTALL_NUT="${TE_INSTALL}/nuts"
fi

if test -z "$TE_INSTALL_SUITE" ; then
    export TE_INSTALL_SUITE="${TE_INSTALL}/suites"
fi

if test -z "$(which te_log_init 2>/dev/null)" ; then
    if test -z "$BUILDER" ; then
        echo "TE core executables are not installed - exiting." >&2
        exit_with_log
    fi
    #Export path to logging and building scripts, which are not installed yet
    export PATH="$PATH:${TE_BASE}/engine/logger:${TE_BASE}/engine/builder"
fi

# Intitialize log
test -n "${DAEMON}" || te_log_init
te_log_message Engine Dispatcher "Command-line options: ${CMD_LINE_OPTS}"

# Log TRC tool options
if test -n "${TRC_OPTS}" ; then
    te_log_message Engine Dispatcher "TRC options:${TRC_OPTS}"
fi

# Build Test Environment
TE_BUILD_LOG="${TE_RUN_DIR}/build.log"
if test -n "$BUILDER" ; then
    pushd "${TE_BASE}" >/dev/null
    if test ! -e configure ; then
        if test -n "${QUIET}" ; then
            echo "Calling aclocal/autoconf/automake in ${PWD}" \
                >>"${TE_BUILD_LOG}"
        else
            echo "Calling aclocal/autoconf/automake in ${PWD}"
        fi
        aclocal -I "${TE_BASE}/auxdir" || exit_with_log
        autoconf || exit_with_log
        automake || exit_with_log
    fi
    popd >/dev/null
    # FINAL ${TE_BASE}/configure --prefix=${TE_INSTALL} --with-config=${CONF_BUILDER} 2>&1 | te_builder_log
    if test -n "${QUIET}" ; then
        ${TE_BASE}/configure -q --prefix="${TE_INSTALL}" \
            --with-config="${CONF_BUILDER}" >"${TE_BUILD_LOG}" || \
            exit_with_log ;
        make install >>"${TE_BUILD_LOG}" || exit_with_log ;
    else
        ${TE_BASE}/configure -q --prefix="${TE_INSTALL}" \
            --with-config="${CONF_BUILDER}" || exit_with_log ;
        make install || exit_with_log ;
    fi
fi

if test -n "${QUIET}" ; then
    te_builder_opts --quiet="${TE_BUILD_LOG}" $BUILDER_OPTS || exit_with_log
else
    te_builder_opts $BUILDER_OPTS || exit_with_log
fi

# Goto the directory where the script was called ${TE_RUN_DIR}
popd >/dev/null

if test -n "${SUITE_SOURCES}" -a -n "${BUILD_TS}" ; then
    te_build_suite `basename ${SUITE_SOURCES}` $SUITE_SOURCES || exit_with_log
fi


if test -n "${DO_NUTS}" ; then
    if test ! -e "${CONF_NUT}" ; then
        echo "Specified NUTs configuration file does not exists:" >&2
        echo '    '"${CONF_NUT}" >&2
        exit_with_log
    fi
elif test -e "${CONF_NUT}" ; then
    # If NUT configuration file is not specified explicitly,
    # but default exists, then use it
    DO_NUTS=yes
fi

if test -n "${DO_NUTS}" -a -n "${BUILD_NUTS}" ; then
    if test -z "${QUIET}" ; then
        te_build_nuts "${CONF_NUT}" || exit_with_log
    else
        TE_BUILD_NUTS_LOG="${TE_RUN_DIR}/build_nuts.log"
        te_build_nuts "${CONF_NUT}" >"${TE_BUILD_NUTS_LOG}" || exit_with_log
    fi
fi    


rm -f valgrind.* vg.*

# Ignore Ctrl-C when daemons are started
trap "" SIGINT

myecho() {
    if test -z "$LIVE_LOG" ; then
        echo $*
    fi
}

if test -n "${DAEMON}" ; then TE_ID=${DAEMON} ; else TE_ID=$$ ; fi
export TE_RCF="TE_RCF_${TE_ID}"
export TE_LOGGER="TE_LOGGER_${TE_ID}"
export TE_CS="TE_CS_${TE_ID}"

# Run RGT in live mode in background
if test -n "${LIVE_LOG}" ; then
    rgt-conv -m live -f "${TE_LOG_RAW}" &
    LIVE_LOG_PID=$!
fi

te_log_message Engine Dispatcher "Starting TEN applications"
START_OK=0

LOGGER_NAME=Logger
LOGGER_EXEC=te_logger
LOGGER_SHUT=te_log_shutdown
RCF_NAME=RCF
RCF_EXEC=te_rcf
RCF_SHUT=te_rcf_shutdown
CS_NAME=Configurator
CS_EXEC=te_cs
CS_SHUT=te_cs_shutdown

start_daemon() {
    DAEMON=$1
    if test ${START_OK} -eq 0 -a -n "$(eval echo '$'$DAEMON)" ; then
        DAEMON_NAME="$(eval echo '${'$DAEMON'_NAME}')"
        DAEMON_EXEC="$(eval echo '${'$DAEMON'_EXEC}')"
        DAEMON_OPTS="$(eval echo '${'$DAEMON'_OPTS}')"
        DAEMON_CONF="$(eval echo '${CONF_'$DAEMON'}')"
        te_log_message Engine Dispatcher \
            "Start ${DAEMON_NAME}: ${DAEMON_OPTS} ${DAEMON_CONF}"
        myecho -n "--->>> Starting ${DAEMON_NAME}... "
        if test -n "$(eval echo '${VG_'$DAEMON'}')" ; then
            # Run in foreground under valgrind
            valgrind ${VG_OPTIONS} ${DAEMON_EXEC} ${DAEMON_OPTS} \
                "${DAEMON_CONF}" 2>valgrind.${DAEMON_EXEC}
        else
            ${DAEMON_EXEC} ${DAEMON_OPTS} "${DAEMON_CONF}"
        fi
        START_OK=$?
        if test ${START_OK} -eq 0 ; then
            myecho "done"
            eval $(echo $DAEMON'_OK')=yes
        else
            myecho "failed"
        fi
    fi
}

if test -z "${DAEMON}" ; then
    start_daemon LOGGER

    start_daemon RCF

    if test -n "${RCF_OK}" ; then
        # Wakeup Logger when RCF is ready
        TE_LOGGER_PID="$(cat "${TE_LOGGER_PID_FILE}" 2>/dev/null)"
        if test -n "${TE_LOGGER_PID}" ; then
            kill -USR1 ${TE_LOGGER_PID}
        else
            echo "Failed to wake-up TE Logger" >&2
            START_OK=1
        fi
    fi

    start_daemon CS
else
    # It is assumed here that all TE engine daemons are running
    LOGGER_OK=yes
    RCF_OK=yes
    CS_OK=yes
fi

if test ${START_OK} -eq 0 -a -n "${TESTER}" ; then
    te_log_message Engine Dispatcher \
        "Start Tester:${TESTER_OPTS} ${CONF_TESTER}"
    myecho "--->>> Start Tester"
    if test -n "$VG_TESTER" ; then
        VG_TESTS=${VG_TESTS} valgrind ${VG_OPTIONS} te_tester${TESTER_EXT} \
            ${TESTER_OPTS} "${CONF_TESTER}" 2>valgrind.te_tester
    else
        VG_TESTS=${VG_TESTS} te_tester${TESTER_EXT} ${TESTER_OPTS} \
            "${CONF_TESTER}" 
    fi
    START_OK=$?
fi


shutdown_daemon() {
    DAEMON=$1
    if test -n "${SHUTDOWN}" -a -n "$(eval echo '${'$DAEMON'_OK}')" ; then
        DAEMON_NAME="$(eval echo '${'$DAEMON'_NAME}')"
        DAEMON_SHUT="$(eval echo '${'$DAEMON'_SHUT}')"
        te_log_message Engine Dispatcher "Shutdown ${DAEMON_NAME}"
        myecho -n "--->>> Shutdown ${DAEMON_NAME}... "
        ${DAEMON_SHUT}
        if test $? -eq 0 ; then
            myecho "done"
        else
            myecho "failed"
        fi
    fi
}

shutdown_daemon CS

if test -n "${LOGGER_OK}" -a -n "${RCF_OK}" ; then
    te_log_message Engine Dispatcher "Flush log"
    myecho "--->>> Flush Logs"
    te_log_flush
fi

if test ${START_OK} -eq 0 -a -n "${DO_NUTS}" ; then
    te_log_message Engine Dispatcher "Dumping TCE"
    myecho "--->>> Dump TCE"
    te_tce_dump.sh "${CONF_NUT}"
fi

shutdown_daemon RCF 

shutdown_daemon LOGGER

if test -z "${SHUTDOWN}" ; then
    te_log_message Engine Dispatcher "Leave TE daemon ${TE_ID}"
    myecho "--->>> Leave TE deamon ${TE_ID}"
fi

# Wait for RGT in live mode finish
if test -n "${LIVE_LOG}" ; then
    wait ${LIVE_LOG_PID}
fi

#
# RGT processing of the raw log
#
if test -n "${CONF_RGT}" ; then
    CONF_RGT="-c ${CONF_RGT}"
fi
if test -n "${RGT_LOG_TXT}" -o -n "${RGT_LOG_HTML_PLAIN}" ; then
    # Generate XML log do not taking into account control messages
    LOG_XML_PLAIN="log_plain.xml"
    rgt-conv --no-cntrl-msg  -m postponed ${CONF_RGT} \
        -f "${TE_LOG_RAW}" -o "${LOG_XML_PLAIN}"
    if test $? -eq 0 -a -e "${LOG_XML_PLAIN}" ; then
        if test -n "${RGT_LOG_TXT}" ; then
            rgt-xml2text -f "${LOG_XML_PLAIN}" -o "${RGT_LOG_TXT}"
        fi
        if test -n "${RGT_LOG_HTML_PLAIN}" ; then
            rgt-xml2html -f "${LOG_XML_PLAIN}" -o "${RGT_LOG_HTML_PLAIN}"
        fi
    fi
fi
if test -n "${RGT_LOG_HTML}" ; then
    # Generate XML log taking into account control messages
    LOG_XML_STRUCT="log_struct.xml"
    rgt-conv -m postponed ${CONF_RGT} \
        -f "${TE_LOG_RAW}" -o "${LOG_XML_STRUCT}"
    if test $? -eq 0 -a -e "${LOG_XML_STRUCT}" ; then
        rgt-xml2html-multi "${LOG_XML_STRUCT}" "${RGT_LOG_HTML}"
    fi
fi

if test ${START_OK} -eq 0 -a -n "${DO_NUTS}" ; then
    myecho "--->>> TCE processing"
    TCER_OPTS="${TCER_OPTS}" TCES_OPTS="${TCES_OPTS}" \
        te_tce_process "${CONF_NUT}"
fi

# Run TRC, if any its option is provided
if test ${START_OK} -eq 0 -a -n "${TRC_OPTS}" ; then
    te_trc.sh ${TRC_OPTS} "${TE_LOG_RAW}"
fi

test -n "${SHUTDOWN}" && rm -rf "${TE_TMP}"

exit ${START_OK}
