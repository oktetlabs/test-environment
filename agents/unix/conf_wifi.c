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
#if HAVE_IWLIB_H
#include <iwlib.h>
#else
#error iwlib.h is an obligatory header for WiFi support
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
set_private_cmd(int		skfd,		/* Socket */
		const char     *ifname,		/* Dev name */
		const char     *cmdname,	/* Command name */
		iwprivargs     *priv,		/* Private ioctl description */
		int		priv_num,	/* Number of descriptions */
		int		count,		/* Args count */
		va_list ap)                     /* Command arguments */
{
  struct iwreq	wrq;
  u_char	buffer[4096];	/* Only that big in v25 and later */
  int		i = 0;		/* Start with first command arg */
  int		k;		/* Index in private description table */
#if 0
  int		temp;
#endif
  int		subcmd = 0;	/* sub-ioctl index */
  int		offset = 0;	/* Space for sub-ioctl index */

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
      int	j = -1;

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
	  for(; i < wrq.u.data.length; i++) {
	    buffer[i] = (char) va_arg(ap, int);
	  }
	  break;

	case IW_PRIV_TYPE_INT:
	  /* Number of args to fetch */
	  wrq.u.data.length = count;
	  if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
	    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	  /* Fetch args */
	  for(; i < wrq.u.data.length; i++) {
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
	      if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
		wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

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
	  for(; i < wrq.u.data.length; i++) {
	    double		freq;
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
#endif

#if 0
	case IW_PRIV_TYPE_ADDR:
	  /* Number of args to fetch */
	  wrq.u.data.length = count;
	  if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
	    wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	  /* Fetch args */
	  for(; i < wrq.u.data.length; i++) {
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
    }	/* if args to set */
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
	  /* Thirst case : args won't fit in wrq, or variable number of args */
	  wrq.u.data.pointer = (caddr_t) buffer;
	  wrq.u.data.flags = subcmd;
	}
    }

  /* Perform the private ioctl */
  if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
    {
      fprintf(stderr, "Interface doesn't accept private ioctl...\n");
      fprintf(stderr, "%s (%X): %s\n", cmdname, priv[k].cmd, strerror(errno));
      return(-1);
    }

  /* If we have to get some data */
  if((priv[k].get_args & IW_PRIV_TYPE_MASK) &&
     (priv[k].get_args & IW_PRIV_SIZE_MASK))
    {
      int	j;
      int	n = 0;		/* number of args */

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
	    double		freq;
	    /* Display args */
	    for(j = 0; j < n; j++)
	      {
		freq = iw_freq2float(((struct iw_freq *) buffer) + j);
		if(freq >= GIGA)
		  printf("%gG  ", freq / GIGA);
		else
		  if(freq >= MEGA)
		  printf("%gM  ", freq / MEGA);
		else
		  printf("%gk  ", freq / KILO);
	      }
	    printf("\n");
	  }
	  break;
#endif

#if 0
	case IW_PRIV_TYPE_ADDR:
	  {
	    char		scratch[128];
	    struct sockaddr *	hwa;
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
    }	/* if args to set */

  return(0);
}

/*------------------------------------------------------------------*/
/*
 * Execute a private command on the interface
 */
static inline int
set_private(const char *ifname, /* Dev name */
            const char *cmd, 
            int count, /* Args count */
            ...)
{
  iwprivargs *priv;
  int	      number; /* Max of private ioctl */
  int         skfd = wifi_get_skfd();
  va_list     ap;
  int         rc;

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

#if 0
/**
 * Find private ioctl by its name.
 *
 * @param ifname      interface name
 * @param ioctl_name  ioctl name
 * @param priv        private ioctl info to be filled in (OUT)
 */
static int
wifi_ta_priv_find(const char *ifname, const char *ioctl_name,
                  iwprivargs *priv)
{
    iwprivargs *privs;
    int         i;
    int         num; /* Max of private ioctl */
    int         subcmd = 0; /* sub-ioctl index */
    int         offset = 0; /* Space for sub-ioctl index */
    int         skfd = wifi_get_skfd();

    WIFI_CHECK_SKFD(skfd);

    /* Read the private ioctls */
    num = iw_get_priv_info(skfd, ifname, &privs);

    /* Is there any ? */
    if(num <= 0)
    {
        /* Could skip this message ? */
        ERROR("There is no private ioctls available for the WiFi "
              "card on %s interface", ifname);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    for (i = 0; i < num; i++)
    {
        if (strcmp(privs[i].name, ioctl_name) == 0)
        {
            priv = &privs[i];
            break;
        }
    }
    
    if (i = num)
    {
        free(privs);
        ERROR("Cannot find private ioctl '%s' on %s interface",
              ioctl_name, ifname);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if(priv.cmd < SIOCDEVPRIVATE)
    {
        int j = -1;

        /* Find the matching *real* ioctl */
        while ((++j < num) &&
               ((priv[j].name[0] != '\0') ||
                (priv[j].set_args != priv->set_args) ||
                (priv[j].get_args != priv->get_args)));

        /* If not found... */
        if (j == num)
        {
            free(privs);
            ERROR("Invalid private ioctl definition for %s command",
            ioctl_name);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        /* Save sub-ioctl number */
        subcmd = priv[k].cmd;

        /* Reserve one int (simplify alignement issues) */
        offset = sizeof(__u32);

        /* Use real ioctl definition from now on */
        priv = &priv[j];
    }
    
    /*
     * Those two tests are important. They define how the driver
     * will have to handle the data
     */
    if ((priv->set_args & IW_PRIV_SIZE_FIXED) &&
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
	  /* Thirst case : args won't fit in wrq, or variable number of args */
	  wrq.u.data.pointer = (caddr_t) buffer;
	  wrq.u.data.flags = subcmd;
	}
    }

    free(privs);

    return 0;
}
#endif



/**
 * Returns configuration information about WiFi card.
 *
 * @param ifname  Wireless interface name
 * @param cfg     Configuration information (OUT)
 *
 * @return Status of the operation
 */
static int
wifi_get_config(const char *ifname, wireless_config *cfg)
{
    int skfd = wifi_get_skfd();

    WIFI_CHECK_SKFD(skfd);

    memset(cfg, 0, sizeof(*cfg));

    if (iw_get_basic_config(skfd, ifname, cfg) != 0)
    {
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    return 0;
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
    wireless_config cfg;
    int             rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = wifi_get_config(ifname, &cfg)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EOPNOTSUPP)
        {
            /* Interface does not support wireless extension */
            *list = strdup("");
            rc = 0;
        }

        return rc;
    }

    *list = strdup("enabled");
    return 0;
}

/**
 * Get WEP key value used on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value in format "XXXXXXXXXX" (5 bytes)
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_key_get(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    int             rc;
    wireless_config cfg;
    int             i;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = wifi_get_config(ifname, &cfg)) != 0)
        return rc;

    if (!cfg.has_key)
    {
        ERROR("Cannot get information about encryption "
              "on %s interface", ifname);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    value[0] = '\0';
    for (i = 0; i < cfg.key_size; i++)
    {
        sprintf(value + strlen(value), "%02x", cfg.key[i]);
    }

    return 0;
}

/**
 * Update WEP key value on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value in format "XXXXXXXXXX" (5 bytes)
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_key_set(unsigned int gid, const char *oid, char *value,
                 const char *ifname)
{
    int          rc;
    struct iwreq wrq;
    uint8_t      key[IW_ENCODING_TOKEN_MAX];
    int          keylen;
    int          skfd = wifi_get_skfd();
    char         alg_buf[128];
    char         wep_buf[128];
 
    UNUSED(gid);
    UNUSED(oid);

    WIFI_CHECK_SKFD(skfd);

    /* Before setting KEY save authentication algorithm and WEP setting */
    if ((rc = wifi_wep_get(gid, oid, wep_buf, ifname)) != 0 ||
        (rc = wifi_auth_get(gid, oid, alg_buf, ifname)) != 0)
    {
        ERROR("Cannot get current WEP and algorithm settings");
        return rc;
    }

    memset(&wrq, 0, sizeof(wrq));

    keylen = iw_in_key_full(skfd, ifname, value, key, &wrq.u.data.flags);
    if (keylen <= 0)
    {
        ERROR("Cannot set '%s' key on %s interface", value, ifname);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }
    wrq.u.data.length = keylen;
    wrq.u.data.pointer = (caddr_t)key;

    if ((rc = wifi_set_item(ifname, SIOCSIWENCODE, &wrq)) != 0)
        return rc;
    
    /* Restore the previous value for WEP and auth algorithm */
    if ((rc = wifi_wep_set(gid, oid, wep_buf, ifname)) != 0 ||
        (rc = wifi_auth_set(gid, oid, alg_buf, ifname)) != 0)
    {
        ERROR("Cannot restore WEP and algorithm settings");
    }
    
    return rc;
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
    int             rc;
    wireless_config cfg;
    
    UNUSED(gid);
    UNUSED(oid);

    if (priv_ioctl[TA_PRIV_IOCTL_PRIV_INVOKED].supp)
    {
        char rp_inv_buf[128];
        char ex_une_buf[128];

        if (set_private(ifname,
                        priv_ioctl[TA_PRIV_IOCTL_PRIV_INVOKED].g_name,
                        1, rp_inv_buf) != 0 ||
            set_private(ifname,
                        priv_ioctl[TA_PRIV_IOCTL_EXCLUDE_UNENCR].g_name,
                        1, &ex_une_buf) != 0)
        {
            ERROR("Cannot set WEP to %s ioctl", value);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }
        assert(strcmp(rp_inv_buf, ex_une_buf) == 0);
        
        strcpy(value, rp_inv_buf);
        return 0;
    }

    if ((rc = wifi_get_config(ifname, &cfg)) != 0)
        return rc;

    if (!cfg.has_key)
    {
        ERROR("Cannot get information about encryption "
              "on %s interface", ifname);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }
    if (cfg.key_flags & IW_ENCODE_DISABLED || cfg.key_size == 0)
        sprintf(value, "0");
    else
        sprintf(value, "1");

    return 0;
}

/**
 * Update the status of WEP on the wireless interface.
 *
 * @param gid     group identifier (unused)
 * @param oid     full object instence identifier
 * @param value   location for the value ("0" - false or "1" - true)
 * @param ifname  interface name
 *
 * @return error code
 */
static te_errno
wifi_wep_set(unsigned int gid, const char *oid, char *value,
             const char *ifname)
{
    struct iwreq wrq;

    UNUSED(gid);

    if (priv_ioctl[TA_PRIV_IOCTL_PRIV_INVOKED].supp)
    {
        int int_value;
        
        if (sscanf(value, "%d", &int_value) != 1)
        {
            ERROR("Incorrect value for WEP passed %s", value);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        assert(int_value == 1 || int_value == 0);
                
        if (set_private(ifname,
                        priv_ioctl[TA_PRIV_IOCTL_PRIV_INVOKED].s_name,
                        1, int_value) != 0 ||
            set_private(ifname,
                        priv_ioctl[TA_PRIV_IOCTL_EXCLUDE_UNENCR].s_name,
                        1, int_value) != 0)
        {
            ERROR("Cannot set WEP to %s ioctl", value);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        return 0;
    }

    memset(&wrq, 0, sizeof(wrq));

    if (strcmp(value, "0") == 0)
    {
        wrq.u.data.flags |= IW_ENCODE_DISABLED;
    }
    else if (strcmp(value, "1") != 0)
    {
        ERROR("Canot set '%s' instance to '%s'", oid, value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    wrq.u.data.flags |= IW_ENCODE_NOKEY;

    return wifi_set_item(ifname, SIOCSIWENCODE, &wrq);
}

static int
wifi_ta_get_auth_alg(const char *ifname, ta_auth_alg_e *alg)
{
    int             rc;
    wireless_config cfg;

    if (priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].supp)
    {
        unsigned int *algs = priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].data;
        char          buf[128];
        unsigned int  int_alg;
        int           i;

        rc = set_private(ifname,
                         priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].g_name,
                         1, buf);
        if (rc != 0)
        {
            ERROR("Cannot get the value of %s ioctl",
                  priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].g_name);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        if (sscanf(buf, "%u", &int_alg) != 1)
        {
            ERROR("Cannot convert algorithm %s", buf);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }
        
        for (i = 0; i < (int)TA_AUTH_ALG_MAX; i++)
        {
            if (algs[i] == int_alg)
            {
                *alg = (ta_auth_alg_e)i;
                return 0;
            }
        }

        ERROR("Cannot find mapping for %d authentication algorithm",
              int_alg);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    /*
     * There is no private ioctl for authentication algorithm value,
     * so process it in generic way.
     */

    if ((rc = wifi_get_config(ifname, &cfg)) != 0)
        return rc;

    if (!cfg.has_key)
    {
        ERROR("Cannot get information about encryption "
              "on %s interface", ifname);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }
    if (cfg.key_flags & IW_ENCODE_RESTRICTED)
    {
        *alg = TA_AUTH_ALG_SHARED_KEY;
    }
    else
    {
        *alg = TA_AUTH_ALG_OPEN_SYSTEM;

        if (!(cfg.key_flags & IW_ENCODE_DISABLED) && 
            !(cfg.key_flags & IW_ENCODE_OPEN))
        {
            WARN("Although authentication algorithm is not sharedKey, "
                 "WiFi card does not set set IW_ENCODE_DISABLED, nor "
                 "IW_ENCODE_OPEN flag.");
        }
    }

    return 0;
}

static int
wifi_ta_set_auth_alg(const char *ifname, ta_auth_alg_e alg)
{
    int             rc;
    struct iwreq    wrq;

    if (priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].supp)
    {
        enum ta_auth_alg_e *algs = priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].data;

        rc = set_private(ifname,
                         priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].s_name,
                         1, algs[alg]);
        if (rc != 0)
        {
            ERROR("Cannot set the value of %s ioctl",
                  priv_ioctl[TA_PRIV_IOCTL_AUTH_ALG].s_name);
            return TE_RC(TE_TA_UNIX, TE_EFAULT);
        }

        return 0;
    }

    /*
     * There is no private ioctl for authentication algorithm value,
     * so process it in generic way.
     */

    memset(&wrq, 0, sizeof(wrq));

    if (alg == TA_AUTH_ALG_OPEN_SYSTEM)
    {
        wrq.u.data.flags |= IW_ENCODE_OPEN;
    }
    else if (alg == TA_AUTH_ALG_SHARED_KEY)
    {
        wrq.u.data.flags |= IW_ENCODE_RESTRICTED;
    }
    else
    {
        ERROR("Canot set authentication algorithm to '%u'", alg);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    wrq.u.data.flags |= IW_ENCODE_NOKEY;

    return wifi_set_item(ifname, SIOCSIWENCODE, &wrq);
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
    enum ta_auth_alg_e alg;
    int                rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = wifi_ta_get_auth_alg(ifname, &alg)) != 0)
        return rc;

    if (alg == TA_AUTH_ALG_OPEN_SYSTEM)
    {
        sprintf(value, "open");
    }
    else if (alg == TA_AUTH_ALG_SHARED_KEY)
    {
        sprintf(value, "shared");
    }
    else
    {
        ERROR("WiFi card works with unknown authentication algorithm");
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }
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
    UNUSED(gid);
    UNUSED(oid);

    if (strcmp(value, "open") == 0)
    {
        return wifi_ta_set_auth_alg(ifname, TA_AUTH_ALG_OPEN_SYSTEM);
    }
    else if (strcmp(value, "shared") == 0)
    {
        return wifi_ta_set_auth_alg(ifname, TA_AUTH_ALG_SHARED_KEY);
    }
    else
    {
        ERROR("Canot set authentication algorithm to '%s'", value);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    assert(0);
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
    int             rc;
    wireless_config cfg;
    
    UNUSED(gid);
    UNUSED(oid);

    if ((rc = wifi_get_config(ifname, &cfg)) != 0)
        return rc;

    sprintf(value, "%s", cfg.has_essid ? cfg.essid : "");

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

    return wifi_set_item(ifname, SIOCSIWESSID, &wrq);
}


/* Unix Test Agent WiFi configuration subtree */
RCF_PCH_CFG_NODE_RW(node_wifi_wep_key, "key", NULL, NULL,
                    wifi_wep_key_get, wifi_wep_key_set);

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
