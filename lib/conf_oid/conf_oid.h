/** @file
 * @brief Configurator
 *
 * Object identifiers
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CONF_OID_H__
#define __TE_CONF_OID_H__
#ifdef __cplusplus
extern "C" {
#endif

#define CFG_SUBID_MAX       32   /**< Maximum length of sub-identifier 
                                      including trailing 0 */
#define CFG_INST_NAME_MAX   128  /**< Instance name, including trailing 0 */

#define CFG_OID_MAX \
    ((CFG_SUBID_MAX + CFG_INST_NAME_MAX + 2) * CFG_OID_LEN_MAX)

/** Object identifier element */
typedef struct cfg_object_subid {
    char subid[CFG_SUBID_MAX]; /**< For root object "/" */
} cfg_object_subid;

/** Object instance identifier element */
typedef struct cfg_inst_subid {
    char subid[CFG_SUBID_MAX];    /**< For root instance "/" */
    char name[CFG_INST_NAME_MAX]; /**< For root instance empty */
} cfg_inst_subid;

/** Object identifier */
typedef struct cfg_oid {
    uint8_t len;  /**< Number of identifier elements */
    te_bool inst; /**< TRUE, if OID is object instance identifier */
    void   *ids;  /**< Pointer to array of identifier elements */
} cfg_oid;


/**
 * Get instance name of the i-th sub-identifier.
 *
 * @param _p      pointer to cfg_oid
 * @param _i      index
 *
 * @return Pointer to string (char *).
 */
#define CFG_OID_GET_INST_NAME(_p, _i) \
    (((cfg_inst_subid *)(_p->ids))[(_i)].name)

/**
 * Allocate memory for object identifier or object instance identifier.
 *
 * @param length      number of identifier elements
 * @param inst        if TRUE - object instance identifier should be
 *                    allocated
 *
 * @return newly allocated structure pointer or NULL
 */
extern cfg_oid *cfg_allocate_oid(int length, te_bool inst);

/**
 * Convert object identifier or object instance identifier in
 * string representation to cfg_oid structure.
 *
 * @param oid         OID in string representation
 *
 * @return newly allocated structure pointer or NULL
 */
extern cfg_oid *cfg_convert_oid_str(const char *oid);

/**
 * Convert object identifier or object instance identifier in
 * structure representation to string (memory is allocated by the
 * routine using malloc()).
 *
 * @param oid         OID in structure representation
 *
 * @return newly allocated string or NULL
 */
extern char *cfg_convert_oid(const cfg_oid *oid);

/**
 * Free memory allocated for OID stucture.
 *
 * @param oid         oid structure
 */
extern void cfg_free_oid(cfg_oid *oid);

/**
 * Compare object identifiers.
 *
 * @param o1      the first OID
 * @param o2      the second OID
 *
 * @retval 0      equal
 * @retval 1      not equal
 */
extern int cfg_oid_cmp(const cfg_oid *o1, const cfg_oid *o2);

/**
 * Convert instance identifier to object identifier.
 *
 * @param inst_oid       instance identifier
 * @param obj_oid        object identifier (CFG_OID_MAX length)
 */
static inline void
cfg_oid_inst2obj(const char *inst_oid, char *obj_oid)
{
    cfg_oid *oid = cfg_convert_oid_str(inst_oid);
    
    *obj_oid = 0;
    
    if (oid != NULL && oid->inst && oid->len > 1)
    {
        int i;
        
        for (i = 1; i < oid->len; i++)
            obj_oid += sprintf(obj_oid, "/%s", 
                               ((cfg_inst_subid *)(oid->ids))[i].subid);
    }
    
    cfg_free_oid(oid);
}

#ifdef __cplusplus
}
#endif
#endif /* __TE_CONF_OID_H__ */

