#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/of.h>

#include "plclkfreq.h"

MODULE_LICENSE("GPL v2");

#define DEV_NAME "plclkfreq"
static dev_t dev_num;
static struct cdev *cdev_p;

static struct clk *clk;

static int register_device(void);
static void unregister_device(void);

static long plclkfreq_ioctl(
    struct file *filp,
    unsigned int cmd,
    unsigned long arg)
{

  int ret;
  unsigned long rate;
  unsigned long __user *u_ratep;

  switch (cmd) {

    case PLCLKFREQ_IOCTFREQ:

      rate = arg;
      // TODO: range check

      ret = clk_set_rate(clk, rate);

      return ret;

    case PLCLKFREQ_IOCGFREQ:

      /* check accessibility */
      u_ratep = (unsigned long __user *)arg;
      if (!access_ok(VERIFY_WRITE, u_ratep, sizeof(unsigned long)))
        return -EFAULT;

      rate = clk_get_rate(clk);
      return __put_user(rate, u_ratep);

    case PLCLKFREQ_IOCTENABLE:

      if (arg) {

        /* enable clock */

        ret = clk_enable(clk);
        if (ret < 0) return ret;

        return 0;

      }

      else {

        /* disable clock */

        clk_disable(clk);

        return 0;

      }

    default:

      return -ENOTTY;

  }

}

static const struct file_operations plclkfreq_fops = {
  .owner          = THIS_MODULE,
  .unlocked_ioctl = plclkfreq_ioctl,
};

static int register_device(void)
{

  int ret;

  printk(KERN_NOTICE DEV_NAME ": registering device\n");
  /* allocate device number */
  ret = alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
  if (ret < 0) {
    printk(KERN_WARNING DEV_NAME ": failed to allocate device number with error %d\n", ret);
    goto errexit;
  }
  printk(KERN_NOTICE DEV_NAME ": successfully allocated device number %d:%d\n", MAJOR(dev_num), MINOR(dev_num));

  /* register char device */
  cdev_p = cdev_alloc();
  if (!cdev_p) {
    printk(KERN_WARNING DEV_NAME ": failed to allocate cdev struct\n");
    ret = -ENOMEM;
    goto errexit;
  }
  cdev_init(cdev_p, &plclkfreq_fops);
  ret = cdev_add(cdev_p, dev_num, 1);
  if (ret < 0) {
    printk(KERN_WARNING DEV_NAME ": failed to add cdev with error %d\n", ret);
    goto errexit;
  }

  return 0;

errexit:
  unregister_device();
  return ret;

}

static void unregister_device(void)
{

  printk(KERN_NOTICE DEV_NAME ": unregistering device\n");
  if (cdev_p)
    cdev_del(cdev_p);
  if (dev_num)
    unregister_chrdev_region(dev_num, 1);

}

static int __init plclkfreq_init(void)
{

  int ret;
  struct device_node *np = NULL;
  
  ret = register_device();
  if (ret < 0)
    goto err_reg_dev;

  np = of_find_node_by_name(NULL, "fclk0");
  if (!np) {
    printk(KERN_WARNING DEV_NAME ": failed to find fclk node\n");
    goto err_get_clk;
  }

  clk = of_clk_get(np, 0);
  of_node_put(np);
  np = NULL;
  if (IS_ERR(clk)) {
    printk(KERN_WARNING DEV_NAME ": failed to acquire clock with error %ld\n", -PTR_ERR(clk));
    ret = PTR_ERR(clk);
    goto err_get_clk;
  }

  return 0;

err_get_clk:
  if (np) of_node_put(np);
  unregister_device();
err_reg_dev:
  return ret;

}

static void __exit plclkfreq_exit(void)
{
  clk_put(clk);
  unregister_device();
}

module_init(plclkfreq_init);
module_exit(plclkfreq_exit);
