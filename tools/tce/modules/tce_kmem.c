/*
 *  linux/drivers/char/mem.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Added devfs support. 
 *    Jan-11-1998, C. Scott Ananian <cananian@alumni.princeton.edu>
 *  Shared /dev/zero mmaping support, Feb 2000, Kanoj Sarcar <kanoj@sgi.com>
 */

#include <linux/config.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/module.h>
#include "kvread.h"

#include <asm/uaccess.h>
#include <asm/io.h>

/** Major devno for alternate kernel memory access */
#define TCE_KMEM_MAJOR 231

#ifdef CONFIG_IA64
# include <linux/efi.h>
#endif

#if defined(CONFIG_S390_TAPE) && defined(CONFIG_S390_TAPE_CHAR)
extern void tapechar_init(void);
#endif

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
static inline int uncached_access(struct file *file, unsigned long addr)
{
#if defined(__i386__)
	/*
	 * On the PPro and successors, the MTRRs are used to set
	 * memory types for physical addresses outside main memory,
	 * so blindly setting PCD or PWT on those pages is wrong.
	 * For Pentiums and earlier, the surround logic should disable
	 * caching for the high addresses through the KEN pin, but
	 * we maintain the tradition of paranoia in this code.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
 	return !( test_bit(X86_FEATURE_MTRR, boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_K6_MTRR, boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_CYRIX_ARR, boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_CENTAUR_MCR, boot_cpu_data.x86_capability) )
	  && addr >= __pa(high_memory);
#elif defined(__x86_64__)
	/* 
	 * This is broken because it can generate memory type aliases,
	 * which can cause cache corruptions
	 * But it is only available for root and we have to be bug-to-bug
	 * compatible with i386.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
	/* same behaviour as i386. PAT always set to cached and MTRRs control the
	   caching behaviour. 
	   Hopefully a full PAT implementation will fix that soon. */	   
	return 0;
#elif defined(CONFIG_IA64)
	/*
	 * On ia64, we ignore O_SYNC because we cannot tolerate memory attribute aliases.
	 */
	return !(efi_mem_attributes(addr) & EFI_MEMORY_WB);
#elif defined(CONFIG_PPC64)
	/* On PPC64, we always do non-cacheable access to the IO hole and
	 * cacheable elsewhere. Cache paradox can checkstop the CPU and
	 * the high_memory heuristic below is wrong on machines with memory
	 * above the IO hole... Ah, and of course, XFree86 doesn't pass
	 * O_SYNC when mapping us to tap IO space. Surprised ?
	 */
	return !page_is_ram(addr >> PAGE_SHIFT);
#else
	/*
	 * Accessing memory above the top the kernel knows about or through a file pointer
	 * that was marked O_SYNC will be done non-cached.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
	return addr >= __pa(high_memory);
#endif
}

static long (*vread_pointer)
    (char *buf, char *addr, unsigned long count) = (void *)VREAD_OFFSET;

static int open_kmem(struct inode * inode, struct file * filp)
{
    if (vread_pointer == NULL)
        return -ENXIO;
	return capable(CAP_SYS_RAWIO) ? 0 : -EPERM;
}

static int mmap_kmem(struct file * file, struct vm_area_struct * vma)
{
	return -ENOSYS;
}

/*
 * This function reads the *virtual* memory as seen by the kernel.
 */
static ssize_t read_kmem(struct file *file, char __user *buf, 
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t read = 0;
	ssize_t virtr = 0;
	char * kbuf; /* k-addr because vread() takes vmlist_lock rwlock */
		
	if (p < (unsigned long) high_memory) {
		read = count;
		if (count > (unsigned long) high_memory - p)
			read = (unsigned long) high_memory - p;

#if defined(__sparc__) || (defined(__mc68000__) && defined(CONFIG_MMU))
		/* we don't have page 0 mapped on sparc and m68k.. */
		if (p < PAGE_SIZE && read > 0) {
			size_t tmp = PAGE_SIZE - p;
			if (tmp > read) tmp = read;
			if (clear_user(buf, tmp))
				return -EFAULT;
			buf += tmp;
			p += tmp;
			read -= tmp;
			count -= tmp;
		}
#endif
		if (copy_to_user(buf, (char *)p, read))
			return -EFAULT;
		p += read;
		buf += read;
		count -= read;
	}

	if (count > 0) {
		kbuf = (char *)__get_free_page(GFP_KERNEL);
		if (!kbuf)
			return -ENOMEM;
		while (count > 0) {
			int len = count;

			if (len > PAGE_SIZE)
				len = PAGE_SIZE;
			len = vread_pointer(kbuf, (char *)p, len);
			if (!len)
				break;
			if (copy_to_user(buf, kbuf, len)) {
				free_page((unsigned long)kbuf);
				return -EFAULT;
			}
			count -= len;
			buf += len;
			virtr += len;
			p += len;
		}
		free_page((unsigned long)kbuf);
	}
 	*ppos = p;
 	return virtr + read;
}

/*
 * This function writes to the *virtual* memory as seen by the kernel.
 */
static ssize_t write_kmem(struct file * file, const char __user * buf, 
			  size_t count, loff_t *ppos)
{
    return -EACCES;
}


/*
 * The memory devices use the full 32/64 bits of the offset, and so we cannot
 * check against negative addresses: they are ok. The return value is weird,
 * though, in that case (0).
 *
 * also note that seeking relative to the "end of file" isn't supported:
 * it has no meaning, so it returns -EINVAL.
 */
static loff_t kmem_lseek(struct file * file, loff_t offset, int orig)
{
	loff_t ret;

	down(&file->f_dentry->d_inode->i_sem);
	switch (orig) {
		case 0:
			file->f_pos = offset;
			ret = file->f_pos;
			force_successful_syscall_return();
			break;
		case 1:
			file->f_pos += offset;
			ret = file->f_pos;
			force_successful_syscall_return();
			break;
		default:
			ret = -EINVAL;
	}
	up(&file->f_dentry->d_inode->i_sem);
	return ret;
}

static struct file_operations kmem_fops = {
	.llseek		= kmem_lseek,
	.read		= read_kmem,
	.write		= write_kmem,
	.mmap		= mmap_kmem,
	.open		= open_kmem,
};

static int kmemory_open(struct inode * inode, struct file * filp)
{
	switch (iminor(inode)) {
		case 1:
			filp->f_op = &kmem_fops;
			break;
		default:
			return -ENXIO;
	}
	if (filp->f_op && filp->f_op->open)
		return filp->f_op->open(inode,filp);
	return 0;
}

static struct file_operations kmemory_fops = {
	.open		= kmemory_open,	/* just a selector for the real open */
};

static const struct {
	unsigned int		minor;
	char			*name;
	umode_t			mode;
	struct file_operations	*fops;
} kmem_device = {1, "tce_kmem", S_IRUSR | S_IWUSR | S_IRGRP, &kmem_fops};

static struct class_simple *kmem_class;

static int __init chr_dev_init(void)
{
	if (register_chrdev(TCE_KMEM_MAJOR,"tce_kmem", &kmemory_fops))
		printk("unable to get major %d for memory devs\n", TCE_KMEM_MAJOR);

	kmem_class = class_simple_create(THIS_MODULE, "tce_kmem");
    class_simple_device_add(kmem_class,
                            MKDEV(TCE_KMEM_MAJOR, kmem_device.minor),
                            NULL, kmem_device.name);
    devfs_mk_cdev(MKDEV(TCE_KMEM_MAJOR, kmem_device.minor),
                  S_IFCHR | kmem_device.mode, kmem_device.name);
	
	return 0;
}

static void chr_dev_cleanup(void)
{
    unregister_chrdev(TCE_KMEM_MAJOR, "tce_kmem");
    devfs_remove("tce_kmem");
    class_simple_device_remove(MKDEV(TCE_KMEM_MAJOR, kmem_device.minor));
    class_simple_destroy(kmem_class);
}

module_init(chr_dev_init);
module_exit(chr_dev_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem V. Andreev");
MODULE_DESCRIPTION("Accessing /dev/kmem");
