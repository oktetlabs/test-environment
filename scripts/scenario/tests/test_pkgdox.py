# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
"""Tests for the package.md doxygen input filter."""

import subprocess
from pathlib import Path

import pytest

from model import Package
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
