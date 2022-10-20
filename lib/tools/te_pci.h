/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief PCI-related constants and supplementary functions.
 *
 * Most constants are actually defined in te_pci_ids.h which
 * is included multiple times with varying macro expansions to
 * ensure consistency of enums and mapping tables.
 *
 * The actual data are obtained semi-automatically from
 * PCI ids database, see http://pci-ids.ucw.cz/.
 *
 * @defgroup te_tools_te_pci PCI definitions
 * @ingroup te_tools
 * @{
 * PCI-related constants and supplementary functions.
 */

#ifndef __TE_PCI_H__
#define __TE_PCI_H__

/** @enum te_pci_class
 * A set of defined PCI class values.
 *
 * @attention
 * Though it is an enum, it is not a closed list.
 * The user should be prepared to deal with values outside
 * of the defined list.
 */

/** @enum te_pci_class
 * A set of defined PCI subclass values.
 *
 * The values are actually class + subclass.
 *
 * @attention
 * Though it is an enum, it is not a closed list.
 * The user should be prepared to deal with values outside
 * of the defined list.
 */

/** @enum te_pci_prog_interface
 * A set of defined PCI programming interface values.
 *
 * The values are actually class + subclass + intf.
 *
 * @note
 * Most devices do not distinguish between several
 * programming interfaces, so the programming interface
 * value for them will be just class + subclass +
 * eight trailing zero bits.
 *
 * @attention
 * Though it is an enum, it is not a closed list.
 * The user should be prepared to deal with values outside
 * of the defined list.
 */

#define STARTDEF(_name) typedef enum te_pci_##_name {
#define ENDDEF(_name) } te_pci_##_name;
#define ITEM(_key, _value) TE_PCI_##_key = (_value),
#include "te_pci_ids.h"
#undef ITEM
#undef ENDDEF
#undef STARTDEF

/**
 * Provide a string description of a PCI @p class.
 *
 * An unknown value will be treated as @c TE_PCI_CLASS_UNCLASSIFIED_DEVICE.
 *
 * @param  class   class ID (see te_pci_class)
 *
 * @return the class label
 */
extern const char *te_pci_class_id2str(unsigned int class);

/**
 * Provide a PCI class ID matching a given @p label.
 *
 * @param  label   PCI class label
 *
 * @return the class ID
 * @retval TE_PCI_CLASS_UNCLASSIFIED_DEVICE if the label is unknown
 */
extern unsigned int te_pci_class_str2id(const char *label);

/**
 * Provide a string description of a PCI @p subclass.
 *
 * If @p subclass is not defined, the function will return
 * a label for a class part as per te_pci_class_id2str().
 *
 * @param  subclass   subclass ID (see te_pci_subclass)
 *
 * @return the subclass label
 */
extern const char *te_pci_subclass_id2str(unsigned int subclass);

/**
 * Provide a PCI class ID matching a given @p label.
 *
 * The function may be passed any label accepted by te_pci_class_str2id(),
 * then the shifted value of a class ID will be returned.
 *
 * @param  label   PCI subclass label
 *
 * @return the subclass ID
 */
extern unsigned int te_pci_subclass_str2id(const char *label);

/**
 * Provide a string description of a PCI @p progintf.
 *
 * If @p progintf is not defined, the function will return
 * a label for a subclass part as per te_pci_subclass_id2str().
 *
 * @param  progintf   programming interface ID (see te_pci_prog_inteface)
 *
 * @return the subclass label
 */
extern const char *te_pci_progintf_id2str(unsigned int progintf);

/**
 * Provide a PCI prog inteface ID matching a given @p label.
 *
 * The function may be passed any label accepted by te_pci_subclass_str2id(),
 * then the shifted value of a subclass ID will be returned.
 *
 * @param  label   PCI subclass label
 *
 * @return the programming interface ID
 */
extern unsigned int te_pci_progintf_str2id(const char *label);


/**
 * Get a class part of a PCI subclass ID.
 *
 * @param subclass   PCI subclass ID
 *
 * @return class ID
 */
static inline unsigned int
te_pci_subclass2class(unsigned int subclass)
{
    return (subclass >> 8) & 0xff;
}

/**
 * Get a default PCI subclass from a PCI class ID.
 *
 * @param class   PCI class ID
 *
 * @return subclass ID
 */
static inline unsigned int
te_pci_subclass_default(unsigned int class)
{
    return class << 8;
}

/**
 * Get a subclass part of a PCI programming interface ID.
 *
 * @param progintf Programming interace ID
 *
 * @return Subclass ID
 */
static inline unsigned int
te_pci_progintf2subclass(unsigned int progintf)
{
    return (progintf >> 8) & 0xffff;
}

/**
 * Get a default PCI programming inteface ID from a PCI subclass ID.
 *
 * @param subclass   PCI subclass ID
 *
 * @return programming interface ID
 */
static inline unsigned int
te_pci_progintf_default(unsigned int subclass)
{
    return subclass << 8;
}

/**
 * Get a class part of a PCI programming interface ID.
 *
 * @param progintf Programming interace ID
 *
 * @return Class ID
 */
static inline unsigned int
te_pci_progintf2class(unsigned int progintf)
{
    return te_pci_subclass2class(te_pci_progintf2subclass(progintf));
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_PCI_H__ */
/**@} <!-- END te_tools_te_pci --> */
