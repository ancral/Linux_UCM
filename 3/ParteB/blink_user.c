#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char** argv ) {
	char* leds;
	char buff[10];
	FILE *inp; 
	FILE *f;
    
	f = fopen("/dev/usb/blinkstick0", "wb");

		if (f == NULL){
			printf("Error: opening file!\n");
			exit(1);
		}
	
	while(1) {
		
		if(!(inp = popen("free | grep Mem | awk '{print $3/$2 * 100.0}'", "r"))) exit(1);
		fgets(buff, sizeof(buff), inp);

		float porcentaje = atof(buff);
		sleep(1);
		printf("%f \n", porcentaje);
		
		
		leds = "0:000000,1:000000,2:000000,3:000000,4:000000,5:000000,6:000000,7:000000";
		fwrite(leds,strlen(leds),1,f);
		
		if(porcentaje > 87.5) {
			leds = "0:000011,1:000111,2:001111,3:001110,4:001100,5:011100,6:111000,7:110000";

		}else if(porcentaje > 75) {
			leds = "0:000011,1:000111,2:001111,3:001110,4:001100,5:011100,6:111000";

		}else if(porcentaje > 62.5) {
			leds = "0:000011,1:000111,2:001111,3:001110,4:001100,5:011100";
			
		}else if(porcentaje > 50) {
			leds = "0:000011,1:000111,2:001111,3:001110,4:001100";

		}else if(porcentaje > 37.5) {
			leds = "0:000011,1:000111,2:001111,3:001110";

		}else if(porcentaje > 25) {
			leds = "0:000011,1:000111,2:001111";

		}else if(porcentaje > 12.5) {
			leds = "0:000011,1:000111";

		}else if(porcentaje > 0) {
			leds = "0:000011";

		}
		
		fwrite(leds,strlen(leds),1,f);
		pclose(inp);
		fclose(f);
		
		f = fopen("/dev/usb/blinkstick0", "wb");
    }  
    
    fclose(f);
    return 0;
}
