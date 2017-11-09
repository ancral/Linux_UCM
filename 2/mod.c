#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt_kern.h>
#include <asm-generic/errno.h>

static struct proc_dir_entry *proc_entry;
const int MAX_TAM=256;



static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
	char kbuf[MAX_TAM];
	int num;

	printk(KERN_INFO "modleds: loading\n");
	struct tty_driver* kbd_driver= vc_cons[fg_console].d->port.tty->driver;
	printk(KERN_INFO "modleds: fgconsole is in %x\n", fg_console);


	if ((*off)>0) return 0;

	if (len > MAX_TAM-1){
		printk(KERN_INFO "Mod: No hay espacio\n");
		return -ENOSPC;
	}

	if (copy_from_user(kbuf, buf, len)) return -EFAULT;

	kbuf[len] = '\0'; 

	if (sscanf(kbuf, "set 0x%i", &num) == 1){
		  int bit0 = (num & 1);
		  int bit1 = (num & (1 << 2)) >> 1;
		  int bit2 = (num & (1 << 1)) << 1;
		 (kbd_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, bit2 | bit1 | bit0);
	}
	else return -EINVAL;

	*off += len;

	return len;
}



static const struct file_operations proc_entry_fops = {
	.write = modlist_write,
};


int init_mod_module(void) {
	int ret = 0;

	proc_entry = proc_create("mod", 0666, NULL, &proc_entry_fops); 

	if (proc_entry == NULL){
		ret = -ENOMEM;
		printk(KERN_ALERT "Mod: No se puede crear la entrada\n");
	}
	else {
		printk(KERN_INFO "Mod: Modulo cargado\n");
	}
	return ret;
}


void exit_mod_module(void) {
	remove_proc_entry("mod", NULL);
	printk(KERN_INFO "Mod: Modulo descargado.\n");
}



module_init(init_mod_module);
module_exit(exit_mod_module);
