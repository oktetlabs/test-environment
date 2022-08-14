/** @file
 * @brief TE project. Logger subsystem.
 *
 * Internal utilities for Logger configuration file parsing.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include "logger_cnf.h"
#include "te_str.h"

/* See description in logger_cnf.h */
cfg_file_type
get_cfg_file_type(const char *filename)
{
#define PREREAD_SIZE 8
    static const char xml_head[]  = "<?xml";
    static const char yaml_head[] = "---";

    int      res = 0;
    char     buf[PREREAD_SIZE + 1];
    FILE    *fp;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return CFG_TYPE_ERROR;

    res = fread(buf, 1, PREREAD_SIZE, fp);
    if (res == 0 && feof(fp))
    {
        /* Config file is empty, nothing to do */
        fclose(fp);
        return CFG_TYPE_EMPTY;
    }
    fclose(fp);
#undef PREREAD_SIZE

    buf[res] = '\0';
    if (strncmp(buf, yaml_head, sizeof(yaml_head) - 1) == 0)
        return CFG_TYPE_YAML;
    else if (strncmp(buf, xml_head, sizeof(xml_head) - 1) == 0)
        return CFG_TYPE_XML;
    else
        return CFG_TYPE_OTHER;
}
