#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "plclkfreq.h"

int setfreq(unsigned long rate)
{

  int fd;
  int ret;

  fd = open("/dev/plclkfreq", O_RDWR);
  if (fd < 0) {
    return fd;
  }

  ret = ioctl(fd, PLCLKFREQ_IOCTFREQ, rate);
  
  close(fd);

  return ret;

}

int getfreq(unsigned long *rate)
{

  int fd;
  int ret;

  fd = open("/dev/plclkfreq", O_RDWR);
  if (fd < 0) {
    return fd;
  }

  ret = ioctl(fd, PLCLKFREQ_IOCGFREQ, rate);
  
  close(fd);

  return ret;

}

int main(int argc, char *argv[])
{

  int ret;
  unsigned long rate;

  if (argc >= 2) {

    if (!strcmp(argv[1], "set")) {
      if (argc < 3) return 1;
      rate = strtoul(argv[2], NULL, 10);
      return setfreq(rate);
    }

    else if (!strcmp(argv[1], "get")) {
      ret = getfreq(&rate);
      if (ret < 0) return ret;
      printf("%lu\n", rate);
      return 0;
    }

    else {
      return 1;
    }
  }

  return 0;

}