/** @file
 * @brief Test API to DLNA UPnP routines
 *
 * Common definitions of DLNA UPnP API for TEN and TA.
 *
 *
 * Copyright (C) 2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_UPNP_H__
#define __TE_UPNP_H__

/** UPnP debug level. Than higher the more verbal. */
#define UPNP_DEBUG 3
#if UPNP_DEBUG
# warning "UPNP_DEBUG is defined"
/*
 * Set log level to full verbose, i.e. place to log all messages: ERROR,
 * WARN, RING, INFO, VERB.
 */
# define TE_LOG_LEVEL    0x003f
#endif /* UPNP_DEBUG */


#if 0
/** Request Type in string representation. */
/** Request for device. */
#define UPNP_CP_REQUEST_FOR_DEVICE  "device"
/** Request for service. */
#define UPNP_CP_REQUEST_FOR_SERVICE "service"
/** Request for action. */
#define UPNP_CP_REQUEST_FOR_ACTION  "action"
#endif

/**
 * Request Type.
 * It is used in requests for UPnP Control Point from TEN over RPC.
 */
typedef enum upnp_cp_request_type {
    UPNP_CP_REQUEST_DEVICE,   /**< Request for UPnP devices. */
    UPNP_CP_REQUEST_SERVICE,  /**< Request for UPnP services. */
    UPNP_CP_REQUEST_ACTION    /**< Request to initiate particular action on
                                   the certain UPnP device. */
} te_upnp_cp_request_type;

/** Direction of a service state variable. */
typedef enum {
    UPNP_ARG_DIRECTION_IN,    /**< "in" variable, to the service. */
    UPNP_ARG_DIRECTION_OUT,   /**< "out" variable, from the service. */
} te_upnp_arg_direction;

/** Device properties list. */
typedef enum {
    DPROPERTY_UDN,
    DPROPERTY_TYPE,
    DPROPERTY_LOCATION,
    DPROPERTY_FRIENDLY_NAME,
    DPROPERTY_MANUFACTURER,
    DPROPERTY_MANUFACTURER_URL,
    DPROPERTY_MODEL_DESCRIPTION,
    DPROPERTY_MODEL_NAME,
    DPROPERTY_MODEL_NUMBER,
    DPROPERTY_MODEL_URL,
    DPROPERTY_SERIAL_NUMBER,
    DPROPERTY_UPC,
    DPROPERTY_ICON_URL,
    DPROPERTY_PRESENTATION_URL,

    DPROPERTY_MAX       /* Total number of device properties. */
} te_upnp_device_property_idx;

/** Service properties list. */
typedef enum {
    SPROPERTY_ID,
    SPROPERTY_UDN,
    SPROPERTY_TYPE,
    SPROPERTY_LOCATION,
    SPROPERTY_SCPD_URL,
    SPROPERTY_CONTROL_URL,
    SPROPERTY_EVENT_SUBSCRIPTION_URL,

    SPROPERTY_MAX       /* Total number of service properties. */
} te_upnp_service_property_idx;

/** State Variable properties list. */
typedef enum {
    VPROPERTY_NAME,
    VPROPERTY_TYPE,
    VPROPERTY_SEND_EVENTS,
    VPROPERTY_DEFAULT_VALUE,
    VPROPERTY_MINIMUM,
    VPROPERTY_MAXIMUM,
    VPROPERTY_STEP,
    VPROPERTY_ALLOWED_VALUES,

    VPROPERTY_MAX       /* Total number of state variable properties. */
} te_upnp_state_variable_property_idx;

/** Action Argument properties list. */
typedef enum {
    APROPERTY_NAME,
    APROPERTY_DIRECTION,
    APROPERTY_STATE_VARIABLE,

    APROPERTY_MAX       /* Total number of action argument properties. */
} te_upnp_action_arg_property_idx;


/* Jansson specific functions. */

/**
 * flags parameter that controls some aspects of how the data is encoded.
 */
#if UPNP_DEBUG
/* Encode with indents (better for debuging, i.e. more human readable). */
# define JSON_ENCODING_FLAGS   (JSON_INDENT(1) | JSON_PRESERVE_ORDER)
#else /* !UPNP_DEBUG */
/* Encode without indents (decrease the size). */
# define JSON_ENCODING_FLAGS   (JSON_COMPACT | JSON_PRESERVE_ORDER)
#endif /* UPNP_DEBUG */

/**
 * Create a new JSON object.
 *
 * @param _obj      JSON object to create.
 * @param _rc       Status code.
 * @param _lbl      Label to go on fails.
 *
 * @sa json_object
 */
#define JSON_OBJECT_CREATE(_obj, _rc, _lbl) \
    do {                                    \
        _obj = json_object();               \
        if (_obj == NULL)                   \
        {                                   \
            ERROR("json_object fails");     \
            _rc = TE_EFAULT;                \
            goto _lbl;                      \
        }                                   \
    } while (0)

/**
 * Set a new value to the JSON object.
 *
 * @param _obj      JSON object.
 * @param _key      Key of value (null-terminated string).
 * @param _val      JSON type value to set as JSON object value.
 * @param _rc       Status code.
 * @param _lbl      Label to go on fails.
 *
 * @sa json_object_set_new
 */
#define JSON_OBJECT_SET_NEW(_obj, _key, _val, _rc, _lbl) \
    do {                                                    \
        if (json_object_set_new(_obj, _key, _val) == -1)    \
        {                                                   \
            ERROR("json_object_set_new fails");             \
            _rc = TE_EFAIL;                                 \
            goto _lbl;                                      \
        }                                                   \
    } while (0)

/**
 * Set a new value to the JSON object.
 *
 * @param _obj      JSON object.
 * @param _key      Key of value (null-terminated string).
 * @param _val      JSON type value to set as JSON object value.
 * @param _rc       Status code.
 * @param _lbl      Label to go on fails.
 *
 * @sa json_object_set
 */
#define JSON_OBJECT_SET(_obj, _key, _val, _rc, _lbl) \
    do {                                                \
        if (json_object_set(_obj, _key, _val) == -1)    \
        {                                               \
            ERROR("json_object_set fails");             \
            _rc = TE_EFAIL;                             \
            goto _lbl;                                  \
        }                                               \
    } while (0)

/**
 * Create a new JSON array.
 *
 * @param _arr      JSON array to create.
 * @param _lbl      Label to go on fails.
 *
 * @sa json_array
 */
#define JSON_ARRAY_CREATE(_arr, _rc, _lbl) \
    do {                                    \
        _arr = json_array();                \
        if (_arr == NULL)                   \
        {                                   \
            ERROR("json_array fails");      \
            _rc = TE_EFAULT;                \
            goto _lbl;                      \
        }                                   \
    } while (0)

/**
 * Append a new value to the JSON array.
 *
 * @param _arr      JSON array.
 * @param _val      JSON type value to set as JSON array value.
 * @param _rc       Status code.
 * @param _lbl      Label to go on fails.
 *
 * @sa json_array_append_new
 */
#define JSON_ARRAY_APPEND_NEW(_arr, _val, _rc, _lbl) \
    do {                                                \
        if (json_array_append_new(_arr, _val) == -1)    \
        {                                               \
            ERROR("json_array_append_new fails");       \
            _rc = TE_EFAIL;                             \
            goto _lbl;                                  \
        }                                               \
    } while (0)

/**
 * Append a new value to the JSON array.
 *
 * @param _arr      JSON array.
 * @param _val      JSON type value to set as JSON array value.
 * @param _rc       Status code.
 * @param _lbl      Label to go on fails.
 *
 * @sa json_array_append
 */
#define JSON_ARRAY_APPEND(_arr, _val, _rc, _lbl) \
    do {                                            \
        if (json_array_append(_arr, _val) == -1)    \
        {                                           \
            ERROR("json_array_append fails");       \
            _rc = TE_EFAIL;                         \
            goto _lbl;                              \
        }                                           \
    } while (0)

#endif /* __TE_UPNP_H__ */
