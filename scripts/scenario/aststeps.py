# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Steps with their control flow, extracted from a test source.

Parses the test with libclang and reports each step together with
the if/else/loop/switch constructs enclosing it, so a conditional
step reads as conditional instead of flattened the way the log
and a plain text scan necessarily flatten it.

The libclang Python package is an optional dependency: install it
with `pip install libclang` to use source mode.
"""

from __future__ import annotations

import contextlib
import json
import os
import shlex
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path

try:
    from clang import cindex

    HAVE_CLANG = True
except ImportError:  # pragma: no cover - exercised only without libclang
    HAVE_CLANG = False

_MACROS = {
    'TEST_STEP': 'STEP',
    'TEST_SUBSTEP': 'SUBSTEP',
    'TEST_STEP_PUSH': 'PUSH',
    'TEST_STEP_POP': 'POP',
    'TEST_STEP_NEXT': 'NEXT',
    'TEST_STEP_PUSH_INFO': 'PUSH_INFO',
    'TEST_STEP_POP_INFO': 'POP_INFO',
    'TEST_STEP_RESET': 'RESET',
}

# TEST_GET_* accessors: the C variable name equals the test
# parameter name, which is what lets package.xml values bind to
# condition identifiers.
_PARAM_MACROS = {
    'TEST_GET_INT_PARAM': 'int',
    'TEST_GET_UINT_PARAM': 'uint',
    'TEST_GET_INT64_PARAM': 'int',
    'TEST_GET_UINT64_PARAM': 'uint',
    'TEST_GET_DOUBLE_PARAM': 'double',
    'TEST_GET_DEFAULT_DOUBLE_PARAM': 'double',
    'TEST_GET_OPT_UINT_PARAM': 'uint',
    'TEST_GET_OPT_DOUBLE_PARAM': 'double',
    'TEST_GET_STRING_PARAM': 'string',
    'TEST_GET_OPT_STRING_PARAM': 'string',
    'TEST_GET_BOOL_PARAM': 'bool',
    'TEST_GET_ENUM_PARAM': 'enum',
}

_BOOL_MAPPING = {'TRUE': 1, 'FALSE': 0}

# Object-like macro definitions with more tokens than this are not
# worth harvesting as scalar values; a mapping table macro like
# MODE_MAP still fits comfortably under it.
_MAX_MACRO_TOKENS = 64

# Compiler arguments that make no sense for a bare parse.
_DROP_ARGS = {'-c', '-MD', '-MMD'}
_DROP_WITH_VALUE = {'-o', '-MQ', '-MF', '-MT'}


@dataclass
class Cond:
    """One control construct enclosing a step.

    Attributes:
        kind: Construct kind: 'if', 'else', 'for', 'while', 'do',
            'switch', or 'goto' for the if (0) landing pad.
        cond: The controlling expression as written in the source
            (for 'else' the paired if's expression; '0' for 'goto').
        desc: Human-readable description, used as the annotation.
        init: The init clause of a for header, when it parsed.
        incr: The increment clause of a for header, when it parsed.
    """

    kind: str
    cond: str
    desc: str
    init: str | None = None
    incr: str | None = None


@dataclass
class SourceStep:
    """One step as written in the source.

    Attributes:
        kind: Step kind, a value of _MACROS ('STEP', 'SUBSTEP', ...).
        line: Line of the macro use in the source file.
        text: The step message with adjacent string literals
            concatenated and escapes decoded.
        func: Name of the function the step is written in; empty
            when no enclosing definition was found.
        conds: The control constructs enclosing the step,
            outermost first.
    """

    kind: str
    line: int
    text: str
    func: str
    conds: list[Cond] = field(default_factory=list)


@dataclass
class Binding:
    """One test parameter read via a TEST_GET_* macro.

    Attributes:
        name: The parameter name (equals the C variable name by TE
            convention).
        kind: Accessor kind, a value of _PARAM_MACROS ('int',
            'bool', 'enum', ...).
        line: Line of the accessor use in the source file.
        mapping: Value-string to number mapping for bool and enum
            parameters; None while (or if) unresolved.
        map_macros: Names of the mapping-list macros an enum
            accessor was given, for later resolution.
    """

    name: str
    kind: str
    line: int
    mapping: dict[str, int] | None = None
    map_macros: list[str] = field(default_factory=list)


@dataclass
class SourceInfo:
    """Everything the analysis reads out of one test source.

    Attributes:
        steps: The steps in source order.
        bindings: Parameter bindings keyed by parameter name.
        enums: Enum constant values seen by the parse, any file.
        macros: Object-like integer macro values, any file.
        macro_tokens: Raw definition tokens of object-like macros,
            kept for decoding mapping-list macros.
    """

    steps: list[SourceStep]
    bindings: dict[str, Binding]
    enums: dict[str, int]
    macros: dict[str, int]
    macro_tokens: dict[str, list[str]]


def compile_args(source: Path, compile_db: Path) -> tuple[list[str], str]:
    """Compiler args and working directory for a source file.

    Args:
        source: The C source to look up; matched by file name suffix
            against the database entries.
        compile_db: The compile_commands.json to read.

    Returns:
        The compile arguments with output/dependency options dropped,
        and the directory the compile command runs from (relative
        include paths are relative to it).

    Raises:
        ValueError: The database has no entry for the file.
    """
    entries = json.loads(compile_db.read_text(encoding='utf-8'))
    name = source.name
    for entry in entries:
        if not entry['file'].endswith(name):
            continue
        raw = entry.get('arguments') or shlex.split(entry['command'])
        args = []
        skip = False
        for arg in raw[1:]:
            if skip:
                skip = False
                continue
            if arg in _DROP_WITH_VALUE:
                skip = True
                continue
            if arg in _DROP_ARGS or arg.endswith(name):
                continue
            args.append(arg)
        return args, entry['directory']
    msg = f'{compile_db}: no entry for {name}'
    raise ValueError(msg)


def find_compile_db(source: Path) -> Path | None:
    """Find a compile_commands.json mentioning the source file.

    Walks the ancestors of the source directory looking for build
    trees, which is where meson leaves the database.

    Args:
        source: The C source the database must have an entry for.

    Returns:
        The first database found under an ancestor's build/
        directory that mentions the source, or None.
    """
    for ancestor in source.resolve().parents:
        build = ancestor / 'build'
        if not build.is_dir():
            continue
        for cand in sorted(build.rglob('compile_commands.json')):
            try:
                compile_args(source, cand)
            except (ValueError, OSError, json.JSONDecodeError):
                continue
            return cand
    return None


def _builtin_include() -> str | None:
    """A clang builtin-headers directory (stddef.h and friends).

    The libclang wheel ships no builtin headers, so without one of
    these the system headers fail to parse and function bodies are
    dropped from the AST.

    Returns:
        The newest directory containing stddef.h under the known
        distro locations, or None when there is none.
    """
    from pathlib import Path  # noqa: PLC0415 - only needed in source mode

    for root, pattern in (
        (Path('/usr/lib/clang'), '*/include'),
        (Path('/usr/lib'), 'llvm-*/lib/clang/*/include'),
    ):
        for cand in sorted(root.glob(pattern), reverse=True):
            if (cand / 'stddef.h').exists():
                return str(cand)
    return None


@lru_cache(maxsize=32)
def _file_text(path: str) -> str:
    """Cached text of a source file, decoded permissively."""
    return Path(path).read_text(encoding='utf-8', errors='replace')


def _source_text(cursor: object) -> str:
    """The source text of a cursor, sliced by extent offsets.

    Slicing the file keeps the spelling exactly as written; joining
    tokens instead would mangle spacing around unary operators and
    macro calls (probe finding: "- TE_RC (TE_RPC, ...)").

    Returns:
        The cursor's source text with whitespace runs collapsed to
        single spaces; empty for cursors without a file location.
    """
    ext = cursor.extent  # type: ignore[attr-defined]
    if ext.start.file is None:
        return ''
    text = _file_text(ext.start.file.name)[ext.start.offset : ext.end.offset]
    return ' '.join(text.split())


def _span_text(cursor: object, until: object) -> str:
    """Source text of a cursor up to the start of a child cursor.

    Used for loop headers: the text of the statement up to its body.

    Args:
        cursor: The cursor whose text starts the span.
        until: The child cursor the span stops in front of.

    Returns:
        The whitespace-collapsed text with a trailing opening brace
        stripped; empty for cursors without a file location.
    """
    ext = cursor.extent  # type: ignore[attr-defined]
    stop = until.extent.start.offset  # type: ignore[attr-defined]
    if ext.start.file is None:
        return ''
    text = _file_text(ext.start.file.name)[ext.start.offset : stop]
    return ' '.join(text.split()).rstrip('{ ').rstrip()


def _step_text(cursor: object) -> str:
    """The concatenated format string literal of a step macro use.

    Reads the macro's tokens up to the first top-level comma, so
    only the format string is taken, not the format arguments.

    Returns:
        Adjacent string literals joined, with escape sequences
        decoded and escaped newlines turned into spaces.
    """
    parts = []
    for tok in cursor.get_tokens():  # type: ignore[attr-defined]
        if tok.kind == cindex.TokenKind.LITERAL and tok.spelling.startswith('"'):
            raw = tok.spelling[1:-1]
            parts.append(raw.replace('\\"', '"').replace('\\\\', '\\').replace('\\n', ' '))
        elif tok.spelling == ',':
            break
    return ''.join(parts)


def _extent_key(cursor: object) -> tuple[int, int, int, int]:
    """(start line, start column, end line, end column) of a cursor."""
    ext = cursor.extent  # type: ignore[attr-defined]
    return (ext.start.line, ext.start.column, ext.end.line, ext.end.column)


def _contains(extent: tuple[int, int, int, int], line: int, col: int) -> bool:
    """Whether a source position falls inside an extent key."""
    sl, sc, el, ec = extent
    if line < sl or line > el:
        return False
    if line == sl and col < sc:
        return False
    return not (line == el and col > ec)


def _split_top(text: str, sep: str) -> list[str]:
    """Split on a separator at bracket depth zero.

    Args:
        text: The text to split.
        sep: A single-character separator.

    Returns:
        The parts between top-level separators; separators inside
        parentheses or square brackets do not split.
    """
    parts: list[str] = []
    cur: list[str] = []
    depth = 0
    for ch in text:
        if ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
        if ch == sep and depth == 0:
            parts.append(''.join(cur))
            cur = []
        else:
            cur.append(ch)
    parts.append(''.join(cur))
    return parts


def _for_parts(header: str) -> tuple[str | None, str, str | None]:
    """(init, cond, incr) clauses of a for header, best effort.

    Args:
        header: The loop header text, 'for (...)' as written.

    Returns:
        The three clause texts; init and incr are None when empty or
        when the header does not have the three-clause shape, and
        cond is empty then too.
    """
    inner = header.removeprefix('for').strip()
    if not (inner.startswith('(') and inner.endswith(')')):
        return None, '', None
    parts = _split_top(inner[1:-1], ';')
    if len(parts) != 3:  # noqa: PLR2004 - init; cond; incr
        return None, '', None
    init, cond, incr = (p.strip() for p in parts)
    return init or None, cond, incr or None


def _loop_cond(kind: object, kids: list, header: str) -> Cond:
    """The Cond for a for/while loop header.

    Args:
        kind: The statement's cursor kind (FOR_STMT or WHILE_STMT).
        kids: The statement's child cursors.
        header: The header text up to the loop body.
    """
    if kind == cindex.CursorKind.FOR_STMT:
        init, cond_part, incr = _for_parts(header)
        return Cond(kind='for', cond=cond_part, desc=header, init=init, incr=incr)
    cond_part = _source_text(kids[0])
    return Cond(kind='while', cond=cond_part, desc=header)


def _control_regions(func: object) -> list[tuple[tuple[int, int, int, int], Cond]]:
    """(extent, cond) for every control construct in a function.

    A region is the guarded range of an if/else branch, a loop body,
    or a switch body, carrying the Cond that describes the construct.
    Constructs that a macro expansion introduces (the do-while of
    the step macros themselves, TAPI_ON_JMP's hidden if) are
    filtered out by checking that the construct's first token is its
    own keyword.

    Args:
        func: A function definition cursor.

    Returns:
        The regions in tree walk order; a step location contained in
        a region's extent is guarded by that construct.
    """
    regions = []

    def written(child: object, keyword: str) -> bool:
        """True iff the construct is spelled out, not macro-expanded."""
        first = next(child.get_tokens(), None)  # type: ignore[attr-defined]
        return first is not None and first.spelling == keyword

    def walk(cursor: object) -> None:
        for child in cursor.get_children():  # type: ignore[attr-defined]
            kind = child.kind
            kids = list(child.get_children())
            if (
                kind == cindex.CursorKind.IF_STMT
                and len(kids) >= 2  # noqa: PLR2004
                and written(child, 'if')
            ):
                cond = _source_text(kids[0])
                # The if (0) { label: ... } idiom marks a block only
                # ever entered through a goto, typically the error
                # path.  Read it as such, not as dead code.
                if cond == '0':
                    then_cond = Cond(kind='goto', cond='0', desc='if (0), reached by goto')
                else:
                    then_cond = Cond(kind='if', cond=cond, desc=f'if ({cond})')
                regions.append((_extent_key(kids[1]), then_cond))
                if len(kids) >= 3:  # noqa: PLR2004 - if with an else branch
                    # Name what is false, not just "else": the branch may
                    # sit far from its if, and its description has to
                    # stand alone.  For an else-if chain the nested if
                    # adds its own condition after this one.
                    else_cond = Cond(kind='else', cond=cond, desc=f'!({cond})')
                    regions.append((_extent_key(kids[2]), else_cond))
            elif (
                kind in (cindex.CursorKind.FOR_STMT, cindex.CursorKind.WHILE_STMT)
                and kids
                and (written(child, 'for') or written(child, 'while'))
            ):
                header = _span_text(child, kids[-1])
                loop_cond = _loop_cond(kind, kids, header)
                regions.append((_extent_key(kids[-1]), loop_cond))
            elif kind == cindex.CursorKind.DO_STMT and kids and written(child, 'do'):
                # Unlike for/while, clang orders do-while children as
                # [body, condition]: the body is first, and the header
                # worth showing is the trailing while.
                cond = _source_text(kids[-1])
                do_cond = Cond(kind='do', cond=cond, desc=f'do while ({cond})')
                regions.append((_extent_key(kids[0]), do_cond))
            elif (
                kind == cindex.CursorKind.SWITCH_STMT
                and len(kids) >= 2  # noqa: PLR2004
                and written(child, 'switch')
            ):
                cond = _source_text(kids[0])
                switch_cond = Cond(kind='switch', cond=cond, desc=f'switch ({cond})')
                regions.append((_extent_key(kids[1]), switch_cond))
            walk(child)

    walk(func)
    return regions


def _enclosure(
    funcs: list[tuple[object, tuple[int, int, int, int], list]],
    line: int,
    col: int,
) -> tuple[str, list[Cond]]:
    """Function name and control constructs enclosing a location.

    Args:
        funcs: (cursor, extent key, control regions) per function
            definition, as prepared by analyze().
        line: Source line of the location.
        col: Source column of the location.

    Returns:
        The enclosing function's name and the Cond records of the
        constructs around the location, outermost first; an empty
        name and list when no function contains the location.
    """
    for func, extent, regions in funcs:
        if not _contains(extent, line, col):
            continue
        inner = [(ext, cond) for ext, cond in regions if _contains(ext, line, col)]
        inner.sort(key=lambda r: (r[0][0], r[0][1]))
        return func.spelling, [cond for _, cond in inner]  # type: ignore[attr-defined]
    return '', []


def _parse(  # type: ignore[no-any-unimported]
    source: Path, args: list[str], workdir: str | None
) -> cindex.TranslationUnit:
    """Parse the source with libclang from the compile entry directory.

    Relative include paths in the compile command are relative to the
    entry's directory, so the parse runs from there.

    Args:
        source: The C source to parse.
        args: Compiler arguments for the parse.
        workdir: Directory to parse from, or None to stay put.

    Returns:
        The translation unit, parsed with the detailed processing
        record so macro instantiations appear as cursors.

    Raises:
        RuntimeError: libclang could not parse the file at all.
    """
    index = cindex.Index.create()
    resolved = source.resolve()
    cwd = os.getcwd()  # noqa: PTH109 - paired with os.chdir below
    try:
        if workdir is not None:
            os.chdir(workdir)
        return index.parse(
            str(resolved),
            args=args,
            options=cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD,
        )
    except cindex.TranslationUnitLoadError as exc:
        msg = f'{source}: parse failed (stale compile_commands.json?)'
        raise RuntimeError(msg) from exc
    finally:
        with contextlib.suppress(OSError):
            os.chdir(cwd)


def _int_literal(tok: str) -> int | None:
    """The value of an integer literal token (any base), or None."""
    try:
        return int(tok.rstrip('uUlL'), 0)
    except ValueError:
        return None


def _macro_args(cursor: object) -> list[str]:
    """Top-level comma-separated argument texts of a macro use.

    Args:
        cursor: A macro instantiation cursor.

    Returns:
        One space-joined token string per argument; commas nested in
        parentheses do not split.  Empty for a use without
        parentheses.
    """
    toks = [t.spelling for t in cursor.get_tokens()]  # type: ignore[attr-defined]
    args: list[str] = []
    cur: list[str] = []
    depth = 0
    for tok in toks[1:]:
        if tok == '(':
            depth += 1
            if depth == 1:
                continue
        elif tok == ')':
            depth -= 1
            if depth == 0:
                break
        if tok == ',' and depth == 1:
            args.append(' '.join(cur))
            cur = []
            continue
        cur.append(tok)
    if cur:
        args.append(' '.join(cur))
    return args


def _harvest_decl(
    cursor: object,
    enums: dict[str, int],
    macros: dict[str, int],
    macro_tokens: dict[str, list[str]],
) -> bool:
    """Record an enum constant or object-like macro; True if handled.

    Runs unfiltered by source file: enum constants and macros from
    headers are legal condition identifiers too.

    Args:
        cursor: Any cursor from the tree walk.
        enums: Enum constant values, updated in place.
        macros: Integer macro values, updated in place.
        macro_tokens: Raw macro definition tokens, updated in place.

    Returns:
        True when the cursor was one of the harvested kinds (the
        caller skips further dispatch), False otherwise.
    """
    if cursor.kind == cindex.CursorKind.ENUM_CONSTANT_DECL:  # type: ignore[attr-defined]
        enums[cursor.spelling] = cursor.enum_value  # type: ignore[attr-defined]
        return True
    if cursor.kind == cindex.CursorKind.MACRO_DEFINITION:  # type: ignore[attr-defined]
        toks = [t.spelling for t in cursor.get_tokens()]  # type: ignore[attr-defined]
        if 1 < len(toks) <= _MAX_MACRO_TOKENS:
            macro_tokens[toks[0]] = toks[1:]
            value = _int_literal(toks[1])
            if len(toks) == 2 and value is not None:  # noqa: PLR2004
                macros[toks[0]] = value
        return True
    return False


def _param_binding(cursor: object, line: int) -> Binding | None:
    """A Binding for a TEST_GET_*_PARAM use, or None if not one.

    Bool accessors bind with the builtin TRUE/FALSE mapping right
    away; enum accessors record their mapping-list macro names only
    when every extra argument is a plain identifier, leaving the
    mapping unresolved otherwise.

    Args:
        cursor: A macro instantiation cursor.
        line: Source line of the use, stored in the binding.
    """
    pkind = _PARAM_MACROS.get(cursor.spelling)  # type: ignore[attr-defined]
    if pkind is None:
        return None
    margs = _macro_args(cursor)
    if not margs:
        return None
    binding = Binding(name=margs[0], kind=pkind, line=line)
    if pkind == 'bool':
        binding.mapping = dict(_BOOL_MAPPING)
    elif pkind == 'enum':
        maps = [a for a in margs[1:] if a.isidentifier()]
        if len(maps) == len(margs) - 1:
            binding.map_macros = maps
    return binding


def _walk(  # type: ignore[no-any-unimported]
    tu: cindex.TranslationUnit,
    funcs: list[tuple[object, tuple[int, int, int, int], list]],
    source: Path,
) -> SourceInfo:
    """Walk the parsed tree once, collecting steps, bindings, and declarations.

    Declarations are harvested from every file; step and parameter
    macro uses only from the analyzed source itself.

    Args:
        tu: The parsed translation unit.
        funcs: (cursor, extent key, control regions) per function
            definition, for attributing steps.
        source: The analyzed source, for the in-this-file filter.
    """
    steps = []
    bindings: dict[str, Binding] = {}
    enums: dict[str, int] = {}
    macros: dict[str, int] = {}
    macro_tokens: dict[str, list[str]] = {}
    for cursor in tu.cursor.walk_preorder():
        if _harvest_decl(cursor, enums, macros, macro_tokens):
            continue
        if cursor.kind != cindex.CursorKind.MACRO_INSTANTIATION:
            continue
        loc = cursor.location
        if loc.file is None or not loc.file.name.endswith(source.name):
            continue
        kind = _MACROS.get(cursor.spelling)
        if kind is None:
            binding = _param_binding(cursor, loc.line)
            if binding is not None:
                bindings.setdefault(binding.name, binding)
            continue
        func_name, conds = _enclosure(funcs, loc.line, loc.column)
        steps.append(
            SourceStep(
                kind=kind,
                line=loc.line,
                text=_step_text(cursor),
                func=func_name,
                conds=conds,
            )
        )
    steps.sort(key=lambda s: s.line)
    return SourceInfo(
        steps=steps,
        bindings=bindings,
        enums=enums,
        macros=macros,
        macro_tokens=macro_tokens,
    )


def analyze(
    source: Path,
    compile_db: Path | None = None,
    extra_args: list[str] | None = None,
) -> SourceInfo:
    """Analyze a test source: steps, parameter bindings, and declarations.

    Args:
        source: The C source to analyze.
        compile_db: compile_commands.json to take compiler flags
            from; None parses with the extra arguments alone.
        extra_args: Additional compiler arguments, e.g. -D stubs for
            the step macros when no compile database is available.

    Returns:
        The steps in source order plus everything needed to bind
        parameter values to their conditions later.

    Raises:
        RuntimeError: libclang is not installed, or the parse failed.
        ValueError: The compile database has no entry for the file.
    """
    if not HAVE_CLANG:
        msg = 'source mode needs the libclang package (pip install libclang)'
        raise RuntimeError(msg)

    args: list[str] = []
    workdir = None
    if compile_db is not None:
        args, workdir = compile_args(source, compile_db)
    if extra_args:
        args += extra_args
    builtin = _builtin_include()
    if builtin is not None:
        args += ['-isystem', builtin]

    tu = _parse(source, args, workdir)

    # Function definitions in this file, with their control regions.
    funcs = [
        (cursor, _extent_key(cursor), _control_regions(cursor))
        for cursor in tu.cursor.get_children()
        if (
            cursor.kind == cindex.CursorKind.FUNCTION_DECL
            and cursor.is_definition()
            and cursor.location.file is not None
            and cursor.location.file.name.endswith(source.name)
        )
    ]

    return _walk(tu, funcs, source)


def extract(
    source: Path,
    compile_db: Path | None = None,
    extra_args: list[str] | None = None,
) -> list[SourceStep]:
    """Extract the steps of a test source with their control flow.

    A convenience wrapper around analyze() for callers that need
    only the steps; arguments, errors, and ordering are analyze()'s.
    """
    return analyze(source, compile_db, extra_args).steps
