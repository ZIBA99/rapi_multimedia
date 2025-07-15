KDIR = /lib/modules/6.12.36-v8+/build

obj-m := gpio_module.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
