#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <asm/smp.h>


//Variables globales
int timer_period_ms = HZ;
int max_random = 250;
int emergency_threshold = 60;

//Las dos entradas de /prog que se piden. cfg para cambiar los params y la otra para consumir nr
static struct proc_dir_entry *proc_entry_cfg, *proc_entry_timer;

//buffer circular
static struct kfifo cbuffer;

//OPCION 1, como en workqueue3, que sería para una cola de trabajo propia que en la practica no se pide
/* Wrapper for the work_struct structure,
  that makes it possible to pass parameters more easily
  to the work's handler function */
struct my_work_t {
	struct work_struct my_work;
	struct kfifo cbuffer;
};

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


//Lista enlazada
struct list_head mylist; //Lista de nodos

//Nodos de la lista
typedef struct list_item_t{
	int data;
	struct list_head links; //links contiene prev y next
}list_item;




/*se va a encargar de generar el nr aleatorio para introducirdo
que sea va a introducir en la lista*/
int generar_nr_aleatorio(){
return 0;
}

//p4 parte A, permite insertar un elemento en la lista
void insertar_elem_lista(unsigned int elem){
	
	struct list_item_t* nuevoNodo = NULL;

	nuevoNodo = vmalloc(sizeof(struct list_item_t*)); //Reservamos la memoria necesaria

	nuevoNodo->data = dato; //Establecemos el dato
	
	spin_lock(&sp);
	
	list_add_tail(&nuevoNodo->links, &mylist);
	printk(KERN_INFO "Modlist: Dato aniadido %i a la lista\n", dato);

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
static void rellenar_lista( /* le llega el buffer*/){

}

//el open para /proc/modtimer
static int modtimer_open(struct inode *nodo, struct file *file){

}

//el release para /proc/modtimer
static int modtimer_release(struct inode *nodo, struct file *file){

}

//el read para /proc/modtimer, el que permite hacer cat /proc/modtimer
static ssize_t modtimer_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {

}

//el read para cat /proc/modtimerConfig y mostrar las tres variables globales
static ssize_t modtimerConfig_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {

}

//el writte para /proc/modtimerCOnfig que permite modificar las tres variables globales
static ssize_t modtimerConfig_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {

}

static struct file_operations proc_entry_fops_cfg = {
    .read = modtimerConfig_read,
    .write = modtimerConfig_write,
};

static const struct file_operations proc_entry_fops_timer= {
    .read = modtimer_read,
    .open = modtimer_open,
    .release = modtimer_release,
};


int init_module( void ){
	int ret = 0;

	return ret;
}

void exit_module( void ){

}

module_init(init_module);
module_exit(exit_module);