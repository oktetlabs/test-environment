/** @file
 * @brief Sniffer process
 *
 * Sniffer process implementation.
 *
 * Copyright (C) 2012 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * @author Andrey A. Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_errno.h"
#include "te_defs.h"
#include "te_sniffer_proc.h"
#include "te_sniffers.h"

#include <pcap/pcap.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/queue.h>

#define MAXIMUM_SNAPLEN 65535
#define SNIF_MAX_NAME 255

/* Time to wait while interface is down to next try, microseconds */
#define SNIF_WAIT_IF_UP 100000

/* Time to wait while memory is not freed, microseconds */
#define SNIF_WAIT_MEM 500000

/** Overfill type constans */
typedef enum overfill_type {
    ROTATION   = 0, /**< Overfill type rotation */
    TAIL_DROP  = 1, /**< Overfill type tail drop */
} overfill_type;

/**
 * Structure for dump files list
 */
struct file_list_s {
    char                        *name;     /**< File name */
    SIMPLEQ_ENTRY(file_list_s)   pointers;
};

/**
 * Structure contains information about dump files
 */
typedef struct dump_info {
    char           *template_file_name;     /**< File for the raw packets */
    char           *file_path;              /**< Capture file path */
    char           *file_name;              /**< File for the raw packets */
    size_t          file_size;              /**< Max file size */
    size_t          max_fnum;               /**< Max filse num to
                                                 rotation */
    size_t          log_num;                /**< Log sequence number */
    size_t          total_size;             /**< Total files size */
    pcap_dumper_t  *dumper;                 /**< Structure for capture file
                                                 descriptor */
    int             fd;                     /**< File descriptor */
    overfill_type   overfilltype;           /**< Overfill handle method: 
                                                 0 - rotation (default),
                                                 1 - tail drop. */
} dump_info;

static unsigned long long  absolute_offset;   /**< Absolute offset of the 
                                                   first byte in the first
                                                   packet */
static size_t              total_filled_mem;   /**< Total filled memory */
static pcap_t             *handle = NULL;      /**< Session handle. */
static te_bool             fstop;              /**< Stop flag */
static dump_info           dumpinfo;           /**< Dump files info */

/** Capture files list */
SIMPLEQ_HEAD(filelist, file_list_s) head_file_list;

/**
 * Putting file name into the list
 *
 * @param name      Full file name with path.
 *
 * @return Status code.
 */
static int
file_list_put(char *name)
{
    struct file_list_s *file;
    
    file = malloc(sizeof(struct file_list_s));
    if (file == NULL)
        return -1;
    file->name = name;
    
    if (SIMPLEQ_EMPTY(&head_file_list))
        SIMPLEQ_INSERT_HEAD(&head_file_list, file, pointers);
    else
        SIMPLEQ_INSERT_TAIL(&head_file_list, file, pointers);

    return 0;
}

/**
 * Remove first file in the list from disk and the file name from the list
 *
 * @return Status code.
 */
static int
file_list_rm_first(void)
{
    if ((SIMPLEQ_FIRST(&head_file_list) == NULL) || 
        (SIMPLEQ_FIRST(&head_file_list)->name == NULL))
        return -1;

    remove(SIMPLEQ_FIRST(&head_file_list)->name);
    free(SIMPLEQ_FIRST(&head_file_list)->name);
    SIMPLEQ_REMOVE_HEAD(&head_file_list, pointers);
    
    return 0;
}

/**
 * Calculate filled by dump files space on the disk.
 *
 * @return Total filled space on the disk.
 */
static size_t
used_space(void)
{
    size_t              total = 0;
    struct stat         st;
    struct file_list_s *f;
    
    SIMPLEQ_FOREACH(f, &head_file_list, pointers)
    {
        if (stat(f->name, &st) == 0)
            total += st.st_size;
    }
    
    return total;
}

/**
 * Calculate number of dump files on the disk.
 *
 * @return Total number files.
 */
static int
count_fnum(void)
{
    size_t              fnum = 0;
    struct file_list_s *f;
    
    SIMPLEQ_FOREACH(f, &head_file_list, pointers)
    {
        fnum++;
    }
    
    return fnum;
}

/**
 * Waiting for free space.
 */
static void
wait_mem_free(size_t total_size, long curr_offset)
{
    while (((used_space() + curr_offset) >= total_size) && !fstop)
        usleep(SNIF_WAIT_MEM);
}

/**
 * Make file name for raw capture file.
 *
 * @param dumpinfo      Structure contains information about dump files.
 *
 * @return Full file name or NULL.
 */
static char *
make_file_name(void)
{
    char   *file_name;
    int     res;

    file_name = malloc(SNIF_MAX_NAME + 1);
    if (file_name == NULL)
    {
        fprintf(stderr, "make_file_name: malloc\n");
        return NULL;
    }
    
    if (dumpinfo.template_file_name != NULL)
    {
        res = snprintf(file_name, SNIF_MAX_NAME + 1, "%s%s_%zd.pcap", 
                       dumpinfo.file_path, dumpinfo.template_file_name, 
                       dumpinfo.log_num);
    }
    else
    {
        res = snprintf(file_name, SNIF_MAX_NAME, "%s%.12llu_%zd.pcap", 
                       dumpinfo.file_path, absolute_offset,
                       dumpinfo.log_num);
    }
    
    if (res > SNIF_MAX_NAME)
    {
        fprintf(stderr, "make_file_name: Too long file name\n");
        free(file_name);
        return NULL;
    }

    dumpinfo.log_num++;

    return file_name;
}

/**
 * Read filter configuration file.
 * 
 * @param conf_file_name        Configuration file name.
 * 
 * @return Expression for the filter or NULL.
 */
static char *
read_conf_file(char *conf_file_name)
{
    char    *expression = NULL;
    FILE    *f;
    int      size = 0;
    int      res;
    int      i;

    f = fopen(conf_file_name, "r");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0)
    {
        fprintf(stderr, "read_conf_file: fseek\n");
        goto cleanup_rf;
    }
    
    expression = malloc(size + 1);
    if (expression == NULL)
    {
        fprintf(stderr, "read_conf_file: malloc\n");
        goto cleanup_rf;
    }
    
    res = fread(expression, size, 1, f);
    if (res != 1)
    {
        fprintf(stderr, "read_conf_file: read\n");
        goto cleanup_rf;
    }

    fclose(f);
    
    for (i = 0; i < size; i++) 
    {
        if (expression[i] == '#')
            while ((i < size) && (expression[i] != '\n'))
                expression[i++] = ' ';
    }
    expression[size] = '\0';
    
    return expression;
    
cleanup_rf:
    if (f != NULL)
        fclose(f);
    if (expression != NULL)
        free(expression);
    return NULL;
}

/**
 * Inser the marker packet into the capture file.
 * 
 * @param fd_o      A descriptor of the opened file.
 * @param msg   String for a message of packet.
 */
static void
insert_marker(FILE *f, const char *msg, struct timeval *ts)
{
    char                proto[SNIF_MARK_PSIZE];    /**< Protocol */
    int                 res;
    te_pcap_pkthdr      h;
    struct timeval      n_ts;

    if (ts == NULL)
    {
        gettimeofday(&n_ts, 0);
        SNIFFER_TS_CPY(h.ts, n_ts);
    }
    else 
        SNIFFER_TS_CPY(h.ts, *ts);

    h.caplen = strlen(msg) + SNIF_MARK_PSIZE;
    h.len = h.caplen;

    memset(proto, 0 , SNIF_MARK_PSIZE);
    SNIFFER_MARK_H_INIT(proto, strlen(msg));

    res = fwrite((void *)&h, sizeof(te_pcap_pkthdr), 1, f);
    if (res == -1)
        return;
    res = fwrite(proto, SNIF_MARK_PSIZE, 1, f);
    if (res == -1)
        return;
    fwrite(msg, strlen(msg), 1, f);
}

/**
 * Dumping packet to file.
 * 
 * @param user      User params.
 * @param h         Pcap packet header.
 * @param sp        Points to the first byte of a chunk of data containing 
 *                  the entire packet, as sniffed by pcap_loop().
 */
static void
dump_packet(unsigned char *user, const struct pcap_pkthdr *h, 
            const unsigned char *sp)
{
    long            offset;
    struct flock    lock;
    unsigned        fnum;
    UNUSED(user);

    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(dumpinfo.fd, F_SETLKW, &lock);
    lock.l_type = F_UNLCK;

    offset = pcap_dump_ftell(dumpinfo.dumper);

    if ((dumpinfo.file_size != 0) && 
        (offset + h->caplen + sizeof(struct pcap_pkthdr) >
        dumpinfo.file_size))
    {
        pcap_dump_close(dumpinfo.dumper);
        if (dumpinfo.fd != -1)
            close(dumpinfo.fd);

        absolute_offset += offset - SNIF_PCAP_HSIZE;
        total_filled_mem += offset;
        offset = 0;

        if ((dumpinfo.file_name = make_file_name()) == NULL)
        {
            fprintf(stderr, "dump_packet: Couldn't create file name\n");
            pcap_breakloop(handle);
            return;
        }
        dumpinfo.dumper = pcap_dump_open(handle, dumpinfo.file_name);
        if (dumpinfo.dumper == NULL)
        {
            fprintf(stderr, "dump_packet: %s\n", pcap_geterr(handle));
            pcap_breakloop(handle);
            return;
        }
        if (file_list_put(dumpinfo.file_name) != 0)
            fprintf(stderr, "Couldn't add dump file name to list!\n");
        if ((dumpinfo.fd = open(dumpinfo.file_name, O_RDWR)) == -1)
            fprintf(stderr,
                    "Couldn't get file descriptor of the dump file\n");
        fnum = count_fnum();
        if ((dumpinfo.max_fnum != 0) && (fnum > dumpinfo.max_fnum) &&
            (dumpinfo.overfilltype == ROTATION))
        {
            if (file_list_rm_first() == -1)
                fprintf(stderr, "Can't remove dump file\n");
            total_filled_mem = used_space();
        }
    }

    if ((dumpinfo.total_size != 0) && 
        ((total_filled_mem + offset + h->caplen) >= dumpinfo.total_size))
    {
        total_filled_mem = used_space();
        if (total_filled_mem + offset + h->caplen >= dumpinfo.total_size)
        {
            fcntl(dumpinfo.fd, F_SETLK, &lock);
            if (dumpinfo.overfilltype == ROTATION)
            {
                if (file_list_rm_first() == -1)
                    fprintf(stderr, "Can't remove dump file\n");
                total_filled_mem = used_space();
            }
            else
                wait_mem_free(dumpinfo.total_size, offset);
        }
    }

    pcap_dump((unsigned char *)dumpinfo.dumper, h, sp);

    fcntl(dumpinfo.fd, F_SETLK, &lock);
}


/** 
 * Make a clean exit on interrupts 
 */
static void
sign_cleanup(int signo)
{
    UNUSED(signo);
    fstop = 1;
    pcap_breakloop(handle);
}

/**
 * Usage information
 */
static void
usage(void)
{
    printf("Usage: sniffer [OPTION]...\n");
    printf("Example: te_sniffer -i any -s 300 -p -f 'ip' -P tmp/ -c 6000"
           " -C 2000 -q 1 -a snname -o\n");
    printf("    -a name                 Sniffer name, required arg\n");
    printf("    -C file_size            Max dump file size, defualt 0 "
           "(unlimited)\n");
    printf("    -c total_size           Max total files size, defualt 0"
           " (unlimited)\n");
    printf("    -f filter_expression    Filter expression, defualt none\n");
    printf("    -F conf_file            Filter expression file\n");
    printf("    -h                      Help\n");
    printf("    -i interface            Interface name, required arg\n");
    printf("    -o                      Change overfill handle method to"
           " \n");
    printf("                            tail drop, default type is "
           "rotation\n");
    printf("    -p                      Enable promiscuous mode, default "
           "disabled\n");
    printf("    -P file_path            Agent folder path, required arg\n");
    printf("    -r rotation_num         Max number files to rotate, "
           "defualt 0 (unlimited)\n");
    printf("    -q seq_num              Sniffer session sequence number, "
           "required arg\n");
    printf("    -s snaplen              Snapshot length in bytes, "
           "defulat 65535\n");
    printf("    -w file_name            Template for capture file name\n");
    exit(0);
}

/**
 * Wrong arguments information
 */
static void
wrong_arg(char optopt)
{
    switch(optopt)
    {
        case 'a':
            fprintf(stderr, "-%c without sniffer name\n", optopt);
            break;

        case 'c':
            fprintf(stderr, "-%c without total files size\n", optopt);
            break;

        case 'C':
            fprintf(stderr, "-%c without file size\n", optopt);
            break;

        case 'f':
            fprintf(stderr, "-%c without expression string\n", optopt);
            break;

        case 'F':
            fprintf(stderr, "-%c without configuration file name\n",
                    optopt);
            break;

        case 'i':
            fprintf(stderr, "-%c without interface name\n", optopt);
            break;

        case 'P':
            fprintf(stderr, "-%c without Agent folder path\n", optopt);
            break;

        case 'r':
            fprintf(stderr, "-%c without rotation files number\n", optopt);
            break;

        case 'q':
            fprintf(stderr, "-%c without sequence number\n", optopt);
            break;

        case 's':
            fprintf(stderr, "-%c without snaplen\n", optopt);
            break;

        case 'w':
            fprintf(stderr, "-%c without file name\n", optopt);
            break;

        default:
            fprintf(stderr, "-%c wrong argument\n", optopt);
        }
}

/**
 * Initializing global variables by default values
 */
static void
global_init(void)
{
    dumpinfo.template_file_name = NULL;
    dumpinfo.file_name          = NULL;
    dumpinfo.file_path          = NULL;
    dumpinfo.file_size          = 0;
    dumpinfo.total_size         = 0;
    dumpinfo.log_num            = 0;
    dumpinfo.max_fnum           = 0;
    dumpinfo.overfilltype       = ROTATION;

    total_filled_mem            = 0;
    fstop                       = 0;
    absolute_offset             = 0;
}

/* See description in te_sniffer_proc.h */
int 
te_sniffer_process(int argc, char *argv[])
{
    char  ebuf[PCAP_ERRBUF_SIZE];
    char *interface                 = NULL;
    char *conf_file_name            = NULL;
    char *filter_exp                = NULL;
    char *sniffer_name              = NULL;
    int   snaplen                   = 0;

    struct timeval ts;

    struct bpf_program   fp;                  /* The compiled filter */
    te_bool              pflag          = 0;  /* Promiscuous mode flag */
    unsigned long        sequence_num   = 0;
    int                  op;

    global_init();
    SIMPLEQ_INIT(&head_file_list);

    while ((op = getopt(argc, argv, ":a:c:C:f:F:hi:opP:q:r:s:w:")) != -1)
    {
        switch(op) 
        {
            case 'a':
                sniffer_name = optarg;
                break;

            case 'c':
                dumpinfo.total_size = atol(optarg) << 20;
                break;

            case 'C':
                dumpinfo.file_size = atol(optarg) << 20;
                break;

            case 'f':
                filter_exp = optarg;
                break;

            case 'F':
                conf_file_name = optarg;
                break;

            case 'h':
                usage();
                break;

            case 'i':
                interface = optarg;
                break;

            case 'o':
                dumpinfo.overfilltype = TAIL_DROP;
                break;

            case 'p':
                /* Brings the interface into promiscuous mode */
                pflag = 1;
                break;

            case 'P':
                dumpinfo.file_path = strdup(optarg);
                break;

            case 'r':
                dumpinfo.max_fnum = atol(optarg);
                break;

            case 'q':
                sequence_num = atol(optarg);
                break;

            case 's':
                snaplen = atoi(optarg);
                if ((snaplen > MAXIMUM_SNAPLEN) || (snaplen <= 0))
                    snaplen = MAXIMUM_SNAPLEN;
                break;

            case 'w':
                dumpinfo.template_file_name = optarg;
                break;

            case ':':
                wrong_arg(optopt);
                break;

            case '?':
            default:
                fprintf(stderr, "unknown arg %c\n", optopt);

        }
    }

    if ((interface != NULL) && (dumpinfo.file_path != NULL) && 
        (sniffer_name != NULL))
    {
        if (mkdir(dumpinfo.file_path, S_IRWXU | S_IRWXG | S_IRWXO) != 0 &&
            errno != EEXIST)
        {
            fprintf(stderr, "Couldn't create directory, %d\n", errno);
            goto cleanup;
        }
        if ((dumpinfo.file_name = make_file_name()) == NULL)
        {
            fprintf(stderr, "Couldn't create file name\n");
            goto cleanup;
        }
    }
    else
    {
        fprintf(stderr, "Mandatory arguments: interface name, sniffer "
                        "name, sniffer path, sequence number.\n");
        fprintf(stderr, "Type -h for more information.\n");
        goto cleanup;
    }

    if (conf_file_name != NULL)
    {
        filter_exp = read_conf_file(conf_file_name);
        if (filter_exp == NULL)
            fprintf(stderr, "Couldn't read filter file\n");
    }

    while (handle == NULL && !fstop)
    {
        gettimeofday(&ts, 0);
        handle = pcap_open_live(interface, snaplen, pflag, 1000, ebuf);
        usleep(SNIF_WAIT_IF_UP);
    }
    if (handle == NULL)
        goto cleanup;

    if (filter_exp != NULL)
    {
        if (pcap_compile(handle, &fp, filter_exp, 0, 0) == -1) 
        {
            fprintf(stderr, "Couldn't parse filter %s: %s\n",
                    filter_exp, pcap_geterr(handle));
            goto cleanup;
        }
        if (pcap_setfilter(handle, &fp) == -1) 
        {
            fprintf(stderr, "Couldn't install filter %s: %s\n", 
                    filter_exp, pcap_geterr(handle));
            goto cleanup;
        }
    }

    dumpinfo.dumper = pcap_dump_open(handle, dumpinfo.file_name);
    if (dumpinfo.dumper == NULL)
    {
        fprintf(stderr, "Couldn't open dump file %s", pcap_geterr(handle));
        goto cleanup;
    }

    if (file_list_put(dumpinfo.file_name) != 0)
            fprintf(stderr, "Can't add dump file name to list!\n");
    if ((dumpinfo.fd = open(dumpinfo.file_name, O_RDWR)) == -1)
        fprintf(stderr, "Couldn't get file descriptor of the dump file\n");

    signal(SIGTERM, sign_cleanup);
    signal(SIGINT, sign_cleanup);

    insert_marker((FILE *)dumpinfo.dumper,
                  "The sniffer process has been started.", &ts);

    while (!fstop)
        pcap_loop(handle, 0, dump_packet, NULL);

    insert_marker((FILE *)dumpinfo.dumper,
                  "Shutting down the sniffer process.", NULL);

cleanup:
    if (dumpinfo.dumper != NULL)
        pcap_dump_close(dumpinfo.dumper);
    if (handle != NULL)
        pcap_close(handle);

    return 0;
 }
