# Comment/uncomment the following line to disable/enable debugging
#DEBUG = y

EXTRA_CFLAGS += -O2 -I..

ifneq ($(KERNELRELEASE),)
# call from kernel build system

obj-m   := dentry-check.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif


clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules* Mod*

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif
