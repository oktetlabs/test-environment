# Script should be sources to guess TE parameters if they're not
# specified in the shell environment.

TEST_TE_BASE=(
    "../../.."
    "../.."
    ".."
    "."
)

for loop in once ; do
    test -e "${TE_BASE}/dispatcher.sh" && break

    for path in ${TEST_TE_BASE[*]}; do
        test -e "${path}/dispatcher.sh" || continue

        export TE_BASE="$(cd "${path}"; pwd)"
        echo "Guessing TE_BASE=$TE_BASE"
        break
    done

    test -e "${TE_BASE}/dispatcher.sh" || echo "Cannot guess TE_BASE" >&2
done
export TE_TS_DIR=${TS_TOPDIR}/ts
export TE_TS_SSH_KEY_DIR=${TS_TOPDIR}/conf/keys