#ifndef __TE_IF_MANGLE_H__
#define __TE_IF_MANGLE_H__

#define MANGLE_ENSLAVE     (SIOCDEVPRIVATE)
#define MANGLE_EMANCIPATE  (SIOCDEVPRIVATE + 1)
#define MANGLE_CONFIGURE   (SIOCDEVPRIVATE + 2)
#define MANGLE_UPDATE_SLAVE (SIOCDEVPRIVATE + 3)

enum mangle_configuration_params
{
    MANGLE_CONFIGURE_DROP_RATE
};

typedef struct mangle_configure_request
{
    int param;
    int value;
} mangle_configure_request;


#endif /* __TE_IF_MANGLE_H__ */
