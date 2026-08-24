# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 OKTET Ltd.
"""Golden test for the C stub emitter."""

from pathlib import Path

from emit_c import emit_test
from model import Note, Package, Param, Step, Test, Value

GOLDEN = """\
/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Ltd. */
/** @defgroup usecases-set_mtu Set MTU of IUT
 * @ingroup usecases
 * @{
 *
 * @objective Make sure that MTU-sized packets pass and larger do not.
 *
 * @param mtu          MTU on IUT
 * @param ethdev_state The state of the device:
 *                     - @c TEST_ETHDEV_CONFIGURED (configured, not started)
 *                     - @c TEST_ETHDEV_STARTED
 *
 * @type use case
 *
 * @author John Doe <John.Doe@oktet.co.il>
 *
 * @par Scenario:
 */

#define TE_TEST_NAME "usecases/set_mtu"

/* TODO: suite includes */

int
main(int argc, char *argv[])
{
    /* TODO: verify parameter kinds */
    unsigned int mtu;
    const char *ethdev_state;

    TEST_START;
    TEST_GET_UINT_PARAM(mtu);
    /* enum: consider TEST_GET_ENUM_PARAM */
    TEST_GET_STRING_PARAM(ethdev_state);

    TEST_STEP("Initialize EAL and configure iut_port.");
    /* The driver may round the value down; read it back.
     *
     * IMPL: use tapi_cfg_base_if_set_mtu(). */
    TEST_STEP("Set @p mtu on iut_port in @p ethdev_state.");
    TEST_STEP("Transmit and check.");
    TEST_SUBSTEP("Send a packet of size @p mtu.");
    TEST_SUBSTEP("Check it is received.");
    TEST_STEP_PUSH("Poll the Rx queue.");
    TEST_STEP_POP("");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
/** @} */
"""


def make_test() -> tuple[Package, Test]:
    test = Test(
        name='set_mtu',
        summary='Set MTU of IUT',
        path=Path('usecases/package.md'),
        line=1,
        objective=('Make sure that MTU-sized packets pass and larger do not.'),
        type='use case',
        params=[
            Param(name='mtu', description='MTU on IUT'),
            Param(
                name='ethdev_state',
                description='The state of the device',
                values=[
                    Value(
                        name='TEST_ETHDEV_CONFIGURED',
                        comment='configured, not started',
                    ),
                    Value(name='TEST_ETHDEV_STARTED'),
                ],
            ),
        ],
        steps=[
            Step(text='Initialize EAL and configure `iut_port`.', line=1),
            Step(
                text='Set `mtu` on `iut_port` in `ethdev_state`.',
                line=2,
                notes=[
                    Note(text=('The driver may round the value down; read it back.')),
                    Note(text='use tapi_cfg_base_if_set_mtu().', impl=True),
                ],
            ),
            Step(
                text='Transmit and check.',
                line=3,
                sub=[
                    Step(text='Send a packet of size `mtu`.', line=4),
                    Step(
                        text='Check it is received.',
                        line=5,
                        sub=[Step(text='Poll the Rx queue.', line=6)],
                    ),
                ],
            ),
        ],
    )
    pkg = Package(
        name='usecases',
        summary='Reliability in normal use',
        path=Path('usecases/package.md'),
        tests=[test],
    )
    return pkg, test


def test_golden() -> None:
    pkg, test = make_test()
    out = emit_test(
        pkg,
        test,
        author='John Doe <John.Doe@oktet.co.il>',
        copyright_line='Copyright (C) 2026 OKTET Ltd.',
    )
    assert out == GOLDEN


def test_long_step_wraps() -> None:
    pkg, test = make_test()
    test.steps = [
        Step(
            text=(
                'A very long step text that certainly cannot fit into one '
                'eighty column line of generated C source code at all.'
            ),
            line=1,
        )
    ]
    out = emit_test(
        pkg,
        test,
        author='A <a@b.c>',
        copyright_line='Copyright (C) 2026 OKTET Ltd.',
    )
    assert (
        '    TEST_STEP("A very long step text that certainly cannot fit '
        'into one eighty "\n'
        '              "column line of generated C source code at all.");'
    ) in out
    for line in out.splitlines():
        assert len(line) <= 80


def test_special_chars_escaped() -> None:
    pkg, test = make_test()
    test.steps = [Step(text='Run \\--show "fast" mode', line=1)]
    out = emit_test(
        pkg,
        test,
        author='A <a@b.c>',
        copyright_line='Copyright (C) 2026 OKTET Ltd.',
    )
    assert 'TEST_STEP("Run \\\\--show \\"fast\\" mode");' in out
