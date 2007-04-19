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


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem V. Andreev");
MODULE_DESCRIPTION("Accessing /dev/kmem");
