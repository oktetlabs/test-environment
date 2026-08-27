# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Doxygen header comment surgery for C test sources.

A TE test opens with a doxygen comment describing the test: the
@defgroup/@page line, @objective, @param entries, and the
@par Scenario marker.  The tools that read a test back out of its
source share this module to find that comment and take it apart;
they differ only in what they emit from it.
"""

from __future__ import annotations

import re

_DOC_START = re.compile(r'^\s*/\*\*')
_DOC_ONE_LINE = re.compile(r'^\s*/\*\*.*\*/\s*$')


def split_source(text: str) -> tuple[list[str], list[str]] | None:
    """Header comment lines (closing dropped) and trailing doc one-liners.

    The header is the first multi-line doxygen comment carrying a
    test-page tag (@defgroup, @page, or @objective) - a leading
    @file comment does not count - or simply the first one when no
    block carries a tag; None when there is no such comment at all.
    One-line doc comments that open or close a group (the trailing
    "@}" above all) come back verbatim in the second list; other
    member docs are dropped, because the members they document are
    not emitted.
    """
    lines = text.splitlines()
    starts = [
        i
        for i, line in enumerate(lines)
        if _DOC_START.match(line) and not _DOC_ONE_LINE.match(line)
    ]

    def tagged(start: int) -> bool:
        for line in lines[start:]:
            if any(tag in line for tag in ('@defgroup', '@page', '@objective')):
                return True
            if '*/' in line:
                return False
        return False

    start = next((i for i in starts if tagged(i)), starts[0] if starts else None)
    if start is None:
        return None
    header: list[str] = []
    rest = start
    for i in range(start, len(lines)):
        line = lines[i]
        if '*/' in line:
            residue = line[: line.index('*/')].rstrip()
            if residue.strip('* \t'):
                header.append(residue)
            rest = i + 1
            break
        header.append(line)
    trailing = [
        line
        for line in lines[rest:]
        if _DOC_ONE_LINE.match(line) and ('@{' in line or '@}' in line)
    ]
    return header, trailing
