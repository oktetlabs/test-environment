#!/bin/bash

cat "${1}" | sed 's,\@\@DATADIR\@\@,'"${2}"'/..,g;'
