/** @file
 * @brief A program for masterkit mp751 power switches handling for DSL lines
 *
 * Derived from the sowtware supplied with the device, but rewritten almost
 * from scratch. Added device numbering, time parametrization, fixed bugs.
 * Source software and documents can be found in the Storage.
 *
 * Usage: sw_dsl [dev] [A|V|?]
 *  A or V - to turn on ADSL or VDSL
 *  num    - the number of the device in the usb devices order, starting from 1
 *  none of A or V flips the mode
 *  ?      - only prints the current mode
 *
 * Requires libusb-1.0-0-dev to be installed
 *
 * Copyright (C) 2015 OKTET Labs Ltd.
 *
 * @author Piotr Lavrov <Piotr.Lavrov@oktetlabs.ru>
 *
 * $Id: $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <grp.h>
#include "hidapi.h"

#define VENDOR_ID   0x16c0
#define PRODUCT_ID  0x05df

enum mp751_relay {
    REL_OFF      = 0x19,
    REL_ON       = 0x00,
};

enum mp751_cmd {
    CMD_SET      = 0xe7,
    CMD_GET      = 0x7e,
    CMD_TIMER    = 0x5a,
    CMD_IDENTITY = 0x1d,
    CMD_TRY      = 0x0e,
    CMD_TEST     = 0xe0,
};

static hid_device    *handle;  /* The handle for device access */
static unsigned char  buf[10]; /* The buffer for device i/o operations */

static bool
mp751_read(void)
{
    return hid_send_feature_report(handle, buf, 8) == 8 &&
           hid_get_feature_report(handle, buf, 8) == 8;
}

static bool
mp751_write(void)
{
    return hid_send_feature_report(handle, buf, 8) == 8;
}

static bool
mp751_set(unsigned char value)
{
    buf[0] = CMD_SET;
    buf[1] = value;

    return mp751_write();
}

static bool
mp751_get(unsigned char *value)
{
    buf[0] = CMD_GET;

    if (!mp751_read())
        return false;

    *value = buf[1];
    return true;
}

static bool
mp751_identity(unsigned short *word1, unsigned short *word2)
{
    buf[0] = CMD_IDENTITY;

    if (!mp751_read())
        return false;

    *word1 = buf[1] << 8 | buf[0];
    *word2 = buf[3] << 8 | buf[2];
    return true;
}

int
main(int argc, char* argv[])
{
    struct hid_device_info *devs = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    struct hid_device_info *cur_dev;
    unsigned short          signature;
    unsigned short          version;
    unsigned char           current_state;
    int                     device_number = 0;
    int                     num;
    char                    mode = ' ';

    if (argc > 1 &&
        (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("Usage: %s [A|V] [num]\n"
               "    A or V - to turn on ADSL or VDSL\n"
               "    num    - the number of the device in the attachment order,"
               " starting from 1\n"
               "    none of A or V flips the mode\n"
               "    ?      - only prints the current mode\n", argv[0]);
        return 0;
    }

    if (devs == NULL)
    {
        fprintf(stderr, "Failed to find mp751 device\n");
        return 1;
    }

    for (num = 1; num < argc; num++)
    {
        char *c;

        for (c = argv[num]; *c != '\0'; c++)
        {
            if ((*c == 'V' || *c == 'A' || *c == '?') && mode == ' ')
            {
                mode = *c;
            }
            else if (*c == 'v')
            {
                mode = 'V';
            }
            else if (*c == 'a')
            {
                mode = 'A';
            }
            else if (*c >= '1' && *c <= '9' && device_number == 0)
            {
                device_number = *c - '0';
            }
            else
            {
                fprintf(stderr, "Wrong arguments\n");
                hid_free_enumeration(devs);
                return 0;
            }
        }
    }

    if (device_number == 0)
        device_number = 1;

    cur_dev = devs;
    for (num = 1; num < device_number; num++)
        if ((cur_dev = cur_dev->next) == NULL)
        {
            fprintf(stderr, "found only %d mp751 device(s)\n", num);
            hid_free_enumeration(devs);
            return 1;
        }
    handle = hid_open_path(cur_dev->path);
    hid_free_enumeration(devs);

    if (handle == NULL)
    {
        fprintf(stderr, "Failed to open mp751 device\n");

        if (geteuid() != 0)
        {
            fprintf(stderr, "If the switch is installed and functional "
                   "ensure that it is available for users\n"
                   "without root rights: add "
                   "/etc/udev/rules.d/90-usb-permissions.rules\n"
                   "SUBSYSTEM==\"usb\", ATTRS{idVendor}==\"%04x\", "
                   "ATTRS{idProduct}==\"%04x\", MODE=\"0666\","
                   " GROUP=\"%s\"\n",
                   VENDOR_ID, PRODUCT_ID,
                   getgrgid(getegid())->gr_name);
        }

        return 1;
    }

    if (!mp751_identity(&signature, &version) ||
        signature != 0x2c1d || version < 2 ||
        !mp751_get(&current_state))
        goto failure;

    if (mode == '?')
    {
        printf("In %cDSL\n", current_state == REL_OFF ? 'V' : 'A');
    }
    else if (mode == 'A')
    {
        if (current_state == REL_ON)
            fprintf(stderr, "Relay is already in ADSL\n");
        else if (!mp751_set(REL_ON))
            goto failure;
        else
            printf("In ADSL\n");
    }
    else if (mode == 'V')
    {
        if (current_state == REL_OFF)
            fprintf(stderr, "Relay is already in VDSL\n");
        else if (!mp751_set(REL_OFF))
            goto failure;
        else
            printf("In VDSL\n");
    }
    else
    {
        if (!mp751_set(current_state == REL_ON ? REL_OFF : REL_ON))
            goto failure;
        else
            printf("In %cDSL\n", current_state == REL_ON ? 'V' : 'A');
    }

    return 0;

failure:
    fprintf(stderr, "Device error\n");
    return 1;
}
