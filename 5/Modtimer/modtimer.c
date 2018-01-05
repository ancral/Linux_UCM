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
int timer_period_ms = HZ;
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
	
	spin_unlock_irqrestore(&sp, flags);//¿si se ejecuta en otra cpu no hace falta bloquear no?

	tamOcupado = kfifo_len(&cbuffer);
	printk(KERN_INFO "TAMAÑO DEL KFIFO ES: : %i \n", tamOcupado);
	if(tamOcupado >= emergency_threshold){
		//Consulta si un trabajo esta pendiente.
		if(work_pending((struct work_struct *) &my_work)){
			//Si el trabajo esta pendiente espera la finalización de todos los trabajos en workqueue por defecto
            flush_scheduled_work();
        }
        cpuAct = smp_processor_id();

        if(cpuAct % 2 == 0){ // si la cpu actual es par
        	schedule_work_on(1,&my_work);
        }
        else{//impar
			schedule_work_on(0,&my_work);
        }
	}
	mod_timer(&my_timer, jiffies + timer_period_ms);
}

//p4 parte A, permite insertar un elemento en la lista
void insertar_elem_lista(unsigned int elem){
	
	struct list_item_t* nuevoNodo = NULL;

	nuevoNodo = vmalloc(sizeof(struct list_item_t*)); //Reservamos la memoria necesaria

	nuevoNodo->data = elem; //Establecemos el dato
	
	spin_lock(&sp);
	
	list_add_tail(&nuevoNodo->links, &mylist);
	printk(KERN_INFO "Modlist: Dato aniadido %i a la lista\n", elem);

	spin_unlock(&sp);
}

//Se usará cuando haya que vaciar la lista despues de consumir los elems
void cleanUp_list(void) { //Esta funcion tiene que eliminar todos los nodos de la lista
	//p4 parte A
	struct list_item_t* elem = NULL;
	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;

	spin_lock(&sp);
	list_for_each_safe(nodoAct, aux, &mylist){
		elem = list_entry(nodoAct, struct list_item_t, links);
		printk(KERN_INFO "Se ha borrado: %i \n", elem->data);
		list_del(&elem->links);
		vfree(elem); //Liberacion de memoria donde estaba el nodo
	}
	printk(KERN_INFO "Modlist: Lista vaciada \n");
	spin_unlock(&sp);
}

//Pasa los elementos del buffer circular a la lista
/*static void rellenar_lista(                 ){

}*/

//el open para /proc/modtimer
static int modtimer_open(struct inode *nodo, struct file *file){
return 0;
}

//el release para /proc/modtimer
static int modtimer_release(struct inode *nodo, struct file *file){
return 0;
}

//el read para /proc/modtimer, el que permite hacer cat /proc/modtimer
static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
return 0;
}

//el read para cat /proc/modtimerConfig y mostrar las tres variables globales
static ssize_t modconfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
return 0;
}

//el writte para /proc/modtimerCOnfig que permite modificar las tres variables globales
static ssize_t modconfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
return 0;
}

/* La función asociada a la tarea volcará los datos del buffer a la lista
enlazada de enteros:

1º Se extraerán todos los elementos del buffer circular (vacía buffer)
2º Se ha de reservar memoria dinámica para los nodos de la lista vía vmalloc()
3º Si el programa de usuario está bloqueado esperando a que haya ele-mentos en la lista, la función le despertará
*/
static void my_wq_function( struct work_struct *work ){

	int kbuf[MAX_KBUF];
	unsigned long flags;
	int tamOcupado = kfifo_len(&cbuffer);
	int i = 0;
	int elem;

	spin_lock_irqsave(&sp, flags);
	//1º
	for(i=0; i<tamOcupado;i++){
		kfifo_out(&cbuffer,&elem,sizeof(int)); // Insertamos el elem generado en el buffer circular
		kbuf[i] = elem; //meto los elementos primero a este array, porque el sp esta ocupado y no puedo insertar en la lista directamente
	}
	kfifo_reset(&cbuffer);
	spin_unlock_irqrestore(&sp, flags);
	//2º
	for(i=0; i<tamOcupado; i++){
		insertar_elem_lista(kbuf[i]);
	}

	//3º semaforo productor consumidor??

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
    my_timer.expires=jiffies + timer_period_ms;

    add_timer(&my_timer); //Activa el timer la primera vez

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
	del_timer_sync(&my_timer);
}

module_init(init_timer_module);
module_exit(exit_timer_module);