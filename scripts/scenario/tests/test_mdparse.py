# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the scenario markdown parser."""

import textwrap
from pathlib import Path

import pytest

from mdparse import parse_package
from model import ScenarioError


def write_pkg(tmp_path: Path, name: str, body: str) -> Path:
    d = tmp_path / name
    d.mkdir()
    f = d / 'package.md'
    f.write_text(textwrap.dedent(body), encoding='utf-8')
    return f


FULL = """\
# usecases: Reliability in normal use

Free prose about the package.

## set_mtu: Set MTU of IUT

Make sure that MTU-sized packets pass and larger do not.

Type: use case

A test-level remark.

Parameters:

- `mtu`: MTU on IUT
- `ethdev_state`: the state of the device
  - `TEST_ETHDEV_CONFIGURED`: configured, not started
  - `TEST_ETHDEV_STARTED`

Steps:

1. Initialize EAL and configure `iut_port`.
2. Set `mtu` on `iut_port` in `ethdev_state`.

   The driver may round the value down; read it back.

   > impl: use tapi_cfg_base_if_set_mtu().

3. Transmit and check.
   - Send a packet of size `mtu`.
   - Check it is received.
     - Poll the Rx queue.
"""


def test_full_document(tmp_path: Path) -> None:
    pkg = parse_package(write_pkg(tmp_path, 'usecases', FULL))
    assert pkg.name == 'usecases'
    assert pkg.summary == 'Reliability in normal use'
    assert len(pkg.tests) == 1
    t = pkg.tests[0]
    assert t.name == 'set_mtu'
    assert t.summary == 'Set MTU of IUT'
    assert t.objective.startswith('Make sure that MTU-sized')
    assert t.type == 'use case'
    assert t.notes == ['A test-level remark.']
    assert [p.name for p in t.params] == ['mtu', 'ethdev_state']
    vals = t.params[1].values
    assert vals[0].name == 'TEST_ETHDEV_CONFIGURED'
    assert vals[0].comment == 'configured, not started'
    assert vals[1].comment is None
    assert len(t.steps) == 3
    s2 = t.steps[1]
    assert s2.text == 'Set `mtu` on `iut_port` in `ethdev_state`.'
    assert s2.notes[0].impl is False
    assert s2.notes[0].text.startswith('The driver may round')
    assert s2.notes[1].impl is True
    assert s2.notes[1].text == 'use tapi_cfg_base_if_set_mtu().'
    s3 = t.steps[2]
    assert [s.text for s in s3.sub] == [
        'Send a packet of size `mtu`.',
        'Check it is received.',
    ]
    assert s3.sub[1].sub[0].text == 'Poll the Rx queue.'


def test_wrapped_step_text(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    Objective.

    Steps:

    1. A step whose text continues
       on the next line.
    """
    pkg = parse_package(write_pkg(tmp_path, 'p', body))
    assert pkg.tests[0].steps[0].text == ('A step whose text continues on the next line.')


def test_reference_file(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## Referenced tests

    - [other](other.md)
    """
    f = write_pkg(tmp_path, 'p', body)
    (tmp_path / 'p' / 'other.md').write_text(
        textwrap.dedent("""\
        # other: Another test

        Objective here.

        Steps:

        1. Do something.
        """),
        encoding='utf-8',
    )
    pkg = parse_package(f)
    assert [t.name for t in pkg.tests] == ['other']
    assert pkg.tests[0].summary == 'Another test'


def test_package_name_mismatch(tmp_path: Path) -> None:
    f = write_pkg(tmp_path, 'p', '# wrong: S\n')
    with pytest.raises(ScenarioError, match='does not match'):
        parse_package(f)


def test_duplicate_test(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: One

    O.

    Steps:

    1. A.

    ## t: Two

    O.

    Steps:

    1. B.
    """
    with pytest.raises(ScenarioError, match='duplicate'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_heading_inside_test(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    ### Sub

    Steps:

    1. A.
    """
    with pytest.raises(ScenarioError, match='heading'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_blockquote_outside_step(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    > stray quote

    Steps:

    1. A.
    """
    with pytest.raises(ScenarioError, match='blockquote'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_misaligned_continuation(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    Steps:

    1. A step.
         badly indented continuation.
    """
    with pytest.raises(ScenarioError, match='indent'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_deep_nesting_warns(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    Steps:

    1. A.
       - B.
         - C.
           - D.
    """
    parse_package(write_pkg(tmp_path, 'p', body))
    assert 'depth' in capsys.readouterr().err


def test_section_headings_inline(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    ### Parameters

    - `a`: first

    ### Steps:

    1. Do `a`.
    """
    pkg = parse_package(write_pkg(tmp_path, 'p', body))
    t = pkg.tests[0]
    assert [pr.name for pr in t.params] == ['a']
    assert t.steps[0].text == 'Do `a`.'


def test_section_heading_per_test_file(tmp_path: Path) -> None:
    body = """\
    # p: S

    - [other](other.md)
    """
    f = write_pkg(tmp_path, 'p', body)
    (tmp_path / 'p' / 'other.md').write_text(
        textwrap.dedent("""\
        # other: T

        O.

        ## Steps

        1. Do something.
        """),
        encoding='utf-8',
    )
    pkg = parse_package(f)
    assert pkg.tests[0].steps[0].text == 'Do something.'


def test_section_heading_wrong_level(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    #### Steps

    1. A.
    """
    with pytest.raises(ScenarioError, match='heading'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_non_section_heading_still_rejected(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    ### Notes

    Steps:

    1. A.
    """
    with pytest.raises(ScenarioError, match='heading'):
        parse_package(write_pkg(tmp_path, 'p', body))


GROUPED = """\
# p: S

Group: suite_tests

Planning prose that stays in the markdown.

## Description

What the package covers.

### Common rules

A rule every test follows.

## t: T

O.

Steps:

1. A.
"""


def test_group_defaults_to_package_name(tmp_path: Path) -> None:
    pkg = parse_package(write_pkg(tmp_path, 'p', '# p: S\n'))
    assert pkg.group == 'p'


def test_group_label(tmp_path: Path) -> None:
    pkg = parse_package(write_pkg(tmp_path, 'p', GROUPED))
    assert pkg.group == 'suite_tests'


def test_group_label_twice(tmp_path: Path) -> None:
    body = GROUPED.replace('Group: suite_tests\n', 'Group: a\n\nGroup: b\n')
    with pytest.raises(ScenarioError, match='duplicate "Group:"'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_group_label_after_heading(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## Referenced tests

    Group: late
    """
    with pytest.raises(ScenarioError, match='"Group:" must come before'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_group_label_inside_paragraph_is_prose(tmp_path: Path) -> None:
    body = """\
    # p: S

    A paragraph that wraps onto a line
    Group: wrapped
    """
    assert parse_package(write_pkg(tmp_path, 'p', body)).group == 'p'


def test_description(tmp_path: Path) -> None:
    pkg = parse_package(write_pkg(tmp_path, 'p', GROUPED))
    assert pkg.description == [
        'What the package covers.',
        '',
        '### Common rules',
        '',
        'A rule every test follows.',
    ]
    assert pkg.description_line == 7
    assert [t.name for t in pkg.tests] == ['t']


def test_no_description(tmp_path: Path) -> None:
    pkg = parse_package(write_pkg(tmp_path, 'p', FULL.replace('usecases', 'p')))
    assert pkg.description == []


def test_description_after_test(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## t: T

    O.

    Steps:

    1. A.

    ## Description

    Too late.
    """
    with pytest.raises(ScenarioError, match='before the first test'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_description_after_referenced_test(tmp_path: Path) -> None:
    body = """\
    # p: S

    - [other](other.md)

    ## Description

    Too late.
    """
    f = write_pkg(tmp_path, 'p', body)
    other = tmp_path / 'p' / 'other.md'
    other.write_text('# other: T\n\nO.\n\nSteps:\n\n1. A.\n', encoding='utf-8')
    with pytest.raises(ScenarioError, match='before the first test'):
        parse_package(f)


def test_description_twice(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## Description

    One.

    ## Description

    Two.
    """
    with pytest.raises(ScenarioError, match='duplicate "## Description"'):
        parse_package(write_pkg(tmp_path, 'p', body))


def test_description_rejects_test_reference(tmp_path: Path) -> None:
    body = """\
    # p: S

    ## Description

    Covers one test:

    - [other](other.md)
    """
    with pytest.raises(ScenarioError, match='test reference inside "## Description"'):
        parse_package(write_pkg(tmp_path, 'p', body))
