# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the doxygen header comment surgery."""

from cheader import parse_doc_header, split_source

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
