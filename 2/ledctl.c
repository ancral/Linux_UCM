#include <linux/syscalls.h> /* For SYSCALL_DEFINEi() */
#include <linux/kernel.h>
#include <asm-generic/errno.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>

SYSCALL_DEFINE1(ledctl,unsigned int,mask){
	int bit0 = (mask & 1);
	int bit1 = (mask & (1 << 2)) >> 1;
	int bit2 = (mask & (1 << 1)) << 1;
	struct tty_driver* kbd_driver= vc_cons[fg_console].d->port.tty->driver;	

	if(mask < 0 || mask > 7){
	  return -EINVAL;
	}
	return (kbd_driver->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, bit2 | bit1 | bit0);
}
