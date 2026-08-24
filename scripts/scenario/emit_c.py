# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""C test stub emitter for markdown scenarios."""

from __future__ import annotations

import re
import textwrap
from typing import TYPE_CHECKING

from model import flatten_steps, resolve_inline

if TYPE_CHECKING:
    from model import Note, Package, Param, Test

_WIDTH = 80
_INT_RE = re.compile(r'-?\d+')
_CAPS_RE = re.compile(r'[A-Z][A-Z0-9_]+')
_NUMERIC_HINT = re.compile(r'\b(mtu|size|count|number|length|len|num)\b', re.IGNORECASE)


def _param_kind(p: Param) -> str:
    """Classify a parameter: 'uint', 'enum' or 'string'."""
    if p.values and all(_INT_RE.fullmatch(v.name) for v in p.values):
        return 'uint'
    if any(_CAPS_RE.fullmatch(v.name) for v in p.values):
        return 'enum'
    if not p.values and (_NUMERIC_HINT.search(p.name) or _NUMERIC_HINT.search(p.description)):
        return 'uint'
    return 'string'


def _wrap(text: str, first: str, cont: str) -> list[str]:
    """Wrap text at the target width under two prefixes."""
    lines = textwrap.wrap(text, width=_WIDTH - len(first))
    out = [first + lines[0]]
    out.extend(cont + line for line in lines[1:])
    return out


def _doxy_para(out: list[str], key: str, text: str) -> None:
    """Emit ' * @key text' wrapped with a hanging indent."""
    first = ' * '
    cont = ' * ' + ' ' * (len(key) + 1)
    out.extend(_wrap(f'{key} {text}', first, cont))


def _doxy_params(out: list[str], test: Test) -> None:
    names = [p.name for p in test.params]
    values = [v.name for p in test.params for v in p.values]
    width = max(len(p.name) for p in test.params)
    desc_col = len('@param ') + width + 1
    for p in test.params:
        desc = resolve_inline(p.description, names, values)
        if p.values:
            desc = desc.rstrip('.:') + ':'
        first = ' * '
        cont = ' * ' + ' ' * desc_col
        out.extend(_wrap(f'@param {p.name:<{width}} {desc}', first, cont))
        for v in p.values:
            text = f'- @c {v.name}'
            if v.comment:
                text += f' ({v.comment})'
            vfirst = ' * ' + ' ' * desc_col
            vcont = vfirst + '  '
            out.extend(_wrap(text, vfirst, vcont))


def _emit_macro(out: list[str], macro: str, text: str) -> None:
    # The text becomes a C string literal: escape what the compiler
    # would otherwise interpret.
    text = text.replace('\\', '\\\\').replace('"', '\\"')
    prefix = f'    {macro}("'
    lines = textwrap.wrap(text, width=_WIDTH - len(prefix) - 2)
    if len(lines) == 1:
        out.append(f'{prefix}{lines[0]}");')
        return
    out.append(f'{prefix}{lines[0]} "')
    cont = ' ' * (len(prefix) - 1) + '"'
    out.extend(f'{cont}{line} "' for line in lines[1:-1])
    out.append(f'{cont}{lines[-1]}");')


def _emit_notes(
    out: list[str],
    notes: list[Note],
    names: list[str],
    values: list[str],
) -> None:
    if not notes:
        return
    body: list[str] = []
    for n, note in enumerate(notes):
        if n:
            body.append('     *')
        text = resolve_inline(note.text, names, values)
        if note.impl:
            text = f'IMPL: {text}'
        if n == 0:
            body.extend(_wrap(text, '    /* ', '     * '))
        else:
            body.extend(_wrap(text, '     * ', '     * '))
    body[-1] += ' */'
    out.extend(body)


def _emit_steps(out: list[str], test: Test) -> None:
    names = [p.name for p in test.params]
    values = [v.name for p in test.params for v in p.values]
    prev = 0
    for depth, step in flatten_steps(test.steps):
        while prev > max(depth, 2):
            out.append('    TEST_STEP_POP("");')
            prev -= 1
        if depth == 1:
            macro = 'TEST_STEP'
        elif depth == 2:  # noqa: PLR2004 - the two fixed macro levels
            macro = 'TEST_SUBSTEP'
        elif depth == prev:
            macro = 'TEST_STEP_NEXT'
        else:
            macro = 'TEST_STEP_PUSH'
        _emit_notes(out, step.notes, names, values)
        _emit_macro(out, macro, resolve_inline(step.text, names, values))
        prev = depth
    while prev > 2:  # noqa: PLR2004 - pop back to the SUBSTEP level
        out.append('    TEST_STEP_POP("");')
        prev -= 1


def emit_test(
    package: Package,
    test: Test,
    *,
    author: str,
    copyright_line: str,
) -> str:
    """Render the complete C stub for one test."""
    names = [p.name for p in test.params]
    values = [v.name for p in test.params for v in p.values]
    out: list[str] = []
    out.append('/* SPDX-License-Identifier: Apache-2.0 */')
    out.append(f'/* {copyright_line} */')
    out.append(f'/** @defgroup {package.name}-{test.name} {test.summary}')
    out.append(f' * @ingroup {package.name}')
    out.append(' * @{')
    out.append(' *')
    _doxy_para(out, '@objective', resolve_inline(test.objective, names, values))
    for note in test.notes:
        out.append(' *')
        _doxy_para(out, '@note', resolve_inline(note, names, values))
    if test.params:
        out.append(' *')
        _doxy_params(out, test)
    if test.type:
        out.append(' *')
        out.append(f' * @type {test.type}')
    out.append(' *')
    out.append(f' * @author {author}')
    out.append(' *')
    out.append(' * @par Scenario:')
    out.append(' */')
    out.append('')
    out.append(f'#define TE_TEST_NAME "{package.name}/{test.name}"')
    out.append('')
    out.append('/* TODO: suite includes */')
    out.append('')
    _emit_main(out, test)
    return '\n'.join(out) + '\n'


def _emit_main(out: list[str], test: Test) -> None:
    out.append('int')
    out.append('main(int argc, char *argv[])')
    out.append('{')
    if test.params:
        out.append('    /* TODO: verify parameter kinds */')
        for p in test.params:
            if _param_kind(p) == 'uint':
                out.append(f'    unsigned int {p.name};')
            else:
                out.append(f'    const char *{p.name};')
        out.append('')
    out.append('    TEST_START;')
    for p in test.params:
        kind = _param_kind(p)
        if kind == 'uint':
            out.append(f'    TEST_GET_UINT_PARAM({p.name});')
        else:
            if kind == 'enum':
                out.append('    /* enum: consider TEST_GET_ENUM_PARAM */')
            out.append(f'    TEST_GET_STRING_PARAM({p.name});')
    out.append('')
    _emit_steps(out, test)
    out.append('')
    out.append('    TEST_SUCCESS;')
    out.append('')
    out.append('cleanup:')
    out.append('    TEST_END;')
    out.append('}')
    out.append('/** @} */')
