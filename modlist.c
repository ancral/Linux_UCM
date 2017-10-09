#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>

static struct proc_dir_entry *proc_entry;

const int MAX_TAM = 256;

struct list_head mylist;

const int BUF_LEN = 150;
const int MAX_ITEMS = 150;

//Son los nodos de la lista
struct list_item_t{
	int data;
	struct list_head links; //Tiene dentro el *prev y *next
};

/*Probado, no tocar*/
void add(int dato){ //Esta funcion hace el eliminar elementos en el archivo

	struct list_item_t* nuevoNodo = NULL; // va a ser el nodo a añadir a la lista 

	nuevoNodo = vmalloc(sizeof(struct list_item_t*)); // reservamos memoria para el nuevo elemento de la lista

	nuevoNodo->data = dato; // metemos el dato correspondiente al nuevo nodo

	list_add(&nuevoNodo->links, &mylist); // llamamos a la funcion de añadir que solamente necesita que el dato nuevo este en la estructura porque asigna sola los punteros (segun list.h si)

	printk(KERN_INFO "Modlist: Dato aniadido %i a la lista\n", dato);

}
/*Probado, no tocar*/
void remove(int dato){
	struct list_item_t* elem = NULL;
	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;
	int encontrado = 0;
	/*
	list_for_each_safe - iterate over a list safe against removal of list entry
	@pos:	the &struct list_head to use as a loop cursor.
	@n:		another &struct list_head to use as temporary storage
	@head:	the head for your list.
	#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
	pos = n, n = pos->next)
	*/
	//recorrido de la lista
	if (list_empty(&mylist) == 0){
		list_for_each_safe(nodoAct, aux, &mylist){
			elem = list_entry(nodoAct, struct list_item_t, links);
			if (elem->data == dato){
				list_del(&elem->links);
				vfree(elem);
				printk(KERN_INFO "Modlist: El elemento %i, ha sido borrado de la lista\n", dato);
				encontrado++;
			}
		}
		if (encontrado == 0){
			printk(KERN_INFO "Modlist: El elemento %i, no esta en la lista\n", dato);
		}
	}
	else{
		printk(KERN_INFO "Modlist: La lista no contiene elementos por lo que no puede borrar el elemento %i\n", dato);
	}

}

/*Probado, no tocar*/
void cleanUp_list(void) { //Esta funcion tiene que eliminar todos los nodos de la lista
	struct list_item_t* elem = NULL;
	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;
	if (list_empty(&mylist) == 0){//por las pruebas que he hecho, si list empty devuelve 0 es que hay elementos en la lista
		//preguntar si list_for_each_safe o list_for_each vale
		list_for_each_safe(nodoAct, aux, &mylist){
			/* list_entry te devuelve un elemento de la lista, recibe como parametro
			el puntero al list_head, el tipo de estructura de la lista, y el nombre de la estructura de dentro de la lista*/
			elem = list_entry(nodoAct, struct list_item_t, links);
			list_del(&elem->links); // elimina el nodo de la lista y recoloca los punteros (segun list.h)
			vfree(elem);//elibera memoria
		}
	}
	printk(KERN_INFO "Modlist: Lista vaciada\n");

}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
	char kbuf[MAX_TAM];
	int num;
	char cadena[7];

	if ((*off)>0){
		return 0;
	}

	if (len > MAX_TAM){
		printk(KERN_INFO "Modlist: No hay espacio\n");
		return -ENOSPC;
	}
	//pasa los datos desde el usuario al kernel
	if (copy_from_user(&kbuf[0], buf, len)){
		return -EFAULT;
	}

	kbuf[len] = '\0'; // para que no pete

	*off += len; // actualiza el puntero


	//parseamos los comandos que nos llegan y llamamos a las funciones necesarias
	if (sscanf(&kbuf[0], "add %i", &num) == 1){ // corresponde al add, echo add 10 > /proc/modlist
		add(num);
	}
	else if (sscanf(&kbuf[0], "remove %i", &num) == 1){ // corresponde al remove, echo remove 10 > /proc/modlist
		remove(num);
	}
	else if (sscanf(&kbuf[0], " %s cleanup", cadena) == 1){
		int res = strncmp(cadena, "cleanup", 6);
		if (res == 0){
			cleanUp_list();
		}
		else{
			printk(KERN_INFO "Modlist: Comando incorrecto\n");
		}
	}
	else{
		return -EINVAL;
	}
	return len;
}

//EL read es el que hace el cat, por lo que no hay que tener una funicon cat
static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off){ //Parte del cat








	return len;
}

static const struct file_operations proc_entry_fops = {
	.read = modlist_read,
	.write = modlist_write,
};

/*Probado, no tocar*/
int init_modlist_module(void) {
	int ret = 0;
	INIT_LIST_HEAD(&mylist);

	proc_entry = proc_create("modlist", 0666, NULL, &proc_entry_fops);

	if (proc_entry == NULL){
		ret = -ENOMEM;
		printk(KERN_ALERT "Modlist: No se puede crear la entrada\n");
	}
	else {
		printk(KERN_INFO "Modlist: Modulo cargado\n");
	}
	return ret;
}

/*Probado, no tocar*/
void exit_modlist_module(void) {
	remove_proc_entry("modlist", NULL);
	cleanUp_list(); // vacia la lista
	printk(KERN_INFO "Modlist: Modulo descargado.\n");
}



module_init(init_modlist_module);
module_exit(exit_modlist_module);
