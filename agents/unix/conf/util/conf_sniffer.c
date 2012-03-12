/** @file
 * @brief Unix Test Agent sniffers support.
 *
 * Implementation of unix TA sniffers configuring support.
 *
 * Copyright (C) 2004-2011 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: 
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Conf Sniffers"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <ftw.h>

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "te_sockaddr.h"
#include "cs_common.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "unix_internal.h"
#include "te_queue.h"
#include "conf_sniffer.h"
#include "te_sleep.h"
#include "te_sniffers.h"
#include "te_sniffer_proc.h"

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif

/* Default constants */
#define SNIFFER_LIST_SIZE           1024
#define SNIFFER_AGENT_TOTAL_SIZE    256
#define SNIFFER_SPACE               64
#define SNIFFER_ROTATION            4
#define SNIFFER_FILE_SIZE           16
#define SNIFFER_PATH                "/tmp/"
#define SNIFFER_MAX_ARGS_N          25
#define SNIFFER_SSN_F               "next_sniffer_ssn"
#define SNIFFER_MAX_ID_LEN          600
#define SNIFFER_MAX_LOG_SIZE        2147483647

/* Sniffer states constant */
#define SNIF_ST_START 0x01 /* Sniffer was launched */
#define SNIF_ST_HAS_L 0x02 /* Sniffer has capture logs */
#define SNIF_ST_DEL   0x04 /* Sniffer was put to the removal */

/** Temporary executable name of the sniffer process */
#define SNIFFER_EXEC "te_sniffer_process"

/* Size of a PCAP file header. */
#define SNIF_PCAP_HSIZE 24

#define SNIFFER_MALLOC(ptr, size)       \
    if ((ptr = malloc(size)) == NULL)   \
        assert(0);

static te_errno sniffer_add(unsigned int gid, const char *oid, char *ssn,
                            const char *ifname, const char *snifname);

/** Overfill handle methods constans. */
typedef enum overfill_meth_t {
    ROTATION   = 0, /**< Overfill type rotation. */
    TAIL_DROP  = 1, /**< Overfill type tail drop. */
} overfill_meth_t;

/** Structure for the common sniffers settings. */
typedef struct snif_sets_t {
    te_bool             enable;
    char                agt_id[RCF_PCH_MAX_ID_LEN];
    char               *filter_exp_str;
    char               *filter_exp_file;
    int                 snaplen;
    char                ta_path[RCF_MAX_PATH];   /**< Agent-specific folder. */
    char                path[RCF_MAX_PATH];      /**< Sniffers folder. */
    char                ssn_fname[RCF_MAX_PATH]; /**< ssn file path and name. */
    size_t              total_size;
    size_t              file_size;
    int                 rotation;
    overfill_meth_t     overfill_meth;
    int                 ssn;                /**< Sniffer session sequence 
                                                 number. */
    te_bool             lock;
} snif_sets_t;

/** List of the personal sniffer settings. */
typedef struct sniffer_t {
    SLIST_ENTRY(sniffer_t)  ent_l;
    te_bool                 enable;
    sniffer_id              id;
    char                   *filter_exp_str;
    char                   *filter_exp_file;
    int                     snaplen;
    size_t                  sniffer_space;
    size_t                  file_size;
    size_t                  rotation;
    overfill_meth_t         overfill_meth;
    pid_t                   pid;

    char                    path[RCF_MAX_PATH];
    char                   *curr_file_name;
    long                    curr_offset;
    unsigned char           state;
} sniffer_t;

static snif_sets_t          snif_sets;

/**< Head of the sniffer list */
SLIST_HEAD(, sniffer_t)     snifferl_h;

/**
 * Callback function to clean up directory with sniffers logs.
 */
static int
clean_pcap(const char *fpath, const struct stat *sb, int typeflag,
              struct FTW *ftwbuf)
{
    UNUSED(sb);
    UNUSED(typeflag);
    UNUSED(ftwbuf);

    if (strstr(fpath, ".pcap") != NULL)
        return remove(fpath);

    return 0;
}

/**
 * Get ssn from the agent ssn file.
 * 
 * @param fname     The agent ssn file name.
 * 
 * @return Status code.
 */
static te_errno
sniffer_get_ssn_ff(const char *fname)
{
    FILE    *f;
    int      res;
    int      nssn;

    errno = 0;
    f = fopen(fname, "r+");
    if ((f == NULL) && (errno == ENOENT))
    {
        errno = 0;
        if (mkdir(snif_sets.ta_path, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && 
            errno != EEXIST)
        {
            ERROR("Couldn't create directory, %d\n", errno);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        f = fopen(fname, "w");
        if (f == NULL)
        {
            ERROR("Couldn't create the agent ssn file: %s", strerror(errno));
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        snif_sets.ssn = 0;
        nssn = 1;
        if (fwrite(&nssn, sizeof(int), 1, f) != 1)
            WARN("Wrong write to the agent ssn file");
        fclose(f);
        return 0;
    }
    else if (f == NULL)
    {
        ERROR("Couldn't open the agent ssn file: %s", strerror(errno));
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    res = fread(&nssn, sizeof(int), 1, f);
    if (res != 1)
    {
        fclose(f);
        ERROR("Couldn't read from the agent ssn file");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    snif_sets.ssn = nssn;
    nssn++;
    fseek(f, 0, SEEK_SET);
    res = fwrite(&nssn, sizeof(int), 1, f);
    fclose(f);
    if (res != 1)
        WARN("Couldn't write to the agent ssn file");

    return 0;
}

/**
 * Agent identifier, specific folder and ssn initialization.
 * 
 * @return Status code.
 */
static te_errno
sniffer_agent_id_init(void)
{
    int      res;

    rcf_pch_get_id(snif_sets.agt_id);
    if (strlen(snif_sets.agt_id) == 0)
    {
        ERROR("Can't get RCF session ID");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    res = snprintf(snif_sets.ta_path, RCF_MAX_PATH, "%s%s", SNIFFER_PATH,
                   snif_sets.agt_id);
    if (res > RCF_MAX_PATH)
    {
        ERROR("Sniffers path is too long.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    strcpy(snif_sets.path, snif_sets.ta_path);

    res = snprintf(snif_sets.ssn_fname, RCF_MAX_PATH, "%s/%s_%s", 
                   snif_sets.ta_path, ta_name, SNIFFER_SSN_F);
    if (res > RCF_MAX_PATH)
    {
        ERROR("Too long agent ssn file name");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

/**
 * Default initialization of the sniffers settings.
 * 
 * @return Status code.
 */
static te_errno
sniffer_settings_init(void)
{
    te_errno rc;

    snif_sets.ssn               = 0;
    snif_sets.enable            = FALSE;
    snif_sets.lock              = FALSE;
    snif_sets.snaplen           = 0;
    snif_sets.total_size        = SNIFFER_AGENT_TOTAL_SIZE;
    snif_sets.file_size         = SNIFFER_FILE_SIZE;
    snif_sets.overfill_meth     = ROTATION;
    snif_sets.rotation          = SNIFFER_ROTATION;
    snif_sets.filter_exp_str    = strdup("");
    snif_sets.filter_exp_file   = strdup("");

    SLIST_INIT(&snifferl_h);

    memset(snif_sets.agt_id, 0, RCF_PCH_MAX_ID_LEN);
    memset(snif_sets.path, 0, RCF_MAX_PATH);
    memset(snif_sets.ssn_fname, 0, RCF_MAX_PATH);

    rc = sniffer_agent_id_init();

    return rc;
}

/**
 * Free memory and remove sniffer from the list.
 * 
 * @param sniff     The sniffer location.
 */
static void
sniffer_cleanup(sniffer_t *sniff)
{
    if (sniff == NULL)
        return;
    if (sniff->id.snifname != NULL)
        free(sniff->id.snifname);
    if (sniff->id.ifname != NULL)
        free(sniff->id.ifname);
    if (sniff->curr_file_name != NULL)
        free(sniff->curr_file_name);

    /* Clean up the sniffer directory */
    if (strlen(sniff->path) > 0)
    {
        nftw(sniff->path, clean_pcap, 64, FTW_DEPTH | FTW_PHYS);
        remove(sniff->path);
    }

    SLIST_REMOVE(&snifferl_h, sniff, sniffer_t, ent_l);
    free(sniff);
}

/**
 * Searching for the sniffer by snifname and ifname.
 * 
 * @param ifname        Interface name.
 * @param snifname      The sniffer name.
 * 
 * @return The sniffer unit or NULL.
 */
static sniffer_t *
sniffer_find(const char *ifname, const char *snifname)
{
    sniffer_t *sniff;

    if ((ifname == NULL) || (snifname == NULL))
        return NULL;

    SLIST_FOREACH(sniff, &snifferl_h, ent_l)
    {
        if ((strcmp(sniff->id.snifname, snifname) == 0) &&
            (strcmp(sniff->id.ifname, ifname) == 0) &&
            ((sniff->state & SNIF_ST_DEL) != SNIF_ST_DEL))
                return sniff;
    }
    return NULL;
}

/**
 * Common get function for the sniffers settings.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier.
 * @param value        Obtained value (Out).
 *
 * @return Status code.
 */
static te_errno
sniffer_get_params(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);

    if (value == NULL)
    {
       ERROR("A buffer for sniffers settings to get val is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/enable:") != NULL)
        sprintf(value, "%d", snif_sets.enable);
    else if (strstr(oid, "/snaplen:") != NULL)
        sprintf(value, "%d", snif_sets.snaplen);
    else if (strstr(oid, "/total_size:") != NULL)
        sprintf(value, "%d", snif_sets.total_size);
    else if (strstr(oid, "/file_size:") != NULL)
        sprintf(value, "%d", snif_sets.file_size);
    else if (strstr(oid, "/rotation:") != NULL)
        sprintf(value, "%d", snif_sets.rotation);
    else if (strstr(oid, "/overfill_meth:") != NULL)
        sprintf(value, "%d", snif_sets.overfill_meth);
    else if (strstr(oid, "/path:") != NULL)
        sprintf(value, "%s", snif_sets.path);
    else if ((strstr(oid, "/filter_exp_str:") != NULL) && 
             (snif_sets.filter_exp_str != NULL))
        sprintf(value, "%s", snif_sets.filter_exp_str);
    else if ((strstr(oid, "/filter_exp_file:") != NULL) && 
             (snif_sets.filter_exp_str != NULL))
        sprintf(value, "%s", snif_sets.filter_exp_file);

    return 0;
}

/**
 * Common set function for the sniffers settings.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier.
 * @param value        New value.
 *
 * @return Status code.
 */
static te_errno
sniffer_set_params(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);

    if (value == NULL)
    {
       ERROR("A buffer for sniffer settings to set val is not provided.");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* After sniffers start common setting should not be changed. */
    if (snif_sets.lock == TRUE)
        return 0;

    if (strstr(oid, "/enable:") != NULL)
    {
        snif_sets.enable = atoi(value) ? TRUE : FALSE;
        if (snif_sets.enable)
            snif_sets.lock = TRUE;
    }
    else if (strstr(oid, "/snaplen:") != NULL)
        snif_sets.snaplen = atoi(value);
    else if (strstr(oid, "/total_size:") != NULL)
        snif_sets.total_size = (size_t)atoi(value);
    else if (strstr(oid, "/file_size:") != NULL)
        snif_sets.file_size = (size_t)atoi(value);
    else if (strstr(oid, "/rotation:") != NULL)
        snif_sets.rotation = atoi(value);
    else if (strstr(oid, "/overfill_meth:") != NULL)
    {
        if (atoi(value) == 0)
            snif_sets.overfill_meth = ROTATION;
        else
            snif_sets.overfill_meth = TAIL_DROP;
    }
    else if (strstr(oid, "/path:") != NULL)
    {
        strncpy(snif_sets.path, value, RCF_MAX_PATH);
    }
    else if (strstr(oid, "/filter_exp_str:") != NULL)
    {
        if (snif_sets.filter_exp_str != NULL)
            free(snif_sets.filter_exp_str);
        if ((snif_sets.filter_exp_str = strdup(value)) == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    else if (strstr(oid, "/filter_exp_file:") != NULL)
    {
        if (snif_sets.filter_exp_file != NULL)
            free(snif_sets.filter_exp_file);
        if ((snif_sets.filter_exp_file = strdup(value)) == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    return 0;
}

/**
 * Common get function for the sniffer instance.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param value         New value.
 * @param ifname        The interface name.
 * @param snifname      The sniffer name.
 *
 * @return Status code.
 */
static te_errno
sniffer_common_get(unsigned int gid, const char *oid, char *value,
                   const char *ifname, const char *snifname)
{
    UNUSED(gid);

    sniffer_t   *sniff = NULL;

    sniff = sniffer_find(ifname, snifname);
    if (sniff == NULL)
    {
        ERROR("sniffer_common_get: Couldn't find the sniffer for oid %s", oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (value == NULL)
    {
       ERROR("A buffer for sniffer common get is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/enable:") != NULL)
        sprintf(value, "%d", sniff->enable);
    else if (strstr(oid, "/snaplen:") != NULL)
        sprintf(value, "%d", sniff->snaplen);
    else if (strstr(oid, "/sniffer_space:") != NULL)
        sprintf(value, "%d", sniff->sniffer_space);
    else if (strstr(oid, "/file_size:") != NULL)
        sprintf(value, "%d", sniff->file_size);
    else if (strstr(oid, "/rotation:") != NULL)
        sprintf(value, "%d", sniff->rotation);
    else if (strstr(oid, "/overfill_meth:") != NULL)
        sprintf(value, "%d", sniff->overfill_meth);
    else if ((strstr(oid, "/filter_exp_str:") != NULL) && 
             (sniff->filter_exp_str != NULL))
        sprintf(value, "%s", sniff->filter_exp_str);
    else if ((strstr(oid, "/filter_exp_file:") != NULL) && 
             (sniff->filter_exp_str != NULL))
        sprintf(value, "%s", sniff->filter_exp_file);

    return 0;
}

/**
 * Common set function for the sniffer instance.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param value         New value.
 * @param ifname        Interface name.
 * @param snifname      Sniffer name.
 *
 * @return Status code.
 */
static te_errno
sniffer_common_set(unsigned int gid, const char *oid, const char *value,
               const char *ifname, const char *snifname)
{
    sniffer_t   *sniff;

    UNUSED(gid);

    sniff = sniffer_find(ifname, snifname);
    if (sniff == NULL)
    {
        ERROR("Couldn't find the sniffer on the oid %s", oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (value == NULL)
    {
       ERROR("A buffer for sniffer common set is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (sniff->enable == TRUE)
    {
       WARN("The sniffer has been started.");
       return TE_RC(TE_TA_UNIX, TE_EBUSY);
    }

    if (strstr(oid, "/snaplen:") != NULL)
        sniff->snaplen = atoi(value);
    else if (strstr(oid, "/sniffer_space:") != NULL)
        sniff->sniffer_space = (size_t)atoi(value);
    else if (strstr(oid, "/file_size:") != NULL)
        sniff->file_size = (size_t)atoi(value);
    else if (strstr(oid, "/rotation:") != NULL)
        sniff->rotation = atoi(value);
    else if (strstr(oid, "/overfill_meth:") != NULL)
    {
        if (atoi(value) == 0)
            sniff->overfill_meth = ROTATION;
        else
            sniff->overfill_meth = TAIL_DROP;
    }
    else if (strstr(oid, "/filter_exp_str:") != NULL)
    {
        if (sniff->filter_exp_str != NULL)
            free(sniff->filter_exp_str);
        if ((sniff->filter_exp_str = strdup(value)) == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    else if (strstr(oid, "/filter_exp_file:") != NULL)
    {
        if (sniff->filter_exp_file != NULL)
            free(sniff->filter_exp_file);
        if ((sniff->filter_exp_file = strdup(value)) == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    return 0;
}

/**
 * Parse the buffer with sniffer ID.
 * 
 * @param buf   The buffer with sniffer ID.
 * @param id    The sniffer ID (OUT).
 * 
 * return Status code.
 */
static te_errno
sniffer_parse_sniff_id(const char* buf, sniffer_id *id)
{
    unsigned     strl;
    char        *ptr;

/**
 * Skip spaces in the command.
 *
 * @param _ptr  - pointer to string
 */
#define SNIF_SK_SPS(_ptr)       \
        while (*(_ptr) == ' ')  \
            (_ptr)++;

    if (buf == NULL)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    SNIF_SK_SPS(buf);
    ptr = strchr(buf, ' ');
    if (ptr == NULL)
    {
        WARN("Wrong sniffer name in the sniffer id.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    strl = (unsigned)ptr - (unsigned)buf;
    id->snifname = strndup(buf, strl);
    buf = ptr;

    SNIF_SK_SPS(buf);
    ptr = strchr(buf, ' ');
    if (ptr == NULL)
    {
        WARN("Wrong sniffer interface name in the sniffer id.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    strl = (unsigned)ptr - (unsigned)buf;
    id->ifname = strndup(buf, strl);
    buf = ptr;

    SNIF_SK_SPS(buf);
    id->ssn = strtoll(buf, &ptr, 10);
    if (buf == ptr)
    {
        WARN("Wrong sniffer SSN in the sniffer id.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

/**
 * Searching for the sniffer by sniffer ID.
 * 
 * @param id        The sniffer ID.
 * 
 * @return The sniffer unit.
 */
static sniffer_t *
sniffer_find_by_id(sniffer_id *id)
{
    sniffer_t *sniff = NULL;

    if (SLIST_EMPTY(&snifferl_h))
        return NULL;

    SLIST_FOREACH(sniff, &snifferl_h, ent_l)
    {
        if ((strcmp(sniff->id.snifname, id->snifname) == 0) &&
            (strcmp(sniff->id.ifname, id->ifname) == 0) &&
            (sniff->id.ssn == id->ssn))
                return sniff;
    }
    return NULL;
}

/**
 * Get the name of the oldest/newest capture log file of the sniffer.
 * 
 * @param sniff     The sniffer pointer.
 * @param fnum      The number of capture log files (OUT).
 * @param wp_fname  Name of the oldest capture log file without path (OUT).
 * @param newest    If the newest flag is true search for newest capture file,
 *                  else - oldest.
 * 
 * @return Name of the result capture log file or NULL in case of failure.
 */
static char *
sniffer_get_capture_fname(sniffer_t *snif, int *fnum, char **wp_fname,
                          te_bool newest)
{
    DIR           *dir;
    struct dirent *ent;
    char          *fname    = NULL;
    char          *resname  = NULL;
    te_bool        fl       = TRUE;
    int            res;

    *fnum       = 0;
    *wp_fname   = NULL;

    if (strlen(snif->path) == 0)
        return NULL;

    dir = opendir(snif->path);
    if (dir == NULL)
    {
        WARN("Couldn't open the sniffer directory.");
        return NULL;
    }

    /* View capture logs file list and chose the oldest on newest. */
    while ((ent = readdir(dir)) != NULL)
    {
        if ((strcmp(ent->d_name, ".") == 0) ||
            (strcmp(ent->d_name, "..") == 0))
            continue;

        *fnum += 1;
        if (fl)
        {
            if((fname = strdup(ent->d_name)) == NULL)
                return NULL;
            fl = FALSE;
            continue;
        }

        if (((strcmp(fname, ent->d_name) > 0) && !newest) ||
            ((strcmp(fname, ent->d_name) < 0) && newest))
        {
            free(fname);
            if((fname = strdup(ent->d_name)) == NULL)
                return NULL;
        }
    }

    closedir(dir);

    if (fname != NULL)
    {
        *wp_fname = fname;
        SNIFFER_MALLOC(resname, RCF_MAX_PATH);
        res = snprintf(resname, RCF_MAX_PATH, "%s/%s", snif->path, fname);
        if (res > RCF_MAX_PATH)
        {
            WARN("Too long file name string for sniffer cpture file.");
            free(resname);
            return NULL;
        }
    }

    return resname;
}

/**
 * Get absolute offset of the last capture packet. Update the state field of
 * the sniffer.
 * 
 * @param sniff_id_str  A string with the sniffer id.
 * @param snif_arg      Location of the sniffer, if not NULL excludes the
 *                      sniff_id_str param.
 * @param offset        Absolute offset of the last capture packet (OUT).
 * 
 * @return Status code. Zero is correct.
 */
static te_errno
sniffer_get_curr_offset(const char *sniff_id_str, void *snif_arg,
                        unsigned long long *offset)
{
    sniffer_id       id;
    int              rc;
    char            *fname;
    struct flock     lock;
    int              fd;
    sniffer_t       *snif;
    int              fnum;
    char            *wp_fname = NULL;
    long             res;

/* FIXME: Add call to the sniffer process to flush packets. */
    *offset = 0;
    if (snif_arg == NULL)
    {
        if ((rc = sniffer_parse_sniff_id(sniff_id_str, &id)) != 0)
            return rc;
        if ((snif = sniffer_find_by_id(&id)) == NULL)
        {
            WARN("Couldn't find the sniffer to get offset.");
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    else
        snif = (sniffer_t *)snif_arg;

    fname = sniffer_get_capture_fname(snif, &fnum, &wp_fname, TRUE);
    if (fname == NULL)
    {
        snif->state &= ~SNIF_ST_HAS_L;
        return 0;
    }

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    fd = open(fname, O_RDWR);
    if (fd == -1)
    {
        WARN("Couldn't open the capture log file: %s.", fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = fcntl(fd, F_SETLKW, &lock);
    if (rc != 0)
    {
        WARN("Couldn't to lock the capture log file: %s.", fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    res = lseek(fd, 0, SEEK_END);
    if (res <= 0)
        *offset = 0;
    else
        *offset = res - SNIF_PCAP_HSIZE;
    lock.l_type = F_UNLCK;
    rc = fcntl(fd, F_SETLK, &lock);
    if (rc != 0)
    {
        WARN("Couldn't to unlock the capture log file: %s.", fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    close(fd);

    *offset += strtoll(wp_fname, NULL, 10);

    if (wp_fname != NULL)
        free(wp_fname);
    if (fname != NULL)
        free(fname);

    if (*offset > snif->id.abs_offset)
        snif->state |= SNIF_ST_HAS_L;
    else 
        snif->state &= ~SNIF_ST_HAS_L;
    return 0;
}


/**
 * Get agent sniffers list located in a buffer.
 * Buffer format for each sniffer:
 *     <Sniffer name> <Interface name> <SSN> <offset>\0
 * 
 * @param buf       Pointer to the buffer with the list of sniffers.
 * 
 * @return The buffer length or zero if the buffer is empty.
 */
static size_t
sniffer_get_list_buf(char **buf, te_bool sync)
{
    sniffer_t           *sniff;
    int                  str_len;
    size_t               clen;
    int                  mem_size   = 1024;
    unsigned long long   offset     = 0;

    SNIFFER_MALLOC(*buf, mem_size);
    memset(*buf, 0, mem_size);

    clen = 0;
    SLIST_FOREACH(sniff, &snifferl_h, ent_l)
    {
        if (sniff->id.snifname == NULL || sniff->id.ifname == NULL ||
            (sniff->state & SNIF_ST_START) != SNIF_ST_START)
            continue;
        if (sync)
        {
            /* Update the sniffer state. */
            if (sniffer_get_curr_offset(NULL, sniff, &offset) != 0)
                continue;
        }
        if (sync == FALSE || (sniff->state & SNIF_ST_HAS_L) == SNIF_ST_HAS_L)
        {
            str_len = snprintf(*buf + clen, mem_size - clen, "%s %s %u %llu",
                               sniff->id.snifname, sniff->id.ifname, sniff->id.ssn,
                               offset);
            if (str_len > mem_size - clen)
            {
                mem_size = mem_size * 2;
                if ((*buf = realloc(*buf, mem_size)) == NULL)
                    assert(0);
                memset(*buf + clen, 0, mem_size/2);
                str_len = snprintf(*buf + clen, mem_size - clen, "%s %s %u %llu",
                                   sniff->id.snifname, sniff->id.ifname,
                                   sniff->id.ssn, offset);
            }
            clen += str_len + 1;
        }
    }
    return clen;
}

/**
 * Get sniffer capture log file.
 * 
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param buf           Buffer with the sniffer ID.
 * 
 * return Status code.
 */
static te_errno
sniffer_get_dump(struct rcf_comm_connection *handle, char *cbuf, 
                    size_t buflen, size_t answer_plen, const char *buf)
{
    int              rc;
    int              res;
    sniffer_id       id;
    sniffer_t       *snif;
    char            *fname;
    struct flock     lock;
    int              fd;
    off_t            offst;
    int              fnum;
    size_t           size;
    char            *wp_fname;
    size_t           len            = answer_plen;
    te_bool          first_launch   = FALSE;

    if ((rc = sniffer_parse_sniff_id(buf, &id)) != 0)
        return rc;
    if ((snif = sniffer_find_by_id(&id)) == NULL)
    {
        WARN("Couldn't find the sniffer to get capture logs.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    fname = sniffer_get_capture_fname(snif, &fnum, &wp_fname, FALSE);
    if (fname == NULL)
    {
        snif->state &= ~SNIF_ST_HAS_L;
        if ((snif->state & SNIF_ST_DEL) == SNIF_ST_DEL)
            sniffer_cleanup(snif);
        return TE_RC(TE_TA_UNIX, TE_ENODATA);
    }

    lock.l_type     = F_WRLCK;
    lock.l_whence   = SEEK_SET;
    lock.l_start    = 0;
    lock.l_len      = 0;

    fd = open(fname, O_RDWR);
    if (fd == -1)
    {
        WARN("Couldn't open the capture log file: %s.", fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = fcntl(fd, F_SETLKW, &lock);
    if (rc != 0)
    {
        WARN("Couldn't to lock the capture log file: %s.", fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    offst = lseek(fd, 0, SEEK_END);
    lock.l_type = F_UNLCK;
    rc = fcntl(fd, F_SETLK, &lock);
    if (rc != 0)
    {
        WARN("Couldn't to unlock the capture log file: %s.", fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Check for the first log transmission. */
    if (snif->curr_file_name == NULL)
    {
        snif->curr_file_name = fname;
        first_launch = TRUE;
    }
    /* Check the current file still exists. */
    else if (strcmp(snif->curr_file_name, fname) != 0)
    {
        free(snif->curr_file_name);
        snif->curr_file_name = fname;
        snif->curr_offset = SNIF_PCAP_HSIZE;
        snif->id.abs_offset = strtoll(wp_fname, NULL, 10);
    }

    size = offst - snif->curr_offset;
    if (size == 0)
    {
        close(fd);
        snif->state &= ~SNIF_ST_HAS_L;
        return TE_RC(TE_TA_UNIX, TE_ENODATA);
    }
    if (size > SNIFFER_MAX_LOG_SIZE)
    {
        size = SNIFFER_MAX_LOG_SIZE;
        offst = snif->curr_offset + SNIFFER_MAX_LOG_SIZE;
    }

    if (lseek(fd, snif->curr_offset, SEEK_SET) == -1)
    {
        WARN("Couldn't to change offset of the capture log file: %s.", fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    len += snprintf(cbuf + answer_plen, buflen - answer_plen,
                    "0 %llu attach %u", snif->id.abs_offset, size) + 1;
    if (len > buflen)
    {
        WARN("Too long rcf message. File is not passed.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    errno = 0;
    RCF_CH_LOCK;
    rc = rcf_comm_agent_reply(handle, cbuf, len);
    if (rc == 0)
    {
        res = sendfile(*((int *)handle), fd, 0, size);
        /* If data transmission failed, lock the file and try again. */
        if ((size_t)res != size)
        {
            if (res == -1)
                res = 0;
            lock.l_type = F_WRLCK;
            rc = fcntl(fd, F_SETLKW, &lock);
            if (rc != 0)
            {
                RCF_CH_UNLOCK;
                fprintf(stderr, "Couldn't to lock the capture log file:"
                        "%s. rc %d\n", fname, rc);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
            res = sendfile(*((int *)handle), fd, 0, size - res);
            lock.l_type = F_UNLCK;
            rc = fcntl(fd, F_SETLK, &lock);
            if (rc != 0)
            {
                RCF_CH_UNLOCK;
                fprintf(stderr, "Couldn't to unlock the capture log file:"
                        "%s. rc %d\n", fname, rc);
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
        }
    }
    RCF_CH_UNLOCK;

    snif->curr_offset += size;
    snif->id.abs_offset += size;
    if (first_launch)
        snif->id.abs_offset -= SNIF_PCAP_HSIZE;
    snif->state &= ~SNIF_ST_HAS_L;

    /* Remove the fully passed file. */
    if ((fnum > 1) || ((snif->state & SNIF_ST_DEL) == SNIF_ST_DEL))
        remove(snif->curr_file_name);

    if (fnum > 1)
        snif->state |= SNIF_ST_HAS_L;
    else if ((snif->state & SNIF_ST_DEL) == SNIF_ST_DEL)
        sniffer_cleanup(snif);

    close(fd);
    if (wp_fname != NULL)
        free(wp_fname);
    return 0;
}

/**
 * Make argv string to start the sniffer process.
 * 
 * @param sniff     Sniffer configuration structure.
 * @param s_argc    Number of arguments (OUT).
 * 
 * @return The argv arguments set or NULL.
 */
static char **
make_argv_str(sniffer_t *sniff, int *s_argc)
{
    char      **s_argv;
    char       *int_buff;
    int         intb_len    = 100;

/**
 * Push string argument to s_argv massive.
 * 
 * @param str       The string argument.
 */
#define PUSH_ARG(str)                               \
    if ((s_argv[*s_argc] = strdup(str)) == NULL)    \
        goto cleanup_m_argv;                        \
    *s_argc += 1;

/**
 * Push int argument to s_argv massive.
 * 
 * @param val       The int argument.
 */
#define PUSH_INT_ARG(val)                                       \
    if (snprintf(int_buff, intb_len, "%u", val) > intb_len)     \
        goto cleanup_m_argv;                                    \
    if ((s_argv[*s_argc] = strdup(int_buff)) == NULL)           \
        goto cleanup_m_argv;                                    \
    *s_argc += 1;

    SNIFFER_MALLOC(int_buff, intb_len);
    SNIFFER_MALLOC(s_argv, SNIFFER_MAX_ARGS_N * sizeof(char*));

    *s_argc = 0;
    PUSH_ARG(SNIFFER_EXEC);
    PUSH_ARG("-i");
    PUSH_ARG(sniff->id.ifname);
    PUSH_ARG("-s");
    PUSH_INT_ARG(sniff->snaplen);
    PUSH_ARG("-f");
    PUSH_ARG(sniff->filter_exp_str);
    PUSH_ARG("-P");
    PUSH_ARG(sniff->path);
    PUSH_ARG("-c");
    PUSH_INT_ARG(sniff->sniffer_space);
    PUSH_ARG("-C");
    PUSH_INT_ARG(sniff->file_size);
    PUSH_ARG("-q");
    PUSH_INT_ARG(sniff->id.ssn);
    PUSH_ARG("-a");
    PUSH_ARG(sniff->id.snifname);
    if (sniff->overfill_meth == TAIL_DROP)
    {
        PUSH_ARG("-o");
    }
    else
    {
        PUSH_ARG("-r");
        PUSH_INT_ARG(sniff->rotation);
    }
    PUSH_ARG("-p");
    while (*s_argc < SNIFFER_MAX_ARGS_N)
    {
        s_argv[*s_argc] = NULL;
        *s_argc += 1;
    }

    return s_argv;
cleanup_m_argv:
    ERROR("Filed make_argv_str");
    while (*s_argc != 0)
    {
        *s_argc -= 1;
        free(s_argv[*s_argc]);
    }
    free(int_buff);
    free(s_argv);
    return NULL;
#undef PUSH_ARG
#undef PUSH_INT_ARG
}

/**
 * Make a directory for capture logs of the sniffer.
 * 
 * @param sniff      The sniffer location.
 * 
 * @return Status code. Zero is corrected.
 */
static te_errno
sniffer_make_dir(sniffer_t *sniff)
{
    int     res;
    char    sniffers_dir[RCF_MAX_PATH];

    errno = 0;
    if (mkdir(snif_sets.path, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && 
        errno != EEXIST)
    {
        ERROR("Couldn't create directory, %d\n", errno);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    res = snprintf(sniffers_dir, RCF_MAX_PATH, "%s/sniffers",
                   snif_sets.path);
    if (res > RCF_MAX_PATH)
    {
        ERROR("Too long directory name, %d\n", errno);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    errno = 0;
    if (mkdir(sniffers_dir, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && 
        errno != EEXIST)
    {
        ERROR("Couldn't create directory, %d\n", errno);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    /* Make the directory name and create it. */
    res = snprintf(sniff->path, RCF_MAX_PATH, "%s/%s_%s_%s_%u/", 
                   sniffers_dir, ta_name, sniff->id.ifname,
                   sniff->id.snifname, sniff->id.ssn);
    if (res > RCF_MAX_PATH)
    {
        WARN("Too long path string for sniffer logs folder.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    errno = 0;
    if (mkdir(sniff->path, S_IRWXU | S_IRWXG | S_IRWXO) != 0 && 
        errno != EEXIST)
    {
        ERROR("Couldn't create directory, %d\n", errno);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    return 0;
}

/**
 * Start the sniffer process.
 *
 * @param sniff     Sniffer configuration structure.
 * 
 * @return Status code.
 */
static te_errno
sniffer_start_process(sniffer_t *sniff)
{
    int         s_argc;
    char      **s_argv;
    char       *te_snif_path = SNIFFER_EXEC;
    te_errno    rc           = 0;

    rc = sniffer_make_dir(sniff);
    if (rc != 0)
    {
        ERROR("Couldn't make the sniffer directory.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    s_argv = make_argv_str(sniff, &s_argc);
    if ((s_argv == NULL) || (s_argc == 0))
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((rc = rcf_ch_start_process(&sniff->pid, -1, te_snif_path, TRUE, 
                                   s_argc, (void **)s_argv)) != 0)
    {
        ERROR("Start the sniffer process failed.");
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    RING("The sniffer process started, pid %d.", sniff->pid);
    return rc;
}

/**
 * Add clone of the sniffer in the list to delete him after unloading.
 * 
 * @param   sniff   The sniffer location.
 */
static void
sniffer_add_clone(sniffer_t *sniff)
{
    sniffer_t *new_sniff = NULL;

    SNIFFER_MALLOC(new_sniff, sizeof(sniffer_t));
    memcpy(new_sniff, sniff, sizeof(sniffer_t));
    if (sniff->curr_file_name != NULL)
        new_sniff->curr_file_name = strdup(sniff->curr_file_name);
    if (sniff->filter_exp_file != NULL)
        new_sniff->filter_exp_file = strdup(sniff->filter_exp_file);
    if (sniff->filter_exp_str != NULL)
        new_sniff->filter_exp_str = strdup(sniff->filter_exp_str);
    if (sniff->id.snifname != NULL)
        new_sniff->id.snifname = strdup(sniff->id.snifname);
    if (sniff->id.ifname != NULL)
        new_sniff->id.ifname = strdup(sniff->id.ifname);

    new_sniff->state |= SNIF_ST_DEL;

    SLIST_INSERT_HEAD(&snifferl_h, new_sniff, ent_l);
}

/**
 * Set the sniffer enable. Calls to start/stop of the sniffer process fuctions.
 *
 * @param gid           Group identifier (unused).
 * @param oid           Full object instance identifier (unused).
 * @param value         New value.
 * @param ifname        Interface name.
 * @param snifname      Sniffer name.
 *
 * @return Status code.
 */
static te_errno
sniffer_set_enable(unsigned int gid, const char *oid, const char *value,
                   const char *ifname, const char *snifname)
{
    sniffer_t    *sniff;
    int           enable;

    UNUSED(gid);

    sniff = sniffer_find(ifname, snifname);
    if (sniff == NULL)
    {
        ERROR("Couldn't find the sniffer on the oid %s.", oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (value == NULL)
    {
       ERROR("A buffer for sniffer set enable is not provided.");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    enable = atoi(value);

    if (sniff->enable == enable)
    {
        if (enable == 1)
            WARN("The sniffer %s(%s) already started.", snifname, ifname);
        return 0;
    }

    if (enable == 1)
    {
        if (sniffer_start_process(sniff) == 0)
        {
            snif_sets.lock = TRUE;
            sniff->enable = TRUE;
            sniff->state |= SNIF_ST_START;
        }
        else
            WARN("Couldn't start the sniffer process: %s_%s_%d.",
                 ifname, snifname, sniff->id.ssn);
    }
    else if (enable == 0)
    {
        rcf_ch_kill_process(sniff->pid);
        sniff->enable = FALSE;

        sniffer_add_clone(sniff);

        sniff->id.abs_offset = 0;
        sniff->curr_offset = 0;
        sniff->state = SNIF_ST_START;
        memset(sniff->path, 0, RCF_MAX_PATH);
        if (sniff->curr_file_name != NULL)
        {
            free(sniff->curr_file_name);
            sniff->curr_file_name = NULL;
        }
        /* Get ssn from the file. */
        if (sniffer_get_ssn_ff(snif_sets.ssn_fname) != 0)
            WARN("Couldn't get ssn from the file");
        sniff->id.ssn = snif_sets.ssn;
    }
    else
        WARN("Wrong enable value");

    return 0;
}


/**
 * Fake sniffer set function for Configurator.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param value        New value (OUT).
 * @param ifname       Interface name.
 * @param snifname     The sniffer name.
 *
 * @return Status code.
 */
static te_errno
sniffer_set(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *snifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(ifname);
    UNUSED(snifname);

    return 0;
}


/**
 * Sniffer get function write to value (OUT) field the ssn value.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param value        New value (OUT).
 * @param ifname       Interface name.
 * @param snifname     The sniffer name.
 *
 * @return Status code.
 */
static te_errno
sniffer_get(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *snifname)
{
    UNUSED(gid);
    UNUSED(oid);

    sniffer_t   *sniff;

    sniff = sniffer_find(ifname, snifname);
    if (sniff == NULL)
    {
        ERROR("sniffer_get: Couldn't find the sniffer, oid %s", oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (value == NULL)
    {
       ERROR("A buffer to get sniffer val is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    sprintf(value, "%d", sniff->id.ssn);
    return 0;
}

/**
 * Get instance list of the sniffers for object "/agent/interface/sniffer".
 *
 * @param gid       Request's group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param list      Location for the list pointer.
 * @param ifname    Interface name.
 *
 * @return Status code.
 */
static te_errno
sniffers_list(unsigned int gid, const char *oid, char **list, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);

    sniffer_t  *sniff;
    size_t      list_size = 0;
    size_t      list_len  = 0;

    list_size = SNIFFER_LIST_SIZE;
    if ((*list = (char *)calloc(1, list_size)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    list_len = 0;

    SLIST_FOREACH(sniff, &snifferl_h, ent_l)
    {
        if ((strcmp(sniff->id.ifname, ifname) != 0) ||
            ((sniff->state & SNIF_ST_DEL) == SNIF_ST_DEL))
            continue;
        if (list_len + strlen(sniff->id.snifname) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        list_len += sprintf(*list + list_len, "%s ", sniff->id.snifname);
    }

    return 0;
}

/**
 * Check the sniffer folder exists to process the sniffer as after backup.
 * 
 * @param sniff     The nsiffer location.
 * 
 * @return TRUE if the folder exists.
 */
static te_bool
sniffer_check_exst_backup(sniffer_t *sniff)
{
    DIR  *dir;
    int   res;
    int   fnum;
    char *wp_fname;

    /* Make the directory name and create it. */
    res = snprintf(sniff->path, RCF_MAX_PATH, "%s/sniffers/%s_%s_%s_%u/", 
                   snif_sets.path, ta_name, sniff->id.ifname,
                   sniff->id.snifname, sniff->id.ssn);
    if (res > RCF_MAX_PATH)
    {
        WARN("Too long path string for sniffer logs folder.");
        return FALSE;
    }

    dir = opendir(sniff->path);
    if (dir == NULL)
        return FALSE;
    closedir(dir);

    sniff->curr_file_name = sniffer_get_capture_fname(sniff, &fnum,
                                                      &wp_fname, FALSE);
    if (sniff->curr_file_name == NULL)
        return FALSE;
    sniff->id.abs_offset = strtoll(wp_fname, NULL, 10);
    sniff->curr_offset = SNIF_PCAP_HSIZE;
    sniff->state |= SNIF_ST_START;

    return TRUE;
}

/**
 * Add a new Sniffer to the interface.
 *
 * @param gid       Request's group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param ssn       Sniffer session sequence number (unused).
 * @param ifname    Interface name.
 * @param snifname  Sniffer name.
 *
 * @return Status code.
 */
static te_errno
sniffer_add(unsigned int gid, const char *oid, char *ssn,
            const char *ifname, const char *snifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ssn);

    sniffer_t *sniff;
    te_bool    backup = FALSE;

    SNIFFER_MALLOC(sniff, sizeof(sniffer_t));
    sniff->id.snifname      = strdup(snifname);
    sniff->id.ifname        = strdup(ifname);
    sniff->id.ssn           = atoi(ssn);

    sniff->id.abs_offset    = 0;
    sniff->enable           = 0;
    sniff->snaplen          = 0;
    sniff->pid              = 0;
    sniff->filter_exp_file  = strdup(snif_sets.filter_exp_file);
    sniff->filter_exp_str   = strdup(snif_sets.filter_exp_str);
    sniff->overfill_meth    = snif_sets.overfill_meth;
    sniff->file_size        = snif_sets.file_size;
    sniff->rotation         = snif_sets.rotation;
    sniff->sniffer_space    = SNIFFER_SPACE;
    sniff->curr_file_name   = NULL;
    sniff->curr_offset      = 0;
    sniff->state            = 0;

    memset(sniff->path, 0, RCF_MAX_PATH);

    backup = sniffer_check_exst_backup(sniff);
    if (backup)
    {
        sniffer_add_clone(sniff);
        sniff->state = SNIF_ST_START;
        sniff->id.abs_offset = 0;
        sniff->curr_offset = 0;
        if (sniff->curr_file_name != NULL)
        {
            free(sniff->curr_file_name);
            sniff->curr_file_name = NULL;
        }
    }

    if (sniffer_get_ssn_ff(snif_sets.ssn_fname) != 0)
    {
        ERROR("Couldn't get SSN from the file %s", snif_sets.ssn_fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    sniff->id.ssn = snif_sets.ssn;
    memset(sniff->path, 0, RCF_MAX_PATH);

    SLIST_INSERT_HEAD(&snifferl_h, sniff, ent_l);

    return 0;
}

/**
 * Delete the Sniffer from the interface.
 *
 * @param gid       Request's group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param ifname    Interface name.
 * @param snifname  Sniffer name.
 *
 * @return Status code.
 */
static te_errno
sniffer_del(unsigned int gid, const char *oid, const char *ifname,
            const char *snifname)
{
    UNUSED(gid);
    UNUSED(oid);

    sniffer_t           *sniff;
    unsigned long long   offset;

    sniff = sniffer_find(ifname, snifname);
    if (sniff == NULL)
    {
        ERROR("sniffer_del: Couldn't find the sniffer, oid %s", oid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (sniff->enable == 1)
    {
        rcf_ch_kill_process(sniff->pid);
        sniff->enable = 0;
        sniff->id.abs_offset = 0;
    }

    sniffer_get_curr_offset(NULL, sniff, &offset);
    sniff->state |= SNIF_ST_DEL;

    return 0;
}

/*
 * Test Agent /sniffer_settings configuration subtree.
 */
RCF_PCH_CFG_NODE_RW(node_overfill_meth_s, "overfill_meth", NULL, NULL,
                    sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RW(node_rotation_s, "rotation", NULL, &node_overfill_meth_s,
                    sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RW(node_total_size_s, "total_size", NULL, &node_rotation_s,
                    sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RW(node_path_s, "path", NULL, &node_total_size_s,
                    sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RW(node_file_size_s, "file_size", NULL, &node_path_s,
                    sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RO(node_tmp_logs_s, "tmp_logs", &node_file_size_s, NULL,
                    sniffer_get_params);

RCF_PCH_CFG_NODE_RW(node_filter_file_s, "filter_exp_file", NULL, 
                    &node_tmp_logs_s, sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RW(node_snaplen_s, "snaplen", NULL, &node_filter_file_s,
                    sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RW(node_filter_exp_str_s, "filter_exp_str", NULL, 
                    &node_snaplen_s, sniffer_get_params, sniffer_set_params);
                    
RCF_PCH_CFG_NODE_RW(node_enable_s, "enable", NULL, &node_filter_exp_str_s,
                    sniffer_get_params, sniffer_set_params);

RCF_PCH_CFG_NODE_RO(node_sniffer_settings, "sniffer_settings", 
                    &node_enable_s, NULL, sniffer_get);

/*
 * Test Agent /interface/sniffer configuration subtree.
 */
RCF_PCH_CFG_NODE_RW(node_overfill_meth, "overfill_meth", NULL, NULL,
                    sniffer_common_get, sniffer_common_set);

RCF_PCH_CFG_NODE_RW(node_rotation, "rotation", NULL, &node_overfill_meth,
                    sniffer_common_get, sniffer_common_set);

RCF_PCH_CFG_NODE_RW(sniffer_space, "sniffer_space", NULL, &node_rotation,
                    sniffer_common_get, sniffer_common_set);

RCF_PCH_CFG_NODE_RW(node_file_size, "file_size", NULL, &sniffer_space,
                    sniffer_common_get, sniffer_common_set);

RCF_PCH_CFG_NODE_RO(node_tmp_logs, "tmp_logs", &node_file_size, NULL,
                    sniffer_common_get);

RCF_PCH_CFG_NODE_RW(node_filter_file, "filter_exp_file", NULL, 
                    &node_tmp_logs, sniffer_common_get, sniffer_common_set);

RCF_PCH_CFG_NODE_RW(node_snaplen, "snaplen", NULL, &node_filter_file,
                    sniffer_common_get, sniffer_common_set);

RCF_PCH_CFG_NODE_RW(node_filter_exp_str, "filter_exp_str", NULL, 
                    &node_snaplen, sniffer_common_get, sniffer_common_set);

RCF_PCH_CFG_NODE_RW(node_enable, "enable", NULL, &node_filter_exp_str,
                    sniffer_common_get, sniffer_set_enable);

static rcf_pch_cfg_object node_sniffer_inst =
    { "sniffer", 0, &node_enable, NULL,
      (rcf_ch_cfg_get)sniffer_get, (rcf_ch_cfg_set)sniffer_set,
      (rcf_ch_cfg_add)sniffer_add, (rcf_ch_cfg_del)sniffer_del,
      (rcf_ch_cfg_list)sniffers_list, NULL, NULL };

/** 
 * Get capture logs of the sniffer and send it as the answer with a binary
 * attach to TEN.
 * 
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param sniff_id_str  a string with a sniffer ID
 * 
 * @return Status code.
 */
te_errno
rcf_ch_get_snif_dump(struct rcf_comm_connection *handle, char *cbuf, 
                    size_t buflen, size_t answer_plen, 
                    const char *sniff_id_str)
{
    te_errno    rc;
    size_t      len = answer_plen;

    rc = 0;
    rc = sniffer_get_dump(handle, cbuf, buflen, answer_plen, sniff_id_str);
    if (rc != 0)
    {
        len += snprintf(cbuf + answer_plen, buflen - answer_plen, "0") + 1;
        RCF_CH_LOCK;
        rc = rcf_comm_agent_reply(handle, cbuf, len);
        RCF_CH_UNLOCK;
    }

    return rc;
}

/** 
 * Get sniffer list and send it as the answer with a binary attach to TEN.
 * 
 * @param handle        connection handle
 * @param cbuf          command buffer
 * @param buflen        length of the command buffer
 * @param answer_plen   number of bytes in the command buffer to be
 *                      copied to the answer
 * @param sniff_id_str  a string with a sniffer ID
 *
 * @return Status code.
 */
te_errno
rcf_ch_get_sniffers(struct rcf_comm_connection *handle, char *cbuf, 
                    size_t buflen, size_t answer_plen, 
                    const char *sniff_id_str)
{
    te_errno            rc;
    size_t              len      = answer_plen;
    char               *abuf     = NULL;
    size_t              alen     = 0;
    unsigned long long  offset   = 0;

    /* Get sniffer list processing. */
    if (strcmp(sniff_id_str, "sync") == 0)
        alen = sniffer_get_list_buf(&abuf, TRUE);
    else if (strcmp(sniff_id_str, "nosync") == 0)
        alen = sniffer_get_list_buf(&abuf, FALSE);
    /* Mark processing. */
    else if (sniffer_get_curr_offset(sniff_id_str, NULL, &offset) == 0)
    {
        SNIFFER_MALLOC(abuf, RCF_MAX_ID);
        alen = snprintf(abuf, RCF_MAX_ID, "%s %llu", sniff_id_str, offset) + 1;
        if (alen > RCF_MAX_ID)
        {
            ERROR("Too long sniffer id: %s", sniff_id_str);
            alen = 0;
            free(abuf);
        }
    }

    if (alen == 0)
        len += snprintf(cbuf + answer_plen, buflen - answer_plen, "0") + 1;
    else
        len += snprintf(cbuf + answer_plen, buflen - answer_plen,
                        "0 attach %u", alen) + 1;

    RCF_CH_LOCK;
    rc = rcf_comm_agent_reply(handle, cbuf, len);
    if ((rc == 0) && (alen != 0))
    {
        rc = rcf_comm_agent_reply(handle, abuf, alen);
    }
    RCF_CH_UNLOCK;

    if (alen != 0)
        free(abuf);

    return rc;
}

/**
 * Initialization of the sniffers configuration subtrees and default settings.
 * 
 * @return Status code.
 */
te_errno
ta_unix_conf_sniffer_init(void)
{
    int res;

    if ((res = sniffer_settings_init()) != 0)
        return res;
    if ((res = rcf_pch_add_node("/agent/interface", &node_sniffer_inst)) != 0)
        return res;

    return rcf_pch_add_node("/agent", &node_sniffer_settings);
}

/**
 * Cleanup sniffers function.
 * 
 * @return Status code.
 */
te_errno
ta_unix_conf_sniffer_cleanup(void)
{
    char         sniffers_dir[RCF_MAX_PATH];
    sniffer_t   *snif;

    while (!SLIST_EMPTY(&snifferl_h))
    {
        snif = SLIST_FIRST(&snifferl_h);
        sniffer_cleanup(snif);
    }

    if (snprintf(sniffers_dir, RCF_MAX_PATH, "%s/sniffers",
                 snif_sets.path) > RCF_MAX_PATH)
    {
        ERROR("Too long directory name, %d\n", errno);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    remove(snif_sets.ssn_fname);
    remove(sniffers_dir);
    remove(snif_sets.path);    
    if ((strcmp(snif_sets.path, snif_sets.ta_path) != 0) &&
        remove(snif_sets.ta_path) != 0)
        PRINT("Couldn't remove the agent-specific folder: %s",
              snif_sets.ta_path);

    return 0;
}
