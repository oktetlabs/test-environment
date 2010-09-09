#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>

const char *usage = "usage:  power_sw mask mode   (mask is 0xF, mode is on/off/rst)";

int main(int argc, char **argv)
{
    unsigned int   mask;
    int            fd;
    unsigned char  mode = 0;

    if (argc != 3)
    {
        printf("%s\n", usage);
        return -1;
    }

    if (sscanf(argv[1], "%x", &mask) != 1)
    {
        printf("%s - mask", usage);
        return -1;
    }

    if ((mask < 0x1) || (mask > 0xf))
    {
        printf("mask: 0x1-port1, 0x2-port2, 0x4-port3, 0x8-port4\n");
        return -1;
    }

    fd = open("/dev/parport0", O_RDWR);
    if (fd < 0)
        perror("open() failed");

    if (ioctl(fd, PPCLAIM) < 0)
       perror("ioctl(PPCLAIM) failed");

    /* get current per port mode */
    if (ioctl(fd, PPRDATA, &mode) < 0)
        perror("ioctl(PPRDATA) failed - mode 'off'");
#ifdef DBG
    printf("\n ...  current per port mode=%x\n", mode);
#endif

    /* set per port on/off/rst mode */
    if (!strcmp(argv[2], "off"))
    {
        mode &= ~mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            perror("ioctl(PPWDATA) failed - mode 'off'");
    }
    else if (!strcmp(argv[2], "on"))
    {
        mode |= mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            perror("ioctl(PPWDATA) failed - mode 'on'");
    }
    else if (!strcmp(argv[2], "rst"))
    {
        mode &= ~mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            perror("ioctl(PPWDATA) failed - mode 'rst-off'");
        sleep(2);
        mode |= mask;
        if (ioctl(fd, PPWDATA, &mode) < 0)
            perror("ioctl(PPWDATA) failed - mode 'rst-on'");
    }
    else
    {
        printf("%s - mode\n", usage);
        return -1;
    }

    if (ioctl(fd, PPRELEASE) < 0)
        perror("ioctl(PPRELEASE) failed");

    close(fd);

    return 0;
}
