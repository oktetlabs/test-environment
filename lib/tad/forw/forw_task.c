/** @file
 * @breaf TE TAD Forwarder additional module internal declarations
 *
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAD Forw/Tasks"

#include "te_config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* For 'struct timeval' */ 
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "forwarder.h"
#include "asn_usr.h"
#include "ndn_forw.h"

#include "logger_api.h"
#include "logger_ta_fast.h"

#include "send_queue.h"

#define TV_RADIX 1000000

/**
 * Convert struct timeval to the correct form, if microseconds
 * field is too great.
 *
 * @param tv_   'struct timeval' variable.
 */
#define NORM_TIMEVAL(tv_) \
    do {                                                \
        if ((tv_).tv_usec > TV_RADIX)                   \
        {                                               \
            (tv_).tv_sec += (tv_).tv_usec / TV_RADIX;   \
            (tv_).tv_usec = (tv_).tv_usec % TV_RADIX;   \
        }                                               \
    } while (0)

/**
 * Structure with properties of forw task
 */
typedef struct forw_task_descr_t {
    struct forw_task_descr_t *next;  /**< Next descr in list */

    ndn_forw_action_plain  spec;     /**< Parsed parameters of task */

    asn_value      *asn_forw_action; /**< ASN value with task params */
    sendq_t        *sendq;           /**< Associated Send Queue */

    int             reordered_pkts;  /**< Counter of reordered packets */
    struct timeval  reorder_low;     /**< Low margin of current reorder 
                                          interval */
    struct timeval  reorder_up;      /**< High margin of current reorder
                                          interval */
} forw_task_descr_t;


#define HASH_SIZE 97 

static forw_task_descr_t *ftasks_hash_table[HASH_SIZE] = {0, };

/**
 * Caclulate hash function for forw tasks by name string.
 *
 * @param forw_name     name of forw task
 *
 * @return hash value
 */
static inline int 
ftask_hash(const char *forw_name)
{
    int i;
    int h_v;

    for (i = 0, h_v = 0; forw_name[i]; h_v += i * forw_name[i], i++);

    return h_v % HASH_SIZE;
}


/**
 * Insert forw task into hash. Resolve collisions.
 * If collision occures, this function insert forw task structure
 * into the list.
 *
 * @param ftask         pointer to the forw task structure
 *
 * @return zero on success or error code.
 */
static inline int 
ftask_hash_insert(forw_task_descr_t *ftask)
{
    int hash_index;

    if (ftask == NULL || ftask->spec.id == NULL)
        return TE_EWRONGPTR;

    hash_index = ftask_hash(ftask->spec.id);

    if (ftasks_hash_table[hash_index] == NULL)
    { 
        ftasks_hash_table[hash_index] = ftask;
        ftask->next = NULL;
    }
    else
    {
        forw_task_descr_t *ft_p, *prev = NULL;
        int                cmp_result = 1;

        for (ft_p = ftasks_hash_table[hash_index]; 
             ft_p != NULL && 
               ((cmp_result = strcmp(ftask->spec.id, ft_p->spec.id)) > 0);
             prev = ft_p, ft_p = ft_p->next);

        if (ft_p != NULL && cmp_result == 0) 
        {
            WARN("%s: Forw task with name %s is in hash", 
                 __FUNCTION__, ft_p->spec.id);
            return TE_EEXIST;
        }

        if (prev == NULL) 
            ftasks_hash_table[hash_index] = ftask;
        else
            prev->next = ftask;

        ftask->next = ft_p;
    }

    return 0;
}

/**
 * Find in hash forw task with specified name
 *
 * @param forw_name     name of forw task to be found
 *
 * @return pointer to found forw task structure or NULL if not found
 */
static inline forw_task_descr_t *
ftask_hash_find(const char *forw_name)
{
    int hash_value;
    int cmp_result = 1;

    forw_task_descr_t *ft_p;
    
    if (forw_name == NULL)
        return NULL;

    hash_value = ftask_hash(forw_name); 

    for (ft_p = ftasks_hash_table[hash_value];
         ft_p != NULL && 
                 ((cmp_result = strcmp(forw_name, ft_p->spec.id)) < 0);
         ft_p = ft_p->next);

    if (ft_p != NULL && cmp_result == 0)
        return ft_p;

    return NULL;
}

/**
 * Remove from hash forw task descriptor
 *
 * @param ftask_p       pointer to forw task structure
 *
 * @return zero on success or error code.
 */
static inline int
ftask_hash_remove(forw_task_descr_t *ftask_p)
{
    forw_task_descr_t *ft_p, *prev; 

    int hash_value;

    if (ftask_p == NULL)
        return TE_EINVAL;

    hash_value = ftask_hash(ftask_p->spec.id);

    if (hash_value < 0 || hash_value >= HASH_SIZE)
        return TE_EFAULT;

    for (ft_p = ftasks_hash_table[hash_value], prev = NULL; 
         ft_p != NULL;
         prev = ft_p, ft_p = ft_p->next)
    {
        if (ft_p == ftask_p)
        { 
            if (prev == NULL)
                ftasks_hash_table[hash_value] = ft_p->next;
            else
                prev->next = ft_p->next;

            ftask_p->next = NULL;

            return 0;
        }
    }

    return TE_ENOENT;
}

/* See description in forwader.h */
int
tadf_add_forw_task(const char *forw_asn_spec, int sendq_id)
{
    int rc = 0;
    int syms = 0;

    asn_value           *forward_action_spec;
    forw_task_descr_t   *new_task_descr = NULL;

    VERB("%s called", __FUNCTION__);

    do {
        if (forw_asn_spec == NULL)
        {
            rc = TE_EWRONGPTR;
            break;
        }

        rc = asn_parse_value_text(forw_asn_spec, ndn_forw_action,
                                  &forward_action_spec, &syms);
        if (rc)
        {
            ERROR("%s: ASN parse %r, sym %d", __FUNCTION__, rc, syms);
            break;
        }

        new_task_descr = calloc(1, sizeof(*new_task_descr));
        if (new_task_descr == NULL)
        {
            rc = TE_ENOMEM;
            break;
        }

        new_task_descr->asn_forw_action = forward_action_spec;

        rc = ndn_forw_action_asn_to_plain(forward_action_spec,
                                          &(new_task_descr->spec));
        if (rc != 0)
        {
            ERROR("%s: convert ASN to plain value failed: %r",
                    __FUNCTION__, rc);
            break;
        }

        if ((new_task_descr->sendq = tadf_sendq_find(sendq_id)) == NULL)
        {
            ERROR("%s: send queue %d not exists", __FUNCTION__, sendq_id);
            rc = TE_ENOENT;
            break;
        }

        rc = ftask_hash_insert(new_task_descr);
        if (rc != 0)
        {
            ERROR("Forw task %s entry insert fails %r",
                  new_task_descr->spec.id, rc);
            break;
        }
    } while(0);

    if (rc != 0 && new_task_descr != NULL)
    {
        /*
         * Last operation, which may fail, is insertion into hash,
         * so if error occured, in any case, entry is not inserted in
         * hash. Do not try to remove it.
         */
        asn_free_value(new_task_descr->asn_forw_action);
        free(new_task_descr);
    }

    return TE_RC(TE_TA_EXT, rc);
}

/* See description in forwader.h */
int 
tadf_del_forw_task(const char *forw_name)
{
    forw_task_descr_t   *ftask_descr;

    if (forw_name == NULL)
        return TE_RC(TE_TA_EXT, TE_EWRONGPTR);

    ftask_descr = ftask_hash_find(forw_name); 

    if (ftask_descr == NULL)
    {
        ERROR("%s: no such forw task '%s'", __FUNCTION__, forw_name);
        return TE_RC(TE_TA_EXT, TE_EINVAL);
    }

    ftask_hash_remove(ftask_descr);

    asn_free_value(ftask_descr->asn_forw_action);

    free(ftask_descr);

    INFO("%s: forw task '%s' successfully deleted", 
         __FUNCTION__, forw_name);

    return 0;
}

/* See description in forwader.h */
int 
tadf_forw_task_set_param(const char *forw_name, 
                         const char *param_spec, int value)
{
    int rc = 0;

    forw_task_descr_t   *ftask_descr;

    do { 
        if (forw_name == NULL || param_spec == NULL)
        {
            ERROR("%s: Invalid input parameter", __FUNCTION__);
            rc = TE_EWRONGPTR;
            break;
        }

        ftask_descr = ftask_hash_find(forw_name); 

        if (ftask_descr == NULL)
        {
            ERROR("%s: No forw task %s found", __FUNCTION__, forw_name);
            rc = TE_ENOENT;
            break;
        }

        VERB("%s: task %s, param spec %s, value %d", 
             __FUNCTION__, forw_name, param_spec, value);

        rc = asn_write_value_field(ftask_descr->asn_forw_action, 
                                   &value, sizeof(value), param_spec);
        if (rc)
        {
            ERROR("%s: ASN write failed with rc %r", __FUNCTION__, rc);
            break;
        } 

        if (strncmp(param_spec, "drop", strlen("drop")) == 0)
        {
            VERB("%s: drop parameter was passed");
            /* Drop mask cannot be changed dynamically - due to passing
             * value problem */
            switch (ftask_descr->spec.drop.type)
            {
                case FORW_DROP_PATTERN:
                    ERROR("Drop specified by pattern, set param denied");
                    rc = TE_EINVAL;
                    break;

                case FORW_DROP_DISABLED:
                    ftask_descr->spec.drop.type = FORW_DROP_RANDOM;
                    /* fall through */
                case FORW_DROP_RANDOM:
                    ftask_descr->spec.drop.rate = value;
                    break;
            }

            /* No more to do, all further drops will use new value */
            break;
        }

        if (strncmp(param_spec, "delay", strlen("delay")) == 0)
        {
            VERB("%s: delay parameter was passed");
            switch (ftask_descr->spec.delay.type)
            {
                case FORW_DELAY_DISABLED:
                    ftask_descr->spec.delay.type = FORW_DELAY_CONSTANT;
                    /* fall through */
                case FORW_DELAY_CONSTANT: 
                    ftask_descr->spec.delay.min = 
                        ftask_descr->spec.delay.max = value;
                    VERB("Setting delay to constant %d mcs", value);
                    break;

                case FORW_DELAY_RAND_CONT:
                    if (strstr(param_spec, "min") != NULL)
                    {
                        VERB("Setting min delay to %d mcs", value);
                        ftask_descr->spec.delay.min = value;
                    }
                    else
                    {
                        VERB("Setting max delay to %d mcs", value);
                        ftask_descr->spec.delay.max = value;
                    }
                    break;

                case FORW_DELAY_RAND_DISCR:
                    WARN("Set param for discrete delay unsupported");
                    rc = TE_EOPNOTSUPP;
                    break; 
            }
            break;
        }

        if (rc != 0)
            break;

        if (strncmp(param_spec, "reorder", strlen("reorder")) == 0)
        {
            if (strstr(param_spec, "type") != NULL)
            {
                VERB("%s: ftask %s, set reorder type to %d", 
                     __FUNCTION__, ftask_descr->spec.id, value);

                if (ftask_descr->spec.reorder.type == FORW_REORDER_DISABLED &&
                    ftask_descr->spec.reorder.timeout == 0 && 
                    ftask_descr->spec.reorder.r_size  == 0)
                {
                    ftask_descr->spec.reorder.timeout = 1;
                    ftask_descr->spec.reorder.r_size  = 1; 
                    VERB("%s: ftask %s, set reorder timeout and size "
                         "to defaults", 
                         __FUNCTION__, ftask_descr->spec.id);
                }
                ftask_descr->spec.reorder.type = value;
            }
            else if (strstr(param_spec, "timeout") != NULL)
            {
                VERB("%s: ftask %s, set reorder timeout to %d", 
                     __FUNCTION__, ftask_descr->spec.id, value);
                ftask_descr->spec.reorder.timeout = value;
            }
            else if (strstr(param_spec, "size") != NULL)
            {
                VERB("%s: ftask %s, set reorder size to %d", 
                     __FUNCTION__, ftask_descr->spec.id, value);
                ftask_descr->spec.reorder.r_size = value;
            }
            else
            {
                ERROR("unsupported reorder parameter specification: '%s'",
                      param_spec);
            }
        }

    } while (0);

    return TE_RC(TE_TA_EXT, rc);
}




/* See description in forwader.h */
int 
tadf_forw_packet(csap_p csap_descr, const char *forw_name,
                 const uint8_t *pkt, size_t pkt_len)
{
    int rc = 0;

    struct timeval current_tv;
    struct timeval send_tv;

    forw_task_descr_t   *ftask_descr;

    UNUSED(csap_descr);

    gettimeofday(&current_tv, NULL);
    send_tv = current_tv;

    VERB("%s, name: '%s', data ptr %r, len %d, curr %u.%u", 
           __FUNCTION__, forw_name, pkt, pkt_len, 
           current_tv.tv_sec, current_tv.tv_usec);

    do { 
        ftask_descr = ftask_hash_find(forw_name);

        if (ftask_descr == NULL)
        {
            ERROR("%s: no such task %s", __FUNCTION__, forw_name);
            rc = TE_ENOENT;
            break;
        }

        switch (ftask_descr->spec.drop.type) 
        {
            case FORW_DROP_RANDOM:
                {
                    /*random value between 0 and 99 */
                    /* Drop rate 100 means drop everything */
                    int rand_val;
                    
                    if (ftask_descr->spec.drop.rate == 0)
                        break;

                    if (ftask_descr->spec.drop.rate >= 100)
                    {
                        F_VERB("test drop pkt, rate %d",
                               ftask_descr->spec.drop.rate); 
                        return 0;
                    }
                    
                    rand_val = (random() % 100);

                    F_VERB("test drop pkt, rand %d, rate %d",
                           rand_val, ftask_descr->spec.drop.rate); 

                    if (rand_val < ftask_descr->spec.drop.rate)
                        return 0; 
                }
                break;
            case FORW_DROP_PATTERN:
                ERROR("%s, ftask %s: pattern dropping not supported yet",
                        __FUNCTION__, forw_name);
                rc = TE_EOPNOTSUPP;
                break;
            case FORW_DROP_DISABLED:
                F_VERB("No drop specified");
                break;
        }
        if (rc)
            break; 

        if (ftask_descr->spec.reorder.type != FORW_REORDER_DISABLED)
        {
            F_VERB("Reorder specified, num %d, timeout %d mcs", 
                   ftask_descr->spec.reorder.r_size, 
                   ftask_descr->spec.reorder.timeout); 

            if (timeval_compare(ftask_descr->reorder_up, 
                                current_tv) < 0)
            {
                ftask_descr->reorder_up = current_tv;
                ftask_descr->reordered_pkts = 0;
                F_VERB("reorder_up is earlier then current, re-init it");
            }
            else
                F_VERB("reorder_up is later then current, left old");

            if (ftask_descr->reordered_pkts == 0)
            { 
                ftask_descr->reorder_low = ftask_descr->reorder_up; 
                ftask_descr->reorder_up = current_tv;
                ftask_descr->reorder_up.tv_usec += 
                    (ftask_descr->spec.reorder.timeout * 2) / 3; 

                F_VERB("Re-init reorder time limits; [%u.%u, %u.%u]",
                       ftask_descr->reorder_low.tv_sec, 
                       ftask_descr->reorder_low.tv_usec, 
                       ftask_descr->reorder_up.tv_sec, 
                       ftask_descr->reorder_up.tv_usec); 
            } 

            switch (ftask_descr->spec.reorder.type)
            {
                case FORW_REORDER_RANDOM:
                    {
                        int rand_val = random();
                        int interval = ftask_descr->reorder_up.tv_sec - 
                                       ftask_descr->reorder_low.tv_sec;
                        interval *= TV_RADIX;
                        interval += ftask_descr->reorder_up.tv_usec - 
                                    ftask_descr->reorder_low.tv_usec; 

                        rand_val %= interval;
                        send_tv.tv_usec += rand_val;
                        F_VERB("Random delay in reorder: %d", rand_val);
                    }
                    break;

                case FORW_REORDER_REVERSED:
                    send_tv = ftask_descr->reorder_up;
                    F_VERB("Reverse reorder, send at %d.%d ", 
                           send_tv.tv_sec, send_tv.tv_usec);

                    if (ftask_descr->reorder_up.tv_usec > 0)
                        ftask_descr->reorder_up.tv_usec--;
                    else
                        ftask_descr->reorder_up.tv_sec--,
                        ftask_descr->reorder_up.tv_usec = TV_RADIX-1 ;

                    break;

                default:
                    ERROR("Unexpected reorder type %d", 
                          ftask_descr->spec.reorder.type);
                    rc = TE_EINVAL;
                    break;
            }

            ftask_descr->reordered_pkts++;
            if (ftask_descr->reordered_pkts == 
                ftask_descr->spec.reorder.timeout)
            {
                F_VERB("pktnum is reach limit %d, break to zero", 
                       ftask_descr->spec.reorder.timeout);
                ftask_descr->reordered_pkts = 0;
            } 
        }

        if (rc != 0)
            break;

        switch (ftask_descr->spec.delay.type)
        {
            case FORW_DELAY_DISABLED:
                F_VERB("No delay specified");
                break;
            case FORW_DELAY_RAND_CONT:
                {
                    int interval_len = ftask_descr->spec.delay.max -
                                       ftask_descr->spec.delay.min;
                    int rand_val = (random() % interval_len);

                    send_tv.tv_usec += rand_val;
                }
                /* fall through */
            case FORW_DELAY_CONSTANT:
                send_tv.tv_usec += ftask_descr->spec.delay.min;
                NORM_TIMEVAL(send_tv);
                break;
            case FORW_DELAY_RAND_DISCR:
                {
                    int rand_val = (random() % 100); 
                    int prev_threshold = 0;

                    unsigned i;

                    F_VERB("discrete random delay, rand val %d", rand_val);

                    ndn_delay_discr_pair_t *discr_pair_p;

                    for (i = 0, discr_pair_p = ftask_descr->spec.delay.discr; 
                         i < ftask_descr->spec.delay.n_pairs; 
                         i++, discr_pair_p++)
                    {
                        if (prev_threshold <= rand_val && 
                            rand_val < prev_threshold + discr_pair_p->prob)
                        {
                            send_tv.tv_usec += discr_pair_p->delay;
                            F_VERB("fall into %d interval, delay %d mcs", 
                                 i, discr_pair_p->delay);
                            break;
                        }
                        else
                            prev_threshold += discr_pair_p->prob; 
                    }
                }
                NORM_TIMEVAL(send_tv);
                break; 
            default:
                RING("Unexpected delay type %d", 
                     ftask_descr->spec.delay.type);
                break;
        }

        rc = tadf_sendq_put_pkt(ftask_descr->sendq, pkt, pkt_len, send_tv);
        VERB("%s: pkt put to send queue, tv=(%d.%d), rc %r", __FUNCTION__,
             send_tv.tv_sec, send_tv.tv_usec, rc);
    } while (0);

    return TE_RC(TE_TA_EXT, rc);
}
