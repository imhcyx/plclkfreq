#ifndef _PLCLKFREQ_H
#define _PLCLKFREQ_H

#include <linux/ioctl.h>

#define PLCLKFREQ_IOC_MAGIC 0xcf
#define PLCLKFREQ_IOCGETFREQ  _IOR(PLCLKFREQ_IOC_MAGIC, 1, int)
#define PLCLKFREQ_IOCSETFREQ  _IOW(PLCLKFREQ_IOC_MAGIC, 2, int)

#endif /* _PLCLKFREQ_H */
