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

EXPORT_SYMBOL(__bb_init_func);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("some");	
MODULE_DESCRIPTION("nothing");	
