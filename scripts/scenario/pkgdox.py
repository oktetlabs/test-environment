# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
"""Doxygen input filter: the description of a package.md.

Reads a package scenario document and prints its "## Description"
section as documentation of the package's doxygen group, so the
group page carries the prose next to the tests.  The group itself
stays defined where it is (a package.dox, say): the comment only
adds to it, and a build without this filter loses the description,
not the group.

The section is markdown and stays markdown, doxygen renders it.
Headings below it become bold lines with a "<group>-" prefixed
anchor to @ref, not doxygen sections: those would add entries to
the navigation sidebar of the group page, which lists the tests.
"> impl:" blockquotes are advice for the implementor and are left
out.  The comment is written with "///" lines, which a "*/" in the
prose cannot end.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from mdparse import parse_package
from model import Package, ScenarioError

_HEADING = re.compile(r'^#{3,6}\s+(.*?)\s*$')
_FENCE = re.compile(r'^\s*(```|~~~)')
_IMPL_QUOTE = re.compile(r'^\s*>\s*impl:', re.IGNORECASE)
_QUOTE = re.compile(r'^\s*>')


def _anchor(group: str, title: str) -> str:
    slug = re.sub(r'[^a-z0-9]+', '-', title.lower()).strip('-')
    return f'{group}-{slug}'


def _doc_lines(pkg: Package) -> list[str]:
    """The description as doxygen input, before comment markup."""
    out: list[str] = []
    fenced = False
    in_impl = False
    for line in pkg.description:
        if _FENCE.match(line):
            fenced = not fenced
            out.append(line)
            continue
        if fenced:
            out.append(line)
            continue
        if in_impl and _QUOTE.match(line):
            continue
        in_impl = bool(_IMPL_QUOTE.match(line))
        if in_impl:
            continue
        h = _HEADING.match(line)
        if h:
            out.append(f'@anchor {_anchor(pkg.group, h.group(1))}')
            out.append(f'**{h.group(1)}**')
        else:
            out.append(line)
    # A dropped quote leaves its blank separators behind.
    squeezed: list[str] = []
    for line in out:
        if line.strip() or (squeezed and squeezed[-1].strip()):
            squeezed.append(line)
    while squeezed and not squeezed[-1].strip():
        squeezed.pop()
    return squeezed


def render(pkg: Package) -> str:
    """The doxygen comment adding the description to the package group.

    Args:
        pkg: The parsed package document.

    Returns:
        The comment, or an empty string when there is no description.
    """
    if not pkg.description:
        return ''
    lines = [f'@addtogroup {pkg.group}', '', *_doc_lines(pkg)]
    return '\n'.join(f'/// {line}' if line else '///' for line in lines) + '\n'


def filter_text(source: Path) -> str:
    """The filtered form of a package.md, ready for doxygen.

    Args:
        source: The package document doxygen wants filtered.

    Returns:
        The group comment; empty for a document without a description
        or one that does not parse - the markdown itself must never
        reach doxygen, which would read it as C.
    """
    try:
        pkg = parse_package(source)
    except ScenarioError as exc:
        print(f'pkgdox: warning: {exc}; the description is not documented', file=sys.stderr)
        return ''
    return render(pkg)


def main(argv: list[str] | None = None) -> int:
    """Run the filter on one package.md, printing to stdout."""
    parser = argparse.ArgumentParser(description='doxygen filter for package.md documents')
    parser.add_argument('source', help='the package.md doxygen wants filtered')
    args = parser.parse_args(argv)
    sys.stdout.write(filter_text(Path(args.source)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
