#!/bin/bash

print_help_and_exit()
{
cat <<EOF
usage: "$(basename $0)" mask mode  (mask is 0xF, mode is on/off/rst)

EOF
exit 1
}

emul_rst()
{
    sispmctl -q -f ${outlet}
    sleep 1
    sispmctl -q -o ${outlet}
    exit 0
}


if [ $# -ne 2 ]; then
    print_help_and_exit
fi

if [ $1 = 0xf -o $1 = 0xF ]; then
    outlet=all
else
    outlet=$(echo | awk '{print log('$1')/log(2) + 1}')
fi

case "$2" in
    "rst" ) emul_rst ;;
    "on"  ) ACT="-q -o" ;;
    "off" ) ACT="-q -f" ;;
    *     ) print_help_and_exit ;;
esac

sispmctl ${ACT} ${outlet}
