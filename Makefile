ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules ccflags-y="-g -DDEBUG"

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf src/*.o src/*~ src/core src/.depend src/.*.cmd src/*.ko src/*.mod.c src/.tmp_versions
	git rm -f --ignore-unmatch src/*.o src/*.ko src/*.mod.c

.PHONY: modules modules_install clean

else

obj-m := src/lkm_hyp.o
ccflags-y += -mcmodel=kernel

endif

