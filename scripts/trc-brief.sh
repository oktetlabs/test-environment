#! /bin/bash

`dirname \`which $0\``/trc.sh --html=trc-brief.html --no-packages-only \
    --no-total --no-unspec --no-expected --no-stats-not-run $@
