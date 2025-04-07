/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Tester Subsystem
 *
 * Tester flags definitions.
 */

#ifndef __TE_TESTER_FLAGS_H__
#define __TE_TESTER_FLAGS_H__

#include "te_stdint.h"
#include "te_printf.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Flags of the Tester global context */
typedef uint64_t tester_flags;

/** printf()-like length modifier for tester_flags */
#define TE_PRINTF_TESTER_FLAGS TE_PRINTF_64

/** The first level of verbosity: output of run path to stdout */
#define TESTER_VERBOSE                (1LLU << 0)
/**
 * The second level of verbosity: additional output of test IDs
 * on run paths
 */
#define TESTER_VVERB                  (1LLU << 1)
/** Additional output of status together with TRC verdict */
#define TESTER_OUT_EXP                (1LLU << 2)
/** Additional output of TINs */
#define TESTER_OUT_TIN                (1LLU << 3)
/** Fake run */
#define TESTER_FAKE                   (1LLU << 4)
/** Quiet skip of tests */
#define TESTER_QUIET_SKIP             (1LLU << 5)
/** Output test params to the console */
#define TESTER_OUT_TEST_PARAMS        (1LLU << 6)
/** Print 'skipped' in case of silent mode is on by default */
#define TESTER_VERB_SKIP              (1LLU << 7)
/** Don't build any Test Suites */
#define TESTER_NO_BUILD               (1LLU << 8)
/** Don't run any Test Suites */
#define TESTER_NO_RUN                 (1LLU << 9)
/** Don't interact with Configurator */
#define TESTER_NO_CS                  (1LLU << 10)
/** Don't track configuration  changes */
#define TESTER_NO_CFG_TRACK           (1LLU << 11)
/** Prologues/epilogues are disabled */
#define TESTER_NO_LOGUES              (1LLU << 12)
/** Disable simultaneousness */
#define TESTER_NO_SIMULT              (1LLU << 13)
/** Don't use TRC */
#define TESTER_NO_TRC                 (1LLU << 14)
/** Run scripts under valgrind */
#define TESTER_VALGRIND               (1LLU << 15)
/** Run scripts under GDB in interactive mode */
#define TESTER_GDB                    (1LLU << 16)
/** Flag to be used on run path only */
#define TESTER_RUNPATH                (1LLU << 17)
/** End of run path */
#define TESTER_RUN                    (1LLU << 18)
/** Run tests regardless requested path */
#define TESTER_FORCE_RUN              (1LLU << 19)
/** Strip common indentations in string values */
#define TESTER_STRIP_INDENT           (1LLU << 20)

/*
 * These seems to be not used currently.
 */
#define TESTER_MIX_VALUES             (1LLU << 20)
#define TESTER_MIX_ARGS               (1LLU << 21)
#define TESTER_MIX_TESTS              (1LLU << 22)
#define TESTER_MIX_ITERS              (1LLU << 23)
#define TESTER_MIX_SESSIONS           (1LLU << 24)

/** Force testing flow logging to ignore run item name */
#define TESTER_LOG_IGNORE_RUN_NAME    (1LLU << 25)
/** Log list of known requirements to the log file */
#define TESTER_LOG_REQS_LIST          (1LLU << 26)
/** Is in prologue/epilogue */
#define TESTER_INLOGUE                (1LLU << 27)
/** Interactive mode */
#define TESTER_INTERACTIVE            (1LLU << 28)
/** Shutdown test scenario */
#define TESTER_SHUTDOWN               (1LLU << 29)
/** Break test session on Ctrl-C */
#define TESTER_BREAK_SESSION          (1LLU << 30)

/**
 * Run only prologues/epilogues under which at least a single
 * test is run according to requirements passed in command line.
 * This may not work well in case when prologues can add their
 * own requirements at run time via /local:/reqs:.
 */
#define TESTER_ONLY_REQ_LOGUES        (1LLU << 31)
/**
 * Flag which is set for preparatory walk over testing configuration
 * tree
 */
#define TESTER_PRERUN                 (1LLU << 32)

/**
 * If any of these flags are set, tester stops at
 * the first test producing result not matching them
 */
#define TESTER_RUN_WHILE_PASSED       (1LLU << 33)
#define TESTER_RUN_WHILE_FAILED       (1LLU << 34)
#define TESTER_RUN_WHILE_EXPECTED     (1LLU << 35)
#define TESTER_RUN_WHILE_UNEXPECTED   (1LLU << 36)

/**
 * If the flag is set, tester stops at the first test producing
 * verdict matching the verdict provided in tester args.
 */
#define TESTER_RUN_UNTIL_VERDICT      (1LLU << 37)

/** Gather the execution plan */
#define TESTER_ASSEMBLE_PLAN          (1LLU << 38)

/** Fail test scripts if valgrind detects a memory leak. */
#define TESTER_FAIL_ON_LEAK           (1LLU << 39)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TESTER_FLAGS_H__ */
