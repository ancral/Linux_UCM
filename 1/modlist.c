#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/list.h>

static struct proc_dir_entry *proc_entry;
struct list_head mylist;
//const int MAX = 150;

//Son los nodos de la lista
typedef struct{
	int data;
	struct list_head links; //Tiene dentro el *prev y *next 
}list_item_t;

static const struct file_operations proc_entry_fops = {
    //.read = modlist_read,
    .write = modlist_write,    
};

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){ //Parte de add y remove
	char kbuf[MAX];
	int num;

	if((*off)>0) return 0;

	if(copy_from_user(&kbuf[0], buf, len)) return -EFAULT; 

	kbuf[len] = '\0';

	*off += len;

	//Parseo de add
	if(sscanf(&kbuf,"add %i", &num) == 1) add(num) 
	//Parseo de remove
	//else if(sscanf(&kbuf,"remove %i", &num) == 1) remove 
	//Parseo de cat
	//else if(sscanf(&kbuf,"cat")==1) cat 
	//Parseo de cleanup
	//else if(sscanf(&kbuf,"cleanup")==1) cleanup 
	//Mal parseo
	else return -EINVAL;


	return len;
}

/*
static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off){ //Parte del cat
	char kbuf[MAX];
	
	//Dice a la aplicacion que no hay nada más que leer
	if ((*off)>0) return 0;
	
	//Comprueba que la longitud de los bytes no sea mayor a len
	if (len < ???) return -ENOSPC;
	
	//Transfiere datos del kernel al userspace
	if(copy_to_user(buf, ???, ???)) return -EINVAL;

	(*off)+=len;

	return ???	
}*/

int init_modlist_module( void ) {
	int ret = 0;
    	INIT_LIST_HEAD(&mylist);
	proc_entry = proc_create( "modlist", 0666, NULL, &proc_entry_fops);

	if (proc_entry==NULL)
	{
		ret = -ENOMEM;
      	printk(KERN_INFO "Modlist: Can't create /proc entry\n");
	} else {
		printk(KERN_INFO "Modlist: Module loaded\n");
	}

	return ret;
}

void cat_list( void ){ //Esta funcion imprime por pantalla los elementos
	struct list_item_t* item = NULL;
	struct list_head* cur_node = NULL;

	list_for_each(cur_node, &mylist) {
		item = list_entry(cur_node, struct list_item_t, links);
		printf("%i\n", item->data);
	}
}

/*
void remove( int dato ){ //Esta funcion hace el eliminar elementos en el archivo
	struct list_item_t* item = NULL; 
	struct list_head* cur_node = NULL; // Puntero para recorrer la lista
	struct list_item_t* anterior = NULL; // Puntero anterior a item
	struct list_item_t* siguiente = NULL; // Puntero siguiente a item

	list_for_each(cur_node, &mylist) {
		item = list_entry(cur_node, struct list_item_t, links);
		if(item->data == dato){
			anterior = item->links.prev
			siguiente = item->links.next
			
			 //Asignamos los punteros

			anterior->links.next = siguiente
			siguiente->links.prev = anterior
			list_del(item); // ¿Hace la recolocación de nodos?
			// Si hace la recolocacion, quitar la asignación de nodos
			// de arriba.
			vfree(item); // Liberamos memoria
		}
	}
}*/

void add( int dato ) { //Esta funcion hace el añadir elementos en el archivo
	struct list_item_t* it = NULL; // Representa un nodo de la lista
	struct list_head* ptr = NULL; // Puntero para obtener la entrada
	struct list_item_t* first = list_entry(ptr, struct list_item_t, links);

	it = vmalloc(sizeof(struct list_item_t*)); // Reserva de memoria

	it->data = dato; // Asignación del dato

	list_add_tail(it, &mylist); // Se añade el puntero it al final de la lista
}

/*
void cleanup_list( void ) { //Esta funcion tiene que eliminar todos los nodos de la lista
	struct list_item_t* item = NULL;
	struct list_head* cur_node = NULL;

	list_for_each(cur_node, &mylist){
		item = list_entry(cur_node, struct list_item_t, links);
		list_del(item); // Eliminamos el nodo
		vfree(item); // Y liberamos memoria
	}
}*/

void exit_modlist_module( void ) {
	remove_proc_entry("modlist", NULL);
	printk(KERN_INFO "Modlist: Module unloaded.\n");
}

module_init( init_modlist_module );
module_exit( exit_modlist_module );

