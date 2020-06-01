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

spinlock_t * sb_lock_address;
struct list_head * super_blocks_address;
typedef int (*printk_type)(const char *fmt, ...);
typedef void (*prune_dcache_sb_type)(struct super_block *sb, int count);
printk_type printk_address;
prune_dcache_sb_type prune_dcache_sb_address;


static u64  reclaim_dentry_number = 1000;
static u64  reclaim_inode_number = 1000;
static u64  threshold_dentry_number = 100000000;
static u64  interval_timer = 10;
static u64 reclaim_debug = 0;

/*create proc interface */
static struct ctl_table reclaim_dentry_table[] = {
        {
         .procname = "reclaim_dentry_number",
         .data = &reclaim_dentry_number,
         .maxlen = sizeof(int),
         .mode = 0644,
         .proc_handler = proc_dointvec,
         },
        {
         .procname = "reclaim_inode_number",
         .data = &reclaim_inode_number,
         .maxlen = sizeof(int),
         .mode = 0644,
         .proc_handler = proc_dointvec,
         },
        {
         .procname = "threshold_dentry_number",
         .data = &threshold_dentry_number,
         .maxlen = sizeof(int),
         .mode = 0644,
         .proc_handler = proc_dointvec,
         },
        {
         .procname = "interval_timer",
         .data = &interval_timer,
         .maxlen = sizeof(int),
         .mode = 0644,
         .proc_handler = proc_dointvec,
         },
        {
         .procname = "reclaim_debug",
         .data = &reclaim_debug,
         .maxlen = sizeof(int),
         .mode = 0644,
         .proc_handler = proc_dointvec,
         },
        {}
};

static struct ctl_table reclaim_root[] = {
        {
         .procname = "slab",
         .maxlen = 0,
         .mode = 0555,
         .child = reclaim_dentry_table,
         },
        {}
};

static struct ctl_table dev_root[] = {
        {
         .procname = "vm",
         .maxlen = 0,
         .mode = 0555,
         .child = reclaim_root,
         },
        {}
};

static struct ctl_table_header *sysctl_header;

static int proc_reclaim_dentry_init(void)
{
        sysctl_header = register_sysctl_table(dev_root);
        return 0;
}

void proc_reclaim_dentry_exit(void)
{
	unregister_sysctl_table(sysctl_header);
}

/*creat a single workqueue to handle reclaiming slab dentry*/

static void reclaim_func(struct work_struct *work)
{

    struct super_block *sb;
    struct list_head *pos;
    u64 before = 0;
    u64 after = 0;    
    
    sb_lock_address = (spinlock_t *)kallsyms_lookup_name("sb_lock");
    super_blocks_address = (struct list_head *)kallsyms_lookup_name("super_blocks");
    prune_dcache_sb_address=(prune_dcache_sb_type)kallsyms_lookup_name("prune_dcache_sb");

    spin_lock(sb_lock_address);
    list_for_each(pos, super_blocks_address) {
        sb = list_entry(pos, struct super_block, s_list);
        if(sb->s_nr_dentry_unused > threshold_dentry_number ){
    		before = sb->s_nr_dentry_unused;
       		prune_dcache_sb_address(sb,reclaim_dentry_number);
    		after = sb->s_nr_dentry_unused;
	}
      }
    spin_unlock(sb_lock_address);
    if(reclaim_debug)
    	printk("before recaliming dentry %llu,after reclaiming %llu\n",before,after);
}

static DECLARE_WORK(reclaim_work, reclaim_func);
static struct workqueue_struct *reclaim_workqueue;
int wq_init(void)
{
        reclaim_workqueue = create_singlethread_workqueue("reclaim-wq");
//        printk("reclaim-wq init success\n");
        return 0;
}

void wq_exit(void)
{
        destroy_workqueue(reclaim_workqueue);
}

/*create a timer to statup q work*/
struct timer_list reclaim_timer;
void reclaim_timer_fn(unsigned long arg)
{       
	        
    	mod_timer(&reclaim_timer, jiffies + interval_timer*HZ);
        if (queue_work(reclaim_workqueue, &reclaim_work) == 0) {
                printk("add work queue failed\n");
        }
}


static int reclaim_timer_init(void)
{
        setup_timer(&reclaim_timer, reclaim_timer_fn, (unsigned long)"reclaim timer");
        reclaim_timer.expires = jiffies + HZ;
        add_timer(&reclaim_timer);
        return 0;

}

static void  reclaim_timer_exit(void)
{
        del_timer(&reclaim_timer);
}

/*init*/
static int __init reclaim_init(void)
{ 
    printk("loading slab reclaim\n"); 
    proc_reclaim_dentry_init();
    reclaim_timer_init();
    wq_init();
    printk("load slab reclaim success\n"); 
    return 0;
}
/*exit*/
static void __exit reclaim_exit(void)  
{  
        printk("unloading slab reclaim\n"); 
	proc_reclaim_dentry_exit();
        reclaim_timer_exit();
	wq_exit();
        printk("unloaded slab reclaim\n"); 
}  
module_init(reclaim_init);  
module_exit(reclaim_exit);  
MODULE_LICENSE("GPL");
