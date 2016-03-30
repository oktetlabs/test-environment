#!/bin/bash

. `dirname \`which $0\``/guess.sh >/dev/null

echo "\$sh_path=\"$PATH\";"
echo "\$sh_ld_library_path=\"$LD_LIBRARY_PATH\";"

exit 0
