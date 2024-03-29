#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/errno.h>
#include <linux/semaphore.h>
#include <linux/kfifo.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

#define MAX_CBUFFER_LEN 64
#define MAX_KBUF 64
#define DEVICE_NAME "fifoprocDev" /* Dev name as it appears in /proc/devices */

dev_t start; /* Starting (major,minor) pair for the driver */
struct cdev* fifoprocDev=NULL; /* Cdev structure associated with the driver */

static struct proc_dir_entry *proc_entry;
static struct kfifo cbuffer; //Buffer circular

int prod_count = 0; //Numero de procesos que abrieron la entrada /proc para escritura 
			//(productores)
int cons_count = 0; //Numero de procesos que abrieron la entrada /proc para lectura 
			//(consumidores)
struct semaphore mtx; //Para garantizar exclusión mutua
struct semaphore sem_prod; //Cola de espera para productor(es)
struct semaphore sem_cons; //Cola de espera para consumidor(es)
int nr_prod_waiting = 0; //Numero de procesos productores esperando
int nr_cons_waiting = 0; //Numero de procesos consumidores esperando


static int fifoproc_open(struct inode *inode, struct file *file) {

	if(down_interruptible(&mtx)) return -EINTR;

	if(file->f_mode & FMODE_READ){//Consumidor
		cons_count++;

		if(nr_prod_waiting > 0){ //cond_broadcast o cond_signal?
			nr_prod_waiting--;
			up(&sem_prod);
		}

		while(prod_count==0) { //Necesita minimo >0 productor
			nr_cons_waiting++;
			up(&mtx);
			if(down_interruptible(&sem_cons)){
				down(&mtx);
				nr_cons_waiting--;
				cons_count--;
				up(&mtx);
				return -EINTR;

			}
			if(down_interruptible(&mtx)) return -EINTR;
		}
	}else{			//Productor
		prod_count++;

		if(nr_cons_waiting > 0){
			nr_cons_waiting--;
			up(&sem_cons);
		}

		while(cons_count==0) { //Necesita minimo >0 productor
			nr_prod_waiting++;
			up(&mtx);
			if(down_interruptible(&sem_prod)){
				down(&mtx);
				nr_prod_waiting--;
				prod_count--;
				up(&mtx);
				return -EINTR;
			}
			if(down_interruptible(&mtx)) return -EINTR;
		}	
	}  
	up(&mtx);  
	try_module_get(THIS_MODULE);/*Previene el borrado de un módulo cuando está en uso.*/

	return 0;
}

static ssize_t fifoproc_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
	char kbuf[MAX_KBUF];

	if(len > MAX_CBUFFER_LEN || len > MAX_KBUF) return -ENOSPC;
      
	if(copy_from_user(kbuf,buf,len)) return -EFAULT;

	kbuf[len] = '\0';// Añade '\0'
    (*off)+=len;// actualiza el puntero

	//"Adquiere" el candado
	if (down_interruptible(&mtx)) return -EINTR;
	

	//Esperar hasta que haya hueco para insertar (debe haber consumidores)
	while (kfifo_avail(&cbuffer) < len && cons_count > 0) {
		//Incrementar num productor esperando
		nr_prod_waiting++;
		
		//"Libera" el candado
		up(&mtx);
		
		//Se bloquea en la cola
		if (down_interruptible(&sem_prod)) {
			down(&mtx);
			nr_prod_waiting--;
			up(&mtx);
			return -EINTR;
		}
		
		//"Adquiere" el candado
		if (down_interruptible(&mtx)) return -EINTR;
	}
	
	//Detectar fin de comunicacion por error (consumidor cierra FIFO antes)
	if (cons_count==0){
	   up(&mtx);
	   return -EPIPE;
	}

	//Insertamos datos
	kfifo_in(&cbuffer, kbuf, len);

	//Despertar a posible consumidor bloqueado
	while (nr_cons_waiting > 0) {
		up(&sem_cons);
		nr_cons_waiting--;
	}
	
	//"Libera" el candado
	up(&mtx);
	
	return len;
}

static ssize_t fifoproc_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
	char kbuf[MAX_KBUF];
	int res;
	if(len > MAX_CBUFFER_LEN || len > MAX_KBUF) return -ENOSPC;
		
	//"Adquiere" el candado
	if(down_interruptible(&mtx)) return -EINTR;

	while (kfifo_len(&cbuffer) < len && prod_count==0) {
		//Incrementar num consumidores esperando
		nr_cons_waiting++;
		
		//"Liberar" el candado
		up(&mtx);
		
		//Se  bloquea la cola
		if (down_interruptible(&sem_cons)) {
			down(&mtx);
			nr_cons_waiting--;
			up(&mtx);
			return -EINTR;
		}
		
		//"Adquirir" el candado
		if (down_interruptible(&mtx)) return -EINTR;
	}
	
	//Detectar fin de comunicacion por error (productor cierra FIFO antes) o FIFO vacia
	if (prod_count == 0 && kfifo_is_empty(&cbuffer)) {
		up(&mtx);
		return 0;
	}
	
	//Leemos datos
	res = kfifo_out(&cbuffer, kbuf, len);
	printk(KERN_INFO "RESULTADO DEL KFIFO_OUT ES: %i\n", res);
	/*if(kfifo_out(&cbuffer, kbuf, len) != len){//no siempre coinciden
		return -EFAULT;// buscar error adecuado
	}*/
	 kbuf[len] = '\0';
	//Despertar a posible productor bloqueado
	while (nr_prod_waiting > 0) {
		up(&sem_prod);
		nr_prod_waiting--;
	}
	
	//"Libera" el candado
	up(&mtx);

	if(copy_to_user(buf,kbuf,len)) return -EFAULT;
	(*off)+=len;  //actualiza el puntero
	
	return len;
}

static int fifoproc_release(struct inode *inode, struct file *file) {

	if(down_interruptible(&mtx)) return -EINTR;

	if(file->f_mode & FMODE_READ){//Consumidor
		cons_count--;
		if(nr_prod_waiting > 0){
			up(&sem_prod);
			nr_prod_waiting--;
		}

	}else{ //Productor
		prod_count--;
		if(nr_cons_waiting > 0){
			up(&sem_cons);
			nr_cons_waiting--;
		}
	}
	
	if(cons_count + prod_count == 0){
		kfifo_reset(&cbuffer);
	}

	up(&mtx);
	module_put(THIS_MODULE);/*Previene el borrado de un módulo cuando está en uso.*/
	return 0;
}

static const struct file_operations proc_entry_fops = {
	.open = fifoproc_open,
	.release = fifoproc_release,
    .read = fifoproc_read,
    .write = fifoproc_write
};

int init_fifoproc_module(void) {

	if(kfifo_alloc(&cbuffer,(MAX_CBUFFER_LEN*sizeof(int)),GFP_KERNEL)) {
		return -ENOMEM;
	}
	/* Get available (major,minor) range */
	if (alloc_chrdev_region (&start, 0, 1,DEVICE_NAME)){// 0 en caso de éxito valor negativo en caso de error
		printk(KERN_INFO "Can't allocate chrdev_region()");
		return -ENOMEM;
	}
	/* Create associated cdev */
	if ((fifoprocDev=cdev_alloc())==NULL){ //Crea estructura cdev y retorna un puntero no nulo a la misma en caso de éxito
		printk(KERN_INFO "cdev_alloc() failed ");
		return -ENOMEM;
	}
	printk(KERN_INFO "'sudo mknod -m 666 /dev/%s c %d %d'.\n",DEVICE_NAME,MAJOR(start),MINOR(start));

	//Inicializacion a 0 de los semaforos usandos como colas de espera
	sema_init(&sem_prod, 0);
	sema_init(&sem_cons, 0);

	//Inicializacion a 1 del semaforo que permite acceso en exclusion
	//emular mutex
	sema_init(&mtx, 1);
	
	nr_prod_waiting=nr_cons_waiting=0;

	proc_entry = proc_create_data("fifoproc", 0666, NULL, &proc_entry_fops, NULL);
	
	if(proc_entry == NULL) {
		kfifo_free(&cbuffer);
		printk(KERN_INFO "Fifoproc: No puedo crear la entrada en proc.\n");
		return -ENOMEM;
	}

	cdev_init(fifoprocDev,&proc_entry_fops); //Asocia interfaz de operaciones del driver a estructura cdev

	/*Permite que peticiones de programas de usuario sobre el rango
	de (major,minor) especificado mediante parámetros first y
	count sean redirigidas al driver que gestiona estructura cdev */
	if (cdev_add(fifoprocDev,start,1)){ 
		printk(KERN_INFO "cdev_add() failed ");
		return -ENOMEM;
	}
	
	printk(KERN_INFO "Fifoproc: Modulo cargado.\n");
	
	return 0;
}


void exit_fifoproc_module(void) {
	remove_proc_entry("fifoproc", NULL);
	kfifo_free(&cbuffer);
	if (fifoprocDev) /* Destroy chardev */
		cdev_del(fifoprocDev); /*Elimina asociaciones de estructura cdev (rangos de major/minor) y libera memoria asociada a la estructura*/
	unregister_chrdev_region(start, 1);/* Unregister the device */

	printk(KERN_INFO "Fifoproc: Modulo descargado.\n");
}


module_init( init_fifoproc_module );
module_exit( exit_fifoproc_module );
