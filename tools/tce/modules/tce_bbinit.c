#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/pagemap.h>
#include <linux/miscdevice.h>
#include <linux/notifier.h>
#include "tce_bbinit_defs.h"

/****************** TCE structure definitions ************************/

typedef struct tce_info_data 
{
    pid_t              pid;
    __u32              crc;
    __u32              version;
    unsigned           next;
    struct list_head   obj_list;
    spinlock_t         obj_list_lock;
} tce_info_data;

static void tce_info_release(struct kobject *kobj);

TCE_STRUCTURE(tce_info, tce_info_release);

TCE_SIMPLE_RO_ATTR(tce_info, pid, "%d", int);
TCE_SIMPLE_RO_ATTR(tce_info, crc, "%u", unsigned);


static unsigned 
calc_gcov_version_magic(const char *version)
{
    __u32 gcov_version = 0;
    unsigned char v[4];
    unsigned ix;
    const char *ptr = __VERSION__;
    unsigned major, minor = 0;
    
    major = simple_strtoul(ptr, (char **)&ptr, 10);
    if (*ptr)
        minor = simple_strtoul(ptr + 1, (char **)&ptr, 10);
    
    if (major < 3 || (major == 3 && minor < 4))
        return 0;

    v[0] = (major < 10 ? '0' : 'A' - 10) + major;
    v[1] = (minor / 10) + '0';
    v[2] = (minor % 10) + '0';
    ptr  = strchr(ptr, '(');
    v[3] = (ptr == NULL ? '*' : ptr[1]);
    
    for (ix = 0; ix != 4; ix++)
        gcov_version = (gcov_version << 8) | v[ix];

    return gcov_version;
}


TCE_ATTR_STORE(tce_info, version, obj, value, count)
{
    obj->data.version = calc_gcov_version_magic(value);
    return count;
}

TCE_ATTR_SHOW(tce_info, version, obj, result)
{
    TCE_ATTR_FMT(result, "%x", (unsigned)obj->data.version);
}

TCE_SIMPLE_ATTR_DEF(tce_info, version);

static tce_info *add_new_tce_program(pid_t pid, int *pnext);

TCE_ATTR_SHOW(tce_info, seq, obj, result)
{
    int seq;
    
    add_new_tce_program(current->pid, &seq);
    TCE_ATTR_FMT(result, "%d", seq);
}

TCE_SIMPLE_ATTR_RO_DEF(tce_info, seq);

TCE_ATTR_SHOW(tce_info, n_objects, obj, result)
{
    TCE_ATTR_FMT(result, "%d", obj->data.next);
}

TCE_SIMPLE_ATTR_RO_DEF(tce_info, n_objects);

static void
tce_info_release(struct kobject *kobj)
{
    tce_info *info = container_of(kobj, tce_info, kobj);

    kfree(info);
}

typedef struct tce_ctr_page_data
{
    struct page *page;
    unsigned offset;
    unsigned length;
} tce_ctr_page_data;

static void tce_ctr_page_release(struct kobject *kobj);
TCE_STRUCTURE(tce_ctr_page, tce_ctr_page_release);

static void
tce_ctr_page_release(struct kobject *kobj)
{
    tce_ctr_page *pg = container_of(kobj, tce_ctr_page, kobj);

    if (pg->data.page != NULL)
    {
        put_page(pg->data.page);
    }
}


TCE_ATTR_SHOW(tce_ctr_page, data, obj, result)
{
    __u8 *ptr = kmap(obj->data.page);
    memcpy(result, ptr + obj->data.offset, obj->data.length);
    kunmap(obj->data.page);
    return obj->data.length;
}

TCE_SIMPLE_ATTR_RO_DEF(tce_ctr_page, data);

enum tce_counter_merges { 
    TCE_MERGE_UNKNOWN = -1,
    TCE_MERGE_ADD, 
    TCE_MERGE_SINGLE,
    TCE_MERGE_DELTA
};

typedef struct tce_ctr_info_data {
    unsigned n_counters;
    int merger;
    unsigned n_pages;
    tce_ctr_page *pages;
} tce_ctr_info_data;

static void tce_ctr_info_release(struct kobject *kobj);

TCE_STRUCTURE(tce_ctr_info, tce_ctr_info_release);

typedef struct tce_arccount_info_data {   
    unsigned count;
} tce_arccount_info_data;

TCE_STRUCTURE(tce_arccount_info, NULL);

typedef struct tce_fun_info_data
{
    __u32 ident;
    __u32 checksum;
    char *name;
    unsigned next;
    tce_arccount_info counts[GCOV_COUNTERS];
} tce_fun_info_data;

TCE_STRUCTURE(tce_fun_info, NULL);

typedef struct tce_obj_info_data
{
    struct list_head entry;
    __u32 stamp;
    char *filename;
    unsigned n_functions;
    unsigned ctr_mask;
    unsigned next_fn;
    unsigned next_cn;
    struct tce_fun_info *functions;
    struct tce_ctr_info counters[GCOV_COUNTERS];
} tce_obj_info_data;

static void tce_obj_info_release(struct kobject *kobj);

TCE_STRUCTURE(tce_obj_info, tce_obj_info_release);

static void
tce_obj_info_release(struct kobject *kobj)
{
    tce_obj_info *info = container_of(kobj, tce_obj_info, kobj);
    tce_info *parent = container_of(kobj->parent, tce_info, kobj);
    unsigned long flags;

    if (info->data.filename != NULL)
        kfree(info->data.filename);
    if (info->data.functions != NULL)
        kfree(info->data.functions);
    spin_lock_irqsave(&parent->data.obj_list_lock, flags);
    list_del(&info->data.entry);
    spin_unlock_irqrestore(&parent->data.obj_list_lock, flags);

    kfree(info);
}

static void 
init_new_obj(tce_info *parent, tce_obj_info *new)
{
    unsigned long flags;
    
    spin_lock_irqsave(&parent->data.obj_list_lock, flags);
    list_add_tail(&new->data.entry, &parent->data.obj_list);
    spin_unlock_irqrestore(&parent->data.obj_list_lock, flags);
}

TCE_CHILD_ALLOCATOR(add_new_obj, "", tce_obj_info, tce_info, 
                    TCE_DYNAMIC, next, init_new_obj);

TCE_SIMPLE_ATTR(tce_obj_info, stamp, "%x", unsigned);
TCE_SIMPLE_ATTR(tce_obj_info, ctr_mask, "%x", unsigned);

static void
update_crc(__u32 *global, const char *str)
{
    __u32 crc32 = *global;

    do
    {
        unsigned ix;
        __u32 value = *str << 24;
        
        for (ix = 8; ix--; value <<= 1)
        {
            __u32 feedback;
                
            feedback = (value ^ crc32) & 0x80000000 ? 0x04c11db7 : 0;
            crc32 <<= 1;
            crc32 ^= feedback;
        }
    } while (*str++);
        
    *global = crc32;
}

static void
tce_update_crc(tce_obj_info *obj)
{
    tce_info *program = TCE_PARENT(obj, tce_info);
    update_crc(&program->data.crc, obj->data.filename);
}

TCE_STRING_ATTR(tce_obj_info, filename, tce_update_crc);

static void
tce_obj_alloc_functions(tce_obj_info *obj, int n_functions)
{
    obj->data.n_functions = n_functions;
    obj->data.functions = kmalloc(sizeof(*obj->data.functions) *
                                  obj->data.n_functions, GFP_KERNEL);
    memset(obj->data.functions, 0, sizeof(*obj->data.functions) * 
           obj->data.n_functions);
}

TCE_ATTR_STORE(tce_obj_info, n_functions, obj, value, count)
{
    if (obj->data.functions != NULL)
        return -EBUSY;
    tce_obj_alloc_functions(obj, simple_strtoul(value, NULL, 10));
    return count;
}

TCE_ATTR_SHOW(tce_obj_info, n_functions, obj, result)
{
    TCE_ATTR_FMT(result, "%u", obj->data.n_functions);
}

TCE_SIMPLE_ATTR_DEF(tce_obj_info, n_functions);

TCE_SIMPLE_ATTR(tce_arccount_info, count, "%u", unsigned);

TCE_CHILD_ALLOCATOR(add_new_arccount, "", tce_arccount_info, tce_fun_info, 
                    TCE_STATIC(counts), next, TCE_NULL_FUN);

TCE_CHILD_ALLOCATOR(add_new_fun, "fun", tce_fun_info, tce_obj_info,
                    TCE_STATIC(functions), next_fn, TCE_NULL_FUN);

TCE_SIMPLE_ATTR(tce_fun_info, ident, "%x", unsigned);
TCE_SIMPLE_ATTR(tce_fun_info, checksum, "%x", unsigned);
TCE_STRING_ATTR(tce_fun_info, name, TCE_NULL_FUN);

TCE_CHILD_ALLOCATOR(add_new_ctr, "ctr", tce_ctr_info, tce_obj_info, 
                    TCE_STATIC(counters), next_cn, TCE_NULL_FUN);

TCE_SIMPLE_ATTR(tce_ctr_info, merger, "%d", int);
TCE_SIMPLE_ATTR(tce_ctr_info, n_counters, "%u", unsigned);
TCE_SIMPLE_ATTR(tce_ctr_info, n_pages, "%u", unsigned);
 
static int
add_counter_pages(tce_ctr_info *data, unsigned long long ptr, 
                  int n_counters, int n_pages, struct page **pages)
{
    int i;
    unsigned offset = ptr % PAGE_SIZE;
    unsigned length = n_counters * sizeof(long long);

    data->data.n_pages = n_pages;
    data->data.pages = kmalloc(sizeof(*data->data.pages) * n_pages, GFP_KERNEL);
    for (i = 0; i < n_pages; i++, length -= PAGE_SIZE - offset, offset = 0)
    {
        memset(&data->data.pages[i], 0, sizeof(data->data.pages[i]));
        kobject_set_name(&data->data.pages[i].kobj, "%u", i);
        data->data.pages[i].kobj.kset = NULL;
        data->data.pages[i].kobj.ktype = &ktype_tce_ctr_page;
        data->data.pages[i].kobj.parent = &data->kobj;
        data->data.pages[i].data.page = pages[i];
        data->data.pages[i].data.offset = offset;
        data->data.pages[i].data.length = (length > PAGE_SIZE - offset ? 
                                           PAGE_SIZE - offset : length);
        kobject_register(&data->data.pages[i].kobj);
    }
    return 0;
}

static void
add_module_pages(tce_ctr_info *data, void *coreptr, int n_counters)
{
    long n_pages = (n_counters * sizeof(long long) + 
                    (long)coreptr % PAGE_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
    struct page **pages = kmalloc(sizeof(*pages) * n_pages, GFP_KERNEL);
    int i;
    char *vmptr = (char *)((unsigned long)coreptr & PAGE_MASK);
    
    memset(coreptr, 0, n_counters * sizeof(long long));
    for (i = 0; i < n_pages; i++, vmptr += PAGE_SIZE)
    {
        pages[i] = vmalloc_to_page(vmptr);
        get_page(pages[i]);
    }
    add_counter_pages(data, (unsigned long)coreptr, n_counters, 
                      n_pages, pages);
    kfree(pages);
}

static void
tce_ctr_info_release(struct kobject *kobj)
{
    tce_ctr_info *info = container_of(kobj, tce_ctr_info, kobj);
    
    if (info->data.pages != NULL)
    {   
        kfree(info->data.pages);
    }
}

   
TCE_ATTR_SHOW(tce_ctr_info, data, obj, result)
{
    return 0;
}

TCE_ATTR_STORE(tce_ctr_info, data, obj, value, count)
{
    long n_phys;
    int rc;
    unsigned long long ptr;
    struct page **app_pages;

    if (obj->data.pages != NULL)
        return -EBUSY;

    if (sscanf(value, "%llx", &ptr) != 1)
        return -EINVAL;
    
    n_phys = (obj->data.n_counters * sizeof(long long) + ptr % PAGE_SIZE 
              + PAGE_SIZE - 1) / PAGE_SIZE;
    app_pages = kmalloc(sizeof(*app_pages) * n_phys, GFP_KERNEL);

    rc = get_user_pages(current, current->mm, ptr, 
                        obj->data.n_counters * sizeof(long long), 0, 0, 
                        app_pages, NULL);
    if (rc != 0)
    {
        kfree(app_pages);
        return rc;
    }

    rc = add_counter_pages(obj, ptr, obj->data.n_counters, n_phys, app_pages);
    kfree(app_pages);

    if (rc != 0)
        return rc;
    return count;
}

TCE_SIMPLE_ATTR_DEF(tce_ctr_info, data);

static int tce_prog_seqno;
static spinlock_t tce_prog_seqno_lock;


TCE_ATTR_TABLE(tce_info) = {
    TCE_ATTR_REF(tce_info, pid),
    TCE_ATTR_REF(tce_info, crc),
    TCE_ATTR_REF(tce_info, version),
    TCE_ATTR_REF(tce_info, next),
    TCE_ATTR_REF(tce_info, n_objects),
    TCE_ATTR_REF(tce_info, seq),
    NULL
};

TCE_ATTR_TABLE(tce_obj_info) = {
    TCE_ATTR_REF(tce_obj_info, stamp),
    TCE_ATTR_REF(tce_obj_info, ctr_mask),
    TCE_ATTR_REF(tce_obj_info, filename),
    TCE_ATTR_REF(tce_obj_info, n_functions),
    TCE_ATTR_REF(tce_obj_info, next_fn),
    TCE_ATTR_REF(tce_obj_info, next_cn),
    NULL
};

TCE_ATTR_TABLE(tce_fun_info) = {
    TCE_ATTR_REF(tce_fun_info, ident),
    TCE_ATTR_REF(tce_fun_info, checksum),
    TCE_ATTR_REF(tce_fun_info, name),
    TCE_ATTR_REF(tce_fun_info, next),
    NULL
};

TCE_ATTR_TABLE(tce_ctr_info) = {
    TCE_ATTR_REF(tce_ctr_info, n_counters),
    TCE_ATTR_REF(tce_ctr_info, n_pages),
    TCE_ATTR_REF(tce_ctr_info, merger),
    TCE_ATTR_REF(tce_ctr_info, data),
    NULL
};

TCE_ATTR_TABLE(tce_ctr_page) = {
    TCE_ATTR_REF(tce_ctr_page, data),
    NULL
};

TCE_ATTR_TABLE(tce_arccount_info) = {
    TCE_ATTR_REF(tce_arccount_info, count),
    NULL
};

static decl_subsys(tce, &ktype_tce_info, NULL);

static tce_info *
add_new_tce_program(pid_t pid, int *pnext)
{
    int next;
    tce_info *new_prog;
    unsigned long flags;

    spin_lock_irqsave(&tce_prog_seqno_lock, flags);
    next = tce_prog_seqno++;
    spin_unlock_irqrestore(&tce_prog_seqno_lock, flags);

    new_prog = kmalloc(sizeof(*new_prog), GFP_KERNEL);
    memset(new_prog, 0, sizeof(*new_prog));
    kobj_set_kset_s(new_prog, tce_subsys);
    new_prog->data.pid = pid;
    kobject_set_name(&new_prog->kobj, "%u", next);
    INIT_LIST_HEAD(&new_prog->data.obj_list);
    spin_lock_init(&new_prog->data.obj_list_lock);
    kobject_register(&new_prog->kobj);
    if (pnext != NULL)
        *pnext = next;
    return new_prog;
}

static void
release_tce_fun(tce_fun_info *fun)
{
    unsigned i;

    for (i = 0; i < fun->data.next; i++)
    {
        kobject_unregister(&fun->data.counts[i].kobj);
    }

    kobject_unregister(&fun->kobj);
}

static void
release_tce_ctr(tce_ctr_info *ctr)
{
    if (ctr->data.pages != NULL)
    {
        unsigned i;
        
        for (i = 0; i < ctr->data.n_pages; i++)
        {
            kobject_unregister(&ctr->data.pages[i].kobj);
        }
    }
    kobject_unregister(&ctr->kobj);
}

static void
release_tce_obj(tce_obj_info *obj)
{
    unsigned i;
    
    for (i = 0; i < obj->data.next_fn; i++)
    {
        release_tce_fun(&obj->data.functions[i]);
    }
    for (i = 0; i < obj->data.next_cn; i++)
    {
        release_tce_ctr(&obj->data.counters[i]);
    }
    kobject_unregister(&obj->kobj);
}

static void
release_tce_data(void)
{
    struct kobject *prog;
    struct kobject *ptmp;
    tce_info *tce;
    
    list_for_each_entry_safe(prog, ptmp, &tce_subsys.kset.list, entry)
        {
            tce_obj_info *iter;
            tce_obj_info *tmp;
            
            tce = container_of(prog, tce_info, kobj);
            list_for_each_entry_safe(iter, tmp, &tce->data.obj_list, data.entry)
                {
                    release_tce_obj(iter);
                }
            kobject_unregister(prog);
        }
}


static tce_info *current_tce_module;
static struct page **current_module_pages;

#ifdef CONFIG_KALLSYMS
static int
module_load_notifier(struct notifier_block *self, unsigned long val, void *data)
{
    if (val == MODULE_STATE_COMING)
    {
        struct module *mod = data;
        unsigned i;
        Elf_Sym *sym;
        const char *symname;
        unsigned n_pages;
        char *core_ptr;

        current_tce_module = add_new_tce_program(0, NULL);
        current_tce_module->data.version = calc_gcov_version_magic(__VERSION__);
        
        n_pages = ((unsigned long)mod->module_core % PAGE_SIZE +
                   mod->core_size + PAGE_SIZE - 1) / PAGE_SIZE;
        current_module_pages = kmalloc(sizeof(*current_module_pages) * n_pages,
                                       GFP_KERNEL);
        
        for (i = 0, core_ptr = mod->module_core; i < n_pages; i++, core_ptr += PAGE_SIZE)
        {
            current_module_pages[i] = vmalloc_to_page(core_ptr);
        }

        for (sym = mod->symtab, i = 0; i < mod->num_symtab; i++, sym++)
        {
            symname = mod->strtab + sym->st_name;
            if (strstr(symname, "GCOV") != NULL)
            {
                ((void (*)(void))sym->st_value)();
            }
        }
        kfree(current_module_pages);
        current_module_pages = NULL;
    }
    return NOTIFY_OK;
}

static struct notifier_block module_load_nb = {
    .notifier_call = module_load_notifier
};

#endif

int
init_module(void)
{
    spin_lock_init(&tce_prog_seqno_lock);
    subsystem_register(&tce_subsys);
    add_new_tce_program(0, NULL);
#ifdef CONFIG_KALLSYMS
    register_module_notifier(&module_load_nb);
#else
    printk(KERN_WARNING "warning: kernel compiled w/o kallsyms, "
           "TCE for modules will be unavailable");
#endif
    return 0;
}

void
cleanup_module(void)
{
#ifdef CONFIG_KALLSYMS
    unregister_module_notifier(&module_load_nb);
#endif
    release_tce_data();
    subsystem_unregister(&tce_subsys);
}

void
__bb_init_func(struct bb *bb_obj)
{
    tce_obj_info *obj = NULL;
    add_new_obj(current_tce_module, &obj);
    
    if (obj != NULL)
    {       
        struct bb_function_info *fi;
        tce_ctr_info *ctr = NULL;
        int n_functions = 0;       

        tce_obj_info_filename_store(obj, bb_obj->filename, strlen(bb_obj->filename));
        obj->data.ctr_mask = 1;

        add_new_ctr(obj, &ctr);
        ctr->data.n_counters = bb_obj->ncounts;
        add_module_pages(ctr, bb_obj->counts, bb_obj->ncounts);

        for (fi = bb_obj->function_infos; fi->arc_count != -1; fi++)
            n_functions++;
        tce_obj_alloc_functions(obj, n_functions);       

        for (fi = bb_obj->function_infos; fi->arc_count != -1; fi++)
        {
            tce_fun_info *nf = NULL;
            tce_arccount_info *ac = NULL;
            
            add_new_fun(obj, &nf);
            tce_fun_info_name_store(nf, fi->name, strlen(fi->name));
            nf->data.checksum = fi->checksum;
            
            add_new_arccount(nf, &ac);
            ac->data.count = fi->arc_count;
        }
    }
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

void
__gcov_init(struct gcov_info *gobj)
{
    tce_obj_info *obj = NULL;
    add_new_obj(current_tce_module, &obj);
    
    if (obj != NULL)
    {
        int i;
        int actual_counters = 0;
        unsigned fi_stride;
        const struct gcov_fn_info *fi_ptr;

        obj->data.stamp   = gobj->stamp;
        tce_obj_info_filename_store(obj, gobj->filename, 
                                    strlen(gobj->filename));
        tce_obj_alloc_functions(obj, gobj->n_functions);       
        obj->data.ctr_mask = gobj->ctr_mask;
        for (i = 0; i < GCOV_COUNTERS; i++)
        {
            tce_ctr_info *ctr = NULL;

            if ((obj->data.ctr_mask & (1 << i)) != 0)
            {
                add_new_ctr(obj, &ctr);

                ctr->data.n_counters = gobj->counts[i].num;
                if (gobj->counts[i].merge == __gcov_merge_add)
                    ctr->data.merger = TCE_MERGE_ADD;
                else if (gobj->counts[i].merge == __gcov_merge_single)
                    ctr->data.merger = TCE_MERGE_SINGLE;
                else if (gobj->counts[i].merge == __gcov_merge_add)
                    ctr->data.merger = TCE_MERGE_DELTA;
                else
                {
                    printk(KERN_WARNING "unknown merger function\n");
                    ctr->data.merger = TCE_MERGE_UNKNOWN;
                }
                add_module_pages(ctr, gobj->counts[i].values, 
                                 gobj->counts[i].num);
                actual_counters++;
            }
        }

        fi_stride = sizeof (struct gcov_fn_info) + 
            actual_counters * sizeof (unsigned);
        if (__alignof__ (struct gcov_fn_info) > sizeof (unsigned))
        {
            fi_stride += __alignof__ (struct gcov_fn_info) - 1;
            fi_stride &= ~(__alignof__ (struct gcov_fn_info) - 1);
        }

        for (i = 0, fi_ptr = gobj->functions; 
             i < gobj->n_functions; 
             i++, fi_ptr = (struct gcov_fn_info *)((const char *)fi_ptr + 
                                                   fi_stride))
        {
            tce_fun_info *nf = NULL;
            int j;
            int cn;

            add_new_fun(obj, &nf);          
            nf->data.ident = fi_ptr->ident;
            nf->data.checksum = fi_ptr->checksum;
            for (j = 0, cn = 0; j < GCOV_COUNTERS; j++)
            {
                tce_arccount_info *ac = NULL;
                
                if ((obj->data.ctr_mask & (1 << j)) != 0)
                {
                    add_new_arccount(nf, &ac);
                    ac->data.count = fi_ptr->n_ctrs[cn++];
                }
            }
        }      
    }
}

EXPORT_SYMBOL(__gcov_init);
EXPORT_SYMBOL(__gcov_merge_add);
EXPORT_SYMBOL(__gcov_merge_single);
EXPORT_SYMBOL(__gcov_merge_delta);
EXPORT_SYMBOL(__bb_init_func);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem V. Andreev");
MODULE_DESCRIPTION("support for kernel GCOV");
