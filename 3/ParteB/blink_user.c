#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char** argv ) {
	char* leds;
	if(argc != 2){
		printf("Error: arg != 2\n");
		exit(1);
    }

    float porcentaje = atof(argv[1]);
    //printf("%f", porcentaje);
    
	FILE* f = fopen("/dev/usb/blinkstick0", "wb");

	if (f == NULL){
	    printf("Error: opening file!\n");
	    exit(1);
	}

    leds = "0:000000,1:000000,2:000000,3:000000,4:000000,5:000000,6:000000,7:000000";
    fwrite(leds,strlen(leds),1,f);

    if(porcentaje > 87.5) {
        leds = "0:000004,1:000011,2:001111,3:001110,4:001100,5:011100,6:111000,7:110000";
    }else if(porcentaje > 75) {
        leds = "0:000004,1:000011,2:001111,3:001110,4:001100,5:011100,6:111000";
    }else if(porcentaje > 62.5) {
        leds = "0:000004,1:000011,2:001111,3:001110,4:001100,5:011100";
    }else if(porcentaje > 50) {
        leds = "0:000004,1:000011,2:001111,3:001110,4:001100";
    }else if(porcentaje > 37.5) {
        leds = "0:000004,1:000011,2:001111,3:001110";
    }else if(porcentaje > 25) {
        leds = "0:000004,1:000011,2:001111";
    }else if(porcentaje > 12.5) {
        leds = "0:000004,1:000011";
    }else if(porcentaje > 0) {
        leds = "0:000004";
    }
	
    fwrite(leds,strlen(leds),1,f);
    fclose(f);      
    return 0;
}
