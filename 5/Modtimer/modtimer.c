#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <asm/smp.h>
#include <asm/spinlock.h>
#include <linux/kfifo.h>

#define MAX_CBUFFER_LEN		64
#define MAX_KBUF		64

//Variables globales
int timer_period_ms = 250;
int max_random = 100;
int emergency_threshold = 10;

//Las dos entradas de /prog que se piden. cfg para cambiar los params y la otra para consumir nr
static struct proc_dir_entry *proc_entry_cfg, *proc_entry_timer;

//buffer circular
static struct kfifo cbuffer;

/* Work descriptor */
struct work_struct my_work;
/* struct work_struct my_work sigue esta estructura
struct work_struct {
	atomic_long_t data;
	struct list_head entry;
	work_func_t func;
};
*/

struct timer_list my_timer; /* Structure that describes the kernel timer */

static spinlock_t sp;

//Lista enlazada
struct list_head mylist; //Lista de nodos

//Nodos de la lista
typedef struct list_item_t{
	unsigned int data;
	struct list_head links; //links contiene prev y next
}list_item;

int cons_count = 0; //Numero de procesos que abrieron la entrada /proc para lectura 
			//(consumidores)
struct semaphore sem_list; //Para garantizar exclusión mutua
struct semaphore sem_cons; //Cola de espera para consumidor(es)
int nr_cons_waiting = 0; //Numero de procesos consumidores esperando

unsigned long flags;


/*se va a encargar de generar el nr aleatorio y meterlo en el buffer para
que luego se descargue en la lista*/
static void generar_nr_aleatorio(unsigned long data){

	unsigned int nr = get_random_int();
	unsigned int nrAleat = (nr % max_random);
	unsigned long flags;
	int tamOcupado = 0;
	int cpuAct;

	//Protegemos
	spin_lock_irqsave(&sp,flags);

	kfifo_in(&cbuffer,&nrAleat,sizeof(unsigned int)); // Insertamos el elem generado en el buffer circular
	printk(KERN_INFO "Se ha generado el numero: %i \n", nrAleat);

	tamOcupado = kfifo_len(&cbuffer)/4;
	printk(KERN_INFO "TAMANIO DEL KFIFO ES: : %i \n", tamOcupado);

	spin_unlock_irqrestore(&sp, flags);//¿si se ejecuta en otra cpu no hace falta bloquear no?
	
	if(tamOcupado >= emergency_threshold){
		//Consulta si un trabajo esta pendiente.
		if(work_pending((struct work_struct *) &my_work)){
			//Si el trabajo esta pendiente espera la finalización de todos los trabajos en workqueue por defecto
            flush_scheduled_work();
        }
        cpuAct = smp_processor_id();

        if(cpuAct % 2 == 0){ // si la cpu actual es par
        	schedule_work_on(1,&my_work);//Planificamos el trabajo en una cpu impar
        }
        else{//impar
			schedule_work_on(0,&my_work);//Planificamos el trabajo en una cpu par
        }
	}

	mod_timer(&my_timer, jiffies + timer_period_ms);
}


//p4 parte A, permite insertar un elemento en la lista
int insertar_elem_lista(unsigned int elem){
	
	struct list_item_t* nuevoNodo = NULL;

	nuevoNodo = vmalloc(sizeof(struct list_item_t*)); //Reservamos la memoria necesaria

	nuevoNodo->data = elem; //Establecemos el dato
	
	if(down_interruptible(&sem_list)){
		return -EINTR;
	}

	list_add_tail(&nuevoNodo->links, &mylist);
	printk(KERN_INFO "Modlist: Dato aniadido %i a la lista\n", nuevoNodo->data);

	up(&sem_list);

	return 0;
}


/* La función asociada a la tarea volcará los datos del buffer a la lista
enlazada de enteros:
1º Se extraerán todos los elementos del buffer circular (vacía buffer)
2º Se ha de reservar memoria dinámica para los nodos de la lista vía vmalloc()
3º Si el programa de usuario está bloqueado esperando a que haya ele-mentos en la lista, la función le despertará
*/
static void my_wq_function( struct work_struct *work ){

	unsigned int kbuf[MAX_KBUF]; 
	int i = 0;
	int tam = kfifo_len(&cbuffer);
	int res = 0;

	printk(KERN_INFO "Modtimer: Kfifo len vale: %i \n", tam/4);
	
	spin_lock_irqsave(&sp, flags); //El spinlock protege el buffer

	//1º
	res = kfifo_out(&cbuffer,kbuf,tam);

	if(res != tam){
		printk(KERN_INFO "No se ha vaciado el kfifo bien. Tamaño del cbuffer real: %i\n", res/4);
	}

	spin_unlock_irqrestore(&sp, flags);
	
	kbuf[tam+1] = '\0'; 

	//2º
	for(i=0; i<tam/4; i++){
		insertar_elem_lista(kbuf[i]);
	}

	//3º
	if(nr_cons_waiting > 0){
        up(&sem_cons);
     	nr_cons_waiting--;
    }
	
	printk(KERN_INFO "Modtimer: Se han copiado todos los elementos a la lista\n");
}

//Se usará cuando haya que vaciar la lista despues de consumir los elems
int cleanUp_list(void) { //Esta funcion tiene que eliminar todos los nodos de la lista
	//p4 parte A
	struct list_item_t* elem = NULL;
	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;

	if(down_interruptible(&sem_list)){
		return -EINTR;
	}

	list_for_each_safe(nodoAct, aux, &mylist){
		elem = list_entry(nodoAct, struct list_item_t, links);
		printk(KERN_INFO "Se ha borrado: %i \n", elem->data);
		list_del(&elem->links);
		vfree(elem); //Liberacion de memoria donde estaba el nodo
	}
	printk(KERN_INFO "Modlist: Lista vaciada \n");
	up(&sem_list);

	return 0;
}


//el open para /proc/modtimer
static int modtimer_open(struct inode *nodo, struct file *file){

	  //Comprobamos que no hay ningun consumidor
    if(cons_count > 0){
        printk(KERN_INFO "Modtimer: Error, ya hay un consumidor.\n");
        return -ENOSPC; //Ver error a lanzar
    }
    //decimos que somos el consumidor
    cons_count++;

    printk(KERN_INFO "Modtimer: Timer aniadido\n");
	add_timer(&my_timer); //Activa el timer la primera vez

	try_module_get(THIS_MODULE);

	return 0;
}


//el read para /proc/modtimer, el que permite hacer cat /proc/modtimer
static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {

	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;
	struct list_item_t* elem = NULL;

    char kbuf[MAX_KBUF];
    char* dest = kbuf;

    if((*off) > 0) return 0;

    if(list_empty(&mylist)){   //No hay elementos en la lista
        // Bloqueamos al consumidor hasta que haya elementos en la lista enlazada
        nr_cons_waiting++;
        printk(KERN_INFO "Modtimer: Consumidor bloqueado, esperando elementos\n");
		if(down_interruptible(&sem_cons)){
		  nr_cons_waiting--;
		  return -EINTR;
		}
    }
    // Proteccion
    if(down_interruptible(&sem_list)){
    	printk(KERN_INFO "Modtimer: He entrado donde no debo\n");
        return -EINTR;
    }

    if(!list_empty(&mylist)){
    	//Si la lista no esta vacia va consumiendo elementos
    	list_for_each_safe(nodoAct, aux, &mylist){
			elem = list_entry(nodoAct, struct list_item_t, links);
			dest += sprintf(dest, "%u\n", elem->data);
			printk(KERN_INFO "Se ha leido el elemento: %u \n", elem->data);

			//Eliminamos los elementos de la lista para que cuando acabe quede vacia y se puedan seguir metiendo
			list_del(&elem->links);
			vfree(elem); //Liberacion de memoria donde estaba el nodo
   		}
   		kbuf[len] = '\0';	
		if (copy_to_user(buf, kbuf, dest-kbuf)){
			return -EINVAL;
		}
	}

    up(&sem_list);

    return dest-kbuf;
}

/*
1 Desactivar el temporizador con del_timer_sync()
2 Esperar a que termine todo el trabajo planificado hasta el momento
en la workqueue por defecto
3 Vaciar el buffer circular
4 Vaciar la lista enlazada (liberar memoria)
5 Decrementar contador de referencias del módulo

*/
static int modtimer_release(struct inode *nodo, struct file *file){
	
	//1º
	del_timer_sync(&my_timer);
	printk(KERN_INFO "Modtimer: Timer borrado\n");
	// 2º
    if(work_pending((struct work_struct *) &my_work)){
        flush_scheduled_work();
    	printk(KERN_INFO "Modtimer: Trabajos acabados\n");
	}
	//3º
	spin_lock_irqsave(&sp,flags);
	
	kfifo_free(&cbuffer);
	printk(KERN_INFO "Modtimer: Kfifo vaciado\n");
	spin_unlock_irqrestore(&sp,flags);
	
	//4º
	cleanUp_list();

	module_put(THIS_MODULE);

	return 0;
}

static const struct file_operations proc_entry_fops_timer= {
    .read = modtimer_read,
    .open = modtimer_open,
    .release = modtimer_release,
};

//el read para cat /proc/modtimerConfig y mostrar las tres variables globales
static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    char kbuf[MAX_KBUF];
    int tam=0;
    char val[5];

    char timerPeriodMs[35];
	char emergencyThreshold[35];
	char maxRandom[35];

	if ((*off) > 0){
    	return 0;
    }

	strcpy(timerPeriodMs,"timer_period_ms = ");
	strcpy(emergencyThreshold,"emergency_threshold = ");
	strcpy(maxRandom,"max_random = ");

 
	sprintf(val, "%d", timer_period_ms);
	strcat(timerPeriodMs,val);
	
	sprintf(val, "%d", emergency_threshold);
	strcat(emergencyThreshold,val);
	
	sprintf(val, "%d", max_random);
	strcat(maxRandom,val);

	kbuf[0]='\0';

	strcat(kbuf,timerPeriodMs);
	strcat(kbuf,"\n");
	strcat(kbuf,emergencyThreshold);
	strcat(kbuf,"\n");
	strcat(kbuf,maxRandom);
	strcat(kbuf,"\n");

	tam = strlen(kbuf);
	
	kbuf[tam] = '\0';
	tam++;

	printk(KERN_INFO "Modconfig: DATOS: %s \n",kbuf);

	if (copy_to_user(buf, kbuf, tam)){
        return -EINVAL;
    }

    (*off)+=len;

	return tam;
}

//el writte para /proc/modtimerCOnfig que permite modificar las tres variables globales
static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
   
    char kbuf[MAX_KBUF];

    int nuevo_timer_period_ms;
    int nuevo_emergency_threshold;
    int nuevo_max_random;

    if ((*off) > 0){
    	return 0;
    }

    if (len > MAX_KBUF-1) {
        printk(KERN_INFO "Modconfig: No hay espacio disponible\n");
        return -ENOSPC;
    }
    // pasamos el buf en kbuf
    if (copy_from_user(kbuf, buf, len )){
        return -EFAULT;
    }

    kbuf[len] = '\0';
    *off+=len;

    //Parseamos la entrada
    if(sscanf(kbuf,"timer_period_ms %i",&nuevo_timer_period_ms) == 1) {
        timer_period_ms = nuevo_timer_period_ms;
        printk(KERN_INFO "Modconfig: El nuevo timer_period_ms es: %i\n", nuevo_timer_period_ms);
    }
    else if(sscanf(kbuf,"emergency_threshold %i", &nuevo_emergency_threshold) == 1) {
        emergency_threshold = nuevo_emergency_threshold;
        printk(KERN_INFO "Modconfig: El nuevo emergency_threshold es: %i\n", emergency_threshold);
    }
    else if(sscanf(kbuf,"max_random %i", &nuevo_max_random) == 1) {
        max_random = nuevo_max_random;
        printk(KERN_INFO "Modconfig: El nuevo max_random es: %i\n", max_random);
    }
    else {
        return -EINVAL;
    }
    return len;
}

static struct file_operations proc_entry_fops_cfg = {
    .read = modconfig_read,
    .write = modconfig_write,
};


int init_timer_module( void ){
	int ret = 0;

	INIT_LIST_HEAD(&mylist); // inicializamos la lista

	spin_lock_init(&sp); // inicializa el spin lock

	INIT_WORK(&my_work, my_wq_function); // inicializa workqueue con una funcion

	init_timer(&my_timer);

	my_timer.data=0;
    my_timer.function=generar_nr_aleatorio;
    my_timer.expires=jiffies + timer_period_ms;

    sema_init(&sem_list, 1);
    sema_init(&sem_cons, 0);

	if(kfifo_alloc(&cbuffer,(MAX_CBUFFER_LEN*sizeof(int)),GFP_KERNEL)) { // creamos memoria para el kfifo
		return -ENOMEM;
	}

	proc_entry_cfg = proc_create_data("modconfig", 0666, NULL, &proc_entry_fops_cfg, NULL); // Crea la entrada para el modtimer
	
    if (proc_entry_cfg == NULL) {
        printk(KERN_INFO "Modtimer: Error al crear la entrada /proc/modconfig\n");
        kfifo_free(&cbuffer);
        return -ENOMEM;
    }
    else {
		proc_entry_timer = proc_create_data("modtimer", 0666, NULL, &proc_entry_fops_timer, NULL); // Crea la entrada para el modtimer
		if(proc_entry_timer == NULL) {
			printk(KERN_INFO "Modtimer: Error al crear la entrada /proc/modtimer\n");
			kfifo_free(&cbuffer);
			remove_proc_entry("modconfig", NULL);
		return -ENOMEM;
		}
		printk(KERN_INFO "Modtimer: Las dos entradas se han creado correctamente\n");
    }

	printk(KERN_INFO "Modtimer: Modulo cargado.\n");

	return ret;
}

void exit_timer_module( void ){
	
	remove_proc_entry("modconfig", NULL);
    remove_proc_entry("modtimer", NULL);
    kfifo_free(&cbuffer);
}

module_init(init_timer_module);
module_exit(exit_timer_module);