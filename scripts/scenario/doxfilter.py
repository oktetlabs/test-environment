# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Doxygen input filter: test scenario with its control flow.

Reads a C test source and prints its doxygen header comment with
the scenario injected after @par Scenario: steps come from the
libclang extractor, so string concatenation and escapes are
decoded correctly and each step sits under the condition, loop or
step group that guards it.  The @author block moves after the
scenario, at the bottom of the comment - where the classic awk
filter meant to put it before losing it.

The parse runs in degraded mode (step macros stubbed on the
command line, no compile database), which is all a documentation
build can count on.  When libclang is not installed, or the parse
fails outright, the filter execs the classic c2dox filter with a
warning: doxygen ignores filter failures, so a hard error here
would silently document nothing.
"""

from __future__ import annotations

import argparse
import os
import sys
from dataclasses import replace
from pathlib import Path
from typing import TYPE_CHECKING

import aststeps
import cmaps
from cheader import param_spans, split_source
from steptree import Node, build, node_text

if TYPE_CHECKING:
    from typing import NoReturn

# Degraded-mode parse: step, parameter-doc, and parameter-getter
# macros defined so their uses are recorded as macro
# instantiations, plus the TAPI wrappers every test uses around
# the scenario.
_STUBS = [
    f'-D{name}(...)=(void)0'
    for name in (*aststeps.STEP_MACROS, *aststeps.PARAM_MACROS, aststeps.PARAM_DOC_MACRO)
] + [
    '-DTEST_START=(void)0',
    '-DTEST_END=(void)0',
]


def render_dox(nodes: list[Node], depth: int = 0) -> list[str]:
    """Doxygen list markup for a scenario tree, one line per item.

    Args:
        nodes: The scenario items to render.
        depth: Nesting depth; the top level is numbered (-#), deeper
            levels are bullets, indentation carries the nesting.

    Returns:
        The list lines; construct headings mark their expression up
        as markdown code.
    """
    out: list[str] = []
    for node in nodes:
        bullet = '-#' if depth == 0 else '-'
        out.append(f'{"   " * (depth + 1)}{bullet} {node_text(node)}')
        out.extend(render_dox(node.children, depth + 1))
    return out


def _map_paths(source: Path) -> list[Path]:
    """Header files whose mappings and wrappers this test may use.

    The test's own file and directory come first so local
    definitions win; the TAPI headers under TE_BASE provide the
    shared wrappers (TEST_GET_ETHDEV_STATE and friends).
    """
    paths = [source, *sorted(source.parent.glob('*.h'))]
    te_base = os.environ.get('TE_BASE')
    if te_base:
        paths += sorted((Path(te_base) / 'lib' / 'tapi').glob('*.h'))
    return paths


def parse_setup(
    source: Path,
) -> tuple[list[str], dict[str, str], dict[str, list[str]]]:
    """Everything aststeps.analyze needs to parse a source standalone.

    Harvests the suite's wrapper getters and mapping lists, and
    builds the -D stubs for them and for the step and parameter
    macros, so the source parses without a compile database.
    tests_info shares this with the filter itself, keeping the
    logged scenario identical to the documented one.

    Args:
        source: The C test source about to be analyzed.

    Returns:
        The compiler stub arguments, the harvested wrapper getters,
        and the harvested mapping lists.
    """
    wrappers, mappings = cmaps.harvest(_map_paths(source))
    stubs = _STUBS + [
        f'-D{name}(...)=(void)0'
        for name in sorted(wrappers)
        if name not in aststeps.PARAM_MACROS
    ]
    return stubs, wrappers, mappings


def add_values(
    docs: list[aststeps.ParamDoc],
    bindings: dict[str, aststeps.Binding],
    mappings: dict[str, list[str]],
) -> list[aststeps.ParamDoc]:
    """Docs with 'Possible values:' appended from value mappings.

    Only enum parameters gain the line, and only when the doc does
    not already carry an explicit value list (a markdown bullet
    line): a hand-written list explaining each value wins over the
    mechanical one.

    Args:
        docs: The inline docs, in emission order.
        bindings: Parameter bindings by name, for the mapping link.
        mappings: Value names by mapping-list macro name.
    """
    out: list[aststeps.ParamDoc] = []
    for doc in docs:
        binding = bindings.get(doc.name)
        names: list[str] = []
        if binding is not None and binding.kind == 'enum':
            for macro in binding.map_macros:
                names += mappings.get(macro, [])
        listed = any(line.lstrip().startswith('-') for line in doc.text.split('\n'))
        if not names or listed:
            out.append(doc)
            continue
        joined = ', '.join(f'`{name}`' for name in names)
        # A bullet, not a plain continuation line: doxygen would fold
        # a plain line into the description paragraph.
        out.append(replace(doc, text=f'{doc.text}\n- Possible values: {joined}'))
    return out


def render_params(docs: list[aststeps.ParamDoc]) -> list[str]:
    """@param lines for inline docs, continuation-indented.

    Args:
        docs: The inline docs to render, in the order to emit.

    Returns:
        One '@param name text' entry per doc, names aligned within
        this set, further lines indented under the text column.
    """
    width = max(len(d.name) for d in docs)
    cont = ' * ' + ' ' * (len('@param ') + width + 1)
    out: list[str] = []
    for d in docs:
        first, *rest = d.text.split('\n')
        out.append(f' * @param {d.name:<{width}} {first}')
        out.extend(f'{cont}{line}' for line in rest)
    return out


def _replace_spans(
    header: list[str], spans: list[tuple[str, int, int]], block: list[str]
) -> list[str]:
    """Header lines with every span's range dropped, block inserted at the first.

    Lines outside the spans are kept in place, so non-@param content
    sitting between two @param blocks survives - after the merged
    block rather than where it originally sat, since all the spans
    collapse to the first one's position.
    """
    drop = {i for _, start, stop in spans for i in range(start, stop)}
    first = spans[0][1]
    out: list[str] = []
    for i, line in enumerate(header):
        if i == first:
            out.extend(block)
        if i not in drop:
            out.append(line)
    return out


def merge_params(header: list[str], docs: list[aststeps.ParamDoc]) -> list[str]:
    """Header lines with the inline param docs merged in.

    Inline docs win: a header @param block for a name that also has
    an inline doc is dropped, surviving header blocks keep their
    text and lead the block (they are the transition leftovers),
    and the inline docs follow in source order.  Non-@param content
    sitting between @param blocks (an @note paragraph, a blank
    separator line...) is not dropped, but it is not interleaved
    either: all @param blocks collapse to one place, so that content
    survives after the merged block instead of where it originally
    sat.  Without any header @param the block lands in front of
    @par Scenario, or at the end of the header when there is no
    marker yet.
    """
    if not docs:
        return header
    spans = param_spans(header)
    inline = {d.name for d in docs}
    kept = [
        line
        for name, start, stop in spans
        if name not in inline
        for line in header[start:stop]
    ]
    block = kept + render_params(docs)
    if spans:
        return _replace_spans(header, spans, block)
    for i, line in enumerate(header):
        if '@par Scenario' in line:
            return header[:i] + block + [' *'] + header[i:]
    return [*header, ' *', *block]


def _split_authors(header: list[str]) -> tuple[list[str], list[str]]:
    """Header lines without the @author blocks, and those blocks.

    The scenario goes to the end of the comment, and the authors
    read better after it - which is also where the classic c2dox
    filter meant to put them.
    """
    kept: list[str] = []
    authors: list[str] = []
    in_author = False
    for line in header:
        stripped = line.strip().lstrip('/').lstrip('*').strip()
        if stripped.startswith('@author'):
            in_author = True
            authors.append(line)
            continue
        if in_author and stripped and not stripped.startswith('@'):
            authors.append(line)
            continue
        in_author = False
        kept.append(line)
    return kept, authors


def filter_text(source: Path) -> str:
    """The filtered form of a test source, ready for doxygen.

    Args:
        source: The C test source doxygen wants filtered.

    Returns:
        The header comment with the scenario injected after
        @par Scenario (added when missing), then the group-closing
        one-line comments; empty for a file without a doxygen
        header comment.

    Raises:
        OSError: The file cannot be read.
        RuntimeError: libclang could not parse it.
    """
    text = source.read_text(encoding='utf-8', errors='replace')
    split = split_source(text)
    if split is None:
        return ''
    header, trailing = split
    header, authors = _split_authors(header)
    stubs, wrappers, mappings = parse_setup(source)
    info = aststeps.analyze(source, extra_args=stubs, wrappers=wrappers)
    steps = info.steps
    docs = add_values(info.param_docs, info.bindings, mappings)
    documented = {d.name for d in docs}
    documented.update(name for name, _, _ in param_spans(header))
    for name in sorted(set(info.bindings) - documented):
        print(
            f'doxfilter: warning: {source}: parameter {name} is undocumented',
            file=sys.stderr,
        )
    header = merge_params(header, docs)
    out = list(header)
    if steps:
        if not any('@par Scenario' in line for line in header):
            out.append(' * @par Scenario:')
        out.extend(render_dox(build(steps)))
    if authors:
        out.append(' *')
        out.extend(authors)
    out.append(' */')
    out.extend(trailing)
    return '\n'.join(out) + '\n'


def _fallback(source: Path, reason: str) -> NoReturn:
    """Exec the classic c2dox filter, never returning."""
    print(f'doxfilter: {reason}; using c2dox for {source}', file=sys.stderr)
    c2dox = Path(__file__).resolve().parent.parent / 'c2dox'
    argv = ['gawk', '-f', str(c2dox), str(source)]
    os.execvp('gawk', argv)  # noqa: S606,S607 - gawk from PATH


def main(argv: list[str] | None = None) -> int:
    """Run the filter on one source file, printing to stdout.

    Falls back to the classic c2dox filter when libclang is missing
    or the parse fails, so a documentation build always gets pages.
    """
    parser = argparse.ArgumentParser(description='doxygen filter for C test sources')
    parser.add_argument('source', help='the C source doxygen wants filtered')
    args = parser.parse_args(argv)
    source = Path(args.source)
    if not aststeps.HAVE_CLANG:
        _fallback(source, 'libclang is not installed')
    try:
        sys.stdout.write(filter_text(source))
    except (OSError, RuntimeError) as exc:
        _fallback(source, str(exc))
    return 0


if __name__ == '__main__':
    sys.exit(main())
