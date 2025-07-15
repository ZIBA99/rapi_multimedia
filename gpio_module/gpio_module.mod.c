#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0xdcb764ad, "memset" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0xd9ec4b80, "gpio_to_desc" },
	{ 0x178f2652, "gpiod_set_raw_value" },
	{ 0x7682ba4e, "__copy_overflow" },
	{ 0x98cf60b3, "strlen" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x1a115338, "try_module_get" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0x5d9d9fd4, "cdev_init" },
	{ 0xcc335c1c, "cdev_add" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xf0024008, "gpiod_direction_output_raw" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x607587f4, "cdev_del" },
	{ 0xfe990052, "gpio_free" },
	{ 0x18b177f2, "module_put" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "7B53BA14C836FA7D7E30DD5");
