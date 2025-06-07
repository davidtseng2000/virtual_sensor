#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>  
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/kobject.h>

#define PROC_NAME "virtual_temp_sensor"

static struct timer_list temp_timer;
static int virtual_temp = 0;  // 預設 0，初始化時更新
static struct proc_dir_entry *proc_entry;
static struct kobject *sensor_kobj;

// 模擬溫度更新
static void update_temp(struct timer_list *t) {
    virtual_temp = get_random_u32() % 31;
    printk(KERN_INFO "Virtual Sensor: Temperature updated to %d\n", virtual_temp);
    mod_timer(&temp_timer, jiffies + msecs_to_jiffies(5000));
}

// proc 讀取函式
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char temp_str[16];
    int len = snprintf(temp_str, sizeof(temp_str), "%d\n", virtual_temp);
    return simple_read_from_buffer(buf, count, ppos, temp_str, len);
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

// sysfs 讀取函式
static ssize_t temp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", virtual_temp);
}

static struct kobj_attribute temp_attr = __ATTR_RO(temp);

static int __init virtual_sensor_init(void) {
    int ret;

    printk(KERN_INFO "Virtual Sensor: Module loaded\n");

    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_fops);
    if (!proc_entry) {
        printk(KERN_ERR "Virtual Sensor: Failed to create /proc entry\n");
        return -ENOMEM;
    }

    // 預先更新一次溫度
    update_temp(NULL);

    timer_setup(&temp_timer, update_temp, 0);
    mod_timer(&temp_timer, jiffies + msecs_to_jiffies(5000));

    sensor_kobj = kobject_create_and_add("virtual_sensor", kernel_kobj);
    if (!sensor_kobj) {
        printk(KERN_ERR "Virtual Sensor: Failed to create sysfs kobject\n");
        remove_proc_entry(PROC_NAME, NULL);
        del_timer_sync(&temp_timer);
        return -ENOMEM;
    }

    ret = sysfs_create_file(sensor_kobj, &temp_attr.attr);
    if (ret) {
        printk(KERN_ERR "Virtual Sensor: Failed to create sysfs file\n");
        kobject_put(sensor_kobj);
        remove_proc_entry(PROC_NAME, NULL);
        del_timer_sync(&temp_timer);
        return ret;
    }

    printk(KERN_INFO "Virtual Sensor: sysfs file created at /sys/kernel/virtual_sensor/temp\n");

    return 0;
}

static void __exit virtual_sensor_exit(void) {
    printk(KERN_INFO "Virtual Sensor: Module unloaded\n");
    remove_proc_entry(PROC_NAME, NULL);
    del_timer_sync(&temp_timer);
    kobject_put(sensor_kobj);
}

module_init(virtual_sensor_init);
module_exit(virtual_sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dtseng");
MODULE_DESCRIPTION("A virtual temperature sensor with timer and sysfs support");
