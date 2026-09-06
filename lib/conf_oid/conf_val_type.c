/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 OKTET Ltd. All rights reserved. */
/** @file
 * @brief Configurator primary value types: name mapping
 */

#include "te_config.h"

#include "conf_val_type.h"

/* See the description in conf_val_type.h */
const te_enum_map cfg_cvt_mapping[] = {
    {.name = "bool",     .value = CVT_BOOL},
    {.name = "int8",     .value = CVT_INT8},
    {.name = "uint8",    .value = CVT_UINT8},
    {.name = "int16",    .value = CVT_INT16},
    {.name = "uint16",   .value = CVT_UINT16},
    {.name = "int32",    .value = CVT_INT32},
    {.name = "integer",  .value = CVT_INT32},
    {.name = "uint32",   .value = CVT_UINT32},
    {.name = "int64",    .value = CVT_INT64},
    {.name = "uint64",   .value = CVT_UINT64},
    {.name = "double",   .value = CVT_DOUBLE},
    {.name = "string",   .value = CVT_STRING},
    {.name = "address",  .value = CVT_ADDRESS},
    {.name = "none",     .value = CVT_NONE},
    TE_ENUM_MAP_END
};
