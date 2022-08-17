/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Configurator Tester
 *
 * Database implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "db.h"

/** Objects storage */
static obj_p object_list[MAX_OBJECT_NUMBER];

/** Current number of objects in the objects storage */
static int current_object_number;

/** Instances storage */
static inst_p instance_list[MAX_INSTANCE_NUMBER];

/** Current number of instances in the object instances storage */
static int current_inst_number;

/**
 * Read sub-identifier and instance name from the object or object instance
 * identifier.
 *
 * @param oid           object or object instance identifier
 * @param next_level    location where pointer to the next sub-identifier/
 *                      instance name pair should be placed
 * @param sub_id        location for sub-identifier address (memory is
 *                      allocated for sub-identifier using malloc())
 * @param inst_name     location for instance name address (memory is
 *                      allocated for instance name using malloc())
 *
 * @return error code
 * @retval 0            success
 * @retval TE_EINVAL       invalid identifier
 * @retval TE_ENOMEM       malloc() failed
 */
static int
parse_one_level(char *oid, char **next_level, char **sub_id,
                char **inst_name)
{
    char *tmp;
    char  c = '/';

    if (inst_name == NULL)
    {
        if (strcmp(oid, "*") == 0)
        {
            if ((*sub_id = strdup("*")) == NULL)
                return TE_ENOMEM;
            *next_level = oid;
            return 0;
        }
        if (*oid++ != '/')
           return TE_EINVAL;

        if ((*next_level = strchr(oid, '/')) == NULL)
        {
            *next_level = oid + strlen(oid);
            c = 0;
        }
        **next_level = 0;
        if ((*sub_id = strdup(oid)) == NULL)
            return TE_ENOMEM;
        **next_level = c;

        if ((strchr(*sub_id, '*') != NULL && strlen(*sub_id) > 1))
        {
            free(*sub_id);
            return TE_EINVAL;
        }

        return 0;
    }

    if (strcmp(oid, "*:*") == 0)
    {
        if ((*sub_id = strdup("*")) == NULL)
            return TE_ENOMEM;
        if ((*inst_name = strdup("*")) == NULL)
        {
            free(sub_id);
            return TE_ENOMEM;
        }
        *next_level = oid;
        return 0;
    }

    if (*oid != '/')
        return TE_EINVAL;

    oid++;

    if ((*next_level = strchr(oid, '/')) == NULL)
    {
        *next_level = oid + strlen(oid);
        c = 0;
    }
    else
        **next_level = 0;

    if ((tmp = strchr(oid, ':')) != NULL)
    {
        *tmp = 0;
        if ((*sub_id = strdup(oid)) == NULL)
            return TE_ENOMEM;
        if ((*inst_name = strdup(tmp + 1)) == NULL)
        {
            free(sub_id);
            return TE_ENOMEM;
        }
        *tmp = ':';
    }
    else
    {
        if (strcmp(oid, "*") != 0)
            return TE_EINVAL;

        if ((*sub_id = strdup("*")) == NULL)
            return TE_ENOMEM;
        if ((*inst_name = strdup("*")) == NULL)
        {
            free(sub_id);
            return TE_ENOMEM;
        }
    }

    **next_level = c;

    if ((strchr(*sub_id, '*') != NULL && strlen(*sub_id) > 1) ||
        (strchr(*inst_name, '*') != NULL && strlen(*inst_name) > 1) ||
        (**sub_id == '*' && **inst_name != '*'))
    {
        free(*sub_id);
        free(*inst_name);
        return TE_EINVAL;
    }

    return 0;
}

/**
 * Function determines if the oid matches the given pattern.
 * It works only with object IDs.
 *
 * @param pattern    The pattern.
 * @param oid        The oid.
 *
 * @return           0 if the oid doesn't match the pattern,
 *                   1 if the oid matches the pattern,
 *                   -errno if there was an error
 */
static int
match_obj(const char *pattern, const char *oid)
{
    char *tmp_patt;
    char *tmp_oid;
    char *patt_sub_id = NULL;
    char *oid_sub_id = NULL;
    char *curr_level;
    char *next_level;
    char *curr_patt;
    char *next_patt;

    if ((tmp_patt = strdup(pattern)) == NULL)
        return -TE_ENOMEM;
    if ((tmp_oid = strdup(oid)) == NULL)
    {
        free(tmp_patt);
        return -TE_ENOMEM;
    }

    curr_patt = tmp_patt;
    curr_level = tmp_oid;

    while (curr_patt != NULL && strcmp(curr_patt, "") != 0 &&
           curr_level != NULL && strcmp(curr_level, "") != 0)
    {
        if (parse_one_level(curr_patt, &next_patt, &patt_sub_id, NULL) != 0)
        {
            free(tmp_patt);
            if (patt_sub_id != NULL)
                free(patt_sub_id);
            free(tmp_oid);
            return -TE_EINVAL;
        }
        if (parse_one_level(curr_level, &next_level, &oid_sub_id, NULL) != 0)
        {
            free(tmp_patt);
            if (patt_sub_id != NULL)
                free(patt_sub_id);
            if (oid_sub_id != NULL)
                free(oid_sub_id);
            free(tmp_oid);
            return -TE_EINVAL;
        }

        if (*patt_sub_id != '*' &&
            strcmp(oid_sub_id, patt_sub_id) != 0)
        {
            free(tmp_patt);
            if (patt_sub_id != NULL)
                free(patt_sub_id);
            if (oid_sub_id != NULL)
                free(oid_sub_id);
            free(tmp_oid);
            return 0;
        }
        curr_patt = next_patt;
        curr_level = next_level;
        free(oid_sub_id);
        free(patt_sub_id);
        oid_sub_id = NULL;
        patt_sub_id = NULL;
    }
    if (curr_patt == NULL || curr_level == NULL)
        return -TE_EINVAL;

    if (strcmp(curr_patt, "") == 0 && strcmp(curr_level, "") == 0)
        return 1;

    return 0;
}

/**
 * Function determines if the oid matches the given pattern.
 * It works only with instance IDs.
 *
 * @param pattern    The pattern.
 * @param oid        The oid.
 *
 * @return           0 if the oid doesn't match the pattern,
 *                   1 if the oid matches the pattern,
 *                   -errno if there was an error
 */
static int
match_inst(const char *pattern, const char *oid)
{
    char *tmp_patt;
    char *tmp_oid;
    char *oid_sub_id = NULL;
    char *oid_inst = NULL;
    char *patt_sub_id = NULL;
    char *patt_inst = NULL;
    char *curr_level;
    char *next_level;
    char *curr_patt;
    char *next_patt;

    if ((tmp_patt = strdup(pattern)) == NULL)
        return -TE_ENOMEM;
    if ((tmp_oid = strdup(oid)) == NULL)
    {
        free(tmp_patt);
        return -TE_ENOMEM;
    }

    curr_patt = tmp_patt;
    curr_level = tmp_oid;

    while (curr_patt != NULL && strcmp(curr_patt, "") != 0 &&
           curr_level != NULL && strcmp(curr_level, "") != 0)
    {
        if (parse_one_level(curr_patt, &next_patt,
                            &patt_sub_id, &patt_inst) != 0)
        {
            if (patt_sub_id != NULL)
                free(patt_sub_id);
            if (patt_inst != NULL)
                free(patt_inst);
            free(tmp_patt);
            free(tmp_oid);
            return -TE_EINVAL;
        }
        if (parse_one_level(curr_level, &next_level,
                            &oid_sub_id, &oid_inst) != 0)
        {
            if (patt_sub_id != NULL)
                free(patt_sub_id);
            if (patt_inst != NULL)
                free(patt_inst);
            if (oid_sub_id != NULL)
                free(oid_sub_id);
            if (oid_inst != NULL)
                free(oid_inst);
            free(tmp_patt);
            free(tmp_oid);
            return -TE_EINVAL;
        }

        if (*patt_sub_id != '*' &&
            strcmp(oid_sub_id, patt_sub_id) != 0 &&
            *oid_inst != '*' &&
            strcmp(oid_inst, patt_inst) != 0)
        {
            free(patt_sub_id);
            free(oid_sub_id);
            free(oid_inst);
            free(patt_inst);
            free(tmp_oid);
            free(tmp_patt);
            return 0;
        }
        curr_patt = next_patt;
        curr_level = next_level;

        free(oid_sub_id);
        free(patt_sub_id);
        free(oid_inst);
        free(patt_inst);
        oid_sub_id = NULL;
        patt_sub_id = NULL;
    }

    if (curr_patt == NULL || curr_level == NULL)
    {
        free(tmp_oid);
        free(tmp_patt);
        return -TE_EINVAL;
    }

    if (strcmp(curr_patt, "") == 0 && strcmp(curr_level, "") == 0)
    {
        free(tmp_oid);
        free(tmp_patt);
        return 1;
    }

    free(tmp_oid);
    free(tmp_patt);
    return 0;
}

/**
 * Adds an object to the database.
 */
int
db_add_object(char *oid)
{
    int i = 0;
    obj_p obj;

    while (i < MAX_OBJECT_NUMBER && object_list[i] != NULL)
        i++;

    if (i == MAX_OBJECT_NUMBER)
    {
        ERROR("To many objects in the object list");
        return -TE_ENOMEM;
    }

    obj = calloc(sizeof(object), 1);
    if (obj == NULL)
        return -TE_ENOMEM;

    obj->sn = i;
    strcpy(obj->id, oid);

    object_list[i] = obj;
    current_object_number++;

    return i;
}

/**
 * Adds an instance to the database.
 */
int
db_add_instance(char *oid, char *value)
{
    int i = 0;
    instance *inst;

    while (i < MAX_INSTANCE_NUMBER && instance_list[i] != NULL)
    {
        i++;
    }
    if (i == MAX_INSTANCE_NUMBER)
    {
        ERROR("To many instances in the instances list");
        return -TE_ENOMEM;
    }

    inst = calloc(sizeof(instance), 1);
    if (inst == NULL)
        return -TE_ENOMEM;

    inst->sn = i;
    strcpy(inst->id, oid);
    strcpy(inst->val, value);

    instance_list[i] = inst;
    current_inst_number++;

    return i;
}

/**
 * Changes the instance value.
 */
int
db_set_inst(const char *oid, char *value)
{
    int i;
    for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
    {
        if (instance_list[i] != NULL &&
            strcmp(oid, instance_list[i]->id) == 0)
        {
            strcpy(instance_list[i]->val, value);
            return 0;
        }
    }

    return TE_EINVAL;

}

/**
 * Removes the object from the database.
 */
int
db_del_obj(const char *oid)
{
    int   i;
    obj_p obj;

    for (i = 0; i < MAX_OBJECT_NUMBER; i++)
    {
        if (object_list[i] != NULL &&
            strcmp(oid, object_list[i]->id) == 0)
        {
            obj = object_list[i];
            free(obj);
            object_list[i] = NULL;
            current_object_number--;
            return 0;
        }
    }

    return TE_EINVAL;
}

/**
 * Removes the instance from the database.
 */
int
db_del_inst(const char *oid)
{
    int    i;
    inst_p inst;

    for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
    {
        if (instance_list[i] != NULL &&
            strcmp(oid, instance_list[i]->id) == 0)
        {
            inst = instance_list[i];
            free(inst);
            instance_list[i] = NULL;
            current_inst_number--;
            return 0;
        }
    }

    return TE_EINVAL;
}

/**
 * Removes the entity from the database due to the
 * entity oid.
 */
int
db_del(const char *oid)
{
    int rc;

    if (strchr(oid, ':') != NULL)
    {
        rc = db_del_inst(oid);
    }
    else
    {
        rc = db_del_obj(oid);
    }

    return rc;
}

/**
 * Retrieves the objects from the database in wildcard case.
 */
/**
 * TODO: Now realloc is unused, because int the rcf_emul
 * the max answer size is BASE_ANSWER_SIZE
 */
int
db_get_obj(const char *oid, char **answer)
{
    int   i;
    char *tmp = NULL;

    char *ans = calloc(BASE_ANSWER_SIZE, 1);
    int   ans_len = 0;
    int   ans_buffer_len = BASE_ANSWER_SIZE;

    if (ans == NULL)
        return TE_ENOMEM;

    for (i = 0; i < MAX_OBJECT_NUMBER; i++)
    {
        if (object_list[i] != NULL &&
            match_obj(oid, object_list[i]->id) > 0)
        {
            ans_len += strlen(object_list[i]->id + 1);
            if (ans_len > ans_buffer_len)
            {
                tmp = realloc(ans, ans_buffer_len + BASE_ANSWER_SIZE);
                if (tmp == NULL)
                {
                    ans_len -= strlen(object_list[i]->id + 1);
                    return TE_ENOMEM;
                }
                ans_buffer_len += BASE_ANSWER_SIZE;
            }
            if (*ans != 0)
                strcat(ans, " ");
            strcat(ans, object_list[i]->id);
        }
    }
    *answer = ans;
    return 0;
}

/**
 * Retrieves the instances from the database in wildcard case.
 */
int
db_get_inst(const char *oid, char **answer)
{
    int   i;
    char *tmp = NULL;

    char *ans = calloc(BASE_ANSWER_SIZE, 1);
    int   ans_len = 0;
    int   ans_buffer_len = BASE_ANSWER_SIZE;

    if (ans == NULL)
        return TE_ENOMEM;

    for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
    {
        if (instance_list[i] != NULL &&
            match_inst(oid, instance_list[i]->id) == 0)
        {
            ans_len += strlen(instance_list[i]->id) +
                       strlen(instance_list[i]->val) + 1;
            if (ans_len > ans_buffer_len)
            {
                tmp = realloc(ans, ans_buffer_len + BASE_ANSWER_SIZE);
                if (tmp == NULL)
                {
                    ans_len -= strlen(instance_list[i]->id + 1) +
                               strlen(instance_list[i]->val);
                    return TE_ENOMEM;
                }
                ans_buffer_len += BASE_ANSWER_SIZE;
            }
            if (*ans != 0)
                strcat(ans, " ");
            strcat(ans, instance_list[i]->id);
        }

    }
    *answer = ans;
    return 0;
}

/**
 * Retrieves the entities from the database.
 */
int
db_get(const char *oid, char **answer, int *length)
{
    int rc = 0;
    int i;

    if (strstr(oid, "...") != NULL)
    {
        if (strchr(oid, ':') != NULL)
        {
            char *c;
            char *tmp = strdup(oid);
            char *ans;
            int   ans_len = 0;
            int   ans_buffer_len = BASE_ANSWER_SIZE;

            if (tmp == NULL)
                return TE_ENOMEM;

            ans = calloc(BASE_ANSWER_SIZE, 1);
            if (ans == NULL)
            {
                free(tmp);
                return TE_ENOMEM;
            }

            c = strrchr(tmp, '/');
            if (c == NULL)
            {
                free(tmp);
                free(ans);
                return TE_EINVAL;
            }

            *c = 0;

            for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
            {
                if (instance_list[i] != NULL &&
                    strncmp(instance_list[i]->id, tmp,
                            strlen(tmp)) == 0)
                {
                    ans_len += strlen(instance_list[i]->id) + 1;
                    if (ans_len > ans_buffer_len)
                    {
                        tmp = realloc(ans, ans_buffer_len +
                                      BASE_ANSWER_SIZE);
                        if (tmp == NULL)
                        {
                            ans_len += strlen(instance_list[i]->id + 1);
                            *c = '/';
                            return TE_ENOMEM;
                        }
                        ans_buffer_len += BASE_ANSWER_SIZE;
                    }
                    strcat(ans, " ");
                    strcat(ans, instance_list[i]->id);
                }
            }
            *c = '/';
            *answer = ans;

            free(tmp);
        }
        else
        {
            ERROR("Feature is not supported");
        }
    }
    else if (strchr(oid, '*') == NULL)
    {
        if (strchr(oid, ':') == NULL)
        {
            for (i = 0; i < MAX_OBJECT_NUMBER; i++)
            {
                if (object_list[i] != NULL &&
                    strcmp(oid, object_list[i]->id) == 0)
                {
                    *answer = strdup(object_list[i]->id);
                    if (*answer == NULL)
                        return TE_ENOMEM;
                    break;
                }
            }
        }
        else
        {
            for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
            {
                if (instance_list[i] != NULL &&
                    strcmp(oid, instance_list[i]->id) == 0)
                {
                    *answer = strdup(instance_list[i]->val);
                    if (*answer == NULL)
                        return TE_ENOMEM;
                    break;
                }
            }
        }
    }
    else
    {
        if (strchr(oid, ':') != NULL)
        {
            rc = db_get_inst(oid, answer);
        }
        else
        {
            rc = db_get_obj(oid, answer);
        }
    }

    if (answer != NULL)
        *length = strlen(*answer) + sizeof('\0');
    else
        *length = -1;

    return rc;
}

/**
 * Initalizes the database.
 */
int
db_init(char *db_file_name)
{
    int   result;

    memset(object_list, 0, sizeof(object_list));
    memset(instance_list, 0, sizeof(instance_list));
    current_object_number = 0;
    current_inst_number = 0;

    if (db_file_name != NULL)
    {
        FILE *file_db;
        char *buf = calloc(MAX_NAME_LENGTH, 1);
        char *val = calloc(MAX_NAME_LENGTH, 1);

        if (buf == NULL || val == NULL)
            return TE_ENOMEM;

        file_db = fopen(db_file_name, "r");
        if (file_db == NULL)
            return TE_EINVAL;

        /*
         * reading objects database from the database file
         */
        do {
            fscanf(file_db, "%s", buf);
            if (strcmp(buf, OBJECT_END) == 0)
                break;
            result = db_add_object(buf);
            if (result < 0)
            {
                ERROR("Configuration file is to big");
                break;
            }
        } while(1);

        /*
         * reading instances database from the database file
         */
        do {
            if (*val == '/' || *val == '*')
                memcpy(buf, val, MAX_NAME_LENGTH);
            else
                fscanf(file_db, "%s", buf);

            if (strcmp(buf, INST_END) == 0 || strcmp(val, INST_END) == 0)
                break;

            fscanf(file_db, "%s", val);

            if (*val == '/' || *val == '*')
                result = db_add_instance(buf, "");
            else
                result = db_add_instance(buf, val);

            if (result < 0)
            {
                ERROR("Configuration file is to big");
                break;
            }

        } while(1);
        free(buf);
        free(val);
        fclose(file_db);
    }

    return 0;
}

/**
 * Frees the database.
 */
int
db_free(void)
{
    int i;

    for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
    {
        if (instance_list[i] != NULL)
        {
            free(instance_list[i]);
            instance_list[i] = NULL;
        }
    }
    for (i = 0; i < MAX_OBJECT_NUMBER; i++)
    {
        if (object_list[i] != NULL)
        {
            free(object_list[i]);
            object_list[i] = NULL;
        }
    }
    current_object_number = 0;
    current_inst_number = 0;

    return 0;
}

/**
 * Prints all objects ID to the buffer.
 */
int
db_print_objects(char *buffer, unsigned int *length)
{
    int i;
    int ans_len = 0;

    memset(buffer, 0, *length);

    for (i = 0; i < MAX_OBJECT_NUMBER; i++)
    {
        if (object_list[i] != NULL)
        {
            if (ans_len + strlen(object_list[i]->id) > *length)
                break;

            ans_len += sprintf(buffer + ans_len, "%s ", object_list[i]->id);
        }
    }

    return 0;
}

/**
 * Prints all instances to the buffer.
 */
int
db_print_instances(char *buffer, unsigned int *length)
{
    int i;
    int ans_len = 0;

    memset(buffer, 0, *length);

    for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
    {
        if (instance_list[i] != NULL)
        {
            if (ans_len + strlen(instance_list[i]->id) +
                strlen(instance_list[i]->val) > *length)
                break;

            ans_len += sprintf(buffer + ans_len, "%s%s%s ",
                               instance_list[i]->id, " = ",
                               (char *)instance_list[i]->val);
        }
    }

    return 0;
}

/**
 * Special function. It is used, when the RCF Emulator wants to reboot
 * the agent. All data of the agent should be deleted from the database.
 */
/**
 * TODO: This function is a hack. It is made to make reboot process
 * quicker.
 */
int
db_clear_agents_data(char *agents_name)
{
    int i;

    for (i = 0; i < MAX_INSTANCE_NUMBER; i++)
    {

        char tmp[sizeof("/agent:") + 10];

        memset(tmp, 0, sizeof(tmp));
        strcpy(tmp, "/agent:");
        strcat(tmp, agents_name);

        if (strncmp(tmp, instance_list[i]->id, strlen(tmp)) == 0)
        {
            free(instance_list[i]);
            instance_list[i] = NULL;
        }
    }

    return 0;
}
