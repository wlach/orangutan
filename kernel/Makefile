
ifneq ($(KERNELRELEASE),)

obj-m += orng.o

orng-y := module.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif
