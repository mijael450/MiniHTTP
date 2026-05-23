#include <stdio.h>
#include <unistd.h>
#include "http.h"

/*
 * Stub temporal: simula que siempre hay keep-alive.
 * Aquí irá el parsing real del request HTTP.
 */
int http_handle(int descriptor_cliente, char *buffer_solicitud, int bytes_leidos) {
    printf("Solicitud recibida (%d bytes):\n%s\n", bytes_leidos, buffer_solicitud);
    return 0; /* 0 = cerrar conexión por ahora */
}

/*
 * Stub temporal: solo imprime el error, no envía nada al cliente todavía.
 */
void http_send_error(int descriptor_cliente, int codigo, const char *mensaje) {
    printf("Error %d: %s (descriptor %d)\n", codigo, mensaje, descriptor_cliente);
}