
#include <obstack.h>
#include <string.h>

#include "memory.h"

/* Pointer to an abstack that is used for allocation log_msg data structure */
static struct obstack *log_msg_obstk = NULL;

/* Pointer to an abstack that is used for allocation node_info data structure */
static struct obstack *node_info_obstk = NULL;

static void
internal_obstack_alloc_failed()
{
    /* Go to the main procedure */
    THROW_EXCEPTION;
}

/* These two definitions are for correct working of "obstack_init" function */
#define obstack_chunk_alloc malloc
#define obstack_chunk_free  free

struct obstack *
obstack_initialize()
{
    struct obstack *obstk;

    /* Allocate a new obstack */
    if ((obstk = (struct obstack *)malloc(sizeof(struct obstack))) == NULL)
        return NULL;

    obstack_alloc_failed_handler = &internal_obstack_alloc_failed;
    obstack_init(obstk);

    return obstk;
}

void
obstack_destroy(struct obstack *obstk)
{
    obstack_free(obstk, NULL);
    free(obstk);
}

void initialize_log_msg_pool()
{
    if (log_msg_obstk == NULL && 
        ((log_msg_obstk = obstack_initialize()) == NULL))
    {
        THROW_EXCEPTION;
    }
}

void destroy_log_msg_pool()
{
    if (log_msg_obstk != NULL)
    {
        obstack_destroy(log_msg_obstk);
        log_msg_obstk = NULL;
    }
}

log_msg *
alloc_log_msg()
{
    log_msg *msg;

    msg = (log_msg *)obstack_alloc(log_msg_obstk, sizeof(log_msg));
    msg->obstk = log_msg_obstk;

    return msg;
}

void
free_log_msg(log_msg *msg)
{
    if (msg != NULL)
        obstack_free(msg->obstk, msg);
}

void initialize_node_info_pool()
{
    if (node_info_obstk == NULL && 
        ((node_info_obstk = obstack_initialize()) == NULL))
    {
        THROW_EXCEPTION;
    }
}

void destroy_node_info_pool()
{
    if (node_info_obstk != NULL)
    {
        obstack_destroy(node_info_obstk);
        node_info_obstk = NULL;
    }
}

node_info_t *
alloc_node_info()
{
    return (node_info_t *)obstack_alloc(node_info_obstk, sizeof(node_info_t));
}

void
free_node_info(node_info_t *node)
{
    if (node != NULL)
        obstack_free(node_info_obstk, node);
}

void *
node_info_obstack_alloc(int size)
{
    return obstack_alloc(node_info_obstk, size);
}

void *
node_info_obstack_copy0(const void *address, int size)
{
    return obstack_copy0(node_info_obstk, address, size);
}
