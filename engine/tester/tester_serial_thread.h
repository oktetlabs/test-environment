/** @file
 * @brief Serial console parser events handler thread of the Tester
 *
 * Thread interaction functions definition
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 */

/**
 * Start the Tester thread to handle serial parsers events
 *
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_start_serial_thread(void);

/**
 * Cleanup and stop the tester serial thread
 *
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_stop_serial_thread(void);

/**
 * Set the process identifier of the current test
 *
 * @param i_pid     PID of the current test process
 *
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_set_serial_pid(pid_t i_pid);

/**
 * Release the process identifier of the current test
 *
 * @return Status code
 * @retval 0    Success
 */
extern te_errno tester_release_serial_pid(void);

/**
 * Check the flag to stop test sequence
 *
 * @return
 * @retval TRUE     Test sequence should be stopped
 * @retval FALSE    Continue work
 */
extern te_bool tester_check_serial_stop(void);
