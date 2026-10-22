# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Data model of a markdown test scenario."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import TYPE_CHECKING, ClassVar

if TYPE_CHECKING:
    from collections.abc import Collection, Iterator
    from pathlib import Path

_CODE_SPAN = re.compile(r'`([^`]+)`')
_ALL_CAPS = re.compile(r'[A-Z][A-Z0-9_]+')


class ScenarioError(Exception):
    """A parse or consistency error with a file position."""

    def __init__(self, path: Path, line: int, msg: str) -> None:
        super().__init__(f'{path}:{line}: {msg}')
        self.path = path
        self.line = line
        self.msg = msg


@dataclass
class Value:
    """One value of a parameter, with an optional comment."""

    name: str
    comment: str | None = None


@dataclass
class Param:
    """A test parameter."""

    name: str
    description: str
    values: list[Value] = field(default_factory=list)


@dataclass
class Note:
    """Extra description (impl=False) or implementor advice."""

    text: str
    impl: bool = False


@dataclass
class Step:
    """A step with notes and nested substeps."""

    text: str
    line: int
    notes: list[Note] = field(default_factory=list)
    sub: list[Step] = field(default_factory=list)


@dataclass
class Test:
    """A single test scenario."""

    # Keep pytest from collecting this dataclass as a test class.
    __test__: ClassVar[bool] = False

    name: str
    summary: str
    path: Path
    line: int
    objective: str = ''
    type: str | None = None
    notes: list[str] = field(default_factory=list)
    # Implementor advice for the test as a whole: the blockquotes
    # outside the steps. Never documented, never compared.
    impl: list[str] = field(default_factory=list)
    params: list[Param] = field(default_factory=list)
    steps: list[Step] = field(default_factory=list)


@dataclass
class Package:
    """A package document: the tests of one suite package."""

    name: str
    summary: str
    path: Path
    tests: list[Test] = field(default_factory=list)
    # The doxygen group the tests and the description join: the
    # package name unless a "Group:" label names another one.
    group: str = ''
    # The "## Description" section as raw markdown lines, and the
    # line of its heading (0 when there is none).
    description: list[str] = field(default_factory=list)
    description_line: int = 0

    def __post_init__(self) -> None:
        if not self.group:
            self.group = self.name


def normalize_ws(text: str) -> str:
    """Collapse all whitespace runs to single spaces and strip."""
    return ' '.join(text.split())


def resolve_inline(
    text: str,
    params: Collection[str],
    values: Collection[str],
) -> str:
    """Resolve backticked tokens to TE log conventions.

    Declared parameters become "@p name", listed values and
    ALL_CAPS tokens become "@c TOKEN", anything else is left as
    the bare token.
    """

    def sub(m: re.Match[str]) -> str:
        tok = m.group(1)
        if tok in params:
            return f'@p {tok}'
        if tok in values or _ALL_CAPS.fullmatch(tok):
            return f'@c {tok}'
        return tok

    return _CODE_SPAN.sub(sub, text)


def param_doc_lines(
    p: Param,
    params: Collection[str],
    values: Collection[str],
) -> list[str]:
    """The canonical doc lines of a parameter, description first.

    The single source for what a parameter's documentation looks
    like in C: the emitter writes these lines into TEST_PARAM_DOC
    arguments and the drift check compares against them.  With
    values the description gets a trailing colon and one
    '- @c value (comment)' bullet per value.
    """
    desc = resolve_inline(p.description, params, values)
    if p.values:
        desc = desc.rstrip('.:') + ':'
    lines = [desc]
    for v in p.values:
        text = f'- @c {v.name}'
        if v.comment:
            text += f' ({v.comment})'
        lines.append(text)
    return lines


def flatten_steps(
    steps: list[Step],
    depth: int = 1,
) -> Iterator[tuple[int, Step]]:
    """Yield (depth, step) depth-first, starting at depth 1."""
    for step in steps:
        yield depth, step
        yield from flatten_steps(step.sub, depth + 1)
