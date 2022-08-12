/** @file
 * @brief TE project. Logger subsystem.
 *
 * Internal utilities for Logger configuration file parsing.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_LOGGER_CNF_H__
#define __TE_LOGGER_CNF_H__

#include <yaml.h>

#ifdef _cplusplus
extern "C" {
#endif

/** Configuration file format */
typedef enum cfg_file_type {
    CFG_TYPE_ERROR,
    CFG_TYPE_EMPTY,
    CFG_TYPE_XML,
    CFG_TYPE_YAML,
    CFG_TYPE_OTHER,
} cfg_file_type;

/**
 * Determine config file format.
 *
 * @param filename      path to the config file
 */
extern cfg_file_type get_cfg_file_type(const char *filename);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_CNF_H__ */
