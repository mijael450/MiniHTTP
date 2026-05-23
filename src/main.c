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


int errexit(const char *format, ...){
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    return 0;
}

int socketPasivo(const char *servicio, const char *transporte, int longitud_conexiones) { 
    int descriptorSocket;
    struct addrinfo hints, *resultado, *rp;
    
    // Iniciar en cero la estructura 
    memset(&hints, 0, sizeof(hints));
    
    // Elegir ipV4 
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;  // IMPORTANTE: para socket pasivo/server
    
    // Elegir el protocolo UDP o TCP 
    if(strcmp(transporte, "udp") == 0) { 
        hints.ai_socktype = SOCK_DGRAM; // udp
    } else { 
        hints.ai_socktype = SOCK_STREAM; // tcp 
    }
    
    // Agregar la direccion ip, mapear el nombre del servicio a un puerto
    int estado = getaddrinfo(NULL, servicio, &hints, &resultado);
    if (estado != 0) {
        fprintf(stderr, "%s\n", gai_strerror(estado));
        exit(1);
    }
    
    // Crear el socket (intentar con cada resultado)
    for (rp = resultado; rp != NULL; rp = rp->ai_next) {
        descriptorSocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (descriptorSocket < 0)
            continue;
            
        // Asociar el socket a la direccion 
        if (bind(descriptorSocket, rp->ai_addr, rp->ai_addrlen) == 0)
            break;  // Éxito
            
        close(descriptorSocket);
    }
    
    if (rp == NULL) {
        freeaddrinfo(resultado);
        errexit("No se pudo crear/bindear el socket para el puerto %s: %s\n", servicio, strerror(errno));
    }
    
    freeaddrinfo(resultado);  // Liberar memoria
    
    if (hints.ai_socktype == SOCK_STREAM) {
        if (listen(descriptorSocket, longitud_conexiones) < 0) {
            close(descriptorSocket);
            errexit("No se pudo escuchar por el puerto %s: %s\n", servicio, strerror(errno));
        }
    }
    
    return descriptorSocket;  // Retornar el descriptor
    }

int main(){ 
    int sock = socketPasivo("8080", "tcp", 5);
    if (sock < 0) {
        fprintf(stderr,"Error creando el socket\n");
        return 1;
    }
    printf("Socket creado exitosamente\n");
    return server_run(sock);
}