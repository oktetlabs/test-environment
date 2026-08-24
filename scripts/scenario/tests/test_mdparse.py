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
