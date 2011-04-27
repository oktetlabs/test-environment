/** @file
 * @brief Unix Test Agent
 *
 * Unix WiFi configuring support
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Conf WiFi"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"

#if HAVE_IWLIB_H
#include <iwlib.h>
#else
#error iwlib.h is an obligatory header for WiFi support
#endif

#ifdef ENABLE_8021X
extern te_errno ds_supplicant_network_set(unsigned int gid, const char *oid,
                                          const char *value,
                                          const char *instance, ...);
#endif

#ifndef KILO
#define KILO    1e3
#endif

#if 0
/** The list of ioctls supported by the Agent */
enum ta_priv_ioctl_e {
    TA_PRIV_IOCTL_RESET = 0, /**< ioctl for card reset */
    TA_PRIV_IOCTL_AUTH_ALG = 1, /**< ioctl for authentication algorithm */
    TA_PRIV_IOCTL_PRIV_INVOKED = 2, /**< ioctl for privacy invoked
                                         attribute */
    TA_PRIV_IOCTL_EXCLUDE_UNENCR = 3, /**< ioctl for exclude unencrypted
                                           attribute */
    TA_PRIV_IOCTL_MAX = 4, /**< The number of supported ioctls */
};

/** Structure to keep information about private ioctl calls */
typedef struct ta_priv_ioctl {
    te_bool     supp; /**< Wheter this ioctl is supported by 
                           the card or not */
    const char *g_name; /**< Name of get ioctl as a character string */
    const char *s_name; /**< Name of set ioctl as a character string */
    void       *data; /**< ioctl-specific data pointer */
} ta_priv_ioctl;

/** Authentication algorithms */
typedef enum ta_auth_alg_e {
    TA_AUTH_ALG_OPEN_SYSTEM = 0, /**< OpenSystem authentication algorithm */
    TA_AUTH_ALG_SHARED_KEY = 1, /**< SharedKey authentication algorithm */
    TA_AUTH_ALG_MAX = 2, /**< The number of supported authentication 
                              algorithms */
} ta_auth_alg_e;

/** 
 * Structure for mapping authentication algorithms onto 
 * card-specific constants.
 */
typedef struct ta_auth_alg_map {
    int int_map[TA_AUTH_ALG_MAX]; /**< Array of mappings authentication 
                                       algorithm types onto card specific 
                                       constants */
} ta_auth_alg_map;


/** 
 * Definition of the variable that keeps card-specific 
 * information about ioctls.
 */
#ifdef WIFI_CARD_PRISM54
static ta_auth_alg_map prism54_auth_alg_map = {
    /*
     * OpenSystem is mappen onto - 1
     * SharedKey is mappen onto - 2
     */
    { 1, 2 }
};

static ta_priv_ioctl priv_ioctl[TA_PRIV_IOCTL_MAX] = {
     { TRUE, "reset", "reset", NULL },
     { TRUE, "g_authenable", "s_authenable", &prism54_auth_alg_map },
     { TRUE, "g_privinvok", "s_privinvok", NULL },
     { TRUE, "g_exunencrypt", "s_exunencrypt", NULL }
};
#else

/* Default WiFi card - dupports nothing */
static ta_priv_ioctl priv_ioctl[TA_PRIV_IOCTL_MAX];

#endif
#endif

/** The number of default WEP keys */
#define WEP_KEYS_NUM        4

/** Default key index value of the assigned WEP key */
#define WEP_KEY_ID_DFLT     1

/**
 * WEP 40 key length 5 bytes.
 */
#define WEP40_KEY_LEN       5
/**
 * WEP 104 key length 13 bytes.
 */
#define WEP104_KEY_LEN      13
/**
 * WEP 128 key length 16 bytes.
 */
#define WEP128_KEY_LEN      16

/**
 * Length of 40 bits WEP key is on default.
 */
#define WEP_KEY_LEN_DFLT    WEP40_KEY_LEN
/**
 * Length of 128 bits WEP key is maximal.
 */
#define WEP_KEY_LEN_MAX     WEP128_KEY_LEN

/** Information about station's settings */
typedef struct wifi_sta_info_s {
    te_bool valid; /**< Wheter this structure keeps valid data */
    te_bool wep_enc; /**< Wheter WEP encryption is enabled */
    uint8_t def_key_id; /**< Default TX key index [0..3] */
    uint8_t def_keys[WEP_KEYS_NUM][WEP_KEY_LEN_MAX]; /**< Default WEP keys */
    te_bool auth_open; /**< Wheter authentication algorithm is open */
    te_bool prev_auth_open; /**< Wheter authentication algorithm should be 
                                 open after enabling WEP */
} wifi_sta_info_t;

wifi_sta_info_t wifi_sta_info;

/**
 * Returns ponter to structure that keeps information about WiFi station.
 *
 * @param ifname_  Name of the interface on which WiFi station reside
 * @param ptr_     Location for the pointer
 *
 */
#define GET_WIFI_STA_INFO(ifname_, ptr_) \
    (ptr_) = (&wifi_sta_info)

#if 0
#define CHECK_CONSISTENCY(ifname_, info_) \
    do                                                         \
    {                                                          \
        te_bool check_rc_ = sta_info_check(ifname_, info_);    \
                                                               \
        if (!check_rc_)                                        \
            fprintf(stderr, "%s(): line %d Checking FAILED\n", \
                    __FUNCTION__, __LINE__);                   \
        assert(check_rc_);                                     \
    } while (0)
#else
#define CHECK_CONSISTENCY(ifname_, info_)
#endif

static te_errno sta_restore_encryption(const char *ifname,
                                       wifi_sta_info_t *info);
static te_errno init_sta_info(const char *ifname, wifi_sta_info_t *info);
#if 0
static te_bool sta_info_check(const char *ifname, wifi_sta_info_t *info);
#endif
static te_errno parse_wep_key_index(const char *in_value, int *out_value);


static te_errno wifi_wep_get(unsigned int gid, const char *oid,
                             char *value, const char *ifname);
static te_errno wifi_wep_set(unsigned int gid, const char *oid,
                             char *value, const char *ifname);
static te_errno wifi_auth_get(unsigned int gid, const char *oid,
                              char *value, const char *ifname);
static te_errno wifi_auth_set(unsigned int gid, const char *oid,
                              char *value, const char *ifname);


/**
 * Returns socket descriptor that should be used in ioctl() calls
 * for configuring wireless interface attributes
 *
 * @return Socket descriptor, or -errno in case of failure.
 */
static int
wifi_get_skfd()
{
    static int skfd = -1;

    if (skfd == -1)
    {
        /* We'll never close this socket until agent shutdown */
        if ((skfd = iw_sockets_open()) < 0)
        {
            ERROR("Cannot open socket for wireless extension");

            return -(TE_OS_RC(TE_TA_UNIX, errno));
        }
    }

    return skfd;
}

/**
 * Check if socket descriptor is valid
 *
 * @param skfd_ Socket descriptor to be checked
 */
#define WIFI_CHECK_SKFD(skfd_) \
    do {                       \
        if ((skfd_) < 0)       \
            return -(skfd_);   \
    } while (0);


/*
 * Execute a private command on the interface
 */
static int
set_private_cmd(int             skfd,           /* Socket */
                const char     *ifname,         /* Dev name */
                const char     *cmdname,        /* Command name */
                iwprivargs     *priv,           /* Private ioctl description */
                int             priv_num,       /* Number of descriptions */
                int             count,          /* Args count */
                va_list ap)                     /* Command arguments */
{
    struct iwreq    wrq;
    u_char          buffer[4096];       /* Only that big in v25 and later */
    int             i = 0;              /* Start with first command arg */
    int             k;                  /* Index in private description table */
#if 0
    int             temp;
#endif
    int             subcmd = 0;     /* sub-ioctl index */
    int             offset = 0;     /* Space for sub-ioctl index */

#if 0
    /* Check if we have a token index.
     * Do it know so that sub-ioctl takes precendence, and so that we
     * don't have to bother with it later on... */
    if((count > 1) && (sscanf(args[0], "[%i]", &temp) == 1))
    {
        subcmd = temp;
        args++;
        count--;
    }
#endif

    /* Search the correct ioctl */
    k = -1;

    while((++k < priv_num) && strcmp(priv[k].name, cmdname));

    /* If not found... */
    if(k == priv_num)
    {
        fprintf(stderr, "Invalid command : %s\n", cmdname);
        return(-1);
    }

    /* Watch out for sub-ioctls ! */
    if(priv[k].cmd < SIOCDEVPRIVATE)
    {
        int     j = -1;

        /* Find the matching *real* ioctl */
        while((++j < priv_num) && ((priv[j].name[0] != '\0') ||
              (priv[j].set_args != priv[k].set_args) ||
              (priv[j].get_args != priv[k].get_args)));

        /* If not found... */
        if(j == priv_num)
        {
            fprintf(stderr, "Invalid private ioctl definition for : %s\n",
            cmdname);

            return(-1);
        }

        /* Save sub-ioctl number */
        subcmd = priv[k].cmd;

        /* Reserve one int (simplify alignement issues) */
        offset = sizeof(__u32);

        /* Use real ioctl definition from now on */
        k = j;

        printf("<mapping sub-ioctl %s to cmd 0x%X-%d>\n", cmdname,
               priv[k].cmd, subcmd);
    }

    /* If we have to set some data */
    if((priv[k].set_args & IW_PRIV_TYPE_MASK) &&
       (priv[k].set_args & IW_PRIV_SIZE_MASK))
    {
        switch(priv[k].set_args & IW_PRIV_TYPE_MASK)
        {
            case IW_PRIV_TYPE_BYTE:
                /* Number of args to fetch */
                wrq.u.data.length = count;

                if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

                /* Fetch args */
                for(; i < wrq.u.data.length; i++)
                {
                    buffer[i] = (char) va_arg(ap, int);
                }

                break;

            case IW_PRIV_TYPE_INT:
                /* Number of args to fetch */
                wrq.u.data.length = count;

                if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

                /* Fetch args */
                for(; i < wrq.u.data.length; i++)
                {
                    ((__s32 *) buffer)[i] = (__s32) va_arg(ap, int);
                }

                break;

            case IW_PRIV_TYPE_CHAR:
                if(i < count)
                {
                    char *ptr;

                    ptr = va_arg(ap, char *);

                    /* Size of the string to fetch */
                    wrq.u.data.length = strlen(ptr) + 1;

                    if(wrq.u.data.length > (priv[k].set_args &
                                            IW_PRIV_SIZE_MASK))
                        wrq.u.data.length =
                            priv[k].set_args & IW_PRIV_SIZE_MASK;

                    /* Fetch string */
                    memcpy(buffer, ptr, wrq.u.data.length);

                    buffer[sizeof(buffer) - 1] = '\0';

                    i++;
                }
                else
                {
                    wrq.u.data.length = 1;

                    buffer[0] = '\0';
                }

                break;

#if 0
            case IW_PRIV_TYPE_FLOAT:
                /* Number of args to fetch */
                wrq.u.data.length = count;

                if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

                /* Fetch args */
                for(; i < wrq.u.data.length; i++)
                {
                    double        freq;

                    if(sscanf(args[i], "%lg", &(freq)) != 1)
                    {
                        printf("Invalid float [%s]...\n", args[i]);

                        return(-1);
                    }

                    if(index(args[i], 'G')) freq *= GIGA;

                    if(index(args[i], 'M')) freq *= MEGA;

                    if(index(args[i], 'k')) freq *= KILO;

                    sscanf(args[i], "%i", &temp);

                    iw_float2freq(freq, ((struct iw_freq *) buffer) + i);
                }

                break;

            case IW_PRIV_TYPE_ADDR:
                /* Number of args to fetch */
                wrq.u.data.length = count;

                if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
                    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

                /* Fetch args */
                for(; i < wrq.u.data.length; i++)
                {
                    const char *addr = va_arg(ap, char *);

                    if(iw_in_addr(skfd, ifname, addr,
                       ((struct sockaddr *) buffer) + i) < 0)
                    {
                        printf("Invalid address [%s]...\n", addr);

                        return(-1);
                    }
                }

                break;
#endif

            default:
                fprintf(stderr, "Not yet implemented...\n");

                return(-1);
        }

        if((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
           (wrq.u.data.length != (priv[k].set_args & IW_PRIV_SIZE_MASK)))
        {
            printf("The command %s need exactly %d argument...\n",
                   cmdname, priv[k].set_args & IW_PRIV_SIZE_MASK);

            return(-1);
        }
    }    /* if args to set */
    else
    {
      wrq.u.data.length = 0L;
    }

    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);

    /* Those two tests are important. They define how the driver
     * will have to handle the data */
    if((priv[k].set_args & IW_PRIV_SIZE_FIXED) &&
       ((iw_get_priv_size(priv[k].set_args) + offset) <= IFNAMSIZ))
    {
        /* First case : all SET args fit within wrq */
        if(offset)
            wrq.u.mode = subcmd;

        memcpy(wrq.u.name + offset, buffer, IFNAMSIZ - offset);
    }
    else
    {
        if((priv[k].set_args == 0) &&
           (priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
           (iw_get_priv_size(priv[k].get_args) <= IFNAMSIZ))
        {
            /* Second case : no SET args, GET args fit within wrq */
            if(offset)
                wrq.u.mode = subcmd;
        }
        else
        {
            /* Thirst case : args won't fit in wrq, or
             * variable number of args
             */
            wrq.u.data.pointer = (caddr_t) buffer;
            wrq.u.data.flags = subcmd;
        }
    }

    /* Perform the private ioctl */
    if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
    {
        fprintf(stderr, "Interface doesn't accept private ioctl...\n");

        fprintf(stderr, "%s (%X): %s\n", cmdname,
                priv[k].cmd, strerror(errno));

        return(-1);
    }

    /* If we have to get some data */
    if((priv[k].get_args & IW_PRIV_TYPE_MASK) &&
       (priv[k].get_args & IW_PRIV_SIZE_MASK))
    {
        int    j;
        int    n = 0;        /* number of args */

#if 0
        printf("%-8.8s  %s:", ifname, cmdname);
#endif

        /* Check where is the returned data */
        if((priv[k].get_args & IW_PRIV_SIZE_FIXED) &&
           (iw_get_priv_size(priv[k].get_args) <= IFNAMSIZ))
        {
            memcpy(buffer, wrq.u.name, IFNAMSIZ);

            n = priv[k].get_args & IW_PRIV_SIZE_MASK;
        }
        else
            n = wrq.u.data.length;

        switch(priv[k].get_args & IW_PRIV_TYPE_MASK)
        {
            case IW_PRIV_TYPE_BYTE:
                /* Display args */
                for(j = 0; j < n; j++)
                {
                    if (count != 0)
                    {
                        *((char *) va_arg(ap, char *)) = buffer[j];

                        count--;
                    }
                    else
                    {
                        ERROR("Cannot flush %d byte in %s:%d",
                              j, __FILE__, __LINE__);

                        return -1;
                    }
                }

                break;

            case IW_PRIV_TYPE_INT:
                /* Display args */
                for(j = 0; j < n; j++)
                {
                    if (count != 0)
                    {
                        *((int *) va_arg(ap, int *)) = buffer[j];

                        count--;
                    }
                    else
                    {
                        ERROR("Cannot flush %d byte in %s:%d",
                              j, __FILE__, __LINE__);

                        return -1;
                    }
                }

                break;

            case IW_PRIV_TYPE_CHAR:
                /* Display args */
                if (count == 0)
                {
                    ERROR("Cannot flush string in %s:%d", __FILE__, __LINE__);

                        return -1;
                }

                count--;

                buffer[wrq.u.data.length - 1] = '\0';

                memcpy((char *)va_arg(ap, int *), buffer, wrq.u.data.length);

                break;

#if 0
            case IW_PRIV_TYPE_FLOAT:
                {
                    double        freq;

                    /* Display args */
                    for(j = 0; j < n; j++)
                    {
                        freq = iw_freq2float(((struct iw_freq *) buffer) + j);

                        if(freq >= GIGA)
                            printf("%gG  ", freq / GIGA);
                        else if(freq >= MEGA)
                            printf("%gM  ", freq / MEGA);
                        else
                            printf("%gk  ", freq / KILO);
                    }

                    printf("\n");
                }

                break;

            case IW_PRIV_TYPE_ADDR:
                {
                    char        scratch[128];
                    struct sockaddr *    hwa;

                    /* Display args */
                    for(j = 0; j < n; j++)
                    {
                        hwa = ((struct sockaddr *) buffer) + j;

                        if(j)
                            printf("           %.*s",
                                   (int) strlen(cmdname), "                ");

                        printf("%s\n", iw_pr_ether(scratch, hwa->sa_data));
                    }
                }

                break;
#endif

            default:
                fprintf(stderr, "Not yet implemented...\n");

                return(-1);
        }
    }    /* if args to set */

    return(0);
}

/*------------------------------------------------------------------*/
/*
 * Execute a private command on the interface
 */
static int
set_private(const char *ifname, /* Dev name */
            const char *cmd,
            int count, /* Args count */
            ...)
{
    iwprivargs     *priv;
    int             number; /* Max of private ioctl */
    int             skfd = wifi_get_skfd();
    va_list         ap;
    int             rc;

    WIFI_CHECK_SKFD(skfd);

    va_start(ap, count);

    /* Read the private ioctls */
    number = iw_get_priv_info(skfd, ifname, &priv);

    /* Is there any ? */
    if(number <= 0)
    {
        /* Could skip this message ? */
        fprintf(stderr, "%-8.8s  no private ioctls.\n\n",
                ifname);

        return(-1);
    }

    rc = set_private_cmd(skfd, ifname, cmd,
                         priv, number, count, ap);
    va_end(ap);

    return rc;
}

/**
 * Update a configuration item in WiFi card.
 *
 * @param ifname  Wireless interface name
 * @param req     Request type
 * @param wrp     request value
 *
 * @return Status of the operation
 */
static int
wifi_set_item(const char *ifname, int req, struct iwreq *wrp)
{
    int rc;
    int retry = 0;
    int skfd = wifi_get_skfd();

#define RETRY_LIMIT 500
    WIFI_CHECK_SKFD(skfd);

    while ((rc = iw_set_ext(skfd, ifname, req, wrp)) != 0)
    {
        if (errno == EBUSY)
        {
            /* Try again */
            if (++retry < RETRY_LIMIT)
            {
                usleep(50);

                continue;
            }
        }

        rc = TE_OS_RC(TE_TA_UNIX, errno);

        break;
    }
#undef RETRY_LIMIT

    if (retry != 0)
        WARN("%s: The number of retries %d", __FUNCTION__, retry);

    return rc;
}

/**
 * Get a configuration item from WiFi card.
 *
 * @param ifname  Wireless interface name
 * @param req     Request type
 * @param wrp     request value (OUT)
 *
 * @return Status of the operation
 */
static int
wifi_get_item(const char *ifname, int req, struct iwreq *wrp)
{
    int rc;
    int retry = 0;
    int skfd = wifi_get_skfd();

#define RETRY_LIMIT 500
    WIFI_CHECK_SKFD(skfd);

    while ((rc = iw_get_ext(skfd, ifname, req, wrp)) != 0)
    {
        if (errno == EBUSY)
        {
            /* Try again */
            if (++retry < RETRY_LIMIT)
            {
                usleep(50);

                continue;
            }
        }

        rc = TE_OS_RC(TE_TA_UNIX, errno);

        break;
    }
#undef RETRY_LIMIT

    if (retry != 0)
        WARN("%s: The number of retries %d", __FUNCTION__, retry);

    return rc;
}

static te_errno
sta_restore_encryption(const char *ifname, wifi_sta_info_t *info)
{
    te_errno rc = 0;

    if (!info->wep_enc)
    {
        struct iwreq wrq;

        wrq.u.data.pointer = (caddr_t)NULL;
        wrq.u.data.flags = IW_ENCODE_DISABLED | IW_ENCODE_NOKEY;
        wrq.u.data.length = 0;

        if ((rc = wifi_set_item(ifname, SIOCSIWENCODE, &wrq)) != 0)
        {
            ERROR("%s(): Cannot disable WEP encryption: %r",
                  __FUNCTION__, rc);

            return rc;
        }
    }

    CHECK_CONSISTENCY(ifname, info);

    return 0;
}

static te_errno
init_sta_info(const char *ifname, wifi_sta_info_t *info)
{
    struct iwreq    wrq;
    uint8_t         key[IW_ENCODING_TOKEN_MAX];
    int             i;
    te_errno        rc;

    memset(info, 0, sizeof(*info));

    wrq.u.data.pointer = (caddr_t)key;
    wrq.u.data.length = sizeof(key);
    wrq.u.data.flags = 0; /* Set index to zero to get current */

    if ((rc = wifi_get_item(ifname, SIOCGIWENCODE, &wrq)) == 0)
    {
        if ((wrq.u.data.flags & IW_ENCODE_INDEX) == 0)
            /* IOCTL returns XX00 in wrq.u.data.flags when
             * WEP encryption is disabled. We may consider
             * that def_key_id is 0 in this case.
             */
            info->def_key_id = 0;
        else
            info->def_key_id = (wrq.u.data.flags & IW_ENCODE_INDEX) - 1;

        info->auth_open = TRUE;

        if (wrq.u.data.flags & IW_ENCODE_RESTRICTED)
        {
            assert(!(wrq.u.data.flags & IW_ENCODE_DISABLED));

            info->auth_open = FALSE;
        }
        info->prev_auth_open = info->auth_open;

        if (!(wrq.u.data.flags & IW_ENCODE_DISABLED))
            info->wep_enc = TRUE;

#if 0
        /* Set default keys to zero */
        for (i = 1; i <= WEP_KEYS_NUM; i++)
        {
            wrq.u.data.pointer = (caddr_t)info->def_keys[i];
            wrq.u.data.flags = 0;
            wrq.u.data.length = sizeof(info->def_keys[i]);
            wrq.u.encoding.flags = i;

            if ((rc = wifi_set_item(ifname, SIOCSIWENCODE, &wrq)) != 0)
            {
                ERROR("%s(): Cannot initialize Default WEP key [%d]: %r",
                      __FUNCTION__, i, rc);
                return rc;
            }
        }
#endif

        /*
         * Some cards enable WEP when updating default TX Key,
         * so restore encryption configuration here.
         */
        sta_restore_encryption(ifname, info);

        info->valid = TRUE;
    }
    else
    {
        ERROR("%s(): Cannot read wireless configuration on %s interface",
              __FUNCTION__, ifname);
    }

    return rc;
}

#if 0
static te_bool
sta_info_check(const char *ifname, wifi_sta_info_t *info)
{
    struct iwreq wrq;
    uint8_t      key[IW_ENCODING_TOKEN_MAX];
    uint8_t      sta_key_index;
    te_bool      sta_auth_open;
    te_errno     rc;

    wrq.u.data.pointer = (caddr_t)key;
    wrq.u.data.length = sizeof(key);
    wrq.u.data.flags = 0; /* Set index to zero to get current */

    if ((rc = wifi_get_item(ifname, SIOCGIWENCODE, &wrq)) != 0)
    {
        ERROR("%s(): Cannot get Default WEP key index: %r",
              __FUNCTION__, rc);
        fprintf(stderr, "%s(): Cannot get Default WEP key index: %d\n",
              __FUNCTION__, rc);
        return FALSE;
    }

    /* Check that Default WEP Key index is consistent */
    sta_key_index = (wrq.u.data.flags & IW_ENCODE_INDEX) - 1;
    if (sta_key_index != info->def_key_id)
    {
        ERROR("%s(): Inconsistency with Default WEP Key index: "
              "STA expected Default WEP Key index %d, but it is %d",
              __FUNCTION__, info->def_key_id, sta_key_index);
        fprintf(stderr, "%s(): Inconsistency with Default WEP Key index: "
              "STA expected Default WEP Key index %d, but it is %d\n",
              __FUNCTION__, info->def_key_id, sta_key_index);
        return FALSE;
    }

    if ((!(wrq.u.data.flags & IW_ENCODE_DISABLED)) != info->wep_enc)
    {
        ERROR("%s(): Inconsistency with WEP encryption: "
              "STA expected encryption %s, actually it is %s",
              __FUNCTION__,
              info->wep_enc ? "true" : "false",
              !(wrq.u.data.flags & IW_ENCODE_DISABLED) ? "true" : "false");
        fprintf(stderr, "%s(): Inconsistency with WEP encryption: "
              "STA expected encryption %s, actually it is %s\n",
              __FUNCTION__,
              info->wep_enc ? "true" : "false",
              !(wrq.u.data.flags & IW_ENCODE_DISABLED) ? "true" : "false");
        return FALSE;
    }

    sta_auth_open = !(wrq.u.data.flags & IW_ENCODE_RESTRICTED);

    if ((!info->wep_enc || info->auth_open) != sta_auth_open)
    {
        ERROR("%s(): Inconsistency with authentication method being used: "
              "STA expected to operate in %s authentication mode, but "
              "it operated in %s mode", __FUNCTION__,
              info->auth_open ? "Open" : "SharedKey",
              sta_auth_open ? "Open" : "SharedKey");
        fprintf(stderr, "%s(): Inconsistency with authentication"
                        "method being used: "
              "STA expected to operate in %s authentication mode, but "
              "it operated in %s mode\n", __FUNCTION__,
              info->auth_open ? "Open" : "SharedKey",
              sta_auth_open ? "Open" : "SharedKey");
        return FALSE;
    }

    /* Check PrivacyInvoked and ExcludeUnencrypted */
    memset(&wrq, 0, sizeof(wrq));
    wrq.u.param.flags = IW_AUTH_DROP_UNENCRYPTED;
    if ((rc = wifi_get_item(ifname, SIOCGIWAUTH, &wrq)) != 0)
    {
        ERROR("%s(): Cannot get DROP_UNENCRYPTED flag", __FUNCTION__);
        return FALSE;
    }
    if (wrq.u.param.value != info->wep_enc)
    {
        ERROR("%s(): Inconsistency in DROP_UNENCRYPTED flag: "
              "STA is operating with %s WEP, but DROP_UNENCRYPTED is %s",
              __FUNCTION__,
              info->wep_enc ? "enabled" : "disabled",
              wrq.u.param.value ? "enabled" : "disabled");
        fprintf(stderr, "%s(): Inconsistency in DROP_UNENCRYPTED flag: "
              "STA is operating with %s WEP, but DROP_UNENCRYPTED is %s",
              __FUNCTION__,
              info->wep_enc ? "enabled" : "disabled",
              wrq.u.param.value ? "enabled" : "disabled");

        wrq.u.param.flags = IW_AUTH_DROP_UNENCRYPTED;
        wrq.u.param.value = info->wep_enc;
        if ((rc = wifi_set_item(ifname, SIOCSIWAUTH, &wrq)) != 0)
        {
            ERROR("%s(): Cannot restore DROP_UNENCRYPTED flag", __FUNCTION__);
            return FALSE;
        }
        WARN("DROP_UNENCRYPTED flag is restored");
    }
    wrq.u.param.flags = IW_AUTH_PRIVACY_INVOKED;
    if ((rc = wifi_get_item(ifname, SIOCGIWAUTH, &wrq)) != 0)
    {
        ERROR("%s(): Cannot change PRIVACY_INVOKED flag", __FUNCTION__);
        return rc;
    }
    if (wrq.u.param.value != info->wep_enc)
    {
        ERROR("%s(): Inconsistency in PRIVACY_INVOKED flag: "
              "STA is operating with %s WEP, but PRIVACY_INVOKED is %s",
              __FUNCTION__,
              info->wep_enc ? "enabled" : "disabled",
              wrq.u.param.value ? "enabled" : "disabled");
        fprintf(stderr, "%s(): Inconsistency in PRIVACY_INVOKED flag: "
              "STA is operating with %s WEP, but PRIVACY_INVOKED is %s",
              __FUNCTION__,
              info->wep_enc ? "enabled" : "disabled",
              wrq.u.param.value ? "enabled" : "disabled");
        wrq.u.param.flags = IW_AUTH_PRIVACY_INVOKED;
        wrq.u.param.value = info->wep_enc;
        if ((rc = wifi_set_item(ifname, SIOCSIWAUTH, &wrq)) != 0)
        {
            ERROR("%s(): Cannot restore PRIVACY_INVOKED flag", __FUNCTION__);
            return rc;
        }
        WARN("PRIVACY_INVOKED flag is restored");
    }

    return TRUE;
}
#endif

static te_errno
parse_wep_key_index(const char *in_value, int *out_value)
{
    if (sscanf(in_value, "%i", out_value) != 1 ||
        (*out_value < 0) || (*out_value >= IW_ENCODE_INDEX))
    {
        ERROR("Incorrect value for WEP key index: '%s'\n"
              "Allowed values are: 0, 1, 2, 3.", in_value);

        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

/**
 * Determine if interface supports wireless extension or not.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param list    location for the list pointer
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_list(unsigned int gid, const char *oid, char **list,
          const char *ifname)
{
    wifi_sta_info_t *info;
    struct iwreq     wrq;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = wifi_get_item(ifname, SIOCGIWNAME, &wrq)) != 0)
    {
        /* Interface does not support wireless extension */
        VERB("Interface %s does not support WiFi", ifname);

        *list = strdup("");

        return 0;
    }

    /* Fill in station parameters */
    GET_WIFI_STA_INFO(ifname, info);

    if (info->valid)
    {
        *list = strdup("enabled");

        return 0;
    }

    if ((rc = init_sta_info(ifname, info)) != 0)
        return rc;

    *list = strdup("enabled");

    return 0;
}

/**
 * Get Default Tx WEP key index on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   ilocation for Default WEP key index
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_def_key_id_get(unsigned int gid, const char *oid, char *value,
                        const char *ifname)
{
    wifi_sta_info_t *info;

    UNUSED(gid);
    UNUSED(oid);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    sprintf(value, "%d", info->def_key_id);

    return 0;
}

/**
 * Update Default Tx WEP key index on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   new WEP key index value (0..3)
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_def_key_id_set(unsigned int gid, const char *oid, char *value,
                        const char *ifname)
{
    struct iwreq     wrq;
    int              key_index;
    te_errno         rc;
    wifi_sta_info_t *info;

    UNUSED(gid);
    UNUSED(oid);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    if ((rc = parse_wep_key_index(value, &key_index)) != 0)
        return rc;

    memset(&wrq, 0, sizeof(wrq));

    wrq.u.encoding.flags = (key_index + 1);
    wrq.u.data.pointer = (caddr_t)NULL;
    wrq.u.data.flags |= IW_ENCODE_NOKEY;
    wrq.u.data.length = 0;

    if ((rc = wifi_set_item(ifname, SIOCSIWENCODE, &wrq)) != 0)
    {
        ERROR("%s(): Cannot set Default WEP key [%d]: %r",
              __FUNCTION__, key_index, rc);

        return rc;
    }

    info->def_key_id = key_index;

    /*
     * Some cards enable WEP on changing Default TX Key, 
     * so that we need to restore current configuration.
     */
    sta_restore_encryption(ifname, info);

    return 0;
}

/**
 * Get WEP key value used on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value in format "XXXXXXXXXX" (5 bytes)
 * @param ifname  interface name
 * @param key_id  index of the WEP key which value to be returned
 *
 * @return error code
 */
static te_errno
wifi_wep_key_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname, const char *p1, const char *p2,
                 const char *key_id)
{
    int              key_index;
    unsigned int     i;
    te_errno         rc;
    wifi_sta_info_t *info;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(p1);
    UNUSED(p2);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    if ((rc = parse_wep_key_index(key_id, &key_index)) != 0)
        return rc;

    value[0] = '\0';
    for (i = 0; i < sizeof(info->def_keys[key_index]); i++)
    {
        sprintf(value + strlen(value), "%02x", info->def_keys[key_index][i]);
    }

    return 0;
}

/**
 * Update WEP key value on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   WEP key value in format "XXXXXXXXXX" (5 bytes)
 * @param ifname  interface name
 * @param key_id  index of the WEP key which value to be updated
 *
 * @return error code
 */
static te_errno
wifi_wep_key_set(unsigned int gid, const char *oid, char *value,
                 const char *ifname, const char *p1, const char *p2,
                 const char *key_id)
{
    int              rc;
    struct iwreq     wrq;
    uint8_t          key[IW_ENCODING_TOKEN_MAX];
    int              key_index;
    int              keylen;
    wifi_sta_info_t *info;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(p1);
    UNUSED(p2);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    if ((rc = parse_wep_key_index(key_id, &key_index)) != 0)
        return rc;

    memset(&wrq, 0, sizeof(wrq));

    keylen = iw_in_key(value, key);

    if (keylen <= 0)
    {
        ERROR("%s(): Incorrect WEP key value '%s' specified",
              __FUNCTION__, value);

        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    wrq.u.data.length = keylen;
    wrq.u.data.pointer = (caddr_t)key;
    wrq.u.encoding.flags = (key_index + 1);

    if ((rc = wifi_set_item(ifname, SIOCSIWENCODE, &wrq)) != 0)
        return rc;

    memcpy(info->def_keys[key_index], key, sizeof(info->def_keys[key_index]));

    sta_restore_encryption(ifname, info);

    return 0;
}

/**
 * Returns the list of Default WEP keys (always four keys present
 * in the system).
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param list    location for the list pointer
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_key_list(unsigned int gid, const char *oid, char **list,
                  const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    /* Any interface supporting WEP should keep four default WEP keys */
    *list = strdup("0 1 2 3");

    return 0;
}

/**
 * Get the status of WEP on the wireless interface - wheter it is on or off.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value ("0" - false or "1" - true)
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_get(unsigned int gid, const char *oid, char *value,
             const char *ifname)
{
    wifi_sta_info_t *info;

    UNUSED(gid);
    UNUSED(oid);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    sprintf(value, "%d", info->wep_enc);

    return 0;
}

/**
 * Update the status of WEP on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   the new status value ("0" - false or "1" - true)
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_set(unsigned int gid, const char *oid, char *value,
             const char *ifname)
{
    int                 int_val;
    te_bool             new_wep_enc;
    uint8_t             key[IW_ENCODING_TOKEN_MAX];
    wifi_sta_info_t    *info;
    te_errno            rc;
    struct iwreq        wrq;

    UNUSED(gid);
    UNUSED(oid);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    /*
     * Here I use variable of type "int" to be sure that 
     * there is enough space for integer value, which is 
     * not the case for "te_bool"!
     */
    if (sscanf(value, "%d", &int_val) != 1)
    {
        ERROR("Incorrect value for WEP encryption passed %s", value);

        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    new_wep_enc = (int_val == 1);

    if (new_wep_enc == info->wep_enc)
        return 0;

    if (new_wep_enc)
    {
        /*
         * We enable WEP, which current disabled, so we might
         * need to restore authentication method.
         */
        wrq.u.data.pointer = (caddr_t)key;
        wrq.u.data.length = sizeof(key);
        wrq.u.data.flags = 0;

        if ((rc = wifi_get_item(ifname, SIOCGIWENCODE, &wrq)) != 0)
        {
            ERROR("%s(): Cannot read out current WiFi information",
                  __FUNCTION__);

            return rc;
        }

        if (wrq.u.data.length == WEP40_KEY_LEN ||
            wrq.u.data.length == WEP104_KEY_LEN ||
            wrq.u.data.length == WEP128_KEY_LEN)
        {
            wrq.u.data.flags &= ~IW_ENCODE_DISABLED;

            if ((rc = wifi_set_item(ifname, SIOCSIWENCODE, &wrq)) != 0)
            {
                ERROR("%s(): Cannot enable WEP encryption",
                      __FUNCTION__);

                return rc;
            }
        }
        else
        {
            ERROR("%s(): Invalid value '%d' of encryption "
                  "key length was returned. Try to enable WEP 40 "
                  "encryption with default zero key.",
                  __FUNCTION__, wrq.u.data.length);

            memset(key, 0, WEP_KEY_LEN_DFLT);
            memset(&wrq, 0, sizeof(wrq));

            wrq.u.data.pointer = (caddr_t)key;
            wrq.u.data.length = WEP_KEY_LEN_DFLT;
            wrq.u.encoding.flags = WEP_KEY_ID_DFLT;

            if ((rc = wifi_set_item("ath1", SIOCSIWENCODE, &wrq)) != 0)
            {
                ERROR("%s(): Cannot enable WEP 40 encryption "
                      "with default zero key", __FUNCTION__);

                return rc;
            }
        }

        info->auth_open = info->prev_auth_open;
    }
    else
    {
        wrq.u.data.pointer = (caddr_t) NULL;
        wrq.u.data.flags = IW_ENCODE_DISABLED | IW_ENCODE_NOKEY;
        wrq.u.data.length = 0;

        if ((rc = wifi_set_item(ifname, SIOCSIWENCODE, &wrq)) != 0)
        {
            ERROR("%s(): Cannot disable WEP encryption",
                  __FUNCTION__);

            return rc;
        }

        /*
         * We've disabled WEP encryption, and if we used sharedKey
         * authentication method, it has just been changed to Open.
         * When we turn WEP on in the future we should remember 
         * about that, so save current authentication method.
         */
        info->prev_auth_open = info->auth_open;

        info->auth_open = TRUE;
    }

    info->wep_enc = !info->wep_enc;

    /* Update PrivacyInvoked and ExcludeUnencrypted */
    memset(&wrq, 0, sizeof(wrq));
    wrq.u.param.flags = IW_AUTH_DROP_UNENCRYPTED;
    wrq.u.param.value = info->wep_enc;

    if ((rc = wifi_set_item(ifname, SIOCSIWAUTH, &wrq)) != 0)
    {
        ERROR("%s(): Cannot change DROP_UNENCRYPTED flag", __FUNCTION__);

        return rc;
    }

    wrq.u.param.flags = IW_AUTH_PRIVACY_INVOKED;

    if ((rc = wifi_set_item(ifname, SIOCSIWAUTH, &wrq)) != 0)
    {
        ERROR("%s(): Cannot change PRIVACY_INVOKED flag", __FUNCTION__);

        return rc;
    }

    CHECK_CONSISTENCY(ifname, info);

    return 0;
}

/**
 * Get authentication algorithm currenty enabled on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value
 *                "open"   - open System
 *                "shared" - shared Key
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_auth_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    wifi_sta_info_t *info;

    UNUSED(gid);
    UNUSED(oid);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    if (info->auth_open)
        sprintf(value, "open");
    else
        sprintf(value, "shared");

    return 0;
}

/**
 * Update authentication algorithm used on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value
 *                "open"   - open System
 *                "shared" - shared Key
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_auth_set(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    wifi_sta_info_t *info;
    te_errno         rc;
    struct iwreq     wrq;

    UNUSED(gid);
    UNUSED(oid);

    GET_WIFI_STA_INFO(ifname, info);
    CHECK_CONSISTENCY(ifname, info);

    wrq.u.data.pointer = (caddr_t)NULL;
    wrq.u.data.flags = IW_ENCODE_NOKEY;
    wrq.u.data.length = 0;

    if (strcmp(value, "open") == 0)
    {
        wrq.u.param.value = IW_AUTH_ALG_OPEN_SYSTEM;
    }
    else if (strcmp(value, "shared") == 0)
    {
        if (!info->wep_enc)
        {
            ERROR("SharedKey authentication can't be enabled when "
                  "WEP is disabled on the interface.");

            return TE_RC(TE_TA_UNIX, EPERM);
        }

        wrq.u.param.value = IW_AUTH_ALG_SHARED_KEY;
    }
    else
    {
        ERROR("Canot set authentication algorithm to '%s'", value);

        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    wrq.u.param.flags = IW_AUTH_80211_AUTH_ALG;

    if ((rc = wifi_set_item(ifname, SIOCSIWAUTH, &wrq)) != 0)
    {
        ERROR("%s(): Cannot change Authentication algorithm", __FUNCTION__);

        return rc;
    }

    info->prev_auth_open = info->auth_open = (strcmp(value, "open") == 0);

    if (!info->auth_open)
        sta_restore_encryption(ifname, info);

    CHECK_CONSISTENCY(ifname, info);

    return 0;
}

/**
 * Get channel number used on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - channel number
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_channel_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    int             rc;
    struct iw_range range;
    struct iwreq    wrq;
    double          freq;
    int             channel;
    int             skfd = wifi_get_skfd();

    UNUSED(gid);
    UNUSED(oid);

    WIFI_CHECK_SKFD(skfd);

    memset(&wrq, 0, sizeof(wrq));

    if (iw_get_range_info(skfd, ifname, &range) < 0)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    if ((rc = wifi_get_item(ifname, SIOCGIWFREQ, &wrq)) != 0)
        return rc;

    freq = iw_freq2float(&(wrq.u.freq));

    channel = iw_freq_to_channel(freq, &range);

    if (freq < KILO)
    {
        WARN("iw_freq2float() function returns channel, not frequency");

        channel = (int)freq;
    }

    if (channel < 0)
    {
        ERROR("Cannot get current channel number on %s interface", ifname);

        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    sprintf(value, "%d", channel);

    return 0;
}

/**
 * Set channel number on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - channel number
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_channel_set(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    struct iwreq    wrq;
    struct iw_range range;
    int             skfd = wifi_get_skfd();
    double          freq;
    int             channel;

    UNUSED(gid);
    UNUSED(oid);

    WIFI_CHECK_SKFD(skfd);

    if (sscanf(value, "%d", &channel) != 1)
    {
        ERROR("Incorrect format of channel value '%s'", value);

        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (iw_get_range_info(skfd, ifname, &range) < 0)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    if (iw_channel_to_freq(channel, &freq, &range) < 0)
    {
        ERROR("Cannot convert %d channel to an appropriate "
              "frequency value", channel);

        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    iw_float2freq(freq, &(wrq.u.freq));

    return wifi_set_item(ifname, SIOCSIWFREQ, &wrq);
}

/**
 * Get the list of supported channels on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - colon separated list of channels
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_channels_get(unsigned int gid, const char *oid, char *value,
                  const char *ifname)
{
    struct iw_range range;
    int             skfd = wifi_get_skfd();
    int             i;
    double          freq;
    int             channel;

    UNUSED(gid);
    UNUSED(oid);

    WIFI_CHECK_SKFD(skfd);

    if (iw_get_range_info(skfd, ifname, &range) < 0)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    *value = '\0';

    for (i = 0; i < range.num_frequency; i++)
    {
        freq = iw_freq2float(&(range.freq[i]));

        channel = iw_freq_to_channel(freq, &range);

        sprintf(value + strlen(value), "%d%c", channel,
                i == (range.num_frequency - 1) ? '\0' : ':');
    }

    return 0;
}

/**
 * Get AP MAC address the STA is associated with.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - AP MAC address,
 *                00:00:00:00:00:00 in case of STA is not associated
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_ap_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    int          rc;
    struct iwreq wrq;
    int          i;

    UNUSED(gid);
    UNUSED(oid);

    memset(&wrq, 0, sizeof(wrq));

    if ((rc = wifi_get_item(ifname, SIOCGIWAP, &wrq)) != 0)
        return rc;

    for (i = 0; i < (ETHER_ADDR_LEN - 1); i++)
    {
        if (wrq.u.ap_addr.sa_data[i] != wrq.u.ap_addr.sa_data[i + 1])
        {
            sprintf(value, "%02x:%02x:%02x:%02x:%02x:%02x",
                    (uint8_t)wrq.u.ap_addr.sa_data[0],
                    (uint8_t)wrq.u.ap_addr.sa_data[1],
                    (uint8_t)wrq.u.ap_addr.sa_data[2],
                    (uint8_t)wrq.u.ap_addr.sa_data[3],
                    (uint8_t)wrq.u.ap_addr.sa_data[4],
                    (uint8_t)wrq.u.ap_addr.sa_data[5]);

            return 0;
        }
    }

    sprintf(value, "00:00:00:00:00:00");

    return 0;
}

/**
 * Get ESSID value configured on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - ESSID
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_essid_get(unsigned int gid, const char *oid, char *value,
               const char *ifname)
{
    struct iwreq    wrq;
    char            essid[IW_ESSID_MAX_SIZE + 1] = {};
    te_errno        rc;
    int             retry = 0;
    int             skfd = wifi_get_skfd();

    UNUSED(gid);
    UNUSED(oid);

    wrq.u.essid.pointer = (caddr_t)essid;
    wrq.u.essid.length = sizeof(essid);
    wrq.u.essid.flags = 0;

#if 0
    if ((rc = wifi_get_item(ifname, SIOCGIWESSID, &wrq)) != 0)
    {
        ERROR("%s(): Cannot read ESSID value for %s interface",
              __FUNCTION__, ifname);

        return rc;
    }
#endif
    WIFI_CHECK_SKFD(skfd);

#define EBUSY_RETRY_LIMIT 500
    while ((rc = iw_get_ext(skfd, ifname, SIOCGIWESSID, &wrq)) != 0)
    {
        if (errno == EBUSY)
        {
            /* Try again */
            if (++retry < EBUSY_RETRY_LIMIT)
            {
                usleep(50);

                continue;
            }
        }

        rc = TE_OS_RC(TE_TA_UNIX, errno);

        break;
    }
#undef EBUSY_RETRY_LIMIT

    if (rc != 0)
    {
        if (errno != E2BIG)
        {
            ERROR("%s(): Cannot read ESSID name for interface %s",
                  __FUNCTION__, ifname);

            return rc;
        }

        /*
         * errno == E2BIG may mean that ESSID is not configured at all.
         * We'll try to bypass this problem by configuring ESSID with
         * empty name
         */
        ERROR("%s(): Error E2BIG on attempt to read ESSID name "
              "for interface %s. "
              "Try to assign empty ESSID name to bypass problem",
              __FUNCTION__, ifname);

        essid[0] = '\0';
        wrq.u.essid.pointer = (caddr_t)essid;
        wrq.u.essid.length = strlen(essid) + 1;
        wrq.u.essid.flags = 0;
        if ((rc = wifi_set_item(ifname, SIOCSIWESSID, &wrq)) != 0)
        {
            ERROR("%s(): Cannot assign empty ESSID name for interface %s",
                  __FUNCTION__, ifname);

            return rc;
        }
    }

    if (retry != 0)
        ERROR("%s(): %d retries to get ESSID name", __FUNCTION__, retry);

    strcpy(value, essid);

    return 0;
}

/**
 * Update ESSID value on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value - ESSID
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_essid_set(unsigned int gid, const char *oid, char *value,
               const char *ifname)
{
    struct iwreq wrq;
    char         essid[IW_ESSID_MAX_SIZE + 1];
    te_errno     rc;

    UNUSED(gid);
    UNUSED(oid);

    if (strcasecmp(value, "off") == 0 || strcasecmp(value, "any") == 0)
    {
        wrq.u.essid.flags = 0;
        essid[0] = '\0';
    }
    else
    {
        if (strlen(value) > IW_ESSID_MAX_SIZE)
        {
            ERROR("ESSID string '%s' is too long. "
                  "Maximum allowed length is %d", value, IW_ESSID_MAX_SIZE);

            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        wrq.u.essid.flags = 1;

        strcpy(essid, value);
    }

    wrq.u.essid.pointer = (caddr_t)essid;
    wrq.u.essid.length = strlen(essid) + 1;

    rc = wifi_set_item(ifname, SIOCSIWESSID, &wrq);

#ifdef ENABLE_8021X
    ds_supplicant_network_set(0, NULL, value, ifname);
#endif

    return rc;
}


/* Unix Test Agent WiFi configuration subtree */

RCF_PCH_CFG_NODE_RW(node_wifi_wep_def_key_id, "def_key_id", NULL, NULL,
                    wifi_wep_def_key_id_get,
                    wifi_wep_def_key_id_set);

static rcf_pch_cfg_object node_wifi_wep_key = {
    "key", 0, NULL, &node_wifi_wep_def_key_id,
    (rcf_ch_cfg_get)wifi_wep_key_get, (rcf_ch_cfg_set)wifi_wep_key_set,
    NULL, NULL, (rcf_ch_cfg_list)wifi_wep_key_list, NULL, NULL
};

RCF_PCH_CFG_NODE_RW(node_wifi_wep, "wep", &node_wifi_wep_key, NULL,
                    wifi_wep_get, wifi_wep_set);

RCF_PCH_CFG_NODE_RW(node_wifi_auth, "auth", NULL, &node_wifi_wep,
                    wifi_auth_get, wifi_auth_set);

RCF_PCH_CFG_NODE_RW(node_wifi_channel, "channel", NULL, &node_wifi_auth,
                    wifi_channel_get, wifi_channel_set);

RCF_PCH_CFG_NODE_RO(node_wifi_channels, "channels", NULL,
                    &node_wifi_channel, wifi_channels_get);

RCF_PCH_CFG_NODE_RO(node_wifi_ap, "ap", NULL, &node_wifi_channels,
                    wifi_ap_get);

RCF_PCH_CFG_NODE_RW(node_wifi_essid, "essid", NULL, &node_wifi_ap,
                    wifi_essid_get, wifi_essid_set);

RCF_PCH_CFG_NODE_COLLECTION(node_wifi, "wifi",
                            &node_wifi_essid, NULL,
                            NULL, NULL,
                            wifi_list, NULL);

/**
 * Initializes ta_unix_conf_wifi support.
 *
 * @return Status code (see te_errno.h)
 */
te_errno
ta_unix_conf_wifi_init()
{
    return rcf_pch_add_node("/agent/interface", &node_wifi);
}
