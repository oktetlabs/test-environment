#!/bin/bash
# Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.

# Interpret all arguments as a list of paths to templates
#
# Write into stdout content of header
# defining enum with names of templates
# and template filenames
# and array of rgt_tmpl_t
# and size of this array

echo -E "enum {"
for i in "$@" ; do
   basename "${i}" | sed 's,\.tmpl,,' |
       tr [:lower:] [:upper:] |
       awk '{ printf "%s,\n", $$1 }'
done;
echo -E "};"
echo -E "const char *xml2fmt_files[] = {"
for i in "$@" ; do
    echo "\""`basename "${i}"`"\","
done;
echo -E "};"
echo -E -n "rgt_tmpl_t xml2fmt_tmpls["
echo -E -n "sizeof(xml2fmt_files) / "
echo -E "sizeof(xml2fmt_files[0])];"
echo -E -n "size_t     xml2fmt_tmpls_num = "
echo -E -n "sizeof(xml2fmt_files) / "
echo -E "sizeof(xml2fmt_files[0]);"
