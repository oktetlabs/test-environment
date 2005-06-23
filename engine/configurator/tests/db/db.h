/** @file
 * @brief Configurator Tester
 *
 * Database structure definition
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author  Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 */

#ifndef __DB_H__
#define __DB_H__

#include "te_config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "rcf_common.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "logger_ten.h"

/**
 * Maximum length of the object name or object instance name + instance
 * value length
 */
#define MAX_NAME_LENGTH 255
/** 
 * Base answer size. If the answer does not fit
 * we make realloc(current size + BASE_ANSWER_SIZE)
 */
#define BASE_ANSWER_SIZE 16384

/** 
 * Maximum number of the objects supported by the
 * object storage.
 */
#define MAX_OBJECT_NUMBER 1000

/**
 * Maximum number of the object instances supported
 * by the instance storage.
 */
#define MAX_INSTANCE_NUMBER 1000

/**
 * End of the object part of the config file.
 */
#define OBJECT_END "***DB_OBJECTS_END***"

/**
 * End of the instance part of the config file.
 */
#define INST_END "***DB_INSTANCES_END***"

/** Object storage class */
typedef struct object {
    int   sn;             /**< Index of the object in the object storage */
    char  id[RCF_MAX_ID]; /**< Full name of the object */
} object;

/** Object Instance storage class */
typedef struct instance {
    int   sn;                /**< Index of the object in the object storage */
    char  id[RCF_MAX_ID];    /**< Full name of the instance */
    char  val[RCF_MAX_VAL];  /**< Value of the instance */
} instance;

/** Object pointer type */
typedef object *obj_p;

/** Object Instance pointer type */
typedef instance *inst_p;


/**
 * Find objects matching to specified wildcard identifier.
 *
 * @param oid    object ID
 * @param answer pointer to the string which contains all the objects
 *               from the database satisfying the oid (OUT)
 *
 * @return       0 - on success
 *               errno otherwise
 */
extern int
db_get_obj(const char *oid, char **answer);

/** 
 * Find instances matching to specified wildcard identifier.
 * 
 * @param oid    object instance ID
 * @param answer pointer to the string which contains all the object
 *               instances from the database satisfying the oid (OUT)
 *
 * @return       0 - on success
 *               errno otherwise
 */
extern int
db_get_inst(const char *oid, char **answer);

/**
 * Find objects or instances matching to specified wildcard identifier.
 * For non-wildcard instance identifiers the value is returned.
 *
 * @param oid    object of object instance ID
 * @param answer pointer ot hte string which contains all the retrieved
 *               information in the string representation (OUT)
 * @param length total answer length (OUT)
 * 
 * @return       0 - success
 *               errno otherwise
 */
extern int
db_get(const char *oid, char **answer, int *length);

/**
 * Set instance value.
 *
 * @param oid    object instance ID
 * @param value  object instance value
 *
 * @return       0 - success
 *               errno otherwise
 */
extern int
db_set_inst(const char *oid, char *value);

/**
 * Delete object or object instance.
 *
 * @param oid    ID of the object or object instance to delete
 *
 * @return       0 - success
 *               errno otherwise
 */
extern int
db_del(const char *oid);

/**
 * Delete object.
 *
 * @param oid    ID of the object to delete
 *
 * @return       0 - success
 *               errno otherwise
 */
extern int
db_del_obj(const char *oid);

/**
 * Delete object instance.
 *
 * @param oid    ID of the object instance to delete
 *
 * @return       0 - success
 *               errno otherwise
 */
extern int
db_del_inst(const char *inst);

/**
 * Add an object to the database.
 *
 * @param oid    ID of the object to add
 *
 * @return       object descriptor if success
 *               -errno otherwise
 */
extern int
db_add_object(char *oid);

/**
 * Add an object instance to the database.
 *
 * @param oid    ID of the object instance to add
 * @param value  value of the object instance
 *
 * @return       object instance descriptor if success
 *               -errno otherwise
 */
extern int 
db_add_instance(char *oid, char *value);

/**
 * Add an object instance or object to the database.
 *
 * @param oid    ID of the object instance or object to add
 * @param value  value of the object instance (or NULL if we
 *               are adding an object)
 *
 * @return       descriptor if success
 *               -errno otherwise
 */
extern int
db_add(char *oid, char *value);

/** 
 * Inits database related structures 
 *
 * @param db_file_name  the file from which the database should
 *                      be loaded (or NULL)
 *
 * @return              0 - on success
 */
extern int
db_init(char *db_file_name);

/**
 * Cleares database related structures.
 *
 * @return       0 - on success
 */
extern int
db_free(void);

/**
 * Prints all ID's of all objects stored in the database 
 * to the user buffer.
 *
 * @param buffer the buffer where the answer is stored
 * @param length the total size of the buffer (IN/OUT)
 *
 * @return       0 - on success
 *               errno otherwise
 */
extern int
db_print_objects(char *buffer, unsigned int *length);

/**                                             
 * Prints all ID's of all objects instances and their values
 * to the user buffer.
 *
 * @param buffer the buffer where the answer is stored
 * @param length the total size of the buffer (IN/OUT)
 *
 * @return       0 - on success
 *               errno otherwise
 */
extern int
db_print_instances(char *buffer, unsigned int *length);

/**
 * Special function. It is used, when the RCF Emulator wants to reboot
 * the agent. All data of the agent should be deleted from the
 * database.
 *
 * @param agents_name  Name of the agents which data should be deleted.
 */
int
db_clear_agents_data(char *agents_name);
#endif /* !__DB_H__ */
