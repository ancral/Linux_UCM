#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xdca08a4c, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x1c64374b, __VMLINUX_SYMBOL_STR(usb_deregister) },
	{ 0xdf717b1c, __VMLINUX_SYMBOL_STR(usb_register_driver) },
	{ 0xbe6e2b29, __VMLINUX_SYMBOL_STR(usb_deregister_dev) },
	{ 0x84580b50, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0x6150d608, __VMLINUX_SYMBOL_STR(usb_register_dev) },
	{ 0xaebc19b7, __VMLINUX_SYMBOL_STR(dev_set_drvdata) },
	{ 0x2c0a7ac4, __VMLINUX_SYMBOL_STR(usb_get_dev) },
	{ 0x7737e50, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x16305289, __VMLINUX_SYMBOL_STR(warn_slowpath_null) },
	{ 0xb2b265cb, __VMLINUX_SYMBOL_STR(dev_get_drvdata) },
	{ 0x6588fa54, __VMLINUX_SYMBOL_STR(usb_find_interface) },
	{ 0x733c3b54, __VMLINUX_SYMBOL_STR(kasprintf) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0x4f92b936, __VMLINUX_SYMBOL_STR(usb_put_dev) },
	{ 0x7049442b, __VMLINUX_SYMBOL_STR(usb_control_msg) },
	{ 0x85df9b6c, __VMLINUX_SYMBOL_STR(strsep) },
	{ 0x9166fada, __VMLINUX_SYMBOL_STR(strncpy) },
	{ 0x20c55ae0, __VMLINUX_SYMBOL_STR(sscanf) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0xb5419b40, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=usbcore";

MODULE_ALIAS("usb:v20A0p41E5d*dc*dsc*dp*ic*isc*ip*in*");
