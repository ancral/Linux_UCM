mutex mtx;
condvar prod, cons;
int prod_count=0, cons_count=0;
struct kfifo cbuffer;


int fifoproc_open(bool abre_para_lectura...) { 
	lock(mtx);
	
	if(abre_para_lectura) { //Consumidor
		cons_count++;
		cond_signal(prod);

		while(prod_count==0) cond_wait(cons,mtx);

	}else{			//Productor
		prod_count++;
		cond_signal(cons);

		while(cons_count==0) cond_wait(prod,mtx);
	}

	unlock(mtx);
	return 0;
}

int fifoproc_write(char* buff, int len...) {
	char kbuffer[MAX_KBUF];
	
	if((*off) > 0) return 0;
	if(len > MAX_CBUFFER_LEN || len > MAX_KBUF) return Error;
	if(copy_from_user(kbuffer,buff,len)) return Error;

	lock(mtx);
	
	//Esperar hasta que haya hueco para insertar (debe haber consumidores)
	while(kfifo_avail(&cbuffer) < len && cons_count > 0) cond_wait(prod,mtx);
	
	//Detectar fin de comunicacion por error (consumidor cierra FIFO antes)
	if (cons_count==0){
		unlock(mtx);
		return -EPIPE;
	}
	
	kfifo_in(&cbuffer, kbuffer, len);
	
	//Despertar a posible consumidor bloqueado
	cond_signal(cons);
	
	unlock(mtx);

	return len;
}

int fifoproc_read(const char* buff, int len...) {
	char kbuf[MAX_KBUF];

	if((*off) > 0) return 0;
	if(len > MAX_CBUFFER_LEN || len > MAX_KBUF) return Error;
        
	lock(mtx);
	
	//Esperar hasta que haya elementos que leer (debe haber productores)
	while(kfifo_len(&cbuffer) < len && prod_count > 0) cond_wait(cons,mtx);
	
	//Detectar fin de comunicacion por error y FIFO vacia
	if(prod_count==0 && kfifo_is_empty(&cbuffer)) {
		unlock(mtx);
		return 0;
	}
	
	kfifo_out(&cbuffer, kbuf, len);
	
	//Despertar a posible productor bloqueado
	cond_signal(prod);
	
	unlock(mtx);	

	if(copy_to_user(buf,kbuf,len)) return Error;

	return len;
}

int fifoproc_release(bool lectura...) { 
	lock(mtx);

	if(lectura) { //Consumidor
		cons_count--;
		cond_signal(prod);
	} else { //Productor
		prod_count--;
		cond_signal(cons);
	}
	Vaciar_Buffer;
	unlock(mtx);
	return 0;
}