/** @file
 * @brief Test API to control rxq_stats BPF program.
 *
 * Implementation of API to control rxq_stats BPF program.
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 */

#include "te_config.h"

#include "te_errno.h"
#include "te_string.h"
#include "te_str.h"
#include "te_alloc.h"
#include "conf_api.h"
#include "logger_api.h"
#include "tapi_test_log.h"
#include "tapi_bpf.h"
#include "tapi_bpf_rxq_stats.h"

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_init(const char *ta, const char *if_name,
                        const char *prog_dir,
                        unsigned int *bpf_id)
{
    char *ta_dir = NULL;
    unsigned int id = 0;
    te_string prog_path = TE_STRING_INIT;
    te_errno rc;

    if (prog_dir == NULL || *prog_dir != '/')
    {
        rc = cfg_get_instance_string_fmt(&ta_dir, "/agent:%s/dir:",
                                         ta);
        if (rc != 0)
            goto finish;

        rc = te_string_append(&prog_path, "%s/", ta_dir);
        if (rc != 0)
            goto finish;
    }

    if (prog_dir != NULL)
    {
        rc = te_string_append(&prog_path, "%s", prog_dir);
        if (rc != 0)
            goto finish;
    }

    rc = te_string_append(&prog_path, "/rxq_stats.o");
    if (rc != 0)
        goto finish;

    rc = tapi_bpf_obj_init(ta, te_string_value(&prog_path),
                           TAPI_BPF_PROG_TYPE_XDP, &id);
    if (rc != 0)
        goto finish;

    if (bpf_id != NULL)
        *bpf_id = id;

    rc = tapi_bpf_prog_link(ta, if_name, id, TAPI_BPF_LINK_XDP,
                            "rxq_stats");
    if (rc != 0)
        goto finish;

    rc = tapi_bpf_map_set_writable(ta, id, "params");
    if (rc != 0)
        goto finish;

finish:

    free(ta_dir);
    te_string_free(&prog_path);

    return rc;
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_fini(const char *ta, const char *if_name,
                        unsigned int bpf_id)
{
    te_errno rc;

    rc = tapi_bpf_prog_unlink(ta, if_name, TAPI_BPF_LINK_XDP);
    if (rc != 0)
        return rc;

    return tapi_bpf_obj_fini(ta, bpf_id);
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_get_id(const char *ta, const char *if_name,
                          unsigned int *bpf_id)
{
    char *prog_oid_str = NULL;
    cfg_oid *prog_oid = NULL;
    const char *prog_name = NULL;
    te_errno rc;

    rc = cfg_get_instance_string_fmt(
                  &prog_oid_str,
                  "/agent:%s/interface:%s/xdp:",
                  ta, if_name);
    if (rc != 0)
        goto finish;

    if (prog_oid_str == NULL || *prog_oid_str == '\0')
    {
        ERROR("%s(): no XDP program is linked to %s interface on agent %s",
              __FUNCTION__, if_name, ta);
        rc = TE_RC(TE_TAPI, TE_ENOENT);
        goto finish;
    }

    prog_oid = cfg_convert_oid_str(prog_oid_str);
    if (prog_oid == NULL)
    {
        ERROR("%s(): failed to convert '%s' to OID", __FUNCTION__,
              prog_oid_str);
        rc = TE_RC(TE_TAPI, TE_EFAIL);
        goto finish;
    }
    if (!prog_oid->inst || prog_oid->len < 4)
    {

        ERROR("%s(): incorrect BPF OID '%s' for agent %s interface %s",
              __FUNCTION__, prog_oid_str, ta, if_name);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto finish;
    }

    prog_name = CFG_OID_GET_INST_NAME(prog_oid, 3);
    if (strcmp(prog_name, "rxq_stats") != 0)
    {
        ERROR("%s(): unexpected XDP program '%s' is linked to interface %s "
              "on agent %s", __FUNCTION__, prog_name, if_name, ta);
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto finish;
    }

    rc = te_strtoui(CFG_OID_GET_INST_NAME(prog_oid, 2), 10, bpf_id);

finish:

    cfg_free_oid(prog_oid);
    free(prog_oid_str);

    return rc;
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_set_params(const char *ta, unsigned int bpf_id,
                              int addr_family,
                              const struct sockaddr *src_addr,
                              const struct sockaddr *dst_addr,
                              int protocol, te_bool enable)
{
    te_bpf_rxq_stats_params params;
    uint32_t key = 0;
    te_errno rc;

    memset(&params, 0, sizeof(params));
    params.enabled = (enable ? 1 : 0);

    rc = tapi_bpf_ip_tcpudp_filter_from_sa(&params.filter,
                                           addr_family, protocol,
                                           src_addr, dst_addr);
    if (rc != 0)
        return rc;

    return tapi_bpf_map_update_kvpair(
                                ta, bpf_id, "params",
                                (uint8_t *)&key, sizeof(key),
                                (uint8_t *)&params, sizeof(params));
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_enable(const char *ta, unsigned int bpf_id,
                          te_bool enable)
{
    te_bpf_rxq_stats_params params;
    uint32_t key = 0;
    te_errno rc;

    rc = tapi_bpf_map_lookup_kvpair(ta, bpf_id, "params",
                                    (uint8_t *)&key, sizeof(uint32_t),
                                    (uint8_t *)&params, sizeof(params));
    if (rc != 0)
        return rc;

    params.enabled = (enable ? 1 : 0);

    return tapi_bpf_map_update_kvpair(
                                ta, bpf_id, "params",
                                (uint8_t *)&key, sizeof(key),
                                (uint8_t *)&params, sizeof(params));
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_clear(const char *ta, unsigned int bpf_id)
{
    te_errno rc = 0;
    te_errno rc_aux = 0;
    uint8_t **keys = NULL;
    unsigned int key_size;
    unsigned int count = 0;
    unsigned int i;

    rc = tapi_bpf_map_get_key_list(ta, bpf_id, "queue_stats",
                                   &key_size, &keys, &count);
    if (rc != 0)
        return rc;

    if (count == 0)
        return 0;

    rc = tapi_bpf_map_set_writable(ta, bpf_id, "queue_stats");
    if (rc != 0)
        goto finish;

    for (i = 0; i < count; i++)
    {
        rc = tapi_bpf_map_delete_kvpair(ta, bpf_id, "queue_stats",
                                        keys[i], key_size);
        if (rc != 0)
            goto finish;
    }

finish:

    for (i = 0; i < count; i++)
        free(keys[i]);

    free(keys);

    rc_aux = tapi_bpf_map_unset_writable(ta, bpf_id, "queue_stats");
    if (rc == 0)
        rc = rc_aux;

    return rc;
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_reset(const char *ta, unsigned int bpf_id)
{
    te_errno rc = 0;
    unsigned int key_size = 0;
    uint8_t **keys = NULL;
    unsigned int count = 0;
    unsigned int i;

    /*
     * Here list of keys is obtained to avoid trying to remove
     * already not existing parameters.
     */
    rc = tapi_bpf_map_get_key_list(ta, bpf_id, "params", &key_size,
                                   &keys, &count);
    if (rc != 0)
        return rc;

    for (i = 0; i < count; i++)
    {
        rc = tapi_bpf_map_delete_kvpair(ta, bpf_id, "params",
                                        keys[i], key_size);
        if (rc != 0)
            goto finish;
    }

finish:

    for (i = 0; i < count; i++)
        free(keys[i]);

    free(keys);

    if (rc != 0)
        return rc;

    return tapi_bpf_rxq_stats_clear(ta, bpf_id);
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_read(const char *ta, unsigned int bpf_id,
                        tapi_bpf_rxq_stats **stats_out,
                        unsigned int *count_out)
{
    uint8_t **keys = NULL;
    unsigned int key_size;
    unsigned int count;
    uint32_t i;
    tapi_bpf_rxq_stats *stats = NULL;
    te_errno rc;

    rc = tapi_bpf_map_get_key_list(ta, bpf_id, "queue_stats",
                                   &key_size, &keys, &count);
    if (rc != 0)
        return rc;

    stats = TE_ALLOC(count * sizeof(*stats));
    if (stats == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_ENOMEM);
        goto finish;
    }

    for (i = 0; i < count; i++)
    {
        stats[i].rx_queue = *(uint32_t *)(keys[i]);
        rc = tapi_bpf_map_lookup_kvpair(ta, bpf_id, "queue_stats",
                                        keys[i], key_size,
                                        (uint8_t *)&stats[i].pkts,
                                        sizeof(uint64_t));
        if (rc != 0)
            goto finish;
    }

    *stats_out = stats;
    *count_out = count;

finish:

    for (i = 0; i < count; i++)
        free(keys[i]);

    free(keys);

    if (rc != 0)
        free(stats);

    return rc;
}

/* See description in tapi_bpf_rxq_stats.h */
void
tapi_bpf_rxq_stats_print(const char *title, tapi_bpf_rxq_stats *stats,
                         unsigned int count)
{
    te_string str = TE_STRING_INIT;
    unsigned int i;

    if (title == NULL || *title == '\0')
        title = "Packets counted by rxq_stats";

    te_string_append(&str, "%s\n", title);
    for (i = 0; i < count; i++)
    {
        te_string_append(&str, "Rx queue %u: %llu packets\n",
                         stats[i].rx_queue,
                         (long long unsigned int)(stats[i].pkts));
    }

    RING("%s", te_string_value(&str));
    te_string_free(&str);
}

/* See description in tapi_bpf_rxq_stats.h */
te_errno
tapi_bpf_rxq_stats_check_single(const char *ta, unsigned int bpf_id,
                                unsigned int exp_queue,
                                unsigned int exp_pkts,
                                rpc_socket_type sock_type,
                                const char *vpref)
{
    tapi_bpf_rxq_stats *stats = NULL;
    unsigned int stats_count;
    unsigned int i;
    unsigned int exp_queue_recv = 0;
    te_bool unexp_num = TRUE;
    te_bool unexp_queue = FALSE;
    te_bool failed = FALSE;
    te_errno rc;

    if (vpref == NULL || *vpref == '\0')
        vpref = "Checking Rx queues statistics";

    rc = tapi_bpf_rxq_stats_read(ta, bpf_id, &stats, &stats_count);
    if (rc != 0)
        return rc;

    tapi_bpf_rxq_stats_print(NULL, stats, stats_count);

    for (i = 0; i < stats_count; i++)
    {
        if (stats[i].rx_queue == exp_queue)
        {
            exp_queue_recv = stats[i].pkts;

            if (stats[i].pkts != exp_pkts)
            {
                /*
                 * TCP may receive more packets if there are
                 * retransmits or some packets are split.
                 */
                if (!(sock_type == RPC_SOCK_STREAM &&
                      stats[i].pkts > exp_pkts))
                {
                    ERROR("Queue %u got %llu packets instead of %u",
                          exp_queue,
                          (long long unsigned int)(stats[i].pkts),
                          exp_pkts);
                }
                else
                {
                    unexp_num = FALSE;
                }
            }
            else
            {
                unexp_num = FALSE;
            }
        }
        else
        {
            if (stats[i].pkts != 0)
            {
                ERROR("Queue %u received packets unexpectedly",
                      stats[i].rx_queue);
                unexp_queue = TRUE;
            }
        }
    }

    if (unexp_num)
    {
        ERROR_VERDICT("%s: expected Rx queue received %s packets",
                      vpref, (exp_queue_recv == 0 ?
                                      "zero" : "unexpected number of"));
        failed = TRUE;
    }
    if (unexp_queue)
    {
        ERROR_VERDICT("%s: other queue than expected received packets",
                      vpref);
        failed = TRUE;
    }

    free(stats);

    if (failed)
        return TE_RC(TE_TAPI, TE_EFAIL);

    return 0;
}
