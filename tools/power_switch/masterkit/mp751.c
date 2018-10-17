/** @file
 * @brief A program for masterkit mp751 power switches handling
 *
 * Derived from the sowtware supplied with the device, but rewritten almost
 * from scratch. Added device numbering, time parametrization, fixed bugs.
 * Source software and documents can be found in the Storage.
 *
 * Usage: mp751 [dev] -on|-off|-ton|-toff [time]
 *     -on   - turn on
 *     -off  - turn off
 *     -ton  - turn off for \"time\" seconds and then on
 *     -toff - turn on for \"time\" seconds and then off
 *     dev   - the number of device to switch (default first)
 *     time  - the time in seconds, default 2
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

#define SWITCH_PERIOD 2

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
mp751_try_and_test(unsigned char code, unsigned char value)
{
    buf[0] = CMD_TRY;
    buf[1] = code;
    buf[2] = value;

    if (!mp751_read() ||
        buf[0] != CMD_TRY || buf[1] != code || buf[2] != value)
        return false;

    buf[0] = CMD_TEST;
    buf[1] = code;
    return mp751_write() &&
           buf[0] == CMD_TEST && buf[1] == code && buf[2] == value;
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

static bool
mp751_timer(unsigned int seconds)
{
    buf[0] = CMD_TIMER;
    buf[1] = seconds & 0xff;
    buf[2] = (seconds >> 8) & 0xff;
    buf[3] = (seconds >> 16) & 0xff;

    return mp751_read();
}

static bool
switch_temporary(bool on, unsigned int tm)
{
    return mp751_set(on ? REL_ON : REL_OFF) &&
           mp751_try_and_test(4, on ? 0xff : 0) &&    /* set relay state */
           mp751_try_and_test(6, 0) &&                /* set timer to 0 */
           mp751_try_and_test(7, 0) &&                /* set timer to 0 */
           mp751_try_and_test(8, 0) &&                /* set timer to 0 */
           mp751_timer(tm);                           /* switch time */
}

int
main(int argc, char* argv[])
{
    struct hid_device_info *devs = hid_enumerate(VENDOR_ID, PRODUCT_ID);
    struct hid_device_info *cur_dev;
    unsigned short          signature;
    unsigned short          version;
    unsigned char           current_state;
    int                     device_number = 1;
    int                     num;
    int                     op_num        = 1; /* the operation arg number */

    if (argc < 2 ||
        strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        printf("Usage: %s [dev] -on|-off|-ton|-toff [time]\n"
               "    -on   - turn on\n"
               "    -off  - turn off\n"
               "    -ton  - turn off for \"time\" seconds and then on\n"
               "    -toff - turn on for \"time\" seconds and then off\n"
               "    dev   - the number of device to switch (default first)\n"
               "    time  - the time in seconds, default %d\n",
               argv[0], SWITCH_PERIOD);
        return 0;
    }

    if (devs == NULL)
    {
        fprintf(stderr, "Failed to find mp751 device\n");
        return 1;
    }

    if (argc > 2)
    {
        char *end;

        num = (int)strtol(argv[1], &end, 10);
        if (end != NULL && *end == '\0')
        {
            device_number = num;
            op_num = 2;
        }
    }

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

    if (strcmp(argv[op_num], "-on") == 0)
    {
        if (current_state == REL_ON)
            fprintf(stderr, "Relay is already ON\n");
        else if (!mp751_set(REL_ON))
            goto failure;
    }
    else if (strcmp(argv[op_num], "-off") == 0)
    {
        if (current_state == REL_OFF)
            fprintf(stderr, "Relay is already OFF\n");
        else if (!mp751_set(REL_OFF))
            goto failure;
    }
    else if (strcmp(argv[op_num], "-ton") == 0 || /* temporary off */
             strcmp(argv[op_num], "-toff") == 0)  /* temporary on */
    {
        int   tm;
        char *end = NULL;

        if (argc > op_num + 1)
            tm = strtol(argv[op_num + 1], &end, 10);

        if (end == NULL || *end != '\0' || tm <= 0)
            tm = SWITCH_PERIOD;

        if (!switch_temporary(strcmp(argv[op_num], "-toff") == 0, tm))
            goto failure;
    }
    else
    {
        fprintf(stderr, "Unknown command %s\n", argv[op_num]);
        return 1;
    }

    return 0;

failure:
    fprintf(stderr, "Device error\n");
    return 1;
}
