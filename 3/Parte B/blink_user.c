#include <stdio.h>
#include <stdlib.h> // For exit() function
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>


const int MAX_TAM_COLORES=1026*300;
const int MAX_TAM_LEDS = 255*3;
const int TAM_MAX_FRASE=256;

typedef struct {
    char letra;
    char codigo[4];
}tCodificacion;

void codificacionMorse(tCodificacion* codMorse);
void codificar(tCodificacion* codMorse, unsigned int* datos, unsigned int posCodigo,unsigned int tam, char opcion);//lineas
void modificarColor(char* datos, char* color,int inicio, int fin);
void leds_locos();
void codigo_morse();
int abrirFichero();
void ram();
void salir();
int fin = 0;
void prepararEjecucion();

int main(){

    printf("%s\n","===============================================================================");
    printf("%s\n","===============================================================================");
    printf("%s\n","Practica 3, Andrei Ionut Vaduva y Angel Cruz Alonso");
    printf("%s\n","===============================================================================");
    printf("%s\n","El blinkstick se configura automaticamente, no hace falta hacer nada!" );
    printf("%s\n","===============================================================================");

    prepararEjecucion();

    printf("%s\n","===============================================================================");
    printf("%s\n","1.-Version leds locos");
    printf("%s\n","2.-Version codigo Morse");
    printf("%s\n","3.-Version lector RAM (interrumpir con CTRL+C)");
    printf("%s\n","0.-Salir");
    printf("%s\n","===============================================================================");

    int opcion=0;
    printf("%s","Escoge una opcion: ");
    scanf("%i",&opcion);
    int fdBlinkstick = 0;
    char* apagar = "0:0x000000,1:0x000000,2:0x000000,3:0x000000,4:0x000000,5:0x000000,6:0x000000,7:0x000000";

    while(opcion !=0){

        if(opcion == 1){
            leds_locos();
        }
        else if(opcion == 2){
            codigo_morse();
        }
        else if (opcion == 3){
            ram();
        }
        else{
            printf("%s\n","===============================================================================");
            printf("%s\n","Opcion incorrecta!!");
        }
        printf("%s\n","===============================================================================");
        printf("%s\n","1.-Version leds locos");
        printf("%s\n","2.-Version codigo Morse");
        printf("%s\n","3.-Version lector RAM (interrumpir con CTRL+C)");
        printf("%s\n","0.-Salir");
        printf("%s\n","===============================================================================");
        printf("%s","Escoge una opcion: ");
        scanf("%i",&opcion);
    }
    printf("%s\n","===============================================================================");
    printf("%s\n","Hasta luego!");
    printf("%s\n","===============================================================================");

    fdBlinkstick = abrirFichero();
    if (write(fdBlinkstick,apagar, strlen(apagar)) != strlen(apagar)) {
        perror("Error al escribir el comando en el archivo ");
        exit(EXIT_FAILURE);
    }
    close(fdBlinkstick);


    return 0;
}


void prepararEjecucion(){
    system("chmod +x auto.sh");
    system("./auto.sh");
}


void leds_locos(){

    int fdColores;
    int fdLeds;
    int fdBlinkstick=0;
    char colores[MAX_TAM_COLORES];
    char leds[MAX_TAM_LEDS];
    char* apagar = "0:0x000000,1:0x000000,2:0x000000,3:0x000000,4:0x000000,5:0x000000,6:0x000000,7:0x000000";

    fdColores = open("colores.txt",O_RDONLY); // read mode
    fdLeds = open("leds.txt",O_RDONLY); // read mode
    //fdBlinkstick = open("/dev/usb/blinkstick0", O_WRONLY);

    if( fdColores < 0 || fdLeds < 0 || fdBlinkstick < 0){
        perror("Error al abrir el archivo ");
        exit(EXIT_FAILURE);
    }
    if (read(fdColores,colores,MAX_TAM_COLORES-1) < 0){
        perror("Error al abrir el fichero de colores ");
        exit(EXIT_FAILURE);
    }
    if(read(fdLeds,leds,MAX_TAM_LEDS-1) < 0){
        perror("Error al abrir el fichero de leds ");
        exit(EXIT_FAILURE);
    }

    close(fdColores);
    close(fdLeds);

    leds[MAX_TAM_LEDS] = '\0';
    colores[MAX_TAM_COLORES] = '\0';

    int k = 0;
    int w = 0;
    printf("%s\n","Procesando... ");

    while(k<MAX_TAM_COLORES-1 && w < MAX_TAM_LEDS-1){
        int i;
        char comando[10]=" :0x      ";
        comando[0]=leds[w];
        for(i=4;i<10;i++){//coje 6 caracteres
            comando[i]=colores[k];
            k++;
        }
       // lseek(fdBlinkstick,0,SEEK_SET);
        fdBlinkstick = abrirFichero();
        if (write(fdBlinkstick, comando, 10) != 10 ) {
            perror("Error al escribir el comando en el archivo ");
            exit(EXIT_FAILURE);
        }
        close(fdBlinkstick);
        w++;
    }
    fdBlinkstick = abrirFichero();
    //lseek(fdBlinkstick,0,SEEK_SET);
    if (write(fdBlinkstick,apagar, strlen(apagar)) != strlen(apagar)) {
        perror("Error al escribir el comando en el archivo ");
        exit(EXIT_FAILURE);
    }
    close(fdBlinkstick);
}

void codigo_morse(){
    tCodificacion codMorse[300];

    char punto[50] = "0:0x110011";
    char linea[100] = "0:0x110011,1:0x110011,2:0x110011,3:0x110011,4:0x110011,5:0x110011,6:0x110011,7:0x110011";
    char* apagar = "0:0x000000,1:0x000000,2:0x000000,3:0x000000,4:0x000000,5:0x000000,6:0x000000,7:0x000000";
    char frase[TAM_MAX_FRASE];
    int fdBlinkstick=0;
    int i,j,k;
    int encontrado = 0;
    char cod;
    int desp = 'a'-'A';


    getchar();
    printf("%s","Introduce una frase: ");
    fgets (frase, TAM_MAX_FRASE-1, stdin);
    frase[TAM_MAX_FRASE]='\0';
    char color[8];
    printf("%s","Cambiar color, por defecto, 0x110011 (Opcional, enter para saltar): ");


    if(strcmp(fgets (color, 9, stdin),"\n") != 0){
        color[8]='\0';
        modificarColor(punto,color,2,10);
        modificarColor(linea,color,2,10);//0
        modificarColor(linea,color,13,21);//1
        modificarColor(linea,color,24,32);//2
        modificarColor(linea,color,35,43);//3
        modificarColor(linea,color,46,54);//4
        modificarColor(linea,color,57,65);//5
        modificarColor(linea,color,68,76);//6
        modificarColor(linea,color,79,87);//7
    }

    codificacionMorse(codMorse);
    /*Dado que lseek no funciona bien con el tipo de archio del dispositivo he tenido que abrir y cerrar el fichero cada vez que escribia*/
    
    printf("%s\n","Traduciendo..." );
    for(i=0;i<strlen(frase);i++){//recorro la frase
        j=0;
        while(j<26 && encontrado==0){
            if(frase[i] == codMorse[j].letra || (frase[i]-desp) == codMorse[j].letra){//si encuentro la letra, tengo su correspondiente codigo. Ignora mayusculas y min
                k=0;
                cod = codMorse[j].codigo[k];
                while(cod !='x' && k<4){
                    fdBlinkstick = abrirFichero();
                    //lseek(fdBlinkstick,0,SEEK_SET);
                    if(cod == '.'){
                        if (write(fdBlinkstick, punto, 10) != 10 ) {
                            perror("Error al escribir el comando punto en el archivo ");
                            exit(EXIT_FAILURE);
                        }
                        close(fdBlinkstick);
                    }
                    else{
                        if (write(fdBlinkstick, linea, 87) != 87 ) {
                            perror("Error al escribir el comando linea en el archivo ");
                            exit(EXIT_FAILURE);
                        }
                        close(fdBlinkstick);
                    }
                    sleep(1);
                    //lseek(fdBlinkstick,0,SEEK_SET);
                    fdBlinkstick = abrirFichero();
                    if (write(fdBlinkstick, apagar, strlen(apagar)) != strlen(apagar) ) {
                        perror("Error al escribir el comando apagar en el archivo ");
                        exit(EXIT_FAILURE);
                    }
                    close(fdBlinkstick);
                    sleep(1);
                    k++;
                    cod = codMorse[j].codigo[k];
                }
                encontrado=1;
            }
            j++;
        }
        encontrado=0;
    }
}

int abrirFichero(){
    int fdBlinkstick = open("/dev/usb/blinkstick0", O_WRONLY);

    if(fdBlinkstick < 0){
        close(fdBlinkstick);
        perror("Error al abrir el archivo ");
        exit(EXIT_FAILURE);
    }
    return fdBlinkstick;
}


void modificarColor(char* datos, char* color,int inicio, int fin){

    int i=0, h;
    for(h=inicio;h<fin;h++){
        datos[h]=color[i];
        i++;
    }

}

void codificacionMorse(tCodificacion* codMorse){

    char dato = 'A';
    char fin = 'Z';
    int i = 0;
    int j;
    //mete el abecedario en letra
    while(dato<=fin){
        codMorse[i].letra=dato;
        for(j=0;j<4;j++){
            codMorse[i].codigo[j] = 'x';
        }
        i++;
        dato++;
    }

    //primer digito
    unsigned int lineaP[] = {1,2,3,6,10,12,13,14,16,19,23,24,25};
    codificar(codMorse,lineaP,0,13,'-');//Ejemplo para codificar en las letras 1,2,3,6,10 en la primera casilla del codigo una linea
    unsigned int puntoP[] = {0,4,5,7,8,9,11,15,17,18,20,21,22};
    codificar(codMorse,puntoP,0,13,'.');
    //segundo digito
    unsigned int lineaS[] = {0,6,9,11,12,14,15,16,17,22,25};
    codificar(codMorse,lineaS,1,11,'-');
    unsigned int puntoS[] = {1,2,3,5,7,8,10,13,18,20,21,23,24};
    codificar(codMorse,puntoS,1,13,'.');
    //tercer digito
    unsigned int lineaT[] = {2,5,9,10,14,15,20,22,24};
    codificar(codMorse,lineaT,2,9,'-');
    unsigned int puntoT[] = {1,3,6,7,11,16,17,18,21,23,25};
    codificar(codMorse,puntoT,2,11,'.');
    //cuarto digito
    unsigned int lineaC[] = {9,16,21,23,24};
    codificar(codMorse,lineaC,3,5,'-');
    unsigned int puntoC[] = {1,2,5,7,11,15,25};
    codificar(codMorse,puntoC,3,7,'.');


}

void codificar(tCodificacion* codMorse, unsigned int* datos, unsigned int posCodigo,unsigned int tam, char opcion){
    int i=0;
    while(i<tam){
        codMorse[datos[i]].codigo[posCodigo]=opcion;//para la posicion que esta dentro de datos, pone una linea en el array de posCodigo
        i++;
    }
}


void ram(){
    char* leds;
    char buff[10];
    FILE *inp; 
    FILE *f;

    f = fopen("/dev/usb/blinkstick0", "wb");

        if (f == NULL){
            printf("Error: opening file!\n");
            exit(1);
        }
    leds = "0:000000,1:000000,2:000000,3:000000,4:000000,5:000000,6:000000,7:000000";
    fwrite(leds,strlen(leds),1,f);
    fclose(f);

    while(fin == 0) {
        
        if(!(inp = popen("free | grep Mem | awk '{print $3/$2 * 100.0}'", "r"))) exit(1);
        fgets(buff, sizeof(buff), inp);

        float porcentaje = atof(buff);
        sleep(1);
        printf("Porcentaje RAM: %f \n", porcentaje);
        
        f = fopen("/dev/usb/blinkstick0", "wb");

        if(porcentaje > 87.5) {
            leds = "0:0x000011,1:000111,2:001111,3:001110,4:001100,5:011100,6:111000,7:110000";

        }else if(porcentaje > 75) {
            leds = "0:0x000011,1:000111,2:001111,3:001110,4:001100,5:011100,6:111000";

        }else if(porcentaje > 62.5) {
            leds = "0:0x000011,1:000111,2:001111,3:001110,4:001100,5:011100";
            
        }else if(porcentaje > 50) {
            leds = "0:0x000011,1:000111,2:001111,3:001110,4:001100";

        }else if(porcentaje > 37.5) {
            leds = "0:0x000011,1:000111,2:001111,3:001110";

        }else if(porcentaje > 25) {
            leds = "0:0x000011,1:000111,2:001111";

        }else if(porcentaje > 12.5) {
            leds = "0:0x000011,1:000111";

        }else if(porcentaje > 0) {
            leds = "0:0x000011";

        }
        signal(SIGINT,salir);

        fwrite(leds,strlen(leds),1,f);
        pclose(inp);
        fclose(f);
        
        f = fopen("/dev/usb/blinkstick0", "wb");
    }  
    
    fclose(f);
    signal (SIGINT, SIG_IGN);
}

void salir(){
     signal (SIGALRM, SIG_IGN);
     fin++;
}
