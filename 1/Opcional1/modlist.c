#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>

static struct proc_dir_entry *proc_entry;

const int MAX_TAM = 256;

struct list_head mylist; //Lista de nodos

#define PARTE_OPCIONAL
#ifdef PARTE_OPCIONAL
	//Nodos de la lista
	typedef struct list_item_t{
		char* cadena;
		struct list_head links; //links contiene prev y next
	}list_item;
#else
	//Nodos de la lista
	typedef struct list_item_t{
		int data;
		struct list_head links; //links contiene prev y next
	}list_item;
#endif

#ifdef PARTE_OPCIONAL
	void add(char* dato){

		struct list_item_t* nuevoNodo = NULL;

		nuevoNodo = vmalloc(sizeof(struct list_item_t*)); //Reservamos la memoria necesaria

		nuevoNodo->cadena = dato; //Establecemos el dato

		list_add_tail(&nuevoNodo->links, &mylist);

		printk(KERN_INFO "Modlist: Dato anyadido a la lista\n");
	}

	void cleanUp_list(void) { //Esta funcion tiene que eliminar todos los nodos de la lista
		struct list_item_t* elem = NULL;
		struct list_head* nodoAct = NULL;
		struct list_head* aux = NULL;

		list_for_each_safe(nodoAct, aux, &mylist){
			elem = list_entry(nodoAct, struct list_item_t, links);
			printk(KERN_INFO "Se ha borrado la lista");
			list_del(&elem->links);
			vfree(elem); //Liberacion de memoria donde estaba el nodo
		}

		printk(KERN_INFO "Modlist: Lista vaciada \n");

	}

	void remove(char* dato){
		struct list_item_t* elem = NULL;
		struct list_head* nodoAct = NULL;
		struct list_head* aux = NULL;
		int encontrado = 0;

		if (list_empty(&mylist) == 0){
			list_for_each_safe(nodoAct, aux, &mylist){
			elem = list_entry(nodoAct, struct list_item_t, links);
			if(strcmp(elem->cadena, dato)) {
					list_del(&elem->links);
					vfree(elem);
					printk(KERN_INFO "Modlist: El array, ha sido borrado de la lista\n");
					encontrado++;
				}
			}
			if (encontrado == 0){
				printk(KERN_INFO "Modlist: El array, no esta en la lista\n");
			}
		}
		else{
			printk(KERN_INFO "Modlist: La lista no contiene elementos por lo que no puede borrar el array \n");
		}

	}

#else
	void add(int dato){

		struct list_item_t* nuevoNodo = NULL;

		nuevoNodo = vmalloc(sizeof(struct list_item_t*)); //Reservamos la memoria necesaria

		nuevoNodo->data = dato; //Establecemos el dato

		list_add_tail(&nuevoNodo->links, &mylist);

		printk(KERN_INFO "Modlist: Dato anyadido %i a la lista\n", dato);
	}

	void cleanUp_list(void) { //Esta funcion tiene que eliminar todos los nodos de la lista
		struct list_item_t* elem = NULL;
		struct list_head* nodoAct = NULL;
		struct list_head* aux = NULL;

		list_for_each_safe(nodoAct, aux, &mylist){
			elem = list_entry(nodoAct, struct list_item_t, links);
			printk(KERN_INFO "Se ha borrado: %i \n", elem->data);
			list_del(&elem->links);
			vfree(elem); //Liberacion de memoria donde estaba el nodo
		}

		printk(KERN_INFO "Modlist: Lista vaciada \n");

	}

	void remove(int dato){
		struct list_item_t* elem = NULL;
		struct list_head* nodoAct = NULL;
		struct list_head* aux = NULL;
		int encontrado = 0;

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
#endif

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
	char kbuf[MAX_TAM];
	#ifdef PARTE_OPCIONAL
		char* cadena = NULL;
	#else
		int dato;
	#endif
	char cad[7];

	if ((*off)>0) return 0;

	//Comprobación de que no pase el espacio, (MAX_TAM-1) porque hay que añadir el \0
	if (len > MAX_TAM-1){
		printk(KERN_INFO "Modlist: No hay espacio\n");
		return -ENOSPC;
	}

	//Pasa a las paginas del userspace a las del kernel, len bytes de buf a kbuf
	if (copy_from_user(kbuf, buf, len)) return -EFAULT;

	kbuf[len] = '\0'; //NULL

	//Parseo
	#ifdef PARTE_OPCIONAL
		if (sscanf(kbuf, "add %c", cadena) == 1) add(cadena); //Add
		else if (sscanf(kbuf, "remove %c", cadena) == 1) remove(cadena); //Remove
		else if (sscanf(kbuf, " %s cleanup", cadena) == 1){ //Cleanup
		int res = strncmp(cad, "cleanup", 6);
		if (res == 0) cleanUp_list();
		else printk(KERN_INFO "Modlist: Comando incorrecto\n");

	}
	#else
		if (sscanf(kbuf, "add %i", &dato) == 1) add(dato); //Add
		else if (sscanf(kbuf, "remove %i", &dato) == 1) remove(dato); //Remove
		else if (sscanf(kbuf, " %s cleanup", dato) == 1){ //Cleanup
		int res = strncmp(cad, "cleanup", 6);
		if (res == 0) cleanUp_list();
		else printk(KERN_INFO "Modlist: Comando incorrecto\n");

	}
	#endif

	
	else return -EINVAL;

	*off += len; //Actualiza el puntero

	return len;
}


//El read es el que hace el cat, por lo que no hay que tener una funicon cat
static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off){ //Parte del cat

	int ret;
	char kbuf[MAX_TAM];
	char *dest = kbuf; //Se establece en el principio con kbuf

	struct list_head* nodoAct = NULL;
	struct list_head* aux = NULL;
	struct list_item_t* elem = NULL;

	list_for_each_safe(nodoAct, aux, &mylist){
		elem = list_entry(nodoAct, struct list_item_t, links);
		#ifdef PARTE_OPCIONAL
			dest += sprintf(dest,"%s \n", elem->cadena); //Usamos dest como puntero para que vaya moviendo
		#else
			dest += sprintf(dest,"%i \n", elem->data); //Usamos dest como puntero para que vaya moviendo
		#endif

	}

	kbuf[len] = '\0'; //NULL

	if((*off) > 0) ret = 0;
	else {
		//Pasa a las paginas del kernel a las del userspace, len bytes de kbuf a buf
		if (copy_to_user(buf, kbuf, len)) return -EINVAL;
		ret = (dest-kbuf);
	}

	(*off) += len; //Actualiza el puntero

	return ret; //Aritmetica de punteros, restamos los punteros (el que se ha movido y el estatico)
				//para devolver cuantos se han leido
}

static const struct file_operations proc_entry_fops = {
	.read = modlist_read,
	.write = modlist_write,
};

/*Probado, no tocar*/
int init_modlist_module(void) {
	int ret = 0;

	#ifdef PARTE_OPCIONAL
		INIT_LIST_HEAD(&mylist);

		proc_entry = proc_create("modlist", 0666, NULL, &proc_entry_fops); //Inicialización con permisos al modlist

		if (proc_entry == NULL){
			ret = -ENOMEM;
			printk(KERN_ALERT "Modlist: No se puede crear la entrada\n");
		}
		else {
			printk(KERN_INFO "Modlist: Modulo cargado\n");
		}

	#else
		INIT_LIST_HEAD(&mylist);

		proc_entry = proc_create("modlist", 0666, NULL, &proc_entry_fops); //Inicialización con permisos al modlist

		if (proc_entry == NULL){
			ret = -ENOMEM;
			printk(KERN_ALERT "Modlist: No se puede crear la entrada\n");
		}
		else {
			printk(KERN_INFO "Modlist: Modulo cargado\n");
		}

	#endif

	return ret;
}

void exit_modlist_module(void) {
	remove_proc_entry("modlist", NULL);
	cleanUp_list(); //Limpia la lista
	printk(KERN_INFO "Modlist: Modulo descargado.\n");
}



module_init(init_modlist_module);
module_exit(exit_modlist_module);
