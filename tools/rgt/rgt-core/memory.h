
#ifndef RGT_MEMORY_H__
#define RGT_MEMORY_H__

#include "log_msg.h"

struct obstack *obstack_initialize();
void obstack_destroy(struct obstack *obstk);
void initialize_log_msg_pool();
void destroy_log_msg_pool();
log_msg *alloc_log_msg();
void free_log_msg(log_msg *);
void initialize_node_info_pool();
void destroy_node_info_pool();
node_info_t *alloc_node_info();
void free_node_info(node_info_t *);
void *node_info_obstack_alloc(int size);
void *node_info_obstack_copy0(const void *address, int size);

#endif /* RGT_MEMORY_H__ */
