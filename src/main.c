#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <server.h>

int descriptorSocket; 
int tipoSocket; 
int puerto; 
struct addrinfo hints, *resultado;

int errexit(const char *format, ...){
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    return 0;
}

int socketPasivo(const char *servicio, const char *transporte, int longitud_conexiones ){ 
    // Iniciar en cero la estructura 
    memset(&hints, 0, sizeof(hints));

    // Elejir ipV4 
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;

    //Elejir el protocolo UDP o TCP 
    if(strcmp(transporte, "udp") == 0){ 
        hints.ai_socktype = SOCK_DGRAM; //udp
    } else { 
        hints.ai_socktype = SOCK_STREAM; //tcp 
    }

    //Agregar la direccion ip, mapear el nombnre del servicio a un puerto
    int estado = getaddrinfo(NULL, servicio, &hints, &resultado);
    if (estado != 0){
        printf("%s", gai_strerror(estado));
        exit(1);
    }

    //Crear el socket
    descriptorSocket = socket(resultado->ai_family, resultado->ai_socktype, resultado->ai_protocol);
    if (descriptorSocket < 0)
    {
        fprintf(stderr, "[ERROR] Fallo al crear el socket\n");
        perror("socket");
        exit(1);
    }

    int opcion = 1;
    setsockopt(descriptorSocket, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion));

    //Asociar el socket a la direccion 
    if(bind(descriptorSocket, resultado->ai_addr, resultado->ai_addrlen) < 0 ){
        errexit("No se puede asociar al puerto %s: %s\n", servicio, strerror(errno));
    } 

    if(resultado->ai_socktype == SOCK_STREAM) {
        if(listen(descriptorSocket, longitud_conexiones) < 0) {
            errexit("No se pudo escuchar por el puerto %s: %s\n", servicio, strerror(errno));
        }
    }
    
    return descriptorSocket;
}

int main(){ 
    int descriptoSocket = socketPasivo("8080", "tcp", 5);
    if (descriptoSocket < 0) {
        fprintf(stderr,"Error creando el socket\n");
        return 1;
    }
    printf("Socket creado exitosamente\n");
    return server_run(descriptoSocket);
}