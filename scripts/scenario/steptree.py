# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Nested scenario tree built from a flat step list.

Documentation wants a scenario shaped the way a human would write
it: steps under the condition or loop that guards them, substeps
under their step, pushed step groups under their heading.  This
module folds the flat, source-ordered step list that aststeps
extracts into such a tree; what markup the tree becomes is the
emitter's business (the doxygen filter today, RST later).
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from aststeps import Cond, SourceStep

# Step kinds that add runtime information rather than scenario
# structure, and so do not belong in documentation.
_INFO_KINDS = frozenset({'PUSH_INFO', 'POP_INFO'})

# Heading phrasing per construct kind: text before and after the
# condition expression.  The kinds not listed here have headings
# that do not follow the simple before-expression-after shape.
_LABELS = {
    'if': ('If ', ':'),
    'else': ('If not ', ':'),
    'while': ('While ', ':'),
    'do': ('Do, repeating while ', ':'),
    'switch': ('Depending on ', ':'),
}


@dataclass
class Node:
    """One scenario item: a step, or a control construct grouping steps.

    Attributes:
        kind: A step kind ('STEP', 'SUBSTEP', 'PUSH', 'NEXT') or
            'COND' for a control construct grouping.
        text: The step text; empty for construct nodes.
        cond: The construct for 'COND' nodes, None otherwise.
        children: Items nested under this one.
    """

    kind: str
    text: str = ''
    cond: Cond | None = None
    children: list[Node] = field(default_factory=list)


def cond_label(cond: Cond) -> tuple[str, str, str]:
    """(before, expression, after) phrasing of a construct heading.

    The middle part is the C expression for the emitter to mark up
    as code; it is empty when the heading is pure prose.

    Args:
        cond: The construct to phrase.

    Returns:
        Text before the expression, the expression itself, and the
        text after it, e.g. ('If ', 'x > 0', ':').
    """
    if cond.kind == 'goto':
        return 'Only on the error path:', '', ''
    if cond.kind == 'for':
        inner = cond.desc.removeprefix('for').strip()
        if inner.startswith('(') and inner.endswith(')'):
            inner = inner[1:-1].strip()
        return 'For each iteration (', inner, '):'
    before, after = _LABELS.get(cond.kind, _LABELS['if'])
    return before, cond.cond, after


class _Builder:
    """Mutable state of one tree construction pass."""

    def __init__(self) -> None:
        self.root: list[Node] = []
        # Open containers, outermost first, in the order they
        # opened: condition regions ('cond', description)
        # interleaved with step groups from TEST_STEP_PUSH
        # ('push', None).  A condition region is identified by its
        # description: consecutive steps whose condition chains
        # share a description prefix sit in the same regions.
        # Closing a context closes everything opened above it.
        self.contexts: list[tuple[str, str | None, Node]] = []
        # The step a TEST_SUBSTEP attaches to, per container list.
        self.last_step: dict[int, Node] = {}

    def container(self) -> list[Node]:
        """The children list new items go into (innermost context)."""
        return self.contexts[-1][2].children if self.contexts else self.root

    def close_push(self, *, first: bool = False) -> None:
        """Close the last (or with first, every) open step group.

        Closing a group also closes the condition regions opened
        inside it.
        """
        at = [i for i, (tag, _, _) in enumerate(self.contexts) if tag == 'push']
        if at:
            del self.contexts[at[0] if first else at[-1] :]

    def enter_regions(self, conds: list[Cond]) -> None:
        """Trim to the shared condition prefix, open the new regions.

        A dropped region takes the step groups opened inside it
        along; groups below the last surviving region stay open.
        """
        descs = [c.desc for c in conds]
        keep = 0
        for i, (tag, desc, _) in enumerate(self.contexts):
            if tag != 'cond':
                continue
            if keep >= len(descs) or desc != descs[keep]:
                del self.contexts[i:]
                break
            keep += 1
        for cond in conds[keep:]:
            node = Node(kind='COND', cond=cond)
            self.container().append(node)
            self.contexts.append(('cond', cond.desc, node))

    def add(self, step: SourceStep) -> None:
        """Place one step into the tree, updating the open contexts."""
        self.enter_regions(step.conds)
        if step.kind == 'POP':
            self.close_push()
            return
        if step.kind == 'RESET':
            self.close_push(first=True)
            return
        if step.kind == 'NEXT':
            self.close_push()
        node = Node(kind=step.kind, text=step.text)
        into = self.container()
        if step.kind == 'SUBSTEP':
            parent = self.last_step.get(id(into))
            (parent.children if parent is not None else into).append(node)
            return
        into.append(node)
        if step.kind in ('PUSH', 'NEXT'):
            self.contexts.append(('push', None, node))
        else:
            self.last_step[id(into)] = node


def build(steps: list[SourceStep]) -> list[Node]:
    """Fold a source-ordered step list into a scenario tree.

    Args:
        steps: The steps as aststeps extracts them, in source order,
            each carrying its enclosing control constructs.

    Returns:
        The top-level scenario items; steps guarded by a construct
        hang under its node, substeps under their step, pushed
        groups under their heading, and the PUSH_INFO/POP_INFO
        runtime markers are dropped.
    """
    builder = _Builder()
    for step in steps:
        if step.kind not in _INFO_KINDS:
            builder.add(step)
    _lift_detail_regions(builder.root)
    return builder.root


def _lift_detail_regions(items: list[Node]) -> None:
    """Attach detail-only construct nodes under their leading step.

    The common shape "TEST_STEP describes the loop, TEST_SUBSTEPs
    inside the loop detail it" builds the construct node as the
    step's sibling; a construct holding only substeps and nested
    constructs reads better as part of the step above it.
    """
    i = 0
    while i < len(items):
        node = items[i]
        _lift_detail_regions(node.children)
        if (
            node.kind == 'COND'
            and i > 0
            and items[i - 1].kind == 'STEP'
            and node.children
            and all(child.kind in ('SUBSTEP', 'COND') for child in node.children)
        ):
            items[i - 1].children.append(items.pop(i))
            continue
        i += 1
