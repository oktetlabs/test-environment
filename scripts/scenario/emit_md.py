# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Emit a scenario in the markdown dialect from source steps.

The inverse of the generate flow: where mdparse/emit_c turn a
package.md scenario into a C stub, this module turns the steps
extracted from an implemented test back into a dialect test
section, ready to paste into a package.md.  The output is built
to round-trip: step texts stay byte-equal to the source (with
"@p x"/"@c X" references turned back into backtick spans), and
list depths follow the same kind rules the drift check uses, so
a bootstrapped section passes "scenario.py check" immediately.

Control constructs cannot be steps in the dialect, so they ride
along as continuation-paragraph notes under the step they guard:
documentation for the reader, a comment in a generated stub, and
invisible to the drift check.
"""

from __future__ import annotations

import re
import textwrap
from typing import TYPE_CHECKING

from cheader import parse_doc_header

if TYPE_CHECKING:
    from aststeps import Cond, SourceStep

_REF = re.compile(r'@[pc]\s+([A-Za-z_][A-Za-z0-9_]*)')
_WRAP = textwrap.TextWrapper(width=68)

# Note phrasing before the backticked expression, per construct
# kind; 'for' and 'goto' have their own shapes in cond_note().
_NOTES = {
    'if': 'Only when ',
    'else': 'Only when not ',
    'while': 'While ',
    'do': 'Repeats while ',
    'switch': 'Depending on ',
}


def uninline(text: str) -> str:
    """Turn "@p name" and "@c NAME" references back into backtick spans."""
    return _REF.sub(lambda m: f'`{m.group(1)}`', text)


def cond_note(cond: Cond) -> str:
    """The note sentence documenting one enclosing construct."""
    if cond.kind == 'goto':
        return 'Only on the error path.'
    if cond.kind == 'for':
        inner = cond.desc.removeprefix('for').strip()
        if inner.startswith('(') and inner.endswith(')'):
            inner = inner[1:-1].strip()
        return f'For each iteration (`{inner}`).'
    before = _NOTES.get(cond.kind, _NOTES['if'])
    return f'{before}`{cond.cond}`.'


def step_items(steps: list[SourceStep]) -> list[tuple[int, str, list[str]]]:
    """(depth, text, notes) per step, with the drift check's depths.

    The depth rules mirror cstep.extract_steps - STEP is 1, SUBSTEP
    is 2, the PUSH/NEXT/POP stack counts from the previous depth -
    so the emitted list compares clean against the source.  The
    INFO variants and empty-text steps do not become items, and each
    enclosing construct becomes a note on its step.
    """
    items: list[tuple[int, str, list[str]]] = []
    prev = 0
    for step in steps:
        kind = step.kind
        if kind in ('PUSH_INFO', 'POP_INFO'):
            continue
        if kind == 'RESET':
            prev = 0
            continue
        if kind == 'STEP':
            depth = 1
        elif kind == 'SUBSTEP':
            depth = 2
        elif kind == 'PUSH':
            depth = prev + 1
        elif kind == 'NEXT':
            depth = prev
        else:  # POP
            depth = prev - 1
        prev = depth
        if not step.text:
            continue
        notes = [cond_note(c) for c in step.conds]
        items.append((max(depth, 1), uninline(step.text), notes))
    return items


def _emit_items(out: list[str], items: list[tuple[int, str, list[str]]]) -> None:
    counters: dict[int, int] = {}
    for depth, text, notes in items:
        indent = 3 * (depth - 1)
        if depth == 1:
            counters[1] = counters.get(1, 0) + 1
            marker = f'{counters[1]}. '
        else:
            marker = '- '
        col = indent + len(marker)
        out.append(f'{" " * indent}{marker}{text}')
        for note in notes:
            out.append('')
            out.append(f'{" " * col}{note}')
        out.append('')


def emit_test_md(name: str, source_text: str, items: list[tuple[int, str, list[str]]]) -> str:
    """A dialect test section for one test.

    Args:
        name: The test name (the source file stem).
        source_text: The C source, for the doxygen header fields.
        items: The step items, as step_items() builds them.

    Returns:
        A "## name: summary" section with the objective, type, and
        parameters recovered from the doxygen header, then the
        steps; ready to append to a package.md.
    """
    summary, objective, type_, params = parse_doc_header(source_text)
    out = [f'## {name}: {summary or name}', '']
    if objective:
        out.extend(_WRAP.wrap(uninline(objective)))
        out.append('')
    if type_:
        out.append(f'Type: {type_}')
        out.append('')
    if params:
        out.append('Parameters:')
        out.append('')
        out.extend(f'- `{pname}`: {uninline(desc)}' for pname, desc in params)
        out.append('')
    out.append('Steps:')
    out.append('')
    _emit_items(out, items)
    while out and not out[-1]:
        out.pop()
    return '\n'.join(out) + '\n'
