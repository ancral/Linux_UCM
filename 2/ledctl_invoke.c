#include <linux/errno.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __i386__
#define __NR_LEDCTL	353
#else
#define __NR_LEDCTL	316
#endif



long int ledctl(int mask){
  return (long) syscall(__NR_LEDCTL,mask,mask);
}

int main(int argc, char *argv[]){
    if(argc != 2){
		printf("Error, tienes que llamar al programa de la forma: sudo ./ledctl_invoke 0xNUM, donde NUM, es un nr entre 0 y 7!!\n");
		return -1;
    }
			//le llega el str, luego un puntero, y luego la base
    long int mask = strtol(argv[1], NULL, 16); // convierte a long int
		if(ledctl(mask) < 0){
			perror("Ledctl error: ");
			return -1;
		}

    return 0;
}
