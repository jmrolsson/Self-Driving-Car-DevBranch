#ifndef CHARVIDEODEV_H_
#define CHARVIDEODEV_H_

#include "vdmadriver.h"
#include "vdma.h"
#include "charvideo_ioctl.h"
//#include <linux/device.h>


struct charvideo_dev {
	int dev_major;
	struct vdmaDriver_local *lp;
	struct cdev cdev;	  /* Char device structure		*/
	struct class *class;
	struct device *node;
};


ssize_t charvideo_read(struct file *fp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct charvideo_dev *dev = fp->private_data;
	struct vdmaController *v = dev->lp->vdmactrl;

	int maxRead = v->height * v->width * v->pixelLength;

	if (*f_pos >= maxRead)
		return 0;
	if (*f_pos + count > maxRead)
		count = maxRead - *f_pos;


	//do read
	printk(KERN_NOTICE "Copying\n");

	if (copy_to_user(buf, v->fb2VirtualAddress, count)) {
		return -EFAULT;
	}


	*f_pos += count;
	return count;

}

long charvideo_ioctl(struct file *fp, unsigned int cmd, unsigned long arg) {
	struct charvideo_dev *dev = fp->private_data;
	struct vdmaController *v = dev->lp->vdmactrl;
	int err = 0, tmp;
	int retval = 0;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != CHARVIDEO_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > CHARVIDEO_IOC_MAXNR)
		return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
	err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
	err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	switch (cmd) {

	case CHARVIDEO_IOCHALT:
		vdma_halt(v);
		break;

	case CHARVIDEO_IOCSTART:
		vdma_start_triple_buffering(v);
		break;

	case CHARVIDEO_IOCSTATUS:
		printk(KERN_NOTICE "Dumping the vdma's status:\n");
		vdma_s2mm_status_dump(v);
		break;

	case CHARVIDEO_IOCQHEIGHT:
		return v->height;

	case CHARVIDEO_IOCQWIDTH:
		return v->width;

	case CHARVIDEO_IOCQPIXELLEN:
		return v->pixelLength;

	case CHARVIDEO_IOCQBUFSIZE:
		return v->width * v->height * v->pixelLength;

	default:
		return -ENOTTY;
	}

	return retval;

}

int charvideo_open(struct inode *in, struct file *fp)
{
	struct charvideo_dev *dev; /* device information */
	printk(KERN_NOTICE "Opened a file\n");

	dev = container_of(in->i_cdev, struct charvideo_dev, cdev);
	fp->private_data = dev; /* for other methods */


	return 0;          /* success */
}


struct file_operations charvideo_fops = {
	.owner =    THIS_MODULE,
	//.llseek =   scull_llseek,
	.read =     charvideo_read,
	//.write =    scull_write,
	//.unlocked_ioctl = scull_ioctl,
	.unlocked_ioctl = charvideo_ioctl,
	.open =     charvideo_open,
	//.release =  scull_release,
};


/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void charvideo_delete(struct charvideo_dev *dev)
{
	dev_t devno = MKDEV(dev->dev_major, 0);

	device_destroy(dev->class, devno);  // Remove the /dev/kmem
	class_destroy(dev->class);

	cdev_del(&dev->cdev);

	kfree(dev);

	unregister_chrdev_region(devno, 1);

	printk(KERN_NOTICE "Unregistered charvideo char device.\n");
}


/*
 * Set up the char_dev structure for this device.
 */
static int charvideo_setup_cdev(struct charvideo_dev *dev)
{
	int err, devno = MKDEV(dev->dev_major, 0);
	struct device *node;


	cdev_init(&dev->cdev, &charvideo_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &charvideo_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err){
		printk(KERN_WARNING "Error %d adding charvideo\n", err);
		return -1;
	}

	dev->class = class_create(THIS_MODULE, "video");
	if (!dev->class) {
		printk(KERN_WARNING "Can't create class");
		return -1;
	}

	printk(KERN_NOTICE "Created class.\n");

	// Create /dev/kmem for this char dev
	node = device_create(dev->class, NULL, devno, NULL, "video");
	if (!node) {
		printk(KERN_WARNING "Can't create device /dev/video\n");
		class_destroy(dev->class);
		return -1;
	}


	return 0;
}


int charvideo_init(struct charvideo_dev *device, struct vdmaDriver_local *lp)
{
	int result;
	dev_t major = 0;

	result = alloc_chrdev_region(&major, 0, 1, "charvideo");

	device->dev_major = MAJOR(major);
	if (result < 0) {
		printk(KERN_WARNING "charvideo: can't get major %d\n", device->dev_major);
		charvideo_delete(device);
		return result;
	}

	device->lp = lp;

	//set the fops
	//device->fops.owner=lp->this;
	//device->fops.read=charvideo_read;
	//device->fops.open=charvideo_open;

	result = charvideo_setup_cdev(device);
	if(result<0)
	{
		charvideo_delete(device);
		return result;
	}

	return 0; /* succeed */
}

struct charvideo_dev* charvideo_alloc(struct vdmaDriver_local *lp){
	struct charvideo_dev *local;

	local = kmalloc(sizeof(struct charvideo_dev), GFP_KERNEL);
	if (!local)
		return NULL;

	int result;
	result = charvideo_init(local, lp);
	if (result<0)
		return NULL;

	return local;
}

#endif
