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

  unsigned long rate;
  unsigned long __user *u_ratep;

  switch (cmd) {

    case PLCLKFREQ_IOCTFREQ:

      rate = arg;
      // TODO: range check

      return clk_set_rate(clk, rate);

    case PLCLKFREQ_IOCGFREQ:

      /* check accessibility */
      u_ratep = (unsigned long __user *)arg;
      if (!access_ok(VERIFY_WRITE, u_ratep, sizeof(unsigned long)))
        return -EFAULT;

      rate = clk_get_rate(clk);
      return __put_user(rate, u_ratep);

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
  int plclkid = -1;
  
  ret = register_device();
  if (ret < 0)
    goto err_reg_dev;

  np = of_find_compatible_node(NULL, NULL, "xlnx,zynqmp-clkc");
  if (!np) {
    printk(KERN_WARNING DEV_NAME ": failed to find clk provider\n");
    goto err_get_clk;
  }

  /* get id of plclk */
  {
    struct property *prop;
    const char *s;
    int i = 0;

    of_property_for_each_string(np, "clock-output-names", prop, s) {
      if (!strcmp(s, "pl0")) {
        plclkid = i;
        break;
      }
      i++;
    }
    if (plclkid < 0) {
      printk(KERN_WARNING DEV_NAME ": failed to get plclkid\n");
      goto err_get_clk;
    }
  }
  printk(KERN_NOTICE DEV_NAME ": index of pl0 is %d\n", plclkid);

  /* acquire clk device */
  {
    struct of_phandle_args clkspec;

    // TODO: should we fill clkspec ourselves?
    clkspec.np = np;
    clkspec.args_count = 1;
    clkspec.args[0] = plclkid;

    clk = of_clk_get_from_provider(&clkspec);
  }

  of_node_put(np);
  np = NULL;

  if (IS_ERR(clk)) {
    printk(KERN_WARNING DEV_NAME ": failed to acquire clock with error %ld\n", -PTR_ERR(clk));
    ret = PTR_ERR(clk);
    goto err_get_clk;
  }

  ret = clk_enable(clk);
  if (ret < 0) {
    printk(KERN_WARNING DEV_NAME ": failed to enable clock with error %d\n", ret);
    goto err_enable_clk;
  }

  return 0;

err_enable_clk:
  clk_put(clk);
err_get_clk:
  if (np) of_node_put(np);
  unregister_device();
err_reg_dev:
  return ret;

}

static void __exit plclkfreq_exit(void)
{
  clk_disable(clk);
  clk_put(clk);
  unregister_device();
}

module_init(plclkfreq_init);
module_exit(plclkfreq_exit);
