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

// Eliminada la declaración global problemática
// struct addrinfo hints, *resultado;  // ← NO PONGAS ESTO

int errexit(const char *format, ...){
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    return 0;
}

int socketPasivo(const char *servicio, const char *transporte, int longitud_conexiones) { 
    int descriptorSocket;
    struct addrinfo hints, *resultado, *rp;  // Solo declaración local
    
    // Iniciar en cero la estructura 
    memset(&hints, 0, sizeof(hints));
    
    // Elegir ipV4 
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    
    // Elegir el protocolo UDP o TCP 
    if(strcmp(transporte, "udp") == 0) { 
        hints.ai_socktype = SOCK_DGRAM;
    } else { 
        hints.ai_socktype = SOCK_STREAM;
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
            break;
            
        close(descriptorSocket);
    }
    
    if (rp == NULL) {
        freeaddrinfo(resultado);
        errexit("No se pudo crear/bindear el socket para el puerto %s: %s\n", servicio, strerror(errno));
    }
    
    freeaddrinfo(resultado);
    
    if (hints.ai_socktype == SOCK_STREAM) {
        if (listen(descriptorSocket, longitud_conexiones) < 0) {
            close(descriptorSocket);
            errexit("No se pudo escuchar por el puerto %s: %s\n", servicio, strerror(errno));
        }
    }
    
    return descriptorSocket;
}

int main(){ 
    int sock = socketPasivo("8080", "tcp", 5);
    if (sock > 0) {
        printf("Socket creado exitosamente\n");
        close(sock);
    }
    return 0;
}