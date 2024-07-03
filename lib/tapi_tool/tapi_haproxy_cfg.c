/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief HAProxy tool config file generation TAPI
 *
 * TAPI to handle HAProxy tool config file generation.
 */

#define TE_LGR_USER "TAPI HAPROXY CFG"

#include "tapi_haproxy_cfg.h"
#include "te_string.h"
#include "conf_api.h"
#include "tapi_file.h"

#define TAPI_HAPROXY_CONF_FILENAME_SUFFIX "haproxy.cfg"
#define INDENT "    "

const tapi_haproxy_cfg_opt tapi_haproxy_cfg_default_opt = {
    .nbthread = TAPI_JOB_OPT_UINT_UNDEF,
    .tune_listener_multi_queue = false,
    .tune_idle_pool_shared = false,
    .timeout_client_ms =
        TAPI_JOB_OPT_UINT_VAL(TAPI_HAPROXY_CONF_DEFAULT_TIMEOUT_MS),
    .timeout_server_ms =
        TAPI_JOB_OPT_UINT_VAL(TAPI_HAPROXY_CONF_DEFAULT_TIMEOUT_MS),
    .timeout_connect_ms =
        TAPI_JOB_OPT_UINT_VAL(TAPI_HAPROXY_CONF_DEFAULT_TIMEOUT_MS),
    .frontend.frontend_addr.addr = NULL,
    .frontend.frontend_addr.port = TAPI_JOB_OPT_UINT_UNDEF,
    .frontend.shards = TAPI_HAPROXY_CFG_SHARDS_BY_THREAD,
    .frontend.shards_n = TAPI_JOB_OPT_UINT_UNDEF,
    .backend.n = 0,
    .backend.backends = NULL,
};

static const te_enum_map tapi_haproxy_cfg_shards_mapping[] = {
    {.name = "", .value = TAPI_HAPROXY_CFG_SHARDS_NUMBER},
    {.name = " by-thread",  .value = TAPI_HAPROXY_CFG_SHARDS_BY_THREAD},
    {.name = " by-group",  .value = TAPI_HAPROXY_CFG_SHARDS_BY_GROUP},
    TE_ENUM_MAP_END
};

static const te_enum_map tapi_haproxy_cfg_bool_mapping[] = {
    {.name = "on", .value = true},
    {.name = "off",  .value = false},
    TE_ENUM_MAP_END
};

static const tapi_job_opt_bind haproxy_cfg_global_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_T("nbthread ", true, NULL,
                        tapi_haproxy_cfg_opt, nbthread),
    TAPI_JOB_OPT_ENUM_BOOL("tune.listener.multi-queue ", true,
                            tapi_haproxy_cfg_opt, tune_listener_multi_queue,
                            tapi_haproxy_cfg_bool_mapping),
    TAPI_JOB_OPT_ENUM_BOOL("tune.idle-pool.shared ", true,
                            tapi_haproxy_cfg_opt, tune_idle_pool_shared,
                            tapi_haproxy_cfg_bool_mapping)
);

static const tapi_job_opt_bind haproxy_cfg_defaults_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_UINT_T("timeout client ", true, NULL, tapi_haproxy_cfg_opt,
                        timeout_client_ms),
    TAPI_JOB_OPT_UINT_T("timeout server ", true, NULL, tapi_haproxy_cfg_opt,
                        timeout_server_ms),
    TAPI_JOB_OPT_UINT_T("timeout connect ", true, NULL, tapi_haproxy_cfg_opt,
                        timeout_connect_ms)
);

static const tapi_job_opt_bind haproxy_cfg_frontend_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_STRUCT("bind ", true, " ", NULL,
			TAPI_JOB_OPT_STRUCT(NULL, false, ":", NULL,
					    TAPI_JOB_OPT_STRING(NULL, false,
								tapi_haproxy_cfg_opt, frontend.frontend_addr.addr),
					    TAPI_JOB_OPT_UINT_T(NULL, false, NULL,
								tapi_haproxy_cfg_opt, frontend.frontend_addr.port)),
			TAPI_JOB_OPT_ENUM("shards", true, tapi_haproxy_cfg_opt,
					  frontend.shards, tapi_haproxy_cfg_shards_mapping),
			TAPI_JOB_OPT_UINT_T(NULL, false, NULL, tapi_haproxy_cfg_opt,
					    frontend.shards_n)),
    TAPI_JOB_OPT_STRING("default_backend ", true, tapi_haproxy_cfg_opt,
                        backend.name)
);

static const tapi_job_opt_bind haproxy_cfg_backends_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_DUMMY("mode http"),
    TAPI_JOB_OPT_DUMMY("balance static-rr"),
    TAPI_JOB_OPT_ARRAY_PTR(tapi_haproxy_cfg_opt,
        backend.n, backend.backends,
        TAPI_JOB_OPT_STRUCT("server ", true, " ", NULL,
			    TAPI_JOB_OPT_STRING(NULL, false,
						tapi_haproxy_cfg_backend, name),
			    TAPI_JOB_OPT_STRUCT(NULL, false, ":", NULL,
						TAPI_JOB_OPT_STRING(NULL, false,
								    tapi_haproxy_cfg_backend, backend_addr.addr),
						TAPI_JOB_OPT_UINT_T(NULL, false, NULL,
								    tapi_haproxy_cfg_backend, backend_addr.port))))
);

static char *
generate_conf_path(const char *ta)
{
    char *ta_tmp_dir;
    char *result;

    ta_tmp_dir = tapi_cfg_base_get_ta_dir(ta, TAPI_CFG_BASE_TA_DIR_TMP);

    result = tapi_file_make_custom_pathname(NULL, ta_tmp_dir,
                "_" TAPI_HAPROXY_CONF_FILENAME_SUFFIX);
    free(ta_tmp_dir);

    return result;
}

static void
build_cfg_group(const char *prefix_base,
                const char *group_name,
                const tapi_haproxy_cfg_opt *opt,
                const tapi_job_opt_bind *binds,
                const char *sep,
                te_string *res,
                bool last_group)
{
    te_string prefix = TE_STRING_INIT;
    te_vec args = TE_VEC_INIT(char *);

    te_string_append(&prefix, prefix_base);
    if (group_name != NULL)
       te_string_append(&prefix, " %s", group_name);

    tapi_job_opt_build_args(prefix.ptr, binds, opt, &args);
    te_string_join_vec(res, &args, sep);
    te_string_append(res, last_group ? "\n" : "\n\n");

    te_string_free(&prefix);
    te_vec_deep_free(&args);
}

/* See description in tapi_haproxy_cfg.h */
te_errno
tapi_haproxy_cfg_create(const char *ta, const tapi_haproxy_cfg_opt *opt,
                        char **result_pathname)
{
    te_string cfg_data = TE_STRING_INIT;
    char *path = NULL;
    te_errno rc;

    assert(result_pathname != NULL);

    if (opt == NULL)
        opt = &tapi_haproxy_cfg_default_opt;

    build_cfg_group("global", NULL, opt, haproxy_cfg_global_binds,
                    "\n" INDENT, &cfg_data, false);
    build_cfg_group("defaults", NULL, opt, haproxy_cfg_defaults_binds,
                    "\n" INDENT, &cfg_data, false);
    build_cfg_group("frontend", opt->frontend.name, opt,
                    haproxy_cfg_frontend_binds, "\n" INDENT, &cfg_data, false);
    build_cfg_group("backend", opt->backend.name, opt,
                    haproxy_cfg_backends_binds, "\n" INDENT, &cfg_data, true);

    path = generate_conf_path(ta);
    rc = tapi_file_create_ta(ta, path, "%s", cfg_data.ptr);
    te_string_free(&cfg_data);
    if (rc != 0)
    {
        free(path);
        ERROR("Failed to create HAProxy config file: %r", rc);
        return rc;
    }

    *result_pathname = path;

    return 0;
}

/* See description in tapi_haproxy_cfg.h */
void
tapi_haproxy_cfg_destroy(const char *ta, const char *cfg_file)
{
    if (cfg_file != NULL)
        tapi_file_ta_unlink_fmt(ta, "%s", cfg_file);
}