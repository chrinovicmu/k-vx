ifeq ($(KERNELRELEASE),)
    # Called from command line
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order

.PHONY: modules modules_install clean

else
    # Called from kernel build system
    obj-m := lkm_hyp.o
    ccflags-y += -mcmodel=kernel -mno-red-zone -g -DDEBUG
endif
