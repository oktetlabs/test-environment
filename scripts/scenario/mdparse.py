# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Parser for the package.md test scenario dialect."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from typing import TYPE_CHECKING

from model import Note, Package, Param, ScenarioError, Step, Test, Value

if TYPE_CHECKING:
    from pathlib import Path

MAX_STEP_DEPTH = 3

_H1 = 1
_H2 = 2

_HEADING = re.compile(r'^(#{1,6})\s+(.*?)\s*$')
_NAMED = re.compile(r'^([a-z0-9_]+):\s+(.+)$')
_ITEM = re.compile(r'^(\s*)((?:[-*+]|\d+[.)])\s+)(.*)$')
_LINK = re.compile(r'^\[([a-z0-9_]+)\]\(([^)]+\.md)\)$')
_PARAM = re.compile(r'^`([^`]+)`(?::\s+(.*))?$')
_SECTION = re.compile(r'^(Parameters|Steps):?$')
_IMPL_TAG = re.compile(r'^impl:\s*', re.IGNORECASE)


@dataclass
class _Item:
    """A raw list item before model conversion."""

    text: str
    line: int
    indent: int
    col: int
    notes: list[Note] = field(default_factory=list)
    children: list[_Item] = field(default_factory=list)


def _read(path: Path) -> list[str]:
    try:
        return path.read_text(encoding='utf-8').splitlines()
    except OSError as exc:
        raise ScenarioError(path, 0, f'cannot read: {exc}') from exc


def _indent_of(line: str) -> int:
    return len(line) - len(line.lstrip(' '))


def _take_para(lines: list[str], i: int, col: int, first: str, path: Path) -> tuple[str, int]:
    """Collect a paragraph whose first line's text is `first`.

    Continuation lines must be non-blank, indented exactly `col`,
    and neither list items nor blockquotes.
    """
    parts = [first]
    while i < len(lines):
        line = lines[i]
        if not line.strip() or _ITEM.match(line):
            break
        ind = _indent_of(line)
        if ind < col or line.lstrip().startswith('>'):
            break
        if ind != col:
            raise ScenarioError(path, i + 1, 'bad continuation indent')
        parts.append(line.strip())
        i += 1
    return ' '.join(parts), i


def _take_quote(lines: list[str], i: int, col: int) -> tuple[str, int]:
    parts: list[str] = []
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            break
        if _indent_of(line) != col or not line.lstrip().startswith('>'):
            break
        parts.append(line.lstrip()[1:].strip())
        i += 1
    text = ' '.join(p for p in parts if p)
    return _IMPL_TAG.sub('', text), i


def _parse_list(lines: list[str], i: int, path: Path) -> tuple[list[_Item], int]:
    """Parse one list block into an item tree."""
    root: list[_Item] = []
    stack: list[_Item] = []
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue
        m = _ITEM.match(line)
        if m:
            indent = len(m.group(1))
            col = indent + len(m.group(2))
            lineno = i + 1
            text, i = _take_para(lines, i + 1, col, m.group(3), path)
            item = _Item(text=text, line=lineno, indent=indent, col=col)
            while stack and indent <= stack[-1].indent:
                stack.pop()
            if stack:
                if indent < stack[-1].col:
                    raise ScenarioError(path, lineno, 'misaligned list item')
                stack[-1].children.append(item)
            else:
                if indent != 0:
                    raise ScenarioError(path, lineno, 'misaligned list item')
                root.append(item)
            stack.append(item)
            continue
        ind = _indent_of(line)
        while stack and ind < stack[-1].col:
            stack.pop()
        if not stack:
            break
        target = stack[-1]
        if line.lstrip().startswith('>'):
            text, i = _take_quote(lines, i, target.col)
            target.notes.append(Note(text=text, impl=True))
        else:
            if ind != target.col:
                raise ScenarioError(path, i + 1, 'bad continuation indent')
            text, i = _take_para(lines, i + 1, target.col, line.strip(), path)
            target.notes.append(Note(text=text, impl=False))
    return root, i


def _items_to_steps(items: list[_Item], path: Path, depth: int = 1) -> list[Step]:
    steps = []
    for it in items:
        if depth > MAX_STEP_DEPTH:
            print(
                f'{path}:{it.line}: warning: step depth {depth}; consider a separate test',
                file=sys.stderr,
            )
        steps.append(
            Step(
                text=it.text,
                line=it.line,
                notes=it.notes,
                sub=_items_to_steps(it.children, path, depth + 1),
            )
        )
    return steps


def _items_to_params(items: list[_Item], path: Path) -> list[Param]:
    params = []
    for it in items:
        m = _PARAM.match(it.text)
        if m is None or m.group(2) is None:
            raise ScenarioError(path, it.line, 'parameter item must be "`name`: description"')
        if it.notes:
            raise ScenarioError(path, it.line, 'notes not allowed here')
        values = []
        for v in it.children:
            vm = _PARAM.match(v.text)
            if vm is None:
                raise ScenarioError(path, v.line, 'value item must be "`value`[: comment]"')
            if v.children or v.notes:
                raise ScenarioError(path, v.line, 'value items cannot nest')
            values.append(Value(name=vm.group(1), comment=vm.group(2)))
        params.append(Param(name=m.group(1), description=m.group(2), values=values))
    return params


def _parse_test_body(
    lines: list[str],
    i: int,
    path: Path,
    name: str,
    summary: str,
    lineno: int,
    level: int = _H2,
) -> tuple[Test, int]:
    test = Test(name=name, summary=summary, path=path, line=lineno)
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue
        h = _HEADING.match(line)
        if h:
            if len(h.group(1)) <= level:
                break
            sm = _SECTION.match(h.group(2)) if len(h.group(1)) == level + 1 else None
            if sm is None:
                raise ScenarioError(path, i + 1, 'unexpected heading in test')
            # A section marker written as a heading: same meaning as
            # the "Parameters:"/"Steps:" label paragraph.
            para = f'{sm.group(1)}:'
            i += 1
        elif line.lstrip().startswith('>'):
            raise ScenarioError(path, i + 1, 'blockquote outside a step')
        elif _ITEM.match(line):
            raise ScenarioError(path, i + 1, 'list outside "Parameters:" or "Steps:"')
        else:
            para, i = _take_para(lines, i + 1, 0, line.strip(), path)
        if para == 'Parameters:':
            items, i = _parse_list(lines, i, path)
            if not items:
                raise ScenarioError(path, i, 'empty "Parameters:" list')
            test.params = _items_to_params(items, path)
        elif para == 'Steps:':
            items, i = _parse_list(lines, i, path)
            if not items:
                raise ScenarioError(path, i, 'empty "Steps:" list')
            test.steps = _items_to_steps(items, path)
        elif para.startswith('Type:'):
            test.type = para[len('Type:') :].strip()
        elif not test.objective:
            test.objective = para
        else:
            test.notes.append(para)
    if not test.objective:
        raise ScenarioError(path, lineno, f'test "{name}" has no objective')
    if not test.steps:
        raise ScenarioError(path, lineno, f'test "{name}" has no steps')
    return test, i


def _parse_h1(path: Path, lines: list[str]) -> tuple[str, str, int]:
    i = 0
    while i < len(lines) and not lines[i].strip():
        i += 1
    m = _HEADING.match(lines[i]) if i < len(lines) else None
    if m is None or len(m.group(1)) != _H1:
        raise ScenarioError(path, i + 1, 'file must start with "# <name>: <summary>"')
    nm = _NAMED.match(m.group(2))
    if nm is None:
        raise ScenarioError(path, i + 1, 'H1 must be "# <name>: <summary>"')
    return nm.group(1), nm.group(2), i + 1


def parse_test_file(path: Path) -> Test:
    """Parse a per-test scenario file (H1 form)."""
    lines = _read(path)
    name, summary, i = _parse_h1(path, lines)
    if name != path.stem:
        raise ScenarioError(path, i, f'test "{name}" does not match file "{path.stem}"')
    test, i = _parse_test_body(lines, i, path, name, summary, i, level=_H1)
    if i < len(lines):
        raise ScenarioError(path, i + 1, 'unexpected content after test')
    return test


def parse_package(path: Path) -> Package:
    """Parse a package.md, following per-test file references."""
    lines = _read(path)
    name, summary, i = _parse_h1(path, lines)
    if name != path.parent.name:
        raise ScenarioError(
            path,
            i,
            f'package "{name}" does not match directory "{path.parent.name}"',
        )
    pkg = Package(name=name, summary=summary, path=path)
    seen: dict[str, int] = {}

    def add(test: Test, lineno: int) -> None:
        if test.name in seen:
            raise ScenarioError(path, lineno, f'duplicate test "{test.name}"')
        seen[test.name] = lineno
        pkg.tests.append(test)

    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue
        h = _HEADING.match(line)
        if h and len(h.group(1)) == _H1:
            raise ScenarioError(path, i + 1, 'only one H1 allowed')
        if h and len(h.group(1)) == _H2:
            nm = _NAMED.match(h.group(2))
            if nm:
                test, i = _parse_test_body(lines, i + 1, path, nm.group(1), nm.group(2), i + 1)
                add(test, test.line)
                continue
            i += 1
            continue
        m = _ITEM.match(line)
        if m and len(m.group(1)) == 0:
            lm = _LINK.match(m.group(3))
            if lm:
                test = parse_test_file(path.parent / lm.group(2))
                if test.name != lm.group(1):
                    raise ScenarioError(path, i + 1, 'link name does not match test')
                add(test, i + 1)
                i += 1
                continue
        # Anything else at top level is prose: skip the line.
        i += 1
    return pkg
