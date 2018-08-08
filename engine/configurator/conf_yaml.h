/** @file
 * @brief YAML configuration file processing facility
 *
 * API definitions.
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Ivan Malov <Ivan.Malov@oktetlabs.ru>
 */

#ifndef __TE_CONF_YAML_H__
#define __TE_CONF_YAML_H__

#include "te_errno.h"

/**
 * Process YAML configuration file.
 *
 * The input file must be a YAML document containing dynamic history
 * statements. One may leverage these statements to create instances
 * for the objects maintained by the primary configuration file. The
 * instances may come with logical expressions either per individual
 * entry or per a bunch of entries to indicate conditions which must
 * be true for the instances to hit the XML document being generated.
 *
 * The output will be written to a file using the
 * original path and XML extension.
 *
 * @param filename The input file path
 *
 * @return Status code.
 */
extern te_errno transform_conf_yaml(const char *filename);

#endif /* __TE_CONF_YAML_H__ */
