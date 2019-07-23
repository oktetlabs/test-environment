/** @file
 * @brief Test API to configure BPF/XDP programs.
 *
 * @defgroup tapi_bpf BPF/XDP configuration of Test Agents
 * @ingroup te_ts_tapi
 * @{
 *
 * Definition of API to configure BPF/XDP programs.
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Damir Mansurov <Damir.Mansurov@oktetlabs.ru>
 */

#ifndef __TE_TAPI_BPF_H__
#define __TE_TAPI_BPF_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * BPF object states
 */
typedef enum tapi_bpf_state {
    TAPI_BPF_STATE_UNLOADED = 0,    /**< Object is not loaded into the kernel */
    TAPI_BPF_STATE_LOADED           /**< Object is loaded into the kernel */
} tapi_bpf_state;

/**
 * BPF program types
 */
typedef enum tapi_bpf_prog_type {
    TAPI_BPF_PROG_TYPE_UNSPEC = 0,
    TAPI_BPF_PROG_TYPE_SOCKET_FILTER,
    TAPI_BPF_PROG_TYPE_KPROBE,
    TAPI_BPF_PROG_TYPE_SCHED_CLS,
    TAPI_BPF_PROG_TYPE_SCHED_ACT,
    TAPI_BPF_PROG_TYPE_TRACEPOINT,
    TAPI_BPF_PROG_TYPE_XDP,
    TAPI_BPF_PROG_TYPE_PERF_EVENT,
    TAPI_BPF_PROG_TYPE_UNKNOWN
} tapi_bpf_prog_type;

/**
 * BPF map types
 */
typedef enum tapi_bpf_map_type {
    TAPI_BPF_MAP_TYPE_UNSPEC = 0,
    TAPI_BPF_MAP_TYPE_HASH,
    TAPI_BPF_MAP_TYPE_ARRAY,
    TAPI_BPF_MAP_TYPE_UNKNOWN
} tapi_bpf_map_type;

/**
 * BPF XDP actions
 */
typedef enum tapi_bpf_xdp_action {
    TAPI_BPF_XDP_ABORTED = 0,
    TAPI_BPF_XDP_DROP,
    TAPI_BPF_XDP_PASS,
    TAPI_BPF_XDP_TX,
    TAPI_BPF_XDP_REDIRECT
} tapi_bpf_xdp_action;

/**
 * Add BPF object
 *
 * @param[in]  ta       Test Agent name
 * @param[in]  fname    File path on agent side, file should exist on agent
 * @param[out] bpf_id   Id of created BPF object
 *
 * @return              Status code
 */
extern te_errno tapi_bpf_obj_add(const char *ta,
                                 const char *fname,
                                 unsigned int *bpf_id);

/**
 * Remove BPF object
 *
 * @param ta        Test Agent name
 * @param bpf_id    Bpf ID
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_obj_del(const char *ta,
                                 unsigned int bpf_id);

/**
 * Load BPF object into the kernel.
 * Only after the BPF object has been loaded into the kernel, it becomes
 * possible to get list of programs/maps and attach an XDP program to
 * network interface.
 *
 * @param ta        Test Agent name
 * @param bpf_id    Bpf ID
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_obj_load(const char *ta,
                                  unsigned int bpf_id);

/**
 * Get state of BPF object
 * @sa tapi_bpf_obj_load()
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[out] bpf_state    Current BPF state
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_obj_get_state(const char *ta,
                                       unsigned int bpf_id,
                                       tapi_bpf_state *bpf_state);

/**
 * Unload BPF object from kernel
 *
 * @param ta        Test Agent name
 * @param bpf_id    Bpf ID
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_obj_unload(const char *ta,
                                    unsigned int bpf_id);

/**
 * Get program type for BPF object
 *
 * @param[in] ta        Test Agent name
 * @param[in] bpf_id    Bpf ID
 * @param[out] type     Type of BPF program
 *
 * @return              Status code
 */
extern te_errno tapi_bpf_obj_get_type(const char *ta,
                                      unsigned int bpf_id,
                                      tapi_bpf_prog_type *type);
/**
 * Set program type in bfp object
 *
 * @param ta            Test Agent name
 * @param bpf_id        Bpf ID
 * @param type          Type of BPF program
 *
 * @return              Status code
 */
extern te_errno tapi_bpf_obj_set_type(const char *ta,
                                      unsigned int bpf_id,
                                      tapi_bpf_prog_type type);


/************** Functions to work with programs ****************/

/**
 * Get list of programs in BPF object, it means that BPF object was loaded
 * before by calling tapi_bpf_obj_load()
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[out] prog         Array of program names, should be freed by user,
 *                          see @ref tapi_bpf_free_array
 * @param[out] prog_count   Pointer to store number of programs in @par prog,
 *                          unused if @c NULL
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_prog_get_list(const char *ta,
                                       unsigned int bpf_id,
                                       char ***prog,
                                       unsigned int *prog_count);

/**
 * Link program to network interface.
 * @note (1) only XDP type of BPF program and
 *       (2) only one XDP program can be linked to interface.
 *
 * @param ta        Test Agent name
 * @param ifname    Interface name
 * @param bpf_id    Bpf ID
 * @param prog      Program name
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_prog_link(const char *ta,
                                   const char *ifname,
                                   unsigned int bpf_id,
                                   const char *prog);

/**
 * Unlink the XDP program from network interface
 *
 * @param ta        Test Agent name
 * @param ifname    Interface name
 * @param bpf_id    Bpf ID
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_prog_unlink(const char *ta,
                                     const char *ifname);


/************** Functions to work with maps ****************/

/**
 * Get list of loaded maps from BPF object
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[out] map          Array of map names, should be freed by user,
 *                          see @ref tapi_bpf_free_array
 * @param[out] prog_count   Pointer to store number of maps in @par map,
 *                          unused if @c NULL
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_map_get_list(const char *ta,
                                      unsigned int bpf_id,
                                      char ***map,
                                      unsigned int *map_count);

/**
 * Get map type for BPF object
 *
 * @param[in] ta        Test Agent name
 * @param[in] bpf_id    Bpf ID
 * @param[in] map       Map name
 * @param[out] type     Pointer to store map type
 *
 * @return              Status code
 */
extern te_errno tapi_bpf_map_get_type(const char *ta,
                                      unsigned int bpf_id, const char *map,
                                      tapi_bpf_map_type *type);
/**
 * Get size of key in the map
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[in] map           Map name
 * @param[out] key_size     Size of keys in bytes
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_map_get_key_size(const char *ta,
                                          unsigned int bpf_id,
                                          const char *map,
                                          unsigned int *key_size);
/**
 * Get size of values in the map
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[in] map           Map name
 * @param[out] val_size     Size of value in bytes
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_map_get_val_size(const char *ta,
                                          unsigned int bpf_id,
                                          const char *map,
                                          unsigned int *val_size);

/**
 * Get maximum number of key/value entries in the map
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[in] map           Map name
 * @param[out] max_entries  Maximum number of entries
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_map_get_max_entries(const char *ta,
                                             unsigned int bpf_id,
                                             const char *map,
                                             unsigned int *max_entries);

/**
 * Get current state of writable view of the map
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[in] map           Map name
 * @param[out] is_writable  State of map
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_map_get_writable_state(const char *ta,
                                                unsigned int bpf_id,
                                                const char *map,
                                                te_bool *is_writable);

/**
 * Enable writable view for the map
 *
 * @param ta        Test Agent name
 * @param bpf_id    Bpf ID
 * @param map       Map name
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_map_set_writable(const char *ta,
                                          unsigned int bpf_id,
                                          const char *map);

/**
 * Update value for given key in the map
 *
 * @param ta        Test Agent name
 * @param bpf_id    Bpf ID
 * @param map       Map name
 * @param key       Pointer to key
 * @param key_size  Size of @p key
 * @param val       Pointer to value which be set to @p key
 * @param val_size  Size of value
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_map_update_kvpair(const char *ta,
                                           unsigned int bpf_id,
                                           const char *map,
                                           const uint8_t *key,
                                           unsigned int key_size,
                                           const uint8_t *val,
                                           unsigned int val_size);

/**
 * Get raw key value for the map
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[in] map           Map name
 * @param[in] key           Pointer to key
 * @param[in] key_size      Size of @p key
 * @param[out] val          Pointer to value of @p key
 * @param[in] val_size      Size of value
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_map_lookup_kvpair(const char *ta,
                                           unsigned int bpf_id,
                                           const char *map,
                                           const uint8_t *key,
                                           unsigned int key_size,
                                           uint8_t *val,
                                           unsigned int val_size);

/**
 * Delete key/value pair from the map
 *
 * @param ta            Test Agent name
 * @param bpf_id        Bpf ID
 * @param map           Map name
 * @param key           Pointer to key
 * @param key_size      Size of @p key
 *
 * @return              Status code
 */
extern te_errno tapi_bpf_map_delete_kvpair(const char *ta,
                                           unsigned int bpf_id,
                                           const char *map,
                                           const uint8_t *key,
                                           unsigned int key_size);

/**
 * Get list of keys (raw value) in the map
 *
 * @param[in] ta            Test Agent name
 * @param[in] bpf_id        Bpf ID
 * @param[in] map           Map name
 * @param[out] key_size     The size of each item in array @p key (in bytes)
 * @param[out] key          Array of keys (raw value), should be freed by user
 * @param[out] count        Number of keys in array @p key
 *
 * @return                  Status code
 */
extern te_errno tapi_bpf_map_get_key_list(const char *ta,
                                          unsigned int bpf_id,
                                          const char *map,
                                          unsigned int *key_size,
                                          uint8_t  ***key,
                                          unsigned int *count);

/************************** Auxiliary functions  *****************************/

/**
 * Add and load BPF object
 *
 * @param[in] ta        Test Agent name
 * @param[in] fname     File path on agent side, file should be exist on agent
 * @param[in] type      Type of BPF program
 * @param[out] bpf_id   Id of created BPF object
 *
 * @return              Status code
 *
 * @note                The function calls the following functions:
 *                      - @ref tapi_bpf_obj_add
 *                      - @ref tapi_bpf_obj_load
 *                      - @ref tapi_bpf_obj_set_type
 */
extern te_errno tapi_bpf_obj_init(const char *ta,
                                  const char *path,
                                  tapi_bpf_prog_type type,
                                  unsigned int *bpf_id);

/**
 * Unload and delete BPF object
 *
 * @param ta        Test Agent name
 * @param bpf_id    Id of BPF object
 *
 * @return          Status code
 */
extern te_errno tapi_bpf_obj_fini(const char *ta, unsigned int bpf_id);

/**
 * Check that program name is in list of loaded programs
 *
 * @param ta        Test Agent name
 * @param bpf_id    Id of BPF object
 * @param prog_name The name of program to search
 *
 * @return              Status code
 */
extern te_errno tapi_bpf_prog_name_check(const char *ta, unsigned int bpf_id,
                                         const char *prog_name);

/**
 * Check that map name is in list of loaded maps
 *
 * @param ta        Test Agent name
 * @param bpf_id    Id of BPF object
 * @param map_name  The name of map to search
 *
 * @return              Status code
 */
extern te_errno tapi_bpf_map_name_check(const char *ta, unsigned int bpf_id,
                                        const char *map_name);

/**@} <!-- END tapi_bpf --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_BPF_H__ */