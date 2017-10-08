#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm-generic/uaccess.h>

static struct proc_dir_entry *proc_entry;
struct list_head mylist;
//const int MAX = 150;

//Son los nodos de la lista
struct list_item_t{
	int data;
	struct list_head links; //Tiene dentro el *prev y *next
};

void add(int dato){ //Esta funcion hace el eliminar elementos en el archivo

	struct list_item_t* it = NULL; // item representa un nodo de la lista
	struct list_head* ptr = NULL; //puntero para obtener la entrada
	struct list_item_t* first = list_entry(ptr, struct list_item_t, links);

	it = vmalloc(sizeof(struct list_item_t*)); // reservo memoria

	it->data = dato; // asigno el dato

	//it->links.next = ptr; 
	//it->links.prev = ptr->prev;
	list_add(ptr, &mylist);


}
static void cat_list(void){ //Esta funcion imprime por pantalla los elementos
	struct list_item_t* item = NULL;
	struct list_head* cur_node = NULL;

	list_for_each(cur_node, &mylist) {
		/* item points to the structure wherein the links are embedded */
		item = list_entry(cur_node, struct list_item_t, links);
		printk(KERN_INFO "%i\n", item->data);
	}
}

static ssize_t modlist_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){ //Parte de add y remove
	char *kbuf;
	int num;

	if ((*off)>0){
		return 0;
	};

	if (copy_from_user(&kbuf, buf, len) < 0){
		return -EFAULT;
	}

	kbuf[len] = '\0';

	*off += len;

	//Parseo de add
	if (sscanf(kbuf, "add %i", &num) == 1){
		add(num);
	}

	else if (sscanf(kbuf, "cat") == 1){
		printk(KERN_INFO "Estoy dentrooooooo");
		cat_list();
	}
	//Parseo de remove
	/*else if(sscanf(kbuf,"remove %i", &num) == 1) {
	remove();
	}

	//Parseo de cat

	//Parseo de cleanup
	else if(sscanf(kbuf,"cleanup")==1){
	cleanup_list();
	} */
	//Mal parseo
	else return -EINVAL;


	return len;
}

/*static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off){ //Parte del cat

}*/

static const struct file_operations proc_entry_fops = {
	//.read = modlist_read,
	.write = modlist_write,

};

/*


void remove( void ){ //Esta funcion hace el eliminar elementos en el archivo

}
*/


/*
void cleanup_list( void ) { //Esta funcion tiene que eliminar todos los nodos de la lista

}*/

int init_modlist_module(void) {
	int ret = 0;
	INIT_LIST_HEAD(&mylist);

	proc_entry = proc_create("modlist", 0666, NULL, &proc_entry_fops);

	if (proc_entry == NULL){
		ret = -ENOMEM;
		printk(KERN_INFO "Modlist: Can't create /proc entry\n");
	}
	else {
		printk(KERN_INFO "Modlist: Module loaded\n");
	}
	return ret;
}

void exit_modlist_module(void) {
	remove_proc_entry("modlist", NULL);
	printk(KERN_INFO "Modlist: Module unloaded.\n");
}



module_init(init_modlist_module);
module_exit(exit_modlist_module);

