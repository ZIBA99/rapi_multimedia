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
	{ 0x18b177f2, "module_put" },
	{ 0x1a115338, "try_module_get" },
	{ 0xbb9ed3bf, "mutex_trylock" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xd9ec4b80, "gpio_to_desc" },
	{ 0x178f2652, "gpiod_set_raw_value" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0xdcb764ad, "memset" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x2d39b0a7, "kstrdup" },
	{ 0x85df9b6c, "strsep" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x24d273d1, "add_timer" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0x6cde1f33, "find_vpid" },
	{ 0x60e82168, "pid_task" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0x7682ba4e, "__copy_overflow" },
	{ 0x74f08e6f, "gpiod_get_raw_value" },
	{ 0xdbc65d90, "send_sig_info" },
	{ 0x98cf60b3, "strlen" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0x5d9d9fd4, "cdev_init" },
	{ 0xcc335c1c, "cdev_add" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xf0024008, "gpiod_direction_output_raw" },
	{ 0xaa6d2cd7, "gpiod_to_irq" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x607587f4, "cdev_del" },
	{ 0xc1514a3b, "free_irq" },
	{ 0xfe990052, "gpio_free" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "BB6F3661A06721A9DD8D90E");
