KDIR = /lib/modules/6.12.36-v8+/build

obj-m := gpio_module.o

#PWD := $(shell pwd)
#ARCH := arm64
#CROSS_COMPILE := aarch64-linux-gnu-
EXTRA_CFLAGS := -fno-stack-protector

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
