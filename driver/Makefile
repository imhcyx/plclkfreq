obj-m := plclkfreq.o
KDIR := /home/free/zypi/prj5-cpu-opt-teacher/software/kernel_phys_os
KBUILD_CFLAGS += -march=armv8-a -mtune=cortex-a53
MAKE := make
ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-
build: kernel_modules
kernel_modules:
	${MAKE} -C $(KDIR) M=$(PWD) modules ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

.PHONY: clean
clean:
	${MAKE} -C $(KDIR) M=$(PWD) clean
