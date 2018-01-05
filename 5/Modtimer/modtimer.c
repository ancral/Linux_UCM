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
static spinlock_t sp;

//Lista enlazada
struct list_head mylist; //Lista de nodos

//Nodos de la lista
typedef struct list_item_t{
	int data;
	struct list_head links; //links contiene prev y next
}list_item;




/*se va a encargar de generar el nr aleatorio para introducirdo
que sea va a introducir en la lista*/
static void generar_nr_aleatorio(unsigned long data){
	
	unsigned int nr = get_random_int();
	unsigned int nrAleat = (nr % max_random);
	unsigned long flags;
	int tamOcupado = 0;

	//Protegemos
	spin_lock_irqsave(&sp,flags);

	kfifo_in(&cbuffer,&nrAleat,sizeof(int)); // Insertamos el elem generado en el buffer circular
	tamOcupado = kfifo_size(&cbuffer);
	printk(KERN_INFO "TAMAÑO DEL KFIFO ES: : %i \n", tamOcupado);

	spin_unlock_irqrestore(&sp, flags);

	printk(KERN_INFO "Se ha generado el número: %d \n", nrAleat);
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

/* Workqueue*/
static void my_wq_function( struct work_struct *work ){
	
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

}

module_init(init_timer_module);
module_exit(exit_timer_module);