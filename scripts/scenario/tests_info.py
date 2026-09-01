#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""tests-info.xml generator: test metadata from C sources.

Emits the per-package <test> entries that Tester reads when parsing
a package: the objective (as the awk extractor always did), the
TEST_PARAM_DOC parameter descriptions, and the declared TEST_STEP
scenario.  A bare import stays standard-library only, by design:
this runs inside every suite build, where python3 itself must be
the only requirement.  When the libclang package happens to be
installed the scenario is upgraded to the control-flow-annotated
form the documentation renders (loop and branch headings from
aststeps/steptree); without it the flat textual extraction is the
fallback, so the build never gains a hard dependency.

Unlike the awk extractor this escapes '&' too, producing valid XML
for any objective text; the @a/@b/@c/@e/@p stripping and whitespace
collapsing reproduce the awk behavior exactly.  Attribute values
(name=, page=, param name=) go through quoteattr rather than escape,
since a literal '"' in a file stem or @page/@defgroup identifier
would otherwise break the surrounding quotes; element text has no
quoting to worry about, so it stays on plain escape().
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from xml.sax.saxutils import escape, quoteattr

from cheader import parse_doc_header
from cparam import extract_docs
from cstep import extract_steps

_PAGE = re.compile(r'@(?:page|defgroup)\s+(\S+)')
_MARKERS = ('@a', '@b', '@c', '@e', '@p')


def _awk_normalize(text: str) -> str:
    """The awk extractor's objective normalization, reproduced."""
    for marker in _MARKERS:
        text = text.replace(marker, '')
    return ' '.join(text.split())


def _param_docs(text: str) -> list[tuple[str, str]]:
    """(name, description): inline docs first, header leftovers after."""
    inline = [(n, d) for n, d in extract_docs(text) if d.strip()]
    seen = {n for n, _ in inline}
    header = [
        (n, d)
        for n, d in parse_doc_header(text)[3]
        if n not in seen and d.strip()
    ]
    return inline + header


def _tree_steps(path: Path) -> list[tuple[int, str]] | None:
    """The control-flow-annotated (depth, text) scenario, or None.

    Renders exactly what the documentation shows: the parse setup
    and the item texts (loop and branch headings included) are
    shared with doxfilter through steptree.  The imports are lazy so
    a bare `import tests_info` stays stdlib-only; without libclang,
    or when the source does not parse, the caller falls back to the
    flat textual extraction.
    """
    import aststeps

    if not aststeps.HAVE_CLANG:
        return None

    import doxfilter
    import steptree

    stubs, wrappers, _ = doxfilter.parse_setup(path)
    try:
        info = aststeps.analyze(path, extra_args=stubs, wrappers=wrappers)
    except (RuntimeError, ValueError) as exc:
        print(f'tests_info: {path}: {exc}', file=sys.stderr)
        return None

    out: list[tuple[int, str]] = []

    def walk(nodes: list[steptree.Node], depth: int) -> None:
        for node in nodes:
            text = steptree.node_text(node)
            if text:
                out.append((depth, text))
            walk(node.children, depth + 1)

    walk(steptree.build(info.steps), 1)
    return out


def emit_test(path: Path) -> str:
    """The <test> block for one C source, '' when it has no objective."""
    text = path.read_text(encoding='utf-8', errors='replace')
    _, objective, _, _ = parse_doc_header(text)
    objective = _awk_normalize(objective)
    if not objective:
        return ''
    page = _PAGE.search(text)
    page_attr = f' page={quoteattr(page.group(1))}' if page else ''
    out = [f'  <test name={quoteattr(path.stem)}{page_attr}>']
    out.append(f'    <objective>{escape(objective)}</objective>')
    for name, descr in _param_docs(text):
        out.append(f'    <param name={quoteattr(name)}>{escape(descr)}</param>')
    steps = _tree_steps(path)
    if steps is None:
        steps = extract_steps(text)
    if steps:
        out.append('    <scenario>')
        out.extend(
            f'      <step depth="{depth}">{escape(step)}</step>'
            for depth, step in steps
        )
        out.append('    </scenario>')
    out.append('  </test>')
    return '\n'.join(out) + '\n'


def emit_dir(directory: Path) -> str:
    """Concatenated <test> blocks for every C source in a directory."""
    return ''.join(
        emit_test(path) for path in sorted(directory.glob('*.c'))
    )


def main(argv: list[str] | None = None) -> int:
    """CLI: print the <test> blocks for one source directory."""
    args = sys.argv[1:] if argv is None else argv
    if len(args) != 1:
        print('usage: tests_info.py <source dir>', file=sys.stderr)
        return 1

    import aststeps  # lazy, as in _tree_steps

    if not aststeps.HAVE_CLANG:
        print(
            'tests_info: scenarios lack control-flow annotations\n'
            '    (pip install libclang to add them)',
            file=sys.stderr,
        )
    sys.stdout.write(emit_dir(Path(args[0])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
