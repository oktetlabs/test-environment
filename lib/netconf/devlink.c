/** @file
 * @brief Device management using netconf library
 *
 * Implementation of devlink device configuration commands.
 *
 * Copyright (C) 2022-2022 OKTET Labs.
 */

#define TE_LGR_USER "Netconf devlink"

#include "config.h"
#include "te_config.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "netconf_internal_genetlink.h"
#include "logger_api.h"
#include "te_alloc.h"

#if defined(HAVE_LINUX_GENETLINK_H) && defined(HAVE_LINUX_DEVLINK_H)

#include "linux/genetlink.h"
#include "linux/devlink.h"

/*
 * Devlink family ID. It can be different on different hosts and
 * should be determined with CTRL_CMD_GETFAMILY request.
 */
static int devlink_family = -1;

/*
 * Check whether devlink family ID is already known; obtain it
 * if it is not.
 */
#define GET_CHECK_DEVLINK_FAMILY(_nh) \
    do {                                                          \
        te_errno _rc;                                             \
        uint16_t _family;                                         \
                                                                  \
        if (devlink_family < 0)                                   \
        {                                                         \
            _rc = netconf_gn_get_family(_nh, DEVLINK_GENL_NAME,   \
                                        &_family);                \
            if (_rc != 0)                                         \
                return _rc;                                       \
                                                                  \
            devlink_family = _family;                             \
        }                                                         \
    } while (0)

#if HAVE_DECL_DEVLINK_CMD_INFO_GET

/* Process attributes of DEVLINK_CMD_INFO_GET message */
static te_errno
info_attr_cb(struct nlattr *na, void *cb_data)
{
    netconf_devlink_info *info = cb_data;
    char **attr_data = NULL;

    switch (na->nla_type)
    {
        case DEVLINK_ATTR_BUS_NAME:
            attr_data = &info->bus_name;
            break;

        case DEVLINK_ATTR_DEV_NAME:
            attr_data = &info->dev_name;
            break;

        case DEVLINK_ATTR_INFO_DRIVER_NAME:
            attr_data = &info->driver_name;
            break;

        case DEVLINK_ATTR_INFO_SERIAL_NUMBER:
            attr_data = &info->serial_number;
            break;
    }

    if (attr_data != NULL)
        return netconf_get_str_attr(na, attr_data);

    return 0;
}

/* Process device information message received from netlink */
static int
info_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    netconf_devlink_info *info;
    te_errno rc = 0;

    UNUSED(cookie);

    if (netconf_list_extend(list, NETCONF_NODE_DEVLINK_INFO) != 0)
        return -1;

    info = &(list->tail->data.devlink_info);

    rc = netconf_gn_process_attrs(h, info_attr_cb, info);
    if (rc != 0)
        return -1;

    return 0;
}

/* See description in netconf.h */
te_errno
netconf_devlink_get_info(netconf_handle nh, const char *bus,
                         const char *dev, netconf_list **list)
{
    int os_rc;
    te_errno rc = 0;
    char req[NETCONF_MAX_REQ_LEN] = { 0, };
    struct nlmsghdr *h;
    uint16_t req_flags = NLM_F_REQUEST;
    netconf_list *list_ptr = NULL;

    if (bus == NULL || dev == NULL)
    {
        if (dev != bus)
        {
            ERROR("%s(): either specify both bus and dev or none of them",
                  __FUNCTION__);
            return TE_EINVAL;
        }

        req_flags |= NLM_F_DUMP;
    }

    GET_CHECK_DEVLINK_FAMILY(nh);

    h = (struct nlmsghdr *)req;

    rc = netconf_gn_init_hdrs(req, sizeof(req), devlink_family, req_flags,
                              DEVLINK_CMD_INFO_GET, DEVLINK_GENL_VERSION,
                              nh);
    if (rc != 0)
        return rc;

    if (bus != NULL)
    {
        rc = netconf_append_attr(req, sizeof(req), DEVLINK_ATTR_BUS_NAME,
                                 bus, strlen(bus) + 1);
        if (rc != 0)
            return rc;

        rc = netconf_append_attr(req, sizeof(req), DEVLINK_ATTR_DEV_NAME,
                                 dev, strlen(dev) + 1);
        if (rc != 0)
            return rc;
    }

    list_ptr = TE_ALLOC(sizeof(*list_ptr));
    if (list_ptr == NULL)
        return TE_ENOMEM;

    os_rc = netconf_talk(nh, req, h->nlmsg_len, info_cb, NULL, list_ptr);
    if (os_rc != 0)
    {
        rc = te_rc_os2te(errno);
        netconf_list_free(list_ptr);
    }
    else
    {
        *list = list_ptr;
    }

    return rc;
}

#endif /* HAVE_DECL_DEVLINK_CMD_INFO_GET */

#if HAVE_DECL_DEVLINK_CMD_PARAM_GET

/*
 * Auxiliary structure for callbacks processing device parameters data
 */
typedef struct param_cb_data {
    netconf_devlink_param *param;     /* Parameter structure to fill */

    struct nlattr *param_attr;        /* Pointer to PARAM attribute */
    struct nlattr *values_list_attr;  /* Pointer to PARAM_VALUES_LIST
                                         attribute */

    struct nlattr *last_val_data;     /* Pointer to the last encountered
                                         PARAM_VALUE_DATA attribute */
    struct nlattr *last_val_cmode;    /* Pointer to the last encountered
                                         PARAM_VALUE_CMODE attribute */
} param_cb_data;

/* Process nested attributes in PARAM_VALUE attribute */
static te_errno
param_value_attr_cb(struct nlattr *na, void *data)
{
    param_cb_data *cb_data = data;

    switch (na->nla_type)
    {
        case DEVLINK_ATTR_PARAM_VALUE_DATA:
            cb_data->last_val_data = na;
            break;

        case DEVLINK_ATTR_PARAM_VALUE_CMODE:
            cb_data->last_val_cmode = na;
            break;
    }

    return 0;
}

/* Process PARAM_VALUE_DATA attribute */
static te_errno
get_param_value_data(netconf_devlink_param *param,
                     netconf_devlink_param_value *value,
                     struct nlattr *na_data)
{
    te_errno rc = 0;

    if (param->type != NETCONF_NLA_FLAG && na_data == NULL)
    {
        ERROR("%s(): PARAM_VALUE_DATA attribute cannot be missing "
              "for parameter %s of type %d", __FUNCTION__,
              param->name, param->type);
        return TE_EINVAL;
    }

    switch (param->type)
    {
        case NETCONF_NLA_FLAG:
            if (na_data != NULL)
                value->data.flag = TRUE;
            break;

        case NETCONF_NLA_U8:
            rc = netconf_get_uint8_attr(na_data, &value->data.u8);
            break;

        case NETCONF_NLA_U16:
            rc = netconf_get_uint16_attr(na_data, &value->data.u16);
            break;

        case NETCONF_NLA_U32:
            rc = netconf_get_uint32_attr(na_data, &value->data.u32);
            break;

        case NETCONF_NLA_U64:
            rc = netconf_get_uint64_attr(na_data, &value->data.u64);
            break;

        case NETCONF_NLA_STRING:
            rc = netconf_get_str_attr(na_data, &value->data.str);
            break;

        default:
            WARN("%s(): not supported type %d of parameter %s",
                 __FUNCTION__, param->type, param->name);
            return 0;
    }

    if (rc != 0)
        return rc;

    value->defined = TRUE;
    return 0;
}

/* Convert native configuration mode to netconf constant */
static netconf_devlink_param_cmode
devlink_param_cmode_h2netconf(uint8_t val)
{
#define CHECK_CMODE(_name) \
    case DEVLINK_PARAM_CMODE_ ## _name:                 \
        return NETCONF_DEVLINK_PARAM_CMODE_ ## _name

    switch (val)
    {
        CHECK_CMODE(RUNTIME);
        CHECK_CMODE(DRIVERINIT);
        CHECK_CMODE(PERMANENT);

        default:
            return NETCONF_DEVLINK_PARAM_CMODE_UNDEF;
    }
#undef CHECK_CMODE
}

/* Process PARAM_VALUE attribute */
static te_errno
param_value_cb(struct nlattr *na, void *data)
{
    te_errno rc;
    param_cb_data *cb_data = data;
    netconf_devlink_param_value *value;
    uint8_t cmode;
    netconf_devlink_param_cmode te_cmode;

    if (na->nla_type != DEVLINK_ATTR_PARAM_VALUE)
    {
        ERROR("%s(): nla_type %d is not expected", __FUNCTION__,
              (int)(na->nla_type));
        return TE_EINVAL;
    }

    cb_data->last_val_data = NULL;
    cb_data->last_val_cmode = NULL;
    rc = netconf_process_nested_attrs(na, param_value_attr_cb, data);
    if (rc != 0)
        return rc;

    if (cb_data->last_val_cmode == NULL)
    {
        ERROR("%s(): PARAM_VALUE_CMODE attribute was not found",
              __FUNCTION__);
        return TE_EINVAL;
    }

    /* last_val_data can be NULL if it is parameter of type flag */

    rc = netconf_get_uint8_attr(cb_data->last_val_cmode, &cmode);
    if (rc != 0)
        return rc;

    te_cmode = devlink_param_cmode_h2netconf(cmode);
    if (te_cmode == NETCONF_DEVLINK_PARAM_CMODE_UNDEF)
    {
        /* Skip unsupported cmode */
        return 0;
    }

    value = &cb_data->param->values[te_cmode];

    return get_param_value_data(cb_data->param, value,
                                cb_data->last_val_data);
}

/* Process nested attributes inside PARAM attribute */
static te_errno
param_nested_attr_cb(struct nlattr *na, void *data)
{
    param_cb_data *cb_data = data;
    netconf_devlink_param *param = cb_data->param;
    te_errno rc;
    uint8_t val;

    switch (na->nla_type)
    {
        case DEVLINK_ATTR_PARAM_NAME:
            return netconf_get_str_attr(na, &param->name);

        case DEVLINK_ATTR_PARAM_GENERIC:
            param->generic = TRUE;
            break;

        case DEVLINK_ATTR_PARAM_TYPE:
            rc = netconf_get_uint8_attr(na, &val);
            if (rc != 0)
                return rc;
            param->type = val;
            break;

        case DEVLINK_ATTR_PARAM_VALUES_LIST:
            cb_data->values_list_attr = na;
            break;
    }

    return 0;
}

/* Process attributes of DEVLINK_ATTR_PARAM message */
static te_errno
param_attr_cb(struct nlattr *na, void *data)
{
    param_cb_data *cb_data = data;

    switch (na->nla_type)
    {
        case DEVLINK_ATTR_BUS_NAME:
            return netconf_get_str_attr(na, &cb_data->param->bus_name);

        case DEVLINK_ATTR_DEV_NAME:
            return netconf_get_str_attr(na, &cb_data->param->dev_name);

        case DEVLINK_ATTR_PARAM:
            cb_data->param_attr = na;
            break;
    }

    return 0;
}

/* Process DEVLINK_ATTR_PARAM message */
static te_errno
param_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    netconf_devlink_param *param;
    te_errno rc = 0;

    param_cb_data cb_data;

    UNUSED(cookie);

    if (netconf_list_extend(list, NETCONF_NODE_DEVLINK_PARAM) != 0)
        return -1;

    param = &(list->tail->data.devlink_param);

    memset(&cb_data, 0, sizeof(cb_data));
    cb_data.param = param;
    param->type = NETCONF_NLA_UNSPEC;

    rc = netconf_gn_process_attrs(h, param_attr_cb, &cb_data);
    if (rc != 0)
        return rc;

    if (param->bus_name == NULL)
    {
        ERROR("%s(): bus name is missed", __FUNCTION__);
        return TE_EINVAL;
    }

    if (param->dev_name == NULL)
    {
        ERROR("%s(): device name is missed", __FUNCTION__);
        return TE_EINVAL;
    }

    if (cb_data.param_attr == NULL)
    {
        ERROR("%s(): PARAM attribute is missing", __FUNCTION__);
        return TE_EINVAL;
    }

    rc = netconf_process_nested_attrs(cb_data.param_attr,
                                      param_nested_attr_cb,
                                      &cb_data);
    if (rc != 0)
        return rc;

    if (param->name == NULL)
    {
        ERROR("%s(): parameter name is missing", __FUNCTION__);
        return TE_EINVAL;
    }

    if (param->type == NETCONF_NLA_UNSPEC)
    {
        ERROR("%s(): type for parameter %s is missing",
              __FUNCTION__, param->name);
        return TE_EINVAL;
    }

    if (cb_data.values_list_attr == NULL)
    {
        /* Assume there are no values */
        return 0;
    }

    return netconf_process_nested_attrs(cb_data.values_list_attr,
                                        param_value_cb,
                                        &cb_data);
}

/* See description in netconf.h */
te_errno
netconf_devlink_param_dump(netconf_handle nh, netconf_list **list)
{
    int os_rc;
    te_errno rc = 0;
    char req[NETCONF_MAX_REQ_LEN] = { 0, };
    struct nlmsghdr *h;
    netconf_list *list_ptr = NULL;

    GET_CHECK_DEVLINK_FAMILY(nh);

    h = (struct nlmsghdr *)req;

    rc = netconf_gn_init_hdrs(req, sizeof(req), devlink_family,
                              NLM_F_REQUEST | NLM_F_DUMP,
                              DEVLINK_CMD_PARAM_GET, DEVLINK_GENL_VERSION,
                              nh);
    if (rc != 0)
        return rc;

    list_ptr = TE_ALLOC(sizeof(*list_ptr));
    if (list_ptr == NULL)
        return TE_ENOMEM;

    os_rc = netconf_talk(nh, req, h->nlmsg_len, param_cb, NULL, list_ptr);
    if (os_rc != 0)
    {
        rc = te_rc_os2te(errno);
        netconf_list_free(list_ptr);
    }
    else
    {
        *list = list_ptr;
    }

    return rc;
}

#endif /* HAVE_DECL_DEVLINK_CMD_PARAM_GET */

#if HAVE_DECL_DEVLINK_CMD_PARAM_SET

/* Convert netconf configuration mode to native value */
static te_errno
devlink_param_cmode_netconf2h(netconf_devlink_param_cmode cmode,
                              uint8_t *val)
{
#define CHECK_CMODE(_name) \
    case NETCONF_DEVLINK_PARAM_CMODE_ ## _name:     \
        *val = DEVLINK_PARAM_CMODE_ ## _name;       \
        break

    switch (cmode)
    {
        CHECK_CMODE(RUNTIME);
        CHECK_CMODE(DRIVERINIT);
        CHECK_CMODE(PERMANENT);

        default:
            return TE_ENOENT;
    }

    return 0;
#undef CHECK_CMODE
}

/* See description in netconf.h */
te_errno
netconf_devlink_param_set(netconf_handle nh, const char *bus,
                          const char *dev, const char *param_name,
                          netconf_nla_type nla_type,
                          netconf_devlink_param_cmode cmode,
                          const netconf_devlink_param_value_data *value)
{
    int os_rc;
    te_errno rc = 0;
    char req[NETCONF_MAX_REQ_LEN] = { 0, };
    struct nlmsghdr *h;
    uint8_t native_cmode;
    uint8_t native_type;

    size_t value_len;
    const void *value_ptr;
    te_bool add_value;

    GET_CHECK_DEVLINK_FAMILY(nh);

    h = (struct nlmsghdr *)req;

    rc = devlink_param_cmode_netconf2h(cmode, &native_cmode);
    if (rc != 0)
        return rc;

    rc = netconf_gn_init_hdrs(req, sizeof(req), devlink_family,
                              NLM_F_REQUEST | NLM_F_ACK,
                              DEVLINK_CMD_PARAM_SET, DEVLINK_GENL_VERSION,
                              nh);
    if (rc != 0)
        return rc;

    rc = netconf_append_attr(req, sizeof(req), DEVLINK_ATTR_BUS_NAME,
                             bus, strlen(bus) + 1);
    if (rc != 0)
        return rc;

    rc = netconf_append_attr(req, sizeof(req), DEVLINK_ATTR_DEV_NAME,
                             dev, strlen(dev) + 1);
    if (rc != 0)
        return rc;

    rc = netconf_append_attr(req, sizeof(req), DEVLINK_ATTR_PARAM_NAME,
                             param_name, strlen(param_name) + 1);
    if (rc != 0)
        return rc;

    rc = netconf_append_attr(req, sizeof(req),
                             DEVLINK_ATTR_PARAM_VALUE_CMODE,
                             &native_cmode, sizeof(native_cmode));
    if (rc != 0)
        return rc;

    native_type = nla_type;
    rc = netconf_append_attr(req, sizeof(req),
                             DEVLINK_ATTR_PARAM_TYPE,
                             &native_type, sizeof(native_type));
    if (rc != 0)
        return rc;

    value_ptr = value;
    add_value = TRUE;
    switch (nla_type)
    {
        case NETCONF_NLA_U8:
            value_len = 1;
            break;

        case NETCONF_NLA_U16:
            value_len = 2;
            break;

        case NETCONF_NLA_U32:
            value_len = 4;
            break;

        case NETCONF_NLA_U64:
            value_len = 8;
            break;

        case NETCONF_NLA_STRING:
            value_len = strlen(value->str) + 1;
            value_ptr = value->str;
            break;

        case NETCONF_NLA_FLAG:
            value_len = 0;
            value_ptr = NULL;
            add_value = value->flag;
            break;

        default:
            ERROR("%s(): type %d is not supported", __FUNCTION__, nla_type);
            return TE_EINVAL;
    }

    if (add_value)
    {
        rc = netconf_append_attr(req, sizeof(req),
                                 DEVLINK_ATTR_PARAM_VALUE_DATA,
                                 value_ptr, value_len);
        if (rc != 0)
            return rc;
    }

    os_rc = netconf_talk(nh, req, h->nlmsg_len, NULL, NULL, NULL);
    if (os_rc != 0)
        rc = te_rc_os2te(errno);

    return rc;
}

#endif /* HAVE_DECL_DEVLINK_CMD_PARAM_SET */

#else /* defined(HAVE_LINUX_GENETLINK_H) && defined(HAVE_LINUX_DEVLINK_H) */

#define NO_DEVLINK_IMPL 1

#endif

#if defined(NO_DEVLINK_IMPL) || !HAVE_DECL_DEVLINK_CMD_INFO_GET
/* See description in netconf.h */
te_errno
netconf_devlink_get_info(netconf_handle nh, const char *bus,
                         const char *dev, netconf_list **list)
{
    UNUSED(nh);
    UNUSED(bus);
    UNUSED(dev);
    UNUSED(list);

    return TE_ENOENT;
}
#endif

#if defined(NO_DEVLINK_IMPL) || !HAVE_DECL_DEVLINK_CMD_PARAM_GET
/* See description in netconf.h */
te_errno
netconf_devlink_param_dump(netconf_handle nh, netconf_list **list)
{
    UNUSED(nh);
    UNUSED(list);

    return TE_ENOENT;
}
#endif

#if defined(NO_DEVLINK_IMPL) || !HAVE_DECL_DEVLINK_CMD_PARAM_SET
/* See description in netconf.h */
te_errno
netconf_devlink_param_set(netconf_handle nh, const char *bus,
                          const char *dev, const char *param_name,
                          netconf_nla_type nla_type,
                          netconf_devlink_param_cmode cmode,
                          const netconf_devlink_param_value_data *value)
{
    UNUSED(nh);
    UNUSED(bus);
    UNUSED(dev);
    UNUSED(param_name);
    UNUSED(nla_type);
    UNUSED(cmode);
    UNUSED(value);

    return TE_ENOENT;
}
#endif

/* See description in netconf.h */
void
netconf_devlink_info_node_free(netconf_node *node)
{
    if (node == NULL)
        return;

    free(node->data.devlink_info.bus_name);
    free(node->data.devlink_info.dev_name);
    free(node->data.devlink_info.driver_name);
    free(node->data.devlink_info.serial_number);
    free(node);
}

/* Release memory allocated for parameter value */
static void
netconf_devlink_param_value_free(netconf_nla_type type,
                                 netconf_devlink_param_value *value)
{
    if (type == NETCONF_NLA_STRING && value->defined)
    {
        free(value->data.str);
        value->data.str = NULL;
    }
}

/* See description in netconf.h */
void
netconf_devlink_param_node_free(netconf_node *node)
{
    netconf_devlink_param *param;
    int i;

    if (node == NULL)
        return;

    param = &node->data.devlink_param;

    free(param->bus_name);
    free(param->dev_name);
    free(param->name);

    for (i = 0; i < NETCONF_DEVLINK_PARAM_CMODES; i++)
    {
        netconf_devlink_param_value_free(param->type,
                                         &param->values[i]);
    }

    free(node);
}

/* See description in netconf.h */
const char *
devlink_param_cmode_netconf2str(netconf_devlink_param_cmode cmode)
{
    switch (cmode)
    {
        case NETCONF_DEVLINK_PARAM_CMODE_RUNTIME:
            return "runtime";

        case NETCONF_DEVLINK_PARAM_CMODE_DRIVERINIT:
            return "driverinit";

        case NETCONF_DEVLINK_PARAM_CMODE_PERMANENT:
            return "permanent";

        default:
            return "<unknown>";
    }
}

/* See description in netconf.h */
netconf_devlink_param_cmode
devlink_param_cmode_str2netconf(const char *cmode)
{
    if (strcmp(cmode, "runtime") == 0)
        return NETCONF_DEVLINK_PARAM_CMODE_RUNTIME;
    else if (strcmp(cmode, "driverinit") == 0)
        return NETCONF_DEVLINK_PARAM_CMODE_DRIVERINIT;
    else if (strcmp(cmode, "permanent") == 0)
        return NETCONF_DEVLINK_PARAM_CMODE_PERMANENT;

    return NETCONF_DEVLINK_PARAM_CMODE_UNDEF;
}

/* See description in netconf.h */
void
netconf_devlink_param_value_data_mv(
                        netconf_nla_type nla_type,
                        netconf_devlink_param_value_data *dst,
                        netconf_devlink_param_value_data *src)
{
    if (nla_type == NETCONF_NLA_STRING)
    {
        free(dst->str);
        dst->str = NULL;
    }

    memcpy(dst, src, sizeof(*src));
    memset(src, 0, sizeof(*src));
}
