#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>

#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define GCC_IS_3_4P 1
#else
#undef GCC_IS_3_4P 
#endif


#ifdef GCC_IS_3_4P
static unsigned volatile gcov_version_magic = 0xdeadbeef;
static struct proc_dir_entry *tce_gcov_magic;

static int read_gcov_magic (char *buffer, char **buf_loc,
                            off_t offset, int buf_len, int *eof,
                            void *data)
{
    if (buf_len < sizeof(gcov_version_magic))
        return -EINVAL;
    if (offset > 0)
    {
        *eof = 1;
        return 0;
    }
    *(unsigned *)buffer = gcov_version_magic;
    return sizeof(gcov_version_magic);
}
#endif


int
init_module(void)
{
#ifdef GCC_IS_3_4P
    if (gcov_version_magic == 0xdeadbeef)
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
        ptr  = strchr(ptr, '(');
        v[3] = (ptr == NULL ? '*' : ptr[1]);
        
        for (ix = 0; ix != 4; ix++)
            gcov_version_magic = (gcov_version_magic << 8) | v[ix];
    }
    if (tce_gcov_magic == NULL)
    {
        tce_gcov_magic = create_proc_entry("tce_gcov_magic", 0444, NULL);
        if (tce_gcov_magic == NULL)
        {
            printk(KERN_ERR "Unable to create /proc/tce_gcov_magic");
            remove_proc_entry("tce_gcov_magic", &proc_root);
            return -ENOMEM;
        }
        tce_gcov_magic->read_proc = read_gcov_magic;
        tce_gcov_magic->owner     = THIS_MODULE;
        tce_gcov_magic->mode      = S_IFREG | S_IRUGO;
        tce_gcov_magic->uid       = 0;
        tce_gcov_magic->gid       = 0;
        tce_gcov_magic->size      = sizeof(gcov_version_magic);
    }
#endif
    return 0;
}

void
cleanup_module(void)
{
#ifdef GCC_IS_3_4P
    if (tce_gcov_magic != NULL)
        remove_proc_entry("tce_gcov_magic", &proc_root);
#endif
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
#else
EXPORT_SYMBOL(__bb_init_func);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem V. Andreev");
MODULE_DESCRIPTION("support for kernel GCOV");
