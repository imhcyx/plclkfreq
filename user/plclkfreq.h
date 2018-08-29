#ifndef _PLCLKFREQ_H
#define _PLCLKFREQ_H

#include <linux/ioctl.h>

#define PLCLKFREQ_IOC_MAGIC 0xcf
#define PLCLKFREQ_IOCTFREQ  _IO(PLCLKFREQ_IOC_MAGIC, 1)
#define PLCLKFREQ_IOCGFREQ  _IOR(PLCLKFREQ_IOC_MAGIC, 2, unsigned long)

#endif /* _PLCLKFREQ_H */
