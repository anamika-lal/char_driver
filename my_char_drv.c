#include<linux/types.h>		// for dev_t type
#include<linux/module.h>	// for all module related apis
#include<linux/fs.h>		// for chrdev register unreg apis
#include<linux/cdev.h>		// for cdev structure and cdev init add apis

MODULE_LICENSE("GPL"); 		// avoids kernel taint.

static dev_t my_dev_maj_min; 	// major and minor holder
static struct cdev my_cdev;	// char device structure.

static int my_open(struct inode *i, struct file *f)
{
	printk("%s\n",__func__);
	return 0;
}

static int my_close(struct inode *i, struct file *f)
{
	printk("%s\n",__func__);
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf,
		size_t len, loff_t *off)
{
	printk("%s\n",__func__);
	return 0;
}

static ssize_t my_write(struct file *f, const char __user *buf,
		size_t len, loff_t *off)
{
	printk("%s\n",__func__);
	return len;
}

static struct file_operations my_fops = {
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_close
};

int my_init(void)
{
	int ret;

	/* ls -l /proc/devices/ */
	ret = alloc_chrdev_region(&my_dev_maj_min, 0, 1, "mydev");
	if (ret < 0)
		pr_info("Unable to allocate, ret = %d\n", ret);
	printk("%s : major = %d, minor = %d\n", __func__,
		MAJOR(my_dev_maj_min), MINOR(my_dev_maj_min));

	/* char device initialization and addition onto the VFS
	 * so that VFS knows about our device and whenever the
	 * device is accessed it can call this driver's fops for the
	 * device
	 */
	cdev_init(&my_cdev, &my_fops);
	ret = cdev_add(&my_cdev, my_dev_maj_min, 2);
	if (ret < 0)
		printk("%s: Cannot add char device\n", __func__);

	return 0;	
}

void my_delete(void)
{
	cdev_del(&my_cdev);
	unregister_chrdev_region(my_dev_maj_min, 2);
	pr_info("%s\n", __func__);
}

module_init(my_init);
module_exit(my_delete);
