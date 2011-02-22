/**
 * Set shell environment for the given agent and may be restart
 * a PCO so it's aware.
 *
 * @param pco            PCO handle
 * @param env_name       Name of the environment variable
 * @param env_value      New value
 * @param force          Should we only add or overwrite?
 * @param restart        Should the PCO be restarted
 *
 * @result errno
 */
inline te_errno tapi_sh_env_setenv(rcf_rpc_server *pco,
                                   const char *env_name,
                                   const char *env_value,
                                   te_bool force,
                                   te_bool restart)
{
    cfg_handle handle;
    te_errno rc = 0;

    rc = cfg_find_fmt(&handle, "/agent:%s/env:%s", pco->ta, env_name);
    if (rc != 0)
    {
        rc = 0;
        /* add the instance */
        CHECK_RC(cfg_add_instance_fmt(&handle,
                                      CFG_VAL(STRING, env_value),
                                      "/agent:%s/env:%s",
                                      pco->ta, env_name));
    }
    else if (force)
        /* exists */
        CHECK_RC(cfg_set_instance_fmt(CFG_VAL(STRING, env_value),
                                      "/agent:%s/env:%s",
                                      pco->ta, env_name));
    else
        rc = TE_EEXIST;

    if (rc == 0 && restart)
        rcf_rpc_server_restart(pco);

    return rc;
}


/**
 * Unset environment for the agent and may be restart given PCO.
 *
 * @param pco         PCO handle
 * @param env_name    Name of the environment variable
 * @param restart     Should the PCO be restarted?
 * @param empty       Ignore if the variable was not set
 */
extern te_errno tapi_sh_env_unset(rcf_rpc_server *pco, const char *env_name,
                                  te_bool restart, te_bool empty)
{
    cfg_handle handle;
    te_errno rc = 0;

    rc = cfg_find_fmt(&handle, "/agent:%s/env:%s", pco->ta, env_name);
    if (rc != 0)
        rc = TE_ENOENT;
    else
        CHECK_RC(cfg_del_instance(handle, 1));

    /* no need to restart if nothing was changed */
    if (rc == 0 && restart)
        rcf_rpc_server_restart(pco);

    return (empty && rc == TE_ENOENT) ? 0 : rc;
}

