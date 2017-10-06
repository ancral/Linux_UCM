#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/list.h>

struct list_head mylist;
const int MAX = 150;

//Son los nodos de la lista
typedef struct
{
	int data;
	struct list_head links; //Tiene dentro el *prev y *next
}list_item_t;

static const struct file_operations proc_entry_fops = {
    .read = modlist_read,
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
	if(sscanf(&kbuf,"add %i", &num) == 1) add 
	//Parseo de remove
	else if(sscanf(&kbuf,"remove %i", &num) == 1) remove 
	//Parseo de cat
	else if(sscanf(&kbuf,"cat")==1) cat 
	//Parseo de cleanup
	else if(sscanf(&kbuf,"cleanup")==1) cleanup 
	//Mal parseo
	else return -EINVAL;


	return len;
}
static ssize_t modlist_read(struct file *filp, char __user *buf, size_t len, loff_t *off){ //Parte del cat

}

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

}

void remove( void ){ //Esta funcion hace el eliminar elementos en el archivo

}

void add( int dato ) { //Esta funcion hace el añadir elementos en el archivo

}

void cleanup_list( void ) { //Esta funcion tiene que eliminar todos los nodos de la lista

}

void exit_modlist_module( void ) {
	remove_proc_entry("modlist", NULL);
	printk(KERN_INFO "Modlist: Module unloaded.\n");
}





module_init( init_modlist_module );
module_exit( exit_modlist_module );

