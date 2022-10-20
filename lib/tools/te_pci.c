/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief PCI-related constants and supplementary functions.
 *
 * Definitions for PCI id mapping functions.
 */

#define TE_LGR_USER     "TE PCI"

#include "te_config.h"

#include "te_enum.h"
#include "te_pci.h"

#define STARTDEF(_name) static const te_enum_map map_##_name[] = {
#define ENDDEF(_name) TE_ENUM_MAP_END };
#define ITEM(_key, _value) {.name = #_key, .value = TE_PCI_##_key },
#include "te_pci_ids.h"
#undef ITEM
#undef ENDDEF
#undef STARTDEF

const char *
te_pci_class_id2str(unsigned int class)
{
    return te_enum_map_from_any_value(map_class, class,
                                      "CLASS_UNCLASSIFIED_DEVICE");
}

unsigned int
te_pci_class_str2id(const char *label)
{
    return te_enum_map_from_str(map_class, label,
                                TE_PCI_CLASS_UNCLASSIFIED_DEVICE);
}

const char *
te_pci_subclass_id2str(unsigned int subclass)
{
    const char *label = te_enum_map_from_any_value(map_subclass, subclass,
                                                   NULL);

    if (label == NULL)
        return te_pci_class_id2str(te_pci_subclass2class(subclass));

    return label;
}

unsigned int
te_pci_subclass_str2id(const char *label)
{
    int id = te_enum_map_from_str(map_subclass, label, -1);

    if (id < 0)
        return te_pci_subclass_default(te_pci_class_str2id(label));

    return (unsigned int)id;
}

const char *
te_pci_progintf_id2str(unsigned int progintf)
{
    const char *label = te_enum_map_from_any_value(map_prog_interface, progintf,
                                                   NULL);

    if (label == NULL)
        return te_pci_subclass_id2str(te_pci_progintf2subclass(progintf));

    return label;
}

unsigned int
te_pci_progintf_str2id(const char *label)
{
    int id = te_enum_map_from_str(map_prog_interface, label, -1);

    if (id < 0)
        return te_pci_progintf_default(te_pci_subclass_str2id(label));

    return (unsigned int)id;
}
