# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the doxygen header comment surgery."""

from cheader import param_spans, parse_doc_header, split_source

HEADER_SRC = """\
/* SPDX-License-Identifier: Apache-2.0 */

/** @defgroup demo-check Demo check
 * @ingroup demo
 * @{
 *
 * @objective Check something
 *
 * @param mode  The mode
 *
 * @author A U Thor <author@example.com>
 *
 * @par Scenario:
 */

/** A member doc that must not survive filtering */
int
main(void)
{
    return 0;
}

/** @} */
"""


def test_split_source() -> None:
    split = split_source(HEADER_SRC)
    assert split is not None
    header, trailing = split
    assert header[0] == '/** @defgroup demo-check Demo check'
    assert ' * @author A U Thor <author@example.com>' in header
    assert ' * @par Scenario:' in header
    assert all('*/' not in line for line in header)
    assert trailing == ['/** @} */']


def test_split_source_without_header() -> None:
    assert split_source('int main(void) { return 0; }\n') is None


FILE_COMMENT_SRC = """\
/** @file
 * @brief Some helper docs
 */

/** @page tools_demo Demo page test
 *
 * @objective Check something.
 *
 * @par Scenario:
 */
int main(void) { return 0; }
"""


def test_split_source_skips_file_comment() -> None:
    split = split_source(FILE_COMMENT_SRC)
    assert split is not None
    header, _ = split
    assert header[0] == '/** @page tools_demo Demo page test'


TAGGED_SRC = """\
/* SPDX-License-Identifier: Apache-2.0 */

/** @defgroup demo-check Demo check test
 * @ingroup demo
 * @{
 *
 * @objective Check that the demo device
 *            works with @p mode set.
 *
 * @param mode      The device mode
 * @param attempts  How many times to try
 *
 * @type use case
 *
 * @author A U Thor <author@example.com>
 *
 * @par Scenario:
 */
int main(void) { return 0; }
"""


def test_parse_doc_header() -> None:
    summary, objective, type_, params = parse_doc_header(TAGGED_SRC)
    assert summary == 'Demo check test'
    assert objective == 'Check that the demo device works with @p mode set.'
    assert type_ == 'use case'
    assert params == [('mode', 'The device mode'), ('attempts', 'How many times to try')]


PAGE_HEADER_SRC = """\
/** @file
 * @brief Helper docs
 */

/** @page tools_demo Strict expansion test
 *
 * @objective Testing strict expansion.
 *
 * @par Test sequence:
 */
int main(void) { return 0; }
"""


def test_parse_doc_header_page_style() -> None:
    summary, objective, type_, params = parse_doc_header(PAGE_HEADER_SRC)
    assert summary == 'Strict expansion test'
    assert objective == 'Testing strict expansion.'
    assert type_ is None
    assert params == []


def test_param_spans() -> None:
    header = [
        '/** @defgroup x-y Title',
        ' * @objective Check',
        ' *',
        ' * @param mode  The mode:',
        ' *              - a',
        ' *              - b',
        ' * @param size  The size',
        ' *',
        ' * @par Scenario:',
    ]
    assert param_spans(header) == [('mode', 3, 6), ('size', 6, 7)]


def test_param_spans_none() -> None:
    assert param_spans(['/** @defgroup x-y T', ' * @objective O']) == []


INLINE_MARKER_SRC = """\
/** @defgroup demo-check Demo check test
 * @ingroup demo
 * @{
 *
 * @objective Set the list of multicast addresses to filter on
 *            @p iut_port port
 *
 * @param iut_port  Interface handle to use
 *                   @p iut_port here too
 *
 * @par Scenario:
 */
int main(void) { return 0; }
"""


def test_parse_doc_header_inline_marker_at_line_start() -> None:
    """An @p/@a/@b/@c/@e at the start of a continuation line is markup."""
    summary, objective, type_, params = parse_doc_header(INLINE_MARKER_SRC)
    assert summary == 'Demo check test'
    assert objective == (
        'Set the list of multicast addresses to filter on @p iut_port port'
    )
    assert params == [
        ('iut_port', 'Interface handle to use @p iut_port here too'),
    ]


def test_param_spans_inline_marker_at_line_start() -> None:
    header = [
        '/** @defgroup x-y Title',
        ' * @objective Check',
        ' *',
        ' * @param mode  The mode of',
        ' *              @p iut_port port',
        ' * @param size  The size',
        ' *',
        ' * @par Scenario:',
    ]
    assert param_spans(header) == [('mode', 3, 5), ('size', 5, 6)]
