# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tri-state evaluation of C condition texts against known values.

Evaluates the condition strings that aststeps slices out of a test
source, with test parameters bound to numbers.  Anything outside
the supported subset (unknown identifiers, function calls, member
or array access, string and char literals) makes the enclosing
subexpression "undecided" instead of failing: `evaluate` answers
True, False, or None for cannot-tell, and a step annotation is
kept exactly when the answer is None.
"""

from __future__ import annotations

import operator
import re
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Callable, Mapping

Num = int | float

# One C token per match, alternatives tried in order:
# - num: a hex integer (0x... with optional uUlL suffixes; f/F are
#   hex digits there, not suffixes) or a decimal integer or float
#   with an optional exponent and uUlLfF suffixes;
# - name: an identifier or keyword;
# - op: multi-character operators before the single-character class,
#   so '<<' wins over '<' and '--' over '-'.  The class also lists
#   characters the evaluator cannot handle (quotes, brackets,
#   assignment, member access) on purpose: they become explicit
#   tokens the parser bails on, instead of holes that would split
#   the expression and silently change what gets parsed.
_TOKEN = re.compile(
    r"""\s*(?:
      (?P<num>0[xX][0-9a-fA-F]+[uUlL]*|(?:\d+\.\d*|\.\d+|\d+)(?:[eE][+-]?\d+)?[uUlLfF]*)
    | (?P<name>[A-Za-z_]\w*)
    | (?P<op><<|>>|<=|>=|==|!=|&&|\|\||->|--|\+\+|[-+*/%()!<>&|^~?:,.\[\]='"])
    )""",
    re.VERBOSE,
)

# C binary operators from loosest to tightest binding.  The index in
# this tuple is the recursion depth of _Parser.binary: level N parses
# level N+1 subexpressions as its operands, and past the last level
# it falls through to unary operators.
_LEVELS: tuple[tuple[str, ...], ...] = (
    ('||',),
    ('&&',),
    ('|',),
    ('^',),
    ('&',),
    ('==', '!='),
    ('<', '<=', '>', '>='),
    ('<<', '>>'),
    ('+', '-'),
    ('*', '/', '%'),
)


class _BailError(Exception):
    """The expression is outside the supported subset."""


def _tokenize(expr: str) -> list[str]:
    """Split a C expression into tokens.

    Raises:
        _BailError: A character no alternative matches remains.
    """
    tokens = []
    pos = 0
    while pos < len(expr):
        match = _TOKEN.match(expr, pos)
        if match is None:
            if expr[pos:].strip():
                raise _BailError
            break
        pos = match.end()
        tokens.append(match.group('num') or match.group('name') or match.group('op'))
    return tokens


def _number(tok: str) -> Num:
    """The value of an integer or float literal token.

    Raises:
        _BailError: The token does not parse as a number.
    """
    if tok.lower().startswith('0x'):
        # f/F are hex digits, not a valid suffix on a hex integer:
        # strip only the actual integer suffixes uUlL.
        text = tok.rstrip('uUlL')
        is_float = False
    else:
        text = tok.rstrip('uUlLfF')
        is_float = '.' in text or 'e' in text.lower()
    try:
        return float(text) if is_float else int(text, 0)
    except ValueError as exc:
        raise _BailError from exc


def _truth(val: Num | None) -> bool | None:
    """C truthiness of a value; None stays None (undecided)."""
    return None if val is None else val != 0


def _c_div(left: Num, right: Num) -> Num:
    """Division with C semantics: integer division truncates."""
    if isinstance(left, int) and isinstance(right, int):
        return int(left / right)  # C division truncates toward zero
    return left / right


_OPS: dict[str, Callable[[Num, Num], Num | bool]] = {
    '==': operator.eq,
    '!=': operator.ne,
    '<': operator.lt,
    '<=': operator.le,
    '>': operator.gt,
    '>=': operator.ge,
    '+': operator.add,
    '-': operator.sub,
    '*': operator.mul,
    '/': _c_div,
    '%': operator.mod,
    '<<': operator.lshift,
    '>>': operator.rshift,
    '&': operator.and_,
    '|': operator.or_,
    '^': operator.xor,
}


def _apply(op: str, left: Num | None, right: Num | None) -> Num | None:  # noqa: PLR0911
    """Apply a binary operator with tri-state operands.

    The logical operators decide with one known side where they can
    (False wins for &&, True for ||); everything else is undecided
    as soon as an operand is, and operator errors (division by
    zero, bit operations on floats) degrade to undecided too.
    """
    if op == '&&':
        lt, rt = _truth(left), _truth(right)
        if lt is False or rt is False:
            return 0
        return 1 if lt is True and rt is True else None
    if op == '||':
        lt, rt = _truth(left), _truth(right)
        if lt is True or rt is True:
            return 1
        return 0 if lt is False and rt is False else None
    if left is None or right is None:
        return None
    try:
        result = _OPS[op](left, right)
    except (ZeroDivisionError, TypeError, ValueError):
        return None
    return int(result) if isinstance(result, bool) else result


class _Parser:
    """Precedence-climbing parser evaluating as it goes."""

    def __init__(self, tokens: list[str], env: Mapping[str, Num]) -> None:
        self.tokens = tokens
        self.env = env
        self.pos = 0

    def peek(self) -> str | None:
        """The next token without consuming it, or None at the end."""
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def take(self) -> str:
        """Consume and return the next token.

        Raises:
            _BailError: No tokens remain.
        """
        tok = self.peek()
        if tok is None:
            raise _BailError
        self.pos += 1
        return tok

    def expression(self) -> Num | None:
        """A full expression: comma operator over ternaries."""
        val = self.ternary()
        while self.peek() == ',':
            self.take()
            val = self.ternary()
        return val

    def ternary(self) -> Num | None:
        """A ternary conditional; an undecided condition undecides it."""
        cond = self.binary(0)
        if self.peek() != '?':
            return cond
        self.take()
        then = self.ternary()
        if self.take() != ':':
            raise _BailError
        other = self.ternary()
        truth = _truth(cond)
        if truth is None:
            return None
        return then if truth else other

    def binary(self, level: int) -> Num | None:
        """Binary operators of one precedence level of _LEVELS.

        Args:
            level: Index into _LEVELS; the level's operands are
                parsed at level + 1, past the last level unary
                expressions take over.
        """
        if level == len(_LEVELS):
            return self.unary()
        val = self.binary(level + 1)
        while self.peek() in _LEVELS[level]:
            op = self.take()
            val = _apply(op, val, self.binary(level + 1))
        return val

    def unary(self) -> Num | None:
        """A prefix operator chain (!, -, +, ~) over a primary."""
        tok = self.peek()
        if tok in ('--', '++'):
            # Prefix increment/decrement is not modeled: bail rather
            # than mistake it for stacked unary minus/plus.
            raise _BailError
        if tok == '!':
            self.take()
            truth = _truth(self.unary())
            return None if truth is None else int(not truth)
        if tok == '-':
            self.take()
            val = self.unary()
            return None if val is None else -val
        if tok == '+':
            self.take()
            return self.unary()
        if tok == '~':
            self.take()
            val = self.unary()
            return ~val if isinstance(val, int) else None
        return self.primary()

    def primary(self) -> Num | None:
        """A literal, a parenthesized expression, or an identifier.

        Raises:
            _BailError: The next token starts none of these.
        """
        tok = self.take()
        if tok == '(':
            val = self.expression()
            if self.take() != ')':
                raise _BailError
            return val
        if tok[0].isdigit() or tok[0] == '.':
            return _number(tok)
        if tok[0].isalpha() or tok[0] == '_':
            return self.postfix(tok)
        raise _BailError

    def postfix(self, name: str) -> Num | None:
        """An identifier with an optional call/member/index chain.

        A chain is not evaluated: it consumes its tokens and yields
        "undecided".  A bare unknown identifier is also undecided.
        """
        opaque = False
        while True:
            tok = self.peek()
            if tok in ('(', '['):
                self.skip_group()
                opaque = True
            elif tok in ('.', '->'):
                self.take()
                self.take()
                opaque = True
            else:
                break
        return None if opaque else self.env.get(name)

    def skip_group(self) -> None:
        """Consume a balanced parenthesis or bracket group."""
        self.take()
        depth = 1
        while depth:
            tok = self.take()
            if tok in ('(', '['):
                depth += 1
            elif tok in (')', ']'):
                depth -= 1


def value(expr: str, env: Mapping[str, Num]) -> Num | None:
    """The numeric value of a C expression, or None when undecidable.

    Args:
        expr: The expression text as written in the source.
        env: Known identifier values; identifiers absent from it
            make the expression undecidable, not an error.

    Returns:
        The value, or None when the expression is outside the
        supported subset, uses unknown identifiers, or has tokens
        left over after parsing.
    """
    try:
        tokens = _tokenize(expr)
        if not tokens:
            return None
        parser = _Parser(tokens, env)
        result = parser.expression()
    except _BailError:
        return None
    return None if parser.pos != len(tokens) else result


# The three clauses of a canonical counting loop, one regex each,
# applied to the clause texts aststeps splits out of a for header:
# - init: 'i = 0', optionally after declaration words ('unsigned
#   int i = 0'); group 1 is the variable, group 2 the start
#   expression;
# - cond: 'i < limit' or 'i <= limit'; groups: variable, operator,
#   limit expression;
# - incr: matched with all whitespace removed, so '++i', 'i++' and
#   'i += 1' are the three alternatives; exactly one group (the
#   variable) is non-None.
# The variable captured by all three must be the same one, which
# trip_count checks after matching.
_FOR_INIT = re.compile(r'^(?:[A-Za-z_]\w*\s+)*([A-Za-z_]\w*)\s*=\s*(.+)$')
_FOR_COND = re.compile(r'^([A-Za-z_]\w*)\s*(<=|<)\s*(.+)$')
_FOR_INCR = re.compile(r'^(?:\+\+([A-Za-z_]\w*)|([A-Za-z_]\w*)\+\+|([A-Za-z_]\w*)\+=1)$')


def init_var(init: str) -> str | None:
    """The variable a for-loop init clause assigns, or None.

    Recognizes the same "[decl] var = ..." shape as trip_count's
    init clause, without requiring the condition/increment to also
    fit a canonical counting loop.

    Args:
        init: The for header's init clause text.
    """
    m_init = _FOR_INIT.match(init.strip())
    return m_init.group(1) if m_init is not None else None


def trip_count(
    init: str | None, cond: str, incr: str | None, env: Mapping[str, Num]
) -> tuple[str, int, int] | None:
    """(variable, start, count) of a canonical counting loop.

    Recognizes "var = START; var < LIMIT; var++" shapes (also <=,
    ++var, var += 1, an optional declaration in the init) when START
    and LIMIT evaluate to integers; anything else is None.

    Args:
        init: The for header's init clause, or None.
        cond: The loop condition text.
        incr: The for header's increment clause, or None.
        env: Known identifier values for the bound evaluation.

    Returns:
        The loop variable, its start value, and the number of trips
        (clamped to zero for loops that never run); None when the
        loop is not of the canonical shape or a bound is undecided.
    """
    if init is None or incr is None:
        return None
    m_init = _FOR_INIT.match(init.strip())
    m_cond = _FOR_COND.match(cond.strip())
    m_incr = _FOR_INCR.match(re.sub(r'\s+', '', incr))
    if m_init is None or m_cond is None or m_incr is None:
        return None
    var = m_init.group(1)
    incr_var = next(g for g in m_incr.groups() if g is not None)
    if m_cond.group(1) != var or incr_var != var:
        return None
    start = value(m_init.group(2), env)
    limit = value(m_cond.group(3), env)
    if not isinstance(start, int) or not isinstance(limit, int):
        return None
    count = limit - start + (1 if m_cond.group(2) == '<=' else 0)
    return var, start, max(count, 0)


def evaluate(expr: str, env: Mapping[str, Num]) -> bool | None:
    """Whether a C condition holds: True, False, or None (cannot tell).

    The truth of value() under C semantics: zero is False, anything
    else True, undecidable stays None.  Arguments are value()'s.
    """
    return _truth(value(expr, env))
