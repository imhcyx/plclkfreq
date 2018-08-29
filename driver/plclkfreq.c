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

ssize_t plclkfreq_read(
    struct file *file_ptr,
    char __user *user_buffer,
    size_t count,
    loff_t *position)
{

  char buf[16];
  int len;
  len = sprintf(buf, "%ld\n", clk_get_rate(clk));

  if (*position >= len) 
    return 0;
  if (*position + count > len)
    count = len - *position;
  if (copy_to_user(user_buffer, buf + *position, count))
    return -EFAULT;
  *position += count;
  return count;

}

static const struct file_operations plclkfreq_fops = {
  .owner  = THIS_MODULE,
  .read   = plclkfreq_read,
};

static int register_device(void)
{

  int ret;

  /* allocate device number */
  printk(KERN_NOTICE DEV_NAME ": allocating device number\n");
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
  printk(KERN_NOTICE DEV_NAME ": successfully added cdev\n");

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
  struct device_node *np;
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

  if (IS_ERR(clk)) {
    printk(KERN_WARNING DEV_NAME ": failed to acquire clock with error %ld\n", -PTR_ERR(clk));
    ret = PTR_ERR(clk);
    goto err_get_clk;
  }

  clk_set_rate(clk, 20000000); // TODO
  //clk_enable(clk);

  return 0;

err_get_clk:
  unregister_device();
err_reg_dev:
  return ret;

}

static void __exit plclkfreq_exit(void)
{
  //clk_disable(clk);
  unregister_device();
}

module_init(plclkfreq_init);
module_exit(plclkfreq_exit);
