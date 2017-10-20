#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/syscall.h>

#define BUFSIZE 512

#ifndef SYS_read 
#define SYS_read          0 
#endif


int main(void)
{
    int read_chars=0;
    int fd=0;
    char buf[BUFSIZE+1];
    
    /* Open /proc entry in read-only mode */
	
	// fd = 0 ==> entrada estandar; fd = 1 ==> salida estandar; fd = 2 ==> salida de error estandar
	//ssize_t sys_write(unsigned int fd, const char * buf, size_t count)
	
	// fd=open("/proc/cpuinfo",O_RDONLY);
	fd=syscall(SYS_open,"/proc/cpuinfo",4); // Abro el archivo en solo 
	
    if (fd<0){
       // fprintf(stderr,"Can't open the file\n");
		syscall(SYS_write,1,"Can't open the file\n",20);
        exit(1);
    }
    
    /* Loop that reads data from the file and prints its contents to stdout */
    while((read_chars=syscall(SYS_read,fd,buf,BUFSIZE))>0){
        buf[read_chars]='\0';
        //printf("%s",buf);   
        syscall(SYS_write,1,buf,strlen(buf));  
    }
     
    if (read_chars<0){
        //fprintf(stderr,"Error while reading the file\n");
        syscall(SYS_write,2,"Error while reading the file\n",strlen(buf));
        exit(1);        
    }
    	
    /* Close the file and exit */ 
    syscall(SYS_close);
   // close(fd);
    return 0;
}











