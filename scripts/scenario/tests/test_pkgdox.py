# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
"""Tests for the package.md doxygen input filter."""

import subprocess
from pathlib import Path

import pytest

from model import Note, Package, Param, Step, Test, Value
from pkgdox import filter_text, render


def make_pkg(description: list[str], group: str = '') -> Package:
    return Package(
        name='dcb',
        summary='DCB tests',
        path=Path('dcb/package.md'),
        group=group,
        description=description,
    )


def test_no_description() -> None:
    assert render(make_pkg([])) == ''


def test_description_joins_the_group() -> None:
    out = render(make_pkg(['What the package covers.', '', '* A bullet.']))
    assert out.splitlines() == [
        '/// @addtogroup dcb',
        '///',
        '/// What the package covers.',
        '///',
        '/// * A bullet.',
    ]


def test_group_label_wins() -> None:
    out = render(make_pkg(['Text.'], group='suite_tests'))
    assert out.splitlines()[0] == '/// @addtogroup suite_tests'


def test_headings_become_anchored_bold_lines() -> None:
    # A section would add an entry to the navigation sidebar of the page.
    out = render(make_pkg(['### Common rules', '', 'Text.', '', '#### Packet priority']))
    assert out.splitlines()[2:] == [
        '/// @anchor dcb-common-rules',
        '/// **Common rules**',
        '///',
        '/// Text.',
        '///',
        '/// @anchor dcb-packet-priority',
        '/// **Packet priority**',
    ]


def test_impl_quotes_are_dropped() -> None:
    out = render(
        make_pkg(
            [
                'Before.',
                '',
                '> impl: call tapi_foo(),',
                '> then tapi_bar().',
                '',
                '> A quote for readers.',
                '',
                'After.',
            ]
        )
    )
    lines = out.splitlines()
    assert not any('tapi_' in line for line in lines)
    assert '/// > A quote for readers.' in lines
    assert lines[-1] == '/// After.'


def test_fenced_code_is_left_alone() -> None:
    out = render(make_pkg(['```', '### not a heading', '> impl: not advice', '```']))
    assert out.splitlines()[2:] == [
        '/// ```',
        '/// ### not a heading',
        '/// > impl: not advice',
        '/// ```',
    ]


def make_test() -> Test:
    return Test(
        name='pfc_tx_pause',
        summary='Tx reaction to received PFC frames',
        path=Path('dcb/package.md'),
        line=1,
        objective='Check that `prio` pauses.',
        notes=['A remark.'],
        params=[
            Param(
                name='mode',
                description='Pause mode',
                values=[Value(name='rx', comment='receive only'), Value(name='both')],
            )
        ],
        steps=[
            Step(
                text='Set up PFC on `prio`.',
                line=2,
                notes=[Note(text='Extra detail.'), Note(text='use tapi_foo().', impl=True)],
                sub=[Step(text='Send a frame.', line=3)],
            ),
            Step(text='Check the pause.', line=4),
        ],
    )


PLANNED = [
    '/// @defgroup dcb-pfc_tx_pause Tx reaction to received PFC frames',
    '/// @ingroup dcb',
    '/// @{',
    '///',
    (
        '/// @warning Not implemented yet: this is the planned scenario from '
        'dcb/package.md, there is no test to run.'
    ),
    '///',
    '/// @objective Check that `prio` pauses.',
    '///',
    '/// @param mode Pause mode:',
    '///   - `rx` (receive only)',
    '///   - `both`',
    '///',
    '/// @note A remark.',
    '///',
    '/// @par Scenario:',
    '///    -# Set up PFC on `prio`.',
    '///       Extra detail.',
    '///       - Send a frame.',
    '///    -# Check the pause.',
    '/// @}',
]


def test_planned_test_page() -> None:
    assert render(make_pkg([]), pending=[make_test()]).splitlines() == PLANNED


def test_planned_page_follows_description() -> None:
    # Two comment blocks: the empty line keeps doxygen from joining them.
    lines = render(make_pkg(['Covers DCB.']), pending=[make_test()]).splitlines()
    assert lines[:3] == ['/// @addtogroup dcb', '///', '/// Covers DCB.']
    assert lines[3:] == ['', *PLANNED]


def test_planned_page_joins_group_label() -> None:
    out = render(make_pkg([], group='suite_tests'), pending=[make_test()])
    assert '/// @ingroup suite_tests' in out.splitlines()


def test_filter_text_plans_only_missing_tests(tmp_path: Path) -> None:
    pkg_dir = tmp_path / 'dcb'
    pkg_dir.mkdir()
    md = pkg_dir / 'package.md'
    md.write_text(
        '# dcb: DCB tests\n\n'
        '## done: Implemented\n\nO.\n\nSteps:\n\n1. A.\n\n'
        '## todo: Planned\n\nO.\n\nSteps:\n\n1. B.\n',
        encoding='utf-8',
    )
    (pkg_dir / 'done.c').write_text('int main(void) { return 0; }\n', encoding='utf-8')
    out = filter_text(md)
    assert '/// @defgroup dcb-todo Planned' in out.splitlines()
    assert 'dcb-done' not in out


def test_filter_text(tmp_path: Path) -> None:
    pkg_dir = tmp_path / 'dcb'
    pkg_dir.mkdir()
    md = pkg_dir / 'package.md'
    md.write_text('# dcb: DCB tests\n\n## Description\n\nCovers DCB.\n', encoding='utf-8')
    assert filter_text(md).splitlines() == ['/// @addtogroup dcb', '///', '/// Covers DCB.']


def test_filter_text_bad_document(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    # doxygen would read the markdown itself as C if the filter printed it.
    pkg_dir = tmp_path / 'dcb'
    pkg_dir.mkdir()
    md = pkg_dir / 'package.md'
    md.write_text('# wrong: Name\n\n## Description\n\nText.\n', encoding='utf-8')
    assert filter_text(md) == ''
    assert 'pkgdox: warning:' in capsys.readouterr().err


def test_md2dox_wrapper(tmp_path: Path) -> None:
    # The filter doxygen runs, by the path a Doxyfile names.
    pkg_dir = tmp_path / 'dcb'
    pkg_dir.mkdir()
    md = pkg_dir / 'package.md'
    md.write_text('# dcb: DCB tests\n\n## Description\n\nCovers DCB.\n', encoding='utf-8')
    wrapper = Path(__file__).resolve().parents[2] / 'md2dox'
    out = subprocess.run(  # noqa: S603 - fixed argv
        [str(wrapper), str(md)], capture_output=True, text=True, check=True
    )
    assert out.stdout.splitlines()[0] == '/// @addtogroup dcb'
