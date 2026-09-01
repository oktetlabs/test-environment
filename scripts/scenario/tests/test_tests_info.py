# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Tests for the tests-info.xml generator."""

import json
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

from tests_info import emit_dir

SRC = """\
/** @page usecases-demo Demo test
 *
 * @objective Check the @p mode   handling & <cornercases>.
 *
 * @param legacy  Header-documented parameter
 *
 * @par Scenario:
 */
int
main(int argc, char *argv[])
{
    TEST_START;
    TEST_PARAM_DOC(mode,
        "Operation mode:",
        "- `FAST`: skip checks");
    TEST_GET_STRING_PARAM(mode);
    TEST_STEP("Prepare");
    TEST_SUBSTEP("Configure");
    TEST_STEP("Run");
    TEST_END;
}
"""

NO_OBJECTIVE = 'int\nmain(void)\n{\n    TEST_STEP("Run");\n    return 0;\n}\n'


def _parse(block: str) -> ET.Element:
    return ET.fromstring(f'<tests-info>{block}</tests-info>')  # noqa: S314 - own fixture text


def test_emit_dir_full(tmp_path: Path) -> None:
    (tmp_path / 'demo.c').write_text(SRC, encoding='utf-8')
    (tmp_path / 'noobj.c').write_text(NO_OBJECTIVE, encoding='utf-8')
    root = _parse(emit_dir(tmp_path))
    tests = root.findall('test')
    assert [t.get('name') for t in tests] == ['demo']
    test = tests[0]
    assert test.get('page') == 'usecases-demo'
    assert test.find('objective').text == (
        'Check the mode handling & <cornercases>.'
    )
    params = test.findall('param')
    assert [p.get('name') for p in params] == ['mode', 'legacy']
    assert params[0].text == 'Operation mode:\n- `FAST`: skip checks'
    assert params[1].text == 'Header-documented parameter'
    steps = test.find('scenario').findall('step')
    assert [(s.get('depth'), s.text) for s in steps] == [
        ('1', 'Prepare'),
        ('2', 'Configure'),
        ('1', 'Run'),
    ]


def test_emit_dir_no_scenario_no_params(tmp_path: Path) -> None:
    (tmp_path / 'bare.c').write_text(
        '/** @page p-bare Bare\n * @objective Only this.\n */\n'
        'int\nmain(void)\n{\n    return 0;\n}\n',
        encoding='utf-8',
    )
    root = _parse(emit_dir(tmp_path))
    test = root.find('test')
    assert test.find('scenario') is None
    assert test.findall('param') == []


def test_param_docs_fallback_and_omission(tmp_path: Path) -> None:
    """Empty inline doc falls back to header @param; blank-only is dropped."""
    src = (
        '/** @page p-fallback Fallback\n'
        ' *\n'
        ' * @objective Fallback param handling.\n'
        ' *\n'
        ' * @param empty_doc  Header description for empty doc\n'
        ' *\n'
        ' * @par Scenario:\n'
        ' */\n'
        'int\n'
        'main(void)\n'
        '{\n'
        '    TEST_PARAM_DOC(empty_doc, "");\n'
        '    TEST_PARAM_DOC(blank_only, "   ");\n'
        '    return 0;\n'
        '}\n'
    )
    (tmp_path / 'fallback.c').write_text(src, encoding='utf-8')
    root = _parse(emit_dir(tmp_path))
    test = root.find('test')
    params = test.findall('param')
    assert [p.get('name') for p in params] == ['empty_doc']
    assert params[0].text == 'Header description for empty doc'


def test_attribute_escaping_round_trips(tmp_path: Path) -> None:
    """A double quote in the file stem or @page id must not break the XML."""
    src = (
        '/** @page a"b Title\n'
        ' *\n'
        ' * @objective Quoted attributes.\n'
        ' *\n'
        ' * @par Scenario:\n'
        ' */\n'
        'int\n'
        'main(void)\n'
        '{\n'
        '    return 0;\n'
        '}\n'
    )
    (tmp_path / 'quote"stem.c').write_text(src, encoding='utf-8')
    root = _parse(emit_dir(tmp_path))
    test = root.find('test')
    assert test.get('name') == 'quote"stem'
    assert test.get('page') == 'a"b'


def test_stdlib_only() -> None:
    """Importing tests_info alone must not pull in libclang or pytest.

    The other test modules in this suite legitimately import
    aststeps/clang when libclang is installed, which would poison a
    plain sys.modules check run inside the same pytest session
    (order-dependent false failure under the libclang venv).  A
    fresh subprocess sidesteps that and tests the actual claim: a
    bare `import tests_info` stays stdlib-only.
    """
    import ast

    import tests_info

    src = Path(tests_info.__file__).read_text(encoding='utf-8')
    top_level = {
        alias.name
        for node in ast.parse(src).body
        if isinstance(node, ast.Import)
        for alias in node.names
    }
    assert not top_level & {'aststeps', 'doxfilter', 'steptree', 'clang'}

    probe = (
        'import json, sys\n'
        'import tests_info\n'
        'print(json.dumps(sorted({m.split(".")[0] for m in sys.modules})))\n'
    )
    proc = subprocess.run(  # noqa: S603 - fixed argv, no untrusted input
        [sys.executable, '-c', probe],
        cwd=Path(tests_info.__file__).resolve().parent,
        capture_output=True,
        text=True,
        check=True,
    )
    mods = set(json.loads(proc.stdout))
    for banned in ('clang', 'pytest', 'jansson'):
        assert banned not in mods


CONTROL_FLOW_SRC = """\
/** @page p-flow Flow
 *
 * @objective Control flow in the scenario.
 *
 * @par Scenario:
 */
int
main(int argc, char *argv[])
{
    unsigned int i;
    int deep = argc > 1;

    TEST_START;
    TEST_STEP("Prepare");
    for (i = 0; i < 3; i++)
    {
        TEST_SUBSTEP("Announce round");
        TEST_STEP_PUSH("Validate");
        TEST_STEP_PUSH("Check the header");
        TEST_STEP_POP("");
        TEST_STEP_POP("");
    }
    if (deep)
        TEST_STEP("Descend");
    else
        TEST_STEP("Stay");
    TEST_END;
}
"""


def _scenario(tmp_path: Path) -> list[tuple[str, str]]:
    (tmp_path / 'flow.c').write_text(CONTROL_FLOW_SRC, encoding='utf-8')
    root = _parse(emit_dir(tmp_path))
    steps = root.find('test').find('scenario').findall('step')
    return [(s.get('depth'), s.text) for s in steps]


def test_scenario_control_flow_annotated(tmp_path: Path) -> None:
    """With libclang the logged scenario matches the documented tree."""
    import aststeps

    if not aststeps.HAVE_CLANG:
        pytest.skip('libclang not installed')
    assert _scenario(tmp_path) == [
        ('1', 'Prepare'),
        ('1', 'For each iteration (`i = 0; i < 3; i++`):'),
        ('2', 'Announce round'),
        ('2', 'Validate'),
        ('3', 'Check the header'),
        ('1', 'If `deep`:'),
        ('2', 'Descend'),
        ('1', 'If not `deep`:'),
        ('2', 'Stay'),
    ]


def test_scenario_flat_fallback(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Without libclang the flat textual extraction still delivers."""
    import aststeps

    monkeypatch.setattr(aststeps, 'HAVE_CLANG', False)
    assert _scenario(tmp_path) == [
        ('1', 'Prepare'),
        ('2', 'Announce round'),
        ('3', 'Validate'),
        ('4', 'Check the header'),
        ('1', 'Descend'),
        ('1', 'Stay'),
    ]
