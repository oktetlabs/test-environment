#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cyaml/cyaml.h>
#include "conf_cyaml.h"

/*
 ******************************************************************************
 * CYAML schema to tell libcyaml about both expected YAML and data structure.
 *
 * (Our CYAML schema is just a bunch of static const data.)
 ******************************************************************************
 */

/** Enumeration for object or instance. It answers if it is an object */
enum cfg_cyaml_scope {
    CFG_CYAML_INSTANCE = 0, /* It is a default value */
    CFG_CYAML_OBJECT
};

/** Mapping from instance or object strings to enum values for schema. */
static const cyaml_strval_t cyaml_scope_strings[] = {
    {"object",   CFG_CYAML_OBJECT},
    {"instance", CFG_CYAML_INSTANCE},
};

/** Mapping from @c access strings to enum values for schema. */
static const cyaml_strval_t cyaml_access_strings[] = {
    {"read_write",  CFG_READ_WRITE },
    {"read_only",   CFG_READ_ONLY  },
    {"read_create", CFG_READ_CREATE},
};

/** Mapping from @c type strings to flag enum for schema. */
static const cyaml_strval_t cyaml_type_strings[] = {
    {"none",    CVT_NONE},
    {"bool",    CVT_BOOL},
    {"int8",    CVT_INT8},
    {"uint8",   CVT_UINT8},
    {"int16",   CVT_INT16},
    {"uint16",  CVT_UINT16},
    {"int32",   CVT_INT32},
    {"integer", CVT_INT32},
    {"uint32",  CVT_UINT32},
    {"int64",   CVT_INT64},
    {"uint64",  CVT_UINT64},
    {"string",  CVT_STRING},
    {"address", CVT_ADDRESS},
};

/** Mapping from @c no_parent_dep strings to enum values for schema. */
static const cyaml_strval_t cyaml_no_parent_dep_strings[] = {
    {"yes", FALSE}, /* it is a default value */
    {"no",  TRUE},
};

/**
 * The depends mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, @c depends_entry.
 */
static const cyaml_schema_field_t cyaml_depends_entry_fields_schema[] = {
    /**
     * The first field in the mapping is an oid.
     *
     * YAML key: @c oid.
     * C structure member for this key: @c oid.
     *
     * Its CYAML type is string pointer, and we have no minimum or maximum
     * string length constraints.
     */
    CYAML_FIELD_STRING_PTR("oid", CYAML_FLAG_POINTER,
                           depends_entry, oid, 0, CYAML_UNLIMITED),

    /**
     * The scope field is an enum.
     *
     * YAML key: @c scope.
     * C structure member for this key: @c scope.
     * Array mapping strings to values: cyaml_scope_strings
     *
     * Its CYAML type is ENUM, so an array of cyaml_strval_t must be
     * provided to map from string to values.
     */
    CYAML_FIELD_ENUM("scope", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
                     depends_entry, scope, cyaml_scope_strings,
                     CYAML_ARRAY_LEN(cyaml_scope_strings)),

    CYAML_FIELD_END
};

/**
 * The value for a depends is a mapping.
 *
 * Its fields are defined in @c depends_entries_fields_schema.
 */
static const cyaml_schema_value_t cyaml_depends_schema = {
        CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, depends_entry,
                            cyaml_depends_entry_fields_schema),
};

/**
 * The object mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, @c object_type.
 */
static const cyaml_schema_field_t cyaml_object_fields_schema[] = {
    /**
     * The first field in the mapping is oid.
     *
     * YAML key: @c oid.
     * C structure member for this key: @c oid.
     *
     * Its CYAML type is string pointer, and we have no minimum or maximum
     * string length constraints.
     */
    CYAML_FIELD_STRING_PTR("oid", CYAML_FLAG_POINTER,
                           object_type, oid, 0, CYAML_UNLIMITED),

    /**
     * The access field is an enum.
     *
     * YAML key: @c access.
     * C structure member for this key: @c access.
     * Array mapping strings to values: cyaml_access_strings
     *
     * Its CYAML type is ENUM, so an array of cyaml_strval_t must be
     * provided to map from string to values.
     */
    CYAML_FIELD_ENUM("access", CYAML_FLAG_DEFAULT,
                     object_type, access, cyaml_access_strings,
                     CYAML_ARRAY_LEN(cyaml_access_strings)),

    /**
     * The type field is an enum.
     *
     * YAML key: @c type.
     * C structure member for this key: @c type.
     * Array mapping strings to values: cyaml_type_strings
     *
     * Its CYAML type is ENUM, so an array of cyaml_strval_t must be
     * provided to map from string to values.
     */
    CYAML_FIELD_ENUM("type", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
                     object_type, type, cyaml_type_strings,
                     CYAML_ARRAY_LEN(cyaml_type_strings)),

    /**
     * The unit field is a te_bool.
     *
     * YAML key: @c unit.
     * C structure member for this key: @c unit.
     */
    CYAML_FIELD_BOOL("unit", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
                     object_type, unit),

    /**
     * The default field in the mapping is a string.
     *
     * YAML key: @c default.
     * C structure member for this key: @c def_val.
     *
     * Its CYAML type is string pointer, and we have no minimum or maximum
     * string length constraints.
     */
    CYAML_FIELD_STRING_PTR("default", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                           object_type, def_val, 0, CYAML_UNLIMITED),

    /**
     * The volat field is a te_bool.
     *
     * YAML key: @volatile.
     * C structure member for this key: @c volat.
     */
    CYAML_FIELD_BOOL("volatile", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
                     object_type, volat),

    /**
     * The substitution field is a te_bool.
     *
     * YAML key: @c substitution.
     * C structure member for this key: @c substitution.
     */
    CYAML_FIELD_BOOL("substitution", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
                     object_type, substitution),

    /**
     * The parent-dep field is a te_bool but we pretend that it is a enum.
     *
     * YAML key: @c parent-dep.
     * C structure member for this key: @c no_parent_dep.
     *
     * Array mapping strings to values: @cyaml_no_parent_dep_strings
     *
     * Its CYAML type is ENUM, so an array of @cyaml_strval_t must be
     * provided to map from string to values.
     */
    CYAML_FIELD_ENUM("parent-dep", CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
                     object_type, no_parent_dep, cyaml_no_parent_dep_strings,
                     CYAML_ARRAY_LEN(cyaml_no_parent_dep_strings)),

    /**
     * The next field is the instances or objects that this object depends on.
     *
     * YAML key: @c depends.
     * C structure member for this key: @c depends.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too.  In this case, it's cyaml_depends_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored.  In this case, it
     * goes in the @c depends_count C structure member in
     * @c object_type.
     * Since this is @c depends with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("depends",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         object_type, depends,
                         &cyaml_depends_schema, 0, CYAML_UNLIMITED),

    /**
     * If the YAML contains a field that our program is not interested in
     * we can ignore it, so the value of the field will not be loaded.
     *
     * Note that unless the OPTIONAL flag is set, the ignored field must
     * be present.
     *
     * Another way of handling this would be to use the config flag
     * to ignore unknown keys.  This config is passed to libcyaml
     * separately from the schema.
     */
    CYAML_FIELD_IGNORE("d", CYAML_FLAG_OPTIONAL),

    CYAML_FIELD_END
};

/**
 * The instance mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure.
 */
static const cyaml_schema_field_t cyaml_instance_fields_schema[] = {
    /**
     * The first field in the mapping is oid.
     *
     * YAML key: @c oid.
     * C structure member for this key: @c oid.
     *
     * Its CYAML type is string pointer, and we have no minimum or maximum
     * string length constraints.
     */
    CYAML_FIELD_STRING_PTR("oid", CYAML_FLAG_POINTER,
                           instance_type, oid, 0, CYAML_UNLIMITED),

    /**
     * The value field in the mapping is a string.
     *
     * YAML key: @c value.
     * C structure member for this key: @c value.
     *
     * Its CYAML type is string pointer, and we have no minimum or maximum
     * string length constraints.
     */
    CYAML_FIELD_STRING_PTR("value", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                           instance_type, value, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * The backup_entry mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, @c backup_entry.
 */
static const cyaml_schema_field_t cyaml_backup_entry_fields_schema[] = {
    /**
     * The first field is the object.
     *
     * YAML key: @c object.
     * C structure member for this key: @c object.
     *
     * Its CYAML type is a mapping.
     *
     * Since it's a mapping type, the schema for its mapping's fields must
     * be provided too. In this case, it's cyaml_object_fields_schema.
     */
    CYAML_FIELD_MAPPING_PTR("object", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                            backup_entry, object, cyaml_object_fields_schema),

    /**
     * The next field is the instance.
     *
     * YAML key: @c instance.
     * C structure member for this key: @c instance.
     *
     * Its CYAML type is a mapping.
     *
     * Since it's a mapping type, the schema for its mapping's fields must
     * be provided too. In this case, it's cyaml_instance_fields_schema.
     */
    CYAML_FIELD_MAPPING_PTR("instance",
                            CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                            backup_entry, instance,
                            cyaml_instance_fields_schema),

    CYAML_FIELD_END
};

/**
 * The value for a backup_entry is a mapping.
 *
 * Its fields are defined in cyaml_backup_entry_fields_schema.
 */
static const cyaml_schema_value_t cyaml_backup_entry_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, backup_entry,
                        cyaml_backup_entry_fields_schema),
};

/**
 * The backup mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, @c backup_seq.
 */
static const cyaml_schema_field_t cyaml_backup_fields_schema[] = {
    /**
     * The only field is the instances or objects.
     *
     * YAML key: @c backup.
     * C structure member for this key: @c entries.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too. In this case, it's cyaml_backup_entry_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored. In this case, it
     * goes in the @c entries_count C structure member in
     * @c backup_seq.
     * Since this is @c entries with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("backup",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         backup_seq, entries,
                         &cyaml_backup_entry_schema, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * Top-level schema of the backup. The top level value for the backup is
 * a sequence.
 *
 * Its fields are defined in cyaml_backup_entry_fields_schema.
 */
static const cyaml_schema_value_t cyaml_backup_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, backup_seq,
                        cyaml_backup_fields_schema),
};

/**
 * The schema of object. Its value is a mapping.
 *
 * Its fields are defined in cyaml_object_fields_schema.
 */
static const cyaml_schema_value_t cyaml_object_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, object_type,
                        cyaml_object_fields_schema),
};

/**
 * The schema of instance. Its value is a mapping.
 *
 * Its fields are defined in cyaml_instance_fields_schema.
 */
static const cyaml_schema_value_t cyaml_instance_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, instance_type,
                        cyaml_instance_fields_schema),
};

/**
 * The history_entry mapping's field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, @c history_entry.
 */
static const cyaml_schema_field_t cyaml_history_entry_fields_schema[] = {
    /**
     * The first field is the register.
     *
     * YAML key: @c register.
     * C structure member for this key: @c reg.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too.  In this case, it's cyaml_object_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored.  In this case, it
     * goes in the @c reg_count C structure member in
     * @c history_entry.
     * Since this is @c reg with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("register",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_entry, reg,
                         &cyaml_object_schema, 0, CYAML_UNLIMITED),

    /**
     * The next field is the unregister.
     *
     * YAML key: @c unregister.
     * C structure member for this key: @c unreg.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too.  In this case, it's cyaml_object_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored.  In this case, it
     * goes in the @c unreg_count C structure member in
     * @c history_entry.
     * Since this is @c unreg with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("unregister",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_entry, unreg,
                         &cyaml_object_schema, 0, CYAML_UNLIMITED),

    /**
     * The next field is the add.
     *
     * YAML key: @c add.
     * C structure member for this key: @c add.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too. In this case, it's cyaml_instance_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored. In this case, it
     * goes in the @c add_count C structure member in
     * @c history_entry.
     * Since this is @c add with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("add",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_entry, add,
                         &cyaml_instance_schema, 0, CYAML_UNLIMITED),

    /**
     * The next field is the get.
     *
     * YAML key: @c get.
     * C structure member for this key: @c get.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too. In this case, it's cyaml_instance_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored. In this case, it
     * goes in the @c get_count C structure member in
     * @c history_entry.
     * Since this is @c get with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("get",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_entry, get,
                         &cyaml_instance_schema, 0, CYAML_UNLIMITED),

    /**
     * The next field is the delete.
     *
     * YAML key: @c delete.
     * C structure member for this key: @c delete.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too. In this case, it's cyaml_instance_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored. In this case, it
     * goes in the @c delete_count C structure member in
     * @c history_entry.
     * Since this is @c delete with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("delete",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_entry, delete,
                         &cyaml_instance_schema, 0, CYAML_UNLIMITED),

    /**
     * The next field is the copy.
     *
     * YAML key: @c copy.
     * C structure member for this key: @c copy.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too. In this case, it's cyaml_instance_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored. In this case, it
     * goes in the @c copy_count C structure member in
     * @c history_entry.
     * Since this is @c copy with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("copy",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_entry, copy,
                         &cyaml_instance_schema, 0, CYAML_UNLIMITED),

    /**
     * The next field is the set.
     *
     * YAML key: @c set.
     * C structure member for this key: @c set.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too. In this case, it's cyaml_instance_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored. In this case, it
     * goes in the @c set_count C structure member in
     * @c history_entry.
     * Since this is @c set with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("set",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_entry, set,
                         &cyaml_instance_schema, 0, CYAML_UNLIMITED),

    /**
     * If the YAML contains a field that our program is not interested in
     * we can ignore it, so the value of the field will not be loaded.
     *
     * Note that unless the OPTIONAL flag is set, the ignored field must
     * be present.
     *
     * Another way of handling this would be to use the config flag
     * to ignore unknown keys.  This config is passed to libcyaml
     * separately from the schema.
     */
    CYAML_FIELD_IGNORE("comment", CYAML_FLAG_OPTIONAL),

    /**
     * The reboot_ta in the mapping is a string.
     *
     * YAML key: @c reboot_ta.
     * C structure member for this key: @c reboot_ta.
     *
     * Its CYAML type is string pointer, and we have no minimum or maximum
     * string length constraints.
     */
    CYAML_FIELD_STRING_PTR("reboot_ta",
                           CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                           history_entry, reboot_ta, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * The value for a @c history_entry is a mapping.
 *
 * Its fields are defined in cyaml_history_entry_fields_schema.
 */
static const cyaml_schema_value_t cyaml_history_entry_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
                        history_entry,
                        cyaml_history_entry_fields_schema),
};

/**
 * The history field definitions schema is an array.
 *
 * All the field entries will refer to their parent mapping's structure,
 * in this case, @c history_seq.
 */
static const cyaml_schema_field_t cyaml_history_fields_schema[] = {
    /**
     * The only field is the register, unregister, add or something.
     *
     * YAML key: @c history.
     * C structure member for this key: @c entries.
     *
     * Its CYAML type is a sequence.
     *
     * Since it's a sequence type, the value schema for its entries must
     * be provided too. In this case, it's cyaml_history_entry_schema.
     *
     * Since it's not a sequence of a fixed-length, we must tell CYAML
     * where the sequence entry count is to be stored. In this case, it
     * goes in the @c entries_count C structure member in
     * @c history_seq.
     * Since this is @c entries with the @c _count postfix, we can use
     * the following macro, which assumes a postfix of @c _count in the
     * struct member name.
     */
    CYAML_FIELD_SEQUENCE("history",
                         CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
                         history_seq, entries,
                         &cyaml_history_entry_schema, 0, CYAML_UNLIMITED),

    CYAML_FIELD_END
};

/**
 * Top-level schema of the history. The top level value for the history is
 * a sequence.
 *
 * Its fields are defined in cyaml_history_entry_fields_schema.
 */
static const cyaml_schema_value_t cyaml_history_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
                        history_seq, cyaml_history_fields_schema),
};


/*
 ******************************************************************************
 * Actual code to load and save YAML doc using libcyaml.
 ******************************************************************************
 */
/**
 * Our CYAML config.
 *
 * If you don't want to change it between calls, make it const.
 *
 * Here we have a very basic config.
 */
cyaml_config_t cyaml_config = {
    .log_fn = cyaml_log,            /** Use the default logging function. */
    .mem_fn = cyaml_mem,            /** Use the default memory allocator. */
    .log_level = CYAML_LOG_WARNING, /** Logging errors and warnings only. */
    /*  .log_level = CYAML_LOG_DEBUG,   for logging debug. */
};

te_errno
cyaml_err2te_errno(cyaml_err_t err)
{
    switch (err)
    {
        /* Success. */
        case CYAML_OK:
            return 0;

        /* Memory allocation failed. */
        case CYAML_ERR_OOM:
            ERROR("There is a CYAML error %s(%d)", cyaml_strerror(err), err);
            TE_FATAL_ERROR("Out of memory");

        default:
        {
            ERROR("There is a CYAML error %s(%d)", cyaml_strerror(err), err);
            return TE_EINVAL;
        }
    }
}

void
cfg_yaml_free_backup_seq(backup_seq *backup)
{
    cyaml_free(&cyaml_config, &cyaml_backup_schema, backup, 0);
}

void
cfg_yaml_free_obj(object_type *obj)
{
    unsigned int i;

    free(obj->d);
    free(obj->oid);
    free(obj->def_val);
    for (i = 0; i < obj->depends_count; i++)
        free(obj->depends[i].oid);
    free(obj->depends);
}

void
cfg_yaml_free_inst(instance_type *inst)
{
    free(inst->if_cond);
    free(inst->oid);
    free(inst->value);
}

void
cfg_yaml_free_b_entry(backup_entry *entry)
{
    if (entry->object != NULL)
    {
        cfg_yaml_free_obj(entry->object);
        free(entry->object);
    }

    if (entry->instance != NULL)
    {
        cfg_yaml_free_inst(entry->instance);
        free(entry->instance);
    }
}

void
cfg_yaml_free_b_seq(backup_seq *b_seq)
{
    unsigned int i;

    for (i = 0; i < b_seq->entries_count; i++)
        cfg_yaml_free_b_entry(&b_seq->entries[i]);
    free(b_seq->entries);
    b_seq->entries = NULL;
    b_seq->entries_count = 0;
}

void
cfg_yaml_free_cond_entry(cond_entry *cond)
{
    free(cond->if_cond);
    if (cond->then_cond != NULL)
    {
        cfg_yaml_free_hist_seq(cond->then_cond);
        free(cond->then_cond);
    }
    if (cond->else_cond != NULL)
    {
        cfg_yaml_free_hist_seq(cond->else_cond);
        free(cond->else_cond);
    }
}

void
cfg_yaml_free_hist_entry(history_entry *entry)
{
    unsigned int i;

    free(entry->comment);

    if (entry->cond != NULL)
    {
        cfg_yaml_free_cond_entry(entry->cond);
        free(entry->cond);
    }

    free(entry->reboot_ta);

#define FREE_SEQUENCE(cmd_, free_func_) \
    do {                                               \
        for (i = 0; i < entry->cmd_ ## _count; i++)    \
            free_func_(&entry->cmd_[i]);               \
        free(entry->cmd_);                             \
        entry->cmd_ = NULL;                            \
        entry->cmd_ ## _count = 0;                     \
    }                                                  \
    while (0)

    FREE_SEQUENCE(incl, free);
    FREE_SEQUENCE(reg, cfg_yaml_free_obj);
    FREE_SEQUENCE(unreg, cfg_yaml_free_obj);
    FREE_SEQUENCE(add, cfg_yaml_free_inst);
    FREE_SEQUENCE(get, cfg_yaml_free_inst);
    FREE_SEQUENCE(delete, cfg_yaml_free_inst);
    FREE_SEQUENCE(copy, cfg_yaml_free_inst);
    FREE_SEQUENCE(set, cfg_yaml_free_inst);

#undef FREE_SEQUENCE
}

void
cfg_yaml_free_hist_seq(history_seq *hist_seq)
{
    unsigned int i;

    for (i = 0; i < hist_seq->entries_count; i++)
        cfg_yaml_free_hist_entry(&hist_seq->entries[i]);
    free(hist_seq->entries);
    hist_seq->entries = NULL;
    hist_seq->entries_count = 0;
}

te_errno
cfg_yaml_save_backup_file(const char *filename, backup_seq *backup)
{
    cyaml_err_t err;

    err = cyaml_save_file(filename, &cyaml_config,
                          &cyaml_backup_schema, backup, 0);
    return cyaml_err2te_errno(err);
}

te_errno
cfg_yaml_parse_backup_file(const char *filename, backup_seq **backup)
{
    cyaml_err_t err;

    err = cyaml_load_file(filename, &cyaml_config,
                          &cyaml_history_schema, (cyaml_data_t **) backup, NULL);
    return cyaml_err2te_errno(err);
}

te_errno
cfg_yaml_save_history_file(const char *filename, history_seq *history)
{
    cyaml_err_t err;

    err = cyaml_save_file(filename, &cyaml_config,
                          &cyaml_history_schema, history, 0);
    return cyaml_err2te_errno(err);
}
