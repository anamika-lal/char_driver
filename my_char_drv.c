#include<linux/types.h>		// for dev_t type
#include<linux/module.h>	// for all module related apis
#include<linux/fs.h>		// for chrdev register unreg apis
#include<linux/cdev.h>		// for cdev structure and cdev init add apis
#include<linux/device.h>	// for class and device create apis
#include<linux/uaccess.h>	// for copy_to/from_user calls

MODULE_LICENSE("GPL"); 		// avoids kernel taint.

static dev_t my_dev_maj_min; 	// major and minor holder
static struct cdev my_cdev;	// char device structure.
static struct class *my_class;	// device class structure
static struct device *my_device;// device structure

struct my_data_struct {
	char name[20];
	struct mutex lock;
} data;

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
	int ret;
	printk("%s\n",__func__);

	/* Can also use a simple mutex_lock but that returns void.
	 * Using mutex_try_lock variant allow us to check the
	 * lock state as it returns 0 if fails to lock
	 */
	if (!mutex_trylock(&data.lock))
		return -EBUSY;

	/* simple_read_from_buffer copies data from kernel buffer
	 * (here, data.name) to the user buffer (here, buf)
	 * using copy_to_user more sophestically.
	 */
	ret = simple_read_from_buffer(buf, len,
		off, data.name, sizeof(data.name));
	if (ret < 0)
		printk("Unable to read\n");

	mutex_unlock(&data.lock);
	return ret;
}

static ssize_t my_write(struct file *f, const char __user *buf,
		size_t len, loff_t *off)
{
	printk("%s\n",__func__);

	if (!mutex_trylock(&data.lock))
		return -EBUSY;
	copy_from_user(data.name, buf, len);
	mutex_unlock(&data.lock);

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

	/* udev daemon automatically picks devices info from /sys/class/
	 * entries and device is populated under /dev. So we need to
	 * create a class under /sys and then create/populate device.
	 */
	/* ls -l /sys/class */
	my_class = class_create(THIS_MODULE, "my_class");
	if (my_class < 0)
		printk("Unable to create class under /sys\n");
	/* create device and populate it with device info i.e its
	 * major and minor number
	 */
	/* ls -l /dev/ */
	my_device = device_create(my_class, NULL, my_dev_maj_min,
			NULL, "my_dev");
	if (my_device < 0)
		printk("Unable to create device under /dev\n");

	/* char device initialization and addition onto the VFS
	 * so that VFS knows about our device and whenever the
	 * device is accessed it can call this driver's fops for the
	 * device
	 */
	cdev_init(&my_cdev, &my_fops);
	ret = cdev_add(&my_cdev, my_dev_maj_min, 1);
	if (ret < 0) {
		printk("%s: Cannot add char device\n", __func__);
		goto ret;
	}

	/* mutex initialized */
	mutex_init(&data.lock);
ret:
	return 0;

}

void my_delete(void)
{
	mutex_destroy(&data.lock);
	cdev_del(&my_cdev);
	device_destroy(my_class, my_dev_maj_min);
	class_destroy(my_class);
	unregister_chrdev_region(my_dev_maj_min, 2);
	pr_info("%s\n", __func__);
}

module_init(my_init);
module_exit(my_delete);
