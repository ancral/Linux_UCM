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
#include <asm-generic/errno.h>
#include <linux/kfifo.h>

#define MAX_CBUFFER_LEN		64
#define MAX_KBUF		64

//Variables globales
int timer_period_hz = 500;
int max_random = 250;
int emergency_threshold = 60;

//Las dos entradas de /prog que se piden. cfg para cambiar los params y la otra para consumir nr
static struct proc_dir_entry *proc_entry_cfg, *proc_entry_timer;

//buffer circular
static struct kfifo cbuffer;


//OPCION 2 que se puede encolar en la cola por defecto con schedule_work(&my_work);
/* Work descriptor */
struct work_struct my_work;
/* //Se supone que struct work_struct my_work sigue esta estructura
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
	int data;
	struct list_head links; //links contiene prev y next
}list_item;

int nrElemsLista = 0;

int cons_count = 0; //Numero de procesos que abrieron la entrada /proc para lectura 
			//(consumidores)
struct semaphore mtx; //Para garantizar exclusión mutua
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

	kfifo_in(&cbuffer,&nrAleat,sizeof(int)); // Insertamos el elem generado en el buffer circular
	printk(KERN_INFO "Se ha generado el número: %d \n", nrAleat);

	tamOcupado = kfifo_len(&cbuffer);
	printk(KERN_INFO "TAMAÑO DEL KFIFO ES: : %i \n", tamOcupado);

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

	mod_timer(&my_timer, jiffies + timer_period_hz);
}

//p4 parte A, permite insertar un elemento en la lista
int insertar_elem_lista(unsigned int elem){
	
	struct list_item_t* nuevoNodo = NULL;

	nuevoNodo = vmalloc(sizeof(struct list_item_t*)); //Reservamos la memoria necesaria

	nuevoNodo->data = elem; //Establecemos el dato
	
	
	list_add_tail(&nuevoNodo->links, &mylist);
	printk(KERN_INFO "Modlist: Dato aniadido %i a la lista\n", elem);
	nrElemsLista++;

	return 0;
}

//Se usará cuando haya que vaciar la lista despues de consumir los elems
int cleanUp_list(void) { //Esta funcion tiene que eliminar todos los nodos de la lista
	//p4 parte A
	struct list_item_t* elem = NULL;
	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;

	if(down_interruptible(&mtx)){
		return -EINTR;
	}

	list_for_each_safe(nodoAct, aux, &mylist){
		elem = list_entry(nodoAct, struct list_item_t, links);
		printk(KERN_INFO "Se ha borrado: %i \n", elem->data);
		list_del(&elem->links);
		vfree(elem); //Liberacion de memoria donde estaba el nodo
	}
	printk(KERN_INFO "Modlist: Lista vaciada \n");
	nrElemsLista = 0;
	up(&mtx);

	return 0;
}

//el open para /proc/modtimer
static int modtimer_open(struct inode *nodo, struct file *file){

	add_timer(&my_timer); //Activa el timer la primera vez

	return 0;
}

//el release para /proc/modtimer
static int modtimer_release(struct inode *nodo, struct file *file){
	struct list_item_t* elem = NULL;
	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;
	
	//Liberamos el timer
	del_timer_sync(&my_timer);

	//Liberamos cbuffer
	spin_lock_irqsave(&sp,flags);
	clear_cbuffer_t(cbuffer);
	spin_unlock_irqrestore(&sp,flags);
	
	//Liberamos lista
	list_for_each_safe(nodoAct, aux, &mylist){
		elem = list_entry(nodoAct, struct list_item_t, links);
		list_del(pos);
		vfree(elem);
	}

	return 0;
}

//el read para /proc/modtimer, el que permite hacer cat /proc/modtimer
static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
	char kbuf[MAX_KBUF];
	char *dest = kbuf;
	int ret;
	struct list_head* nodoAct = NULL;
	struct list_item_t* elem = NULL;
	struct list_head* aux = NULL;
	
	if((*off) > 0) return 0; 
	
	if(len > MAX_CBUFFER_LEN || len > MAX_KBUF) return -ENOSPC;
		
	if(down_interruptible(&mtx)) return -EINTR;

	//Mientras este vacia la lista, sigue en el bucle
	while (list_empty(&mylist) == 0) {
		nr_cons_waiting++;
		
		up(&mtx);
		
		if (down_interruptible(&sem_cons)) {
			down(&mtx);
			nr_cons_waiting--;
			up(&mtx);
			return -EINTR;
		}
		
		if (down_interruptible(&mtx)) return -EINTR;
	}
	
	list_for_each_safe(nodoAct,aux,&mylist){
		elem = list_entry(nodoAct, struct list_item_t, links);     
		list_del(nodoAct);
        printk(KERN_INFO "Modtimer: %i elemento leido",elem->data); 
        dest+=sprintf(dest,"%i\n",elem->data);
		vfree(elem); 
	}
	kbuf[len] = '\0'; 

	up(&mtx);
		
	if((*off) > 0) ret = 0;
	else {
		if (copy_to_user(buf, kbuf, len)) return -EINVAL;
		ret = (dest-kbuf);
	}

	(*off) += len; 

	return ret; 
}

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

 
	sprintf(val, "%d", (1/timer_period_hz)*1000);
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

	printk(KERN_INFO "modtimer: DATOS: %s \n",kbuf);

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
        printk(KERN_INFO "modtimer: No hay espacio disponible\n");
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
        timer_period_hz = (1/nuevo_timer_period_ms)*1000;
        printk(KERN_INFO "modtimer: El nuevo timer_period_ms es: %i\n", nuevo_timer_period_ms);
    }
    else if(sscanf(kbuf,"emergency_threshold %i", &nuevo_emergency_threshold) == 1) {
        emergency_threshold = nuevo_emergency_threshold;
        printk(KERN_INFO "modtimer: El nuevo emergency_threshold es: %i\n", emergency_threshold);
    }
    else if(sscanf(kbuf,"max_random %i", &nuevo_max_random) == 1) {
        max_random = nuevo_max_random;
        printk(KERN_INFO "modtimer: El nuevo max_random es: %i\n", max_random);
    }
    else {
        return -EINVAL;
    }
    return len;
}

/* La función asociada a la tarea volcará los datos del buffer a la lista
enlazada de enteros:

1º Se extraerán todos los elementos del buffer circular (vacía buffer)
2º Se ha de reservar memoria dinámica para los nodos de la lista vía vmalloc()
3º Si el programa de usuario está bloqueado esperando a que haya ele-mentos en la lista, la función le despertará
*/
static void my_wq_function( struct work_struct *work ){

	int kbuf[MAX_KBUF]; 
	int tamOcupado;
	int i = 0;
	
	spin_lock_irqsave(&sp, flags); //El spinlock protege el buffer
	//1º
	//¡¡¡¡ NO COMPROBADO EL KFIFO OUT!!!
	tamOcupado = kfifo_len(&cbuffer); //El tamaño puede cambiar
	kfifo_out(&cbuffer,&kbuf,(nrElemsLista*sizeof(int))); // Insertamos el elem generado en el buffer circular

	spin_unlock_irqrestore(&sp, flags);
	
	kbuf[tamOcupado] = '\0'; 
	
	if(down_interruptible(&mtx)) return -EINTR;
	
	//2º
	for(i=0; i<tamOcupado; i++){
		insertar_elem_lista(kbuf[i]);
	}

	//3º semaforo productor consumidor??--> Respuesta: solo consumidor
	if(nr_cons_waiting > 0){
        up(&sem_cons);
     	nr_cons_waiting--;
    }
	
	up(&mtx);
	
	printk(KERN_INFO "modtimer: Se han copiado todos los elementos a la lista\n");
}

static struct file_operations proc_entry_fops_cfg = {
    .read = modconfig_read,
    .write = modconfig_write,
};

static const struct file_operations proc_entry_fops_timer= {
    .read = modtimer_read,
    .open = modtimer_open,
    .release = modtimer_release,
};


int init_timer_module( void ){
	int ret = 0;

	INIT_LIST_HEAD(&mylist); // inicializamos la lista

	spin_lock_init(&sp); // inicializa el spin lock

	INIT_WORK(&my_work, my_wq_function); // inicializa workqueue con una funcion

	init_timer(&my_timer);

	my_timer.data=0;
    my_timer.function=generar_nr_aleatorio;
    my_timer.expires=jiffies + timer_period_hz;


	if(kfifo_alloc(&cbuffer,(MAX_CBUFFER_LEN*sizeof(int)),GFP_KERNEL)) { // creamos memoria para el kfifo
		return -ENOMEM;
	}

	proc_entry_timer = proc_create_data("modtimer", 0666, NULL, &proc_entry_fops_timer, NULL); // Crea la entrada para el modtimer
	proc_entry_cfg = proc_create_data("modconfig", 0666, NULL, &proc_entry_fops_cfg, NULL); // Crea la entrada para el modtimer

	if(proc_entry_timer == NULL || proc_entry_cfg == NULL) { // si hay error vaciamos el kfifo
		kfifo_free(&cbuffer);
		printk(KERN_INFO "Modtimer: Error al crear la entrada modtimer o modconfig\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "Modtimer: Modulo cargado.\n");


	return ret;
}

void exit_timer_module( void ){
	
	cleanUp_list();
	remove_proc_entry("modconfig", NULL);
    remove_proc_entry("modtimer", NULL);
    kfifo_free(&cbuffer);
}

module_init(init_timer_module);
module_exit(exit_timer_module);
