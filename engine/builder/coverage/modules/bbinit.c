#include <linux/module.h>	
#include <linux/kernel.h>	
#include <linux/init.h>

int init_module(void)
{
	return 0;
}

void cleanup_module(void)
{
}

void __bb_init_func(void *unused)
{
}

#if 1
#undef ntohs
unsigned short int ntohs(unsigned short int x)
{
    return ((x & 0xff) << 8) | (x >> 8);
}
EXPORT_SYMBOL(ntohs);
#undef htons
unsigned short int htons(unsigned short int x)
{
    return ((x & 0xff) << 8) | (x >> 8);
}
EXPORT_SYMBOL(htons);
#endif

EXPORT_SYMBOL(__bb_init_func);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("some");	
MODULE_DESCRIPTION("nothing");	
