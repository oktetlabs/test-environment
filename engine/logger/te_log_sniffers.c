/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Logger sniffers support.
 *
 * Implementation of unix Logger sniffers logging support.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Logr Sniffers"
#endif

#include "te_config.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_POPT_H
#include <popt.h>
#else
#error popt library (development version) is required for Logger
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#else
#error dirent library is required for Logger
#endif

#include <sys/sendfile.h>

#include "te_raw_log.h"
#include "te_str.h"
#include "logger_int.h"
#include "logger_internal.h"
#include "logger_ten.h"
#include "te_sleep.h"

#define SNIFFER_MALLOC(ptr, size)       \
    if ((ptr = malloc(size)) == NULL)   \
        assert(0);

#define SNIF_MIN_LIST_SIZE 1024
#define SNIF_MAX_PATH_LENGTH RCF_MAX_PATH

/* The PCAP file header. */
static char pcap_hbuf[SNIF_PCAP_HSIZE];

static unsigned long long filled_space = 0;

/**
 * Mutex to share agents list
 */
static pthread_mutex_t te_log_sniffer_mutex;

/**
 * Capture files list.
 */
typedef struct file_list_s {
    char                         name[RCF_MAX_PATH];     /**< File name */
    SLIST_ENTRY(file_list_s)     ent_l_f;
} file_list_s;
SLIST_HEAD(flist_h_t, file_list_s);
typedef struct flist_h_t flist_h_t;

/**
 * List of the sniffer parameters.
 */
typedef struct snif_id_l {
    sniffer_id  id;
    te_bool     log_exst;
    char        res_fname[RCF_MAX_PATH];
    te_bool     first_launch;
    flist_h_t   flist_h;
    unsigned    cap_file_ind;
    SLIST_ENTRY(snif_id_l)  ent_l;
} snif_id_l;
SLIST_HEAD(snifidl_h_t, snif_id_l);
typedef struct snifidl_h_t snifidl_h_t;

/**
 * List of the mark messages to sniffers.
 */
typedef struct snif_mark_l {
    char            agent[RCF_MAX_NAME];
    sniffer_id      id;
    te_pcap_pkthdr  h;
    char           *message;
    SLIST_ENTRY(snif_mark_l)  ent_l_m;
} snif_mark_l;
SLIST_HEAD(snif_mrk_h_t, snif_mark_l);
typedef struct snif_mrk_h_t snif_mrk_h_t;
snif_mrk_h_t snif_mrk_h;

/**
 * List of Test Agents with a pointers to lists of sniffers.
 */
typedef struct snif_ta_l {
    char        agent[RCF_MAX_NAME];
    snifidl_h_t snif_hl;
    SLIST_ENTRY(snif_ta_l)  ent_l_ta;
} snif_ta_l;
SLIST_HEAD(snif_ta_h_t, snif_ta_l);
typedef struct snif_ta_h_t snif_ta_h_t;
snif_ta_h_t snif_ta_h;

/** Cpature logs polling settings */
extern snif_polling_sets_t snifp_sets;

static int polling_period;

#define SNIF_LOG_SLEEP usleep(polling_period);

/**
 * Check the sniffer exist.
 *
 * @param snif              Location of sniffer parameters.
 * @param sniflist_head     Head of the sniffers list.
 *
 * @return TRUE if sniffer exists.
 */
static te_bool
check_snif_exist(snifidl_h_t *sniflist_head, snif_id_l *new_snif)
{
    snif_id_l *snif;

    SLIST_FOREACH(snif, sniflist_head, ent_l)
    {
        if (strcmp(snif->id.snifname, new_snif->id.snifname) == 0 &&
            strcmp(snif->id.ifname, new_snif->id.ifname) == 0 &&
            (snif->id.ssn == new_snif->id.ssn))
        {
            snif->log_exst = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Skip spaces in the command.
 *
 * @param _ptr  Pointer to string.
 * @param _cpos Current pointer position.
 */
#define SNIF_SK_SPS(_ptr, _cpos) \
        while (*(_ptr) == ' ')   \
        {                        \
            (_ptr)++;            \
            _cpos++;             \
        }

/**
 * Check a pointer is NULL for brake.
 *
 * @param _ptr  - pointer
 */
#define NULL_BR(_ptr) \
    if (_ptr == NULL)                                       \
    {                                                       \
        ERROR("Wrong sniffer id list");                     \
        break;                                              \
    }

/**
 * Wrong sniffer id macro.
 */
#define WRONG_SNIF_ID(str) \
    {                                           \
        free(snif);                             \
        ptr = strchr(buf, '\0');                \
        ERROR("%s", str);                       \
        NULL_BR(ptr);                           \
        strl = ptr - buf;   \
        clen += strl;                           \
        buf = ptr;                              \
        if (clen >= len)                        \
            break;                              \
        clen++;                                 \
        buf++;                                  \
        continue;                               \
    }

/**
 * Parse sniffer id string.
 *
 * @param buf   Sntring with sniffer id.
 * @param id    Sniffer id (OUT).
 *
 * @return Number of parsed symbols or -1 if errors.
 */
static int
sniffer_parse_id_str(char *buf, sniffer_id *id)
{
    char   *ptr;
    int     strl;
    int     clen    = 0;

    SNIF_SK_SPS(buf, clen);
    ptr = strchr(buf, ' ');
    if (ptr == NULL)
    {
        ERROR("Wrong sniffer name in the sniffer id.");
        return -1;
    }
    strl = ptr - buf;
    id->snifname = strndup(buf, strl);
    clen += strl;
    buf = ptr;

    SNIF_SK_SPS(buf, clen);
    ptr = strchr(buf, ' ');
    if (ptr == NULL)
    {
        ERROR("Wrong iface name in the sniffer id.");
        return -1;
    }
    strl = ptr - buf;
    id->ifname = strndup(buf, strl);
    clen += strl;
    buf = ptr;

    SNIF_SK_SPS(buf, clen);
    id->ssn= strtoll(buf, &ptr, 10);
    if (buf == ptr)
    {
        ERROR("Wrong SSN in the sniffer id.");
        return -1;
    }
    strl = ptr - buf;
    clen += strl;
    buf = ptr;
    return clen;
}

/**
 * Make a name for capture file of the sniffer
 *
 * @param agent     Agent name
 * @param sniff     The sniffer location
 *
 * @return Status code
 * @retval 0        success
 */
static te_errno
sniffer_make_file_name(const char *agent, snif_id_l *snif)
{
    int              len;
    unsigned         offt_templ;
    unsigned         offt_buf;
    char            *templ;
    char            *ptr;

    if (strlen(snifp_sets.name) == 0)
        templ = "%a_%i_%s_%n";
    else
        templ = snifp_sets.name;
    memset(snif->res_fname, 0, RCF_MAX_PATH);

    offt_buf = snprintf(snif->res_fname, RCF_MAX_PATH, "%s/",
                        snifp_sets.dir);
    if (offt_buf > RCF_MAX_PATH)
        return TE_RC(TE_LOGGER, TE_EINVAL);

    offt_templ = 0;
    while ((ptr = strchr(templ + offt_templ, '\%')) != NULL)
    {
        len = ptr - (templ + offt_templ);
        if (len < 0 || (unsigned)len >= RCF_MAX_PATH - offt_buf)
            return TE_RC(TE_LOGGER, TE_EINVAL);
        strncpy(snif->res_fname + offt_buf, templ + offt_templ, len);
        offt_templ += len + 1;
        offt_buf += len;
        if (offt_templ >= strlen(templ))
            break;
        ptr++;
        switch (*ptr)
        {
            case 'a':
                len = snprintf(snif->res_fname + offt_buf,
                               RCF_MAX_PATH - offt_buf, "%s", agent);
                break;

            case 'u':
                len = snprintf(snif->res_fname + offt_buf,
                               RCF_MAX_PATH - offt_buf, "%u", getuid());
                break;

            case 'i':
                len = snprintf(snif->res_fname + offt_buf,
                               RCF_MAX_PATH - offt_buf, "%s",
                               snif->id.ifname);
                break;

            case 's':
                len = snprintf(snif->res_fname + offt_buf,
                               RCF_MAX_PATH - offt_buf, "%s",
                               snif->id.snifname);
                break;

            case 'n':
                len = snprintf(snif->res_fname + offt_buf,
                               RCF_MAX_PATH - offt_buf, "%d", snif->id.ssn);
                break;

            default:
                len = 0;
                printf("Wrong name template: %s\n", templ);
        }

        if ((unsigned)len > RCF_MAX_PATH - offt_buf)
            return TE_RC(TE_LOGGER, TE_EINVAL);
        offt_buf += len;

        offt_templ++;
        if (offt_templ >= strlen(templ))
            break;
    }

    if (offt_templ < strlen(templ))
    {
        len = strlen(templ + offt_templ);
        if ((unsigned)len >= RCF_MAX_PATH - offt_buf)
            return TE_RC(TE_LOGGER, TE_EINVAL);
        te_strlcpy(snif->res_fname + offt_buf, templ + offt_templ,
                RCF_MAX_PATH - offt_buf);
        offt_buf += len;
    }
    if (snif->cap_file_ind > 0)
        len = snprintf(snif->res_fname + offt_buf, RCF_MAX_PATH - offt_buf,
                       "_%u.pcap", snif->cap_file_ind);
    else
        len = snprintf(snif->res_fname + offt_buf, RCF_MAX_PATH - offt_buf,
                       ".pcap");
    if ((unsigned)len > RCF_MAX_PATH - offt_buf)
        return TE_RC(TE_LOGGER, TE_EINVAL);
    offt_buf += len;

    return 0;
}

/**
 * Parse the buffer of binary attachment with the list of sniffers.
 * Buffer format for each sniffer:
 *     <Sniffer name> <Interface name> <SSN> <offset>\0
 *
 * @param buf               Pointer to the buffer with the list of sniffers.
 * @param len               Buffer length.
 * @param sniflist_head     Head of the sniffers list.
 * @param agent             Agent name.
 *
 * @return Status code.
 */
static te_errno
sniffer_parse_list_buf(char *buf, size_t len, snifidl_h_t *sniflist_head,
                       const char *agent)
{
    snif_id_l   *snif;
    unsigned     strl;
    size_t       clen;
    char        *ptr;
    file_list_s *lfile;

    clen = 0;
    ptr = buf;

    while (clen < len)
    {
        SNIFFER_MALLOC(snif, sizeof(snif_id_l));

        SNIF_SK_SPS(buf, clen);
        ptr = strchr(buf, ' ');
        if (ptr == NULL)
            WRONG_SNIF_ID("Wrong sniffer name in the sniffer id.");
        strl = ptr - buf;
        snif->id.snifname = strndup(buf, strl);
        clen += strl;
        buf = ptr;

        SNIF_SK_SPS(buf, clen);
        ptr = strchr(buf, ' ');
        if (ptr == NULL)
            WRONG_SNIF_ID("Wrong iface name in the sniffer id.");
        strl = ptr - buf;
        snif->id.ifname = strndup(buf, strl);
        clen += strl;
        buf = ptr;

        SNIF_SK_SPS(buf, clen);
        snif->id.ssn = strtoll(buf, &ptr, 10);
        if (buf == ptr)
            WRONG_SNIF_ID("Wrong SSN in the sniffer id.");
        strl = ptr - buf;
        clen += strl;
        buf = ptr;

        SNIF_SK_SPS(buf, clen);
        snif->id.abs_offset = strtoll(buf, &ptr, 10);
        if (buf == ptr)
            WRONG_SNIF_ID("Wrong absolute offset in the sniffer id.");
        strl = ptr - buf;
        clen += strl;
        buf = ptr;

        snif->log_exst = TRUE;

        if (check_snif_exist(sniflist_head, snif) == FALSE)
        {
            snif->cap_file_ind = 0;
            if (sniffer_make_file_name(agent, snif) != 0)
                WRONG_SNIF_ID("Couldn't make capture file name.");

            SLIST_INIT(&snif->flist_h);
            SNIFFER_MALLOC(lfile, sizeof(file_list_s));
            strcpy(lfile->name, snif->res_fname);
            snif->first_launch = TRUE;
            SLIST_INSERT_HEAD(&snif->flist_h, lfile, ent_l_f);
            SLIST_INSERT_HEAD(sniflist_head, snif, ent_l);
        }
        else
            free(snif);

        if (*buf != '\0')
        {
            WRONG_SNIF_ID("Garbage in the sniffer id.");
        }

        if (clen >= len)
            break;
        clen++;
        buf++;
    }

    return clen;
}

#undef WRONG_SNIF_ID
#undef NULL_BR
#undef SNIF_SK_SPS

/**
 * Check to mark consists in the capture block.
 *
 * @param size          Size of the received file.
 * @param snif      Location of sniffer parameters.
 * @param ta_name   Test agent name.
 *
 * @return TRUE if marker should be inserted.
 */
static snif_mark_l *
sniffer_check_markers(size_t size, snif_id_l *snif, const char *agent)
{
    snif_mark_l     *mark       = NULL;
    snif_mark_l     *rem        = NULL;
    snif_mark_l     *retmark    = NULL;

    SLIST_FOREACH(mark, &snif_mrk_h, ent_l_m)
    {
        if (strcmp(agent, mark->agent) == 0 &&
            mark->id.ssn == snif->id.ssn)
        {
            if (mark->id.abs_offset >= snif->id.abs_offset  &&
                mark->id.abs_offset < snif->id.abs_offset + size)
            {
                if ((retmark == NULL) ||
                    (retmark->id.abs_offset > mark->id.abs_offset))
                    retmark = mark;
            }
            else if (snif->id.abs_offset > mark->id.abs_offset)
                rem = mark;
        }
    }
    if (rem != NULL)
    {
        ERROR("Marker was late. Message %s, offset %llu", rem->message,
              rem->id.abs_offset);
        SLIST_REMOVE(&snif_mrk_h, rem, snif_mark_l, ent_l_m);
        free(rem);
    }
    return retmark;
}

/**
 * Inser the marker packet into the capture file.
 *
 * @param fd_o  A descriptor of the opened file.
 * @param mark  Location of the marker parameters.
 *
 */
static te_errno
sniffer_insert_marker(int fd_o, snif_mark_l *mark)
{
    char            proto[SNIF_MARK_PSIZE];    /**< Protocol */
    int             res;

    SNIFFER_MARK_H_INIT(proto, strlen(mark->message));

    res = write(fd_o, (void *)&mark->h, sizeof(te_pcap_pkthdr));
    if (res == -1)
        goto insert_marker_error;
    res = write(fd_o, proto, SNIF_MARK_PSIZE);
    if (res == -1)
        goto insert_marker_error;
    res = write(fd_o, mark->message, strlen(mark->message));
    if (res == -1)
        goto insert_marker_error;

    return 0;
insert_marker_error:
    ERROR("Couldn't write marker packet to file.");
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

/**
 * Save information about sniffer to the capture file.
 *
 * @param agent     Agent name
 * @param snif      The sniffer location
 * @param fd_o      Opened file descriptor
 */
static void
sniffer_save_info(const char *agent, snif_id_l *snif, int fd_o)
{
    snif_mark_l     mark;
    int             len;
    int             res;
    struct timeval  ts;

    len = strlen(agent) + strlen(snif->id.ifname) +
          strlen(snif->id.snifname) + 5;
    SNIFFER_MALLOC(mark.message, len);
    res = snprintf(mark.message, len, "%s;%s;%s", agent, snif->id.ifname,
                   snif->id.snifname);

    gettimeofday(&ts, 0);
    SNIFFER_TS_CPY(mark.h.ts, ts);
    mark.h.caplen = res + SNIF_MARK_PSIZE;
    mark.h.len = mark.h.caplen;

    sniffer_insert_marker(fd_o, &mark);
    free(mark.message);
}

/**
 * Capture files processing.
 *
 * @param fname     Name of the received file as it is saved.
 * @param snif      Location of sniffer parameters.
 * @param ta_name   Test agent name.
 *
 * @return Status code.
 */
static te_errno
sniffer_capture_file_proc(const char *fname, snif_id_l *snif,
                          const char *agent)
{
    int             fd_o;
    int             fd_n;
    off_t           size;
    off_t           cur_size;
    int             res;
    snif_mark_l    *mark;
    te_errno        rc = 0;

    errno = 0;
    fd_o = open(snif->res_fname, O_WRONLY);
    if ((fd_o == -1) && (errno == ENOENT))
    {
        fd_o = open(snif->res_fname , O_WRONLY | O_CREAT,
                    S_IRWXU | S_IRWXG | S_IRWXO);
        if (fd_o == -1)
        {
            ERROR("Couldn't open new file: %s", snif->res_fname);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        if (snif->first_launch)
        {
            /* Save capture file header. */
            fd_n = open(fname, O_RDONLY);
            if (fd_n == -1)
            {
                ERROR("Couldn't open received file: %s", fname);
                goto cleanup_snif_fproc;
            }

            res = read(fd_n, (void *)pcap_hbuf, SNIF_PCAP_HSIZE);
            if (res < 0)
            {
                rc = TE_OS_RC(TE_LOGGER, errno);
                ERROR("%s(): failed to read PCAP header, errno=%r",
                      __FUNCTION__, rc);
                goto cleanup_snif_fproc;
            }
            else if (res != SNIF_PCAP_HSIZE)
            {
                rc = TE_RC(TE_LOGGER, TE_ENODATA);
                ERROR("%s(): %d bytes instead of %d were read for "
                      "PCAP header", __FUNCTION__, res, SNIF_PCAP_HSIZE);
                goto cleanup_snif_fproc;
            }

            close(fd_n);
        }

        res = write(fd_o, pcap_hbuf, SNIF_PCAP_HSIZE);
        sniffer_save_info(agent, snif, fd_o);
    }
    else if (fd_o == -1)
    {
        WARN("Couldn't open the old capture log file: %s", snif->res_fname);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    fd_n = open(fname, O_RDWR);
    if (fd_n == -1)
    {
        WARN("Couldn't open the new capture log file: %s.", fname);
        close(fd_o);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    res = lseek(fd_o, 0, SEEK_END);
    if (res == -1)
    {
        WARN("Couldn't read the old capture log file: %s", snif->res_fname);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup_snif_fproc;
    }
    size = lseek(fd_n, 0, SEEK_END);
    if (size == -1)
    {
        WARN("Couldn't read the new capture log file: %s.", fname);
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup_snif_fproc;
    }

    if (snif->first_launch)
    {
        lseek(fd_n, SNIF_PCAP_HSIZE, SEEK_SET);
        snif->first_launch = FALSE;
        size -= SNIF_PCAP_HSIZE;
    }
    else
        lseek(fd_n, 0, SEEK_SET);

    while ((mark = sniffer_check_markers(size, snif, agent)) != NULL)
    {
        cur_size = mark->id.abs_offset - snif->id.abs_offset;
        res = sendfile(fd_o, fd_n, 0, cur_size);
        size -= cur_size;
        sniffer_insert_marker(fd_o, mark);

        SLIST_REMOVE(&snif_mrk_h, mark, snif_mark_l, ent_l_m);
        free(mark);
        snif->id.abs_offset += cur_size;
    }
    res = sendfile(fd_o, fd_n, 0, size);
    if ((res == -1) || (res < size))
    {
        WARN("Couldn't copy capture log res %d: %s --> %s: %s", res, fname,
             snif->res_fname, strerror(errno));
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup_snif_fproc;
    }
    snif->id.abs_offset += size;
    filled_space += size;

cleanup_snif_fproc:
    close(fd_o);
    close(fd_n);
    remove(fname);
    return rc;
}

/**
 * Check overall capture files size for all sniffers.
 *
 * @return TRUE if space is overflowed.
 */
static te_bool
sniffer_check_overall_space(unsigned fsize)
{
    snif_ta_l           *snif_ta;
    snif_id_l           *snif;
    file_list_s         *f;
    struct stat         st;

    filled_space = 0;
    /* Calculate used space. */
    SLIST_FOREACH(snif_ta, &snif_ta_h, ent_l_ta)
    {
        SLIST_FOREACH(snif, &snif_ta->snif_hl, ent_l)
        {
            SLIST_FOREACH(f, &snif->flist_h, ent_l_f)
            {
                if (stat(f->name, &st) == 0)
                    filled_space += st.st_size;
            }
        }
    }

    if (filled_space + fsize > snifp_sets.osize)
        return TRUE;
    return FALSE;
}

/**
 * Check and free the space occupied by capture log files.
 *
 * @param snif      Location of sniffer parameters.
 * @param fname     Name of the received file as it is saved.
 * @param agent     Test agent name.
 *
 * @return Status code. TRUE is a free space, else FALSE.
 */
static te_bool
sniffer_check_capture_space(snif_id_l *snif, const char *fname,
                            const char * agent)
{
    size_t              total    = 0;
    te_bool             overflow = FALSE;
    struct stat         st;
    int                 res;
    file_list_s        *f;
    file_list_s        *flast = NULL; /* Just to quieten GCC */
    int                 fd_n;
    unsigned            fnum;

    /* Check the current file size + received log file. */
    if (stat(snif->res_fname, &st) == 0)
        total += st.st_size;
    if (stat(fname, &st) == 0)
        total += st.st_size;

    if ((snifp_sets.fsize > 0) && (total > snifp_sets.fsize))
    {
        int sysrc;

        snif->cap_file_ind++;
        SNIFFER_MALLOC(f, sizeof(file_list_s));
        res = sniffer_make_file_name(agent, snif);
        if (res != 0)
            return FALSE;

        /* Copy the file header. */
        fd_n = open(snif->res_fname, O_WRONLY | O_CREAT, S_IRWXU |
                    S_IRWXG | S_IRWXO);
        if (fd_n == -1)
        {
            free(f);
            ERROR("Couldn't open/creat file for capture logs.");
            return FALSE;
        }
        sysrc = write(fd_n, pcap_hbuf, SNIF_PCAP_HSIZE);
        if (sysrc != SNIF_PCAP_HSIZE)
        {
            ERROR("Couldn't not write %u bytes (pcap header): %s",
                  SNIF_PCAP_HSIZE, strerror(errno));
            close(fd_n);
            return FALSE;
        }
        sniffer_save_info(agent, snif, fd_n);
        close(fd_n);

        strcpy(f->name, snif->res_fname);
        SLIST_INSERT_HEAD(&snif->flist_h, f, ent_l_f);
    }

    if (snifp_sets.osize > 0 &&
        filled_space + st.st_size > snifp_sets.osize)
        overflow = sniffer_check_overall_space(st.st_size);

    if (snifp_sets.sn_space == 0 && overflow == FALSE)
        return TRUE;

    total = 0;
    fnum  = 0;
    /* Calculate used space. */
    SLIST_FOREACH(f, &snif->flist_h, ent_l_f)
    {
        if (stat(f->name, &st) == 0)
            total += st.st_size;
        fnum++;
        flast = f;
    }
    if (stat(fname, &st) == 0)
        total += st.st_size;

    if (total < snifp_sets.sn_space && overflow == FALSE &&
        ((fnum <= snifp_sets.rotation) || (snifp_sets.rotation == 0)))
        return TRUE;
    if ((snifp_sets.ofill == TAIL_DROP) || (fnum < 2))
        return FALSE;

    if (stat(flast->name, &st) == 0)
        filled_space -= st.st_size;
    /* Remove the oldest capture file to rotation. */
    remove(flast->name);
    SLIST_REMOVE(&snif->flist_h, flast, file_list_s, ent_l_f);
    free(flast);

    return TRUE;
}

/**
 * Get sniffer dump function performed *rcf_get_sniffer_dump* call.
 *
 * @param ta_name   Test agent name.
 * @param snif      Location of sniffer parameters.
 *
 * @return Status code.
 */
static te_errno
ten_get_sniffer_dump(const char *ta_name, snif_id_l *snif)
{
    char                idbuf[RCF_MAX_ID];
    char                fname[RCF_MAX_PATH];
    int                 rc;
    unsigned long long  offset;

    rc = snprintf(idbuf, RCF_MAX_ID, "%s %s %u", snif->id.snifname,
                  snif->id.ifname, snif->id.ssn);
    if (rc > RCF_MAX_VAL)
    {
        ERROR("Too long sniffer id");
        return TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
    }

    rc = snprintf(fname, RCF_MAX_PATH, "%s/%s_%s_%s_%u_t.pcap",
                  snifp_sets.dir, ta_name, snif->id.snifname,
                  snif->id.ifname, snif->id.ssn);
    if (rc > RCF_MAX_PATH)
    {
        ERROR("Too long file name for the cpture logs.");
        return TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
    }

    offset = 0;
    rc = rcf_get_sniffer_dump(ta_name, idbuf, fname, &offset);
    if (rc != 0 && rc != TE_RC(TE_RCF_API, TE_ENODATA))
    {
        if (rc != TE_RC(TE_RCF_API, TE_EIPC))
            ERROR("Couldn't get capture file %s", fname);
        return rc;
    }

    /* It is correct, no data available */
    if (rc == TE_RC(TE_RCF_API, TE_ENODATA))
        return 0;

    snif->id.abs_offset = offset;

    rc = 0;
    if(sniffer_check_capture_space(snif, fname, ta_name))
        rc = sniffer_capture_file_proc(fname, snif, ta_name);
    else
        remove(fname);

    return rc;
}

/**
 * Recursively cleanup directory from .pcap files
 *
 * @param dirname   Directory name
 */
static void
sniffer_cleanup_dir(const char *dirname)
{
    DIR             *dir;
    struct dirent   *ent;
    char             fname[SNIF_MAX_PATH_LENGTH + 1];
    char            *tmp;

    dir = opendir(dirname);
    if (dir == NULL)
        return;

    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0)
            continue;

        snprintf(fname, SNIF_MAX_PATH_LENGTH + 1, "%s/%s", dirname,
                 ent->d_name);
        if (ent->d_type == DT_DIR)
            sniffer_cleanup_dir(fname);
        else if ((tmp = strstr(fname, ".pcap")) != NULL &&
                 *(tmp + strlen(".pcap")) == '\0')
            remove(fname);
    }
    closedir(dir);
}

/**
 * Make folder for capture logs or cleanup existing folder.
 *
 * @param agt_fldr  Full path to the folder.
 */
void
sniffers_logs_cleanup(char *agt_fldr)
{
    errno = 0;
    if (mkdir(agt_fldr, S_IRWXU | S_IRWXG | S_IRWXO) != 0 &&
        errno != EEXIST)
    {
        ERROR("Couldn't create directory, %d\n", errno);
        return;
    }
    /* Cleanup the agent working directory. */
    else if (errno == EEXIST)
        sniffer_cleanup_dir(agt_fldr);
}

/**
 * Search TA unit in the list by name
 *
 * @param ta_name   Agent name
 *
 * @return Pointer to TA unit location or NULL
 */
static snif_ta_l *
sniffer_get_ta_by_name(char *ta_name)
{
    snif_ta_l       *snif_ta;

    SLIST_FOREACH(snif_ta, &snif_ta_h, ent_l_ta)
    {
        if (strcmp(snif_ta->agent, ta_name) == 0)
            return snif_ta;
    }
    return NULL;
}

/**
 * Search the same sniffer in the any sniffer list by ssn
 *
 * @param sniff         Required sniffer
 * @param sniflist_h    Head of a sniffer list
 *
 * @return The sniffer unit location or NULL in case of failure
 */
static snif_id_l *
sniffer_search_same_sniff(snif_id_l *sniff, snifidl_h_t *sniflist_h)
{
    snif_id_l *new_sniff;

    SLIST_FOREACH(new_sniff, sniflist_h, ent_l)
    {
        if (sniff->id.ssn == new_sniff->id.ssn)
            return new_sniff;
    }
    return NULL;
}

/**
 * Add new marker in the list for sniffer
 *
 * @param ta_name   Agent name
 * @param sniff     The sniffer location
 * @param message   The mark message
 * @param ts        Timestamp of mark packet
 *
 * @param Status code
 * @retval 0 success
 */
static te_errno
sniffer_add_new_mark(char *ta_name, snif_id_l *sniff, char *message,
                     struct timeval *ts)
{
    snif_mark_l     *mark;

    SNIFFER_MALLOC(mark, sizeof(snif_mark_l));
    if (strlen(message) > 0)
        mark->message = strdup(message);
    else
        mark->message = strdup("");

    SNIFFER_TS_CPY(mark->h.ts, *ts);
    mark->h.caplen = strlen(mark->message) + SNIF_MARK_PSIZE;
    mark->h.len = mark->h.caplen;

    mark->id.snifname = strdup(sniff->id.snifname);
    mark->id.ifname = strdup(sniff->id.ifname);
    if (mark->id.snifname == NULL || mark->id.ifname == NULL ||
        mark->message == NULL)
        return TE_RC(TE_LOGGER, TE_ENOMEM);
    mark->id.ssn = sniff->id.ssn;
    mark->id.abs_offset = sniff->id.abs_offset;
    te_strlcpy(mark->agent, ta_name, RCF_MAX_NAME);
    SLIST_INSERT_HEAD(&snif_mrk_h, mark, ent_l_m);
    return 0;
}

/**
 * Cleanup sniffers list
 *
 * @param sniflist_h    Head of the list
 */
static void
sniffer_clean_list(snifidl_h_t *sniflist_h)
{
    snif_id_l *sniff;

    while((sniff = SLIST_FIRST(sniflist_h)) != NULL)
    {
        free(sniff->id.snifname);
        free(sniff->id.ifname);
        SLIST_REMOVE_HEAD(sniflist_h, ent_l);
        free(sniff);
    }
}

/**
 * Send request to insert marker for all existing sniffers.
 * @param mark_data     The string contains an agent name and marker
 *                      description.
 */
static void
sniffer_ins_mark_all(char *mark_data)
{
    char             ta_name[RCF_MAX_NAME];
    char            *ptr;
    int              len;
    int              rc;
    size_t           snif_len                   = SNIF_MIN_LIST_SIZE;
    char            *snif_buf                   = NULL;
    snifidl_h_t      new_sniflist_h;
    snif_ta_l       *snif_ta;
    snif_id_l       *sniff;
    snif_id_l       *new_sniff;
    struct timeval   ts;

    gettimeofday(&ts, 0);

    ptr = strchr(mark_data, ';');
    if (ptr == NULL)
        return;
    len = ptr - mark_data;
    if ((len >= RCF_MAX_NAME) || (len < 0))
        return;
    te_strlcpy(ta_name, mark_data, RCF_MAX_NAME);

    if ((snif_ta = sniffer_get_ta_by_name(ta_name)) == NULL)
    {
        ERROR("Wrong agent name to insert mark for all sniffers: %s",
              ta_name);
        return;
    }

    SLIST_INIT(&new_sniflist_h);
    SNIFFER_MALLOC(snif_buf, snif_len);

    rc = rcf_ta_get_sniffers(ta_name, NULL, &snif_buf, &snif_len, TRUE);
    if ((snif_len != 0) && (rc == 0))
    {
        sniffer_parse_list_buf(snif_buf, snif_len, &new_sniflist_h,
                               ta_name);

        SLIST_FOREACH(sniff, &snif_ta->snif_hl, ent_l)
        {
            new_sniff = sniffer_search_same_sniff(sniff, &new_sniflist_h);
            if (new_sniff == NULL)
                new_sniff = sniff;
            sniffer_add_new_mark(ta_name, new_sniff, ptr + 1, &ts);
        }
    }

    sniffer_clean_list(&new_sniflist_h);

    if (snif_buf != NULL)
        free(snif_buf);
    return;
}

/**
 * This is an entry point of sniffers mark message server.
 * This server should be run as separate thread.
 * Mark messages to all sniffers tranmitted by this routine.
 *
 * @param mark_data     Data for the marker packet.
 */
void
sniffer_mark_handler(char *mark_data_in)
{
    char            *ptr;
    int              len;
    int              rc;
    snif_mark_l     *mark;
    char             snif_id_str[RCF_MAX_ID];
    size_t           snif_len                   = RCF_MAX_ID;
    char            *snif_buf                   = NULL;
    char            *mark_data;
    struct timeval   ts;

    mark_data = mark_data_in + 1;
    if (mark_data_in[0] == '1')
    {
        sniffer_ins_mark_all(mark_data);
        free(mark_data_in);
        return;
    }

    SNIFFER_MALLOC(mark, sizeof(snif_mark_l));
    mark->message = NULL;

    ptr = strchr(mark_data, ' ');
    if (ptr == NULL)
        goto snif_mark_cleanup;
    len = ptr - mark_data;
    if ((len >= RCF_MAX_NAME) || (len < 0))
        goto snif_mark_cleanup;
    te_strlcpy(mark->agent, mark_data, RCF_MAX_NAME);
    /* Skip space. */
    ptr++;
    len++;

    rc = sniffer_parse_id_str(ptr, &mark->id);
    if ((rc <= 0) || (rc >= RCF_MAX_ID))
        goto snif_mark_cleanup;

    te_strlcpy(snif_id_str, ptr, RCF_MAX_ID);
    len += rc + 1;
    if ((unsigned)len > strlen(mark_data))
        goto snif_mark_cleanup;
    ptr += rc + 1;
    mark->message = strdup(ptr);

    gettimeofday(&ts, 0);
    SNIFFER_TS_CPY(mark->h.ts, ts);
    mark->h.caplen = strlen(mark->message) + SNIF_MARK_PSIZE;
    mark->h.len = mark->h.caplen;

    SNIFFER_MALLOC(snif_buf, snif_len);
    rc = rcf_ta_get_sniffers(mark->agent, snif_id_str, &snif_buf,
                             &snif_len, TRUE);
    if (snif_len == 0)
    {
        WARN("Couldn't get offset from the sniffer: %s", snif_id_str);
        goto snif_mark_cleanup;
    }

    rc = sniffer_parse_id_str(snif_buf, &mark->id);
    if ((rc <= 0) || (rc > RCF_MAX_ID - 1))
        goto snif_mark_cleanup;
    mark->id.abs_offset = strtoll(snif_buf + rc, NULL, 10);

    SLIST_INSERT_HEAD(&snif_mrk_h, mark, ent_l_m);

    if (snif_buf != NULL)
        free(snif_buf);
    free(mark_data_in);
    return;

snif_mark_cleanup:
    ERROR("Wrong mark message format.");
    if (snif_buf != NULL)
        free(snif_buf);
    free(mark_data_in);
    free(mark);
    return;
}

/**
 * Initialization of components to work of the sniffers.
 */
void
sniffers_init(void)
{
    pthread_mutex_init(&te_log_sniffer_mutex, NULL);
    SLIST_INIT(&snif_ta_h);
    SLIST_INIT(&snif_mrk_h);
}

/**
 * This is an entry point of sniffers message server.
 * This server should be run as separate thread.
 * All log messages from all sniffers entities on the agent
 * will be processed by this routine.
 *
 * @param agent     Agent name.
 */
void
sniffers_handler(char *agent)
{
    char           *snif_buf    = NULL;
    size_t          snif_len    = SNIF_MIN_LIST_SIZE;
    int             rc;
    snif_ta_l      *snif_ta;
    snif_id_l      *snif;

    if (snifp_sets.errors == TRUE)
    {
        ERROR("Sniffer polling configuration contains errors.");
        return;
    }
    polling_period = snifp_sets.period * 1000;

    SNIFFER_MALLOC(snif_ta, sizeof(snif_ta_l));

    te_strlcpy(snif_ta->agent, agent, RCF_MAX_NAME);
    SLIST_INIT(&snif_ta->snif_hl);
    pthread_mutex_lock(&te_log_sniffer_mutex);
    SLIST_INSERT_HEAD(&snif_ta_h, snif_ta, ent_l_ta);
    pthread_mutex_unlock(&te_log_sniffer_mutex);

    SNIFFER_MALLOC(snif_buf, snif_len);

    while (1)
    {
        SNIF_LOG_SLEEP;

        snif_len = SNIF_MIN_LIST_SIZE;
        rc = rcf_ta_get_sniffers(agent, NULL, &snif_buf, &snif_len, TRUE);
        if ((snif_len != 0) && (rc == 0))
        {
            sniffer_parse_list_buf(snif_buf, snif_len, &snif_ta->snif_hl,
                                   agent);

            SLIST_FOREACH(snif, &snif_ta->snif_hl, ent_l)
            {
                if (snif->log_exst)
                {
                    rc = ten_get_sniffer_dump(agent, snif);
                    if (rc == TE_RC(TE_RCF_API, TE_EIPC))
                        goto sniffers_handler_exit;
                }
                snif->log_exst = FALSE;
            }
        }
        else if (rc != 0 && rc != TE_RC(TE_RCF, TE_ENODATA))
            break;
    }

sniffers_handler_exit:

    if (snif_buf != NULL)
        free(snif_buf);

    sniffer_clean_list(&snif_ta->snif_hl);

    pthread_mutex_lock(&te_log_sniffer_mutex);
    SLIST_REMOVE(&snif_ta_h, snif_ta, snif_ta_l, ent_l_ta);
    pthread_mutex_unlock(&te_log_sniffer_mutex);

    free(snif_ta);
}
