/*
 * Authors:     Xiaofeng Yan(yanxiaofeng7@jd.com)
 * License terms: GNU General Public License (GPL) version 2
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/kdev_t.h>
#include <linux/kallsyms.h>
#include <linux/gfp.h>

spinlock_t * sb_lock_address;
struct list_head * super_blocks_address;

static int __init my_init(void)
{  
    struct super_block *sb;  
    struct list_head *pos;
    struct dentry *dent;
    unsigned long long dcache_referenced_nr = 0;
    char *cp = NULL;
    unsigned long path;

    path = __get_free_pages(GFP_KERNEL,0);
    if (!path){
		printk("alloc pages failed!\n");
		return 1;
   }
   else
		printk("allock pages success!\n");

    sb_lock_address = (spinlock_t *)kallsyms_lookup_name("sb_lock"); 
    super_blocks_address = (struct list_head *)kallsyms_lookup_name("super_blocks"); 

    spin_lock(sb_lock_address); 
    list_for_each(pos, super_blocks_address) {
    sb = list_entry(pos, struct super_block, s_list);  
	if (!list_empty(&sb->s_dentry_lru)) {
            list_for_each_entry(dent, &sb->s_dentry_lru, d_lru) {
                spin_lock(&dent->d_lock);
                if (dent->d_flags & DCACHE_REFERENCED) {
                    dcache_referenced_nr++;
                }
                spin_unlock(&dent->d_lock);
				cp = dentry_path_raw(dent,(char *)path, PAGE_SIZE);
                printk("cp=%s\n",cp);
                }  
            }
    }
    spin_unlock(sb_lock_address);
    free_pages(path,0);
    return 0;
}

static void __exit my_exit(void)  
{  
        printk("unloading…\n");  
}  
module_init(my_init);  
module_exit(my_exit);  
MODULE_LICENSE("GPL");  
