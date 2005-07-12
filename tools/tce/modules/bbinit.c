#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define GCC_IS_3_4P 1
#else
#undef GCC_IS_3_4P 
#endif


#ifdef GCC_IS_3_4P
unsigned __gcov_version_magic = 0;
#endif


int
init_module(void)
{
#ifdef GCC_IS_3_4P
    if (__gcov_version_magic != 0)
    {
        unsigned char v[4];
        unsigned ix;
        char *ptr = __VERSION__;
        unsigned major, minor = 0;
        
        major = simple_strtoul(ptr, &ptr, 10);
        if (*ptr)
            minor = simple_strtoul(ptr + 1, &ptr, 10);
    
        v[0] = (major < 10 ? '0' : 'A' - 10) + major;
        v[1] = (minor / 10) + '0';
        v[2] = (minor % 10) + '0';
        v[3] = strchr(ptr, '(') != NULL ? '(' : '*';
        
        for (ix = 0; ix != 4; ix++)
            __gcov_version_magic = (__gcov_version_magic << 8) | v[ix];
    }

#endif
    return 0;
}

void
cleanup_module(void)
{
}

#ifndef GCC_IS_3_4P
void
__bb_init_func(void *unused)
{
}

#else

void
__gcov_init(void *unused)
{
}

void __gcov_merge_add (long long *counters, unsigned n_counters)
{
}

void __gcov_merge_single (long long *counters, unsigned n_counters)
{
}

void __gcov_merge_delta (long long *counters, unsigned n_counters)
{
}


#endif

#if 0
#undef ntohs
unsigned short int
ntohs(unsigned short int x)
{
    return ((x & 0xff) << 8) | (x >> 8);
}
EXPORT_SYMBOL(ntohs);
#undef htons
unsigned short int
htons(unsigned short int x)
{
    return ((x & 0xff) << 8) | (x >> 8);
}
EXPORT_SYMBOL(htons);
#endif

#ifdef GCC_IS_3_4P
EXPORT_SYMBOL(__gcov_init);
EXPORT_SYMBOL(__gcov_merge_add);
EXPORT_SYMBOL(__gcov_merge_single);
EXPORT_SYMBOL(__gcov_merge_delta);
EXPORT_SYMBOL(__gcov_version_magic);
#else
EXPORT_SYMBOL(__bb_init_func);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("some");
MODULE_DESCRIPTION("nothing");
