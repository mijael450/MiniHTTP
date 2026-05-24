#include <stdio.h>
#include <unistd.h>
#include "http.h"
#include <string.h>
#include <stdlib.h>

/* Envía todos los bytes, reintentando si write() no los envió todos de una vez */
static int enviar_todo(int descriptor_cliente, const char *datos, size_t longitud) {
    size_t bytes_enviados = 0;

    while (bytes_enviados < longitud) {
        ssize_t resultado = write(
            descriptor_cliente,
            datos + bytes_enviados,        
            longitud - bytes_enviados      
        );
        if (resultado < 0) {
            perror("write");
            return -1;
        }
        bytes_enviados += resultado;
    }

    return 0;
}

int http_handle(int descriptor_cliente, char *buffer_solicitud, int bytes_leidos) {
    (void)bytes_leidos;

    char metodo[16];
    char uri[MAX_URI];
    char version[16];

    /* Extraer las tres partes de la primera línea */
    int partes_encontradas = sscanf(buffer_solicitud, "%15s %2047s %15s", 
                                     metodo, uri, version);

    /* Si no tiene las 3 partes, la solicitud está malformada */
    if (partes_encontradas != 3) {
        http_send_error(descriptor_cliente, 400, "Bad Request");
        return 0;
    }

    printf("Metodo:  [%s]\n", metodo);
    printf("URI:     [%s]\n", uri);
    printf("Version: [%s]\n", version);

    /* Solo aceptamos GET, cualquier otro método → 405 */
    if (strcmp(metodo, "GET") != 0) {
        http_send_error(descriptor_cliente, 405, "Method Not Allowed");
        return 0;
    }

    /* Si la URI es "/" servir index.html por defecto */
    if (strcmp(uri, "/") == 0) {
        strcpy(uri, "/index.html");
    }

    /* Construir la ruta completa del archivo */
    char ruta_archivo[MAX_URI + 4];
    snprintf(ruta_archivo, sizeof(ruta_archivo), "www%s", uri);

    printf("Buscando archivo: [%s]\n", ruta_archivo);

    /* Abrir el archivo */
    FILE *archivo = fopen(ruta_archivo, "rb");
    if (archivo == NULL) {
        http_send_error(descriptor_cliente, 404, "Not Found");
        return 0;
    }

    /* Obtener el tamaño del archivo */
    fseek(archivo, 0, SEEK_END);
    long longitud_archivo = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    /* Leer el contenido completo */
    char *contenido = malloc(longitud_archivo);
    if (contenido == NULL) {
        fclose(archivo);
        http_send_error(descriptor_cliente, 500, "Internal Server Error");
        return 0;
    }
    fread(contenido, 1, longitud_archivo, archivo);
    fclose(archivo);

    /* Enviar headers 200 OK */
    char headers[256];
    int longitud_headers = snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        longitud_archivo);

    enviar_todo(descriptor_cliente, headers, longitud_headers);
    enviar_todo(descriptor_cliente, contenido, longitud_archivo);

    free(contenido);
    return 0;
}


void http_send_error(int descriptor_cliente, int codigo, const char *mensaje) {
    char cuerpo[512]; 
    int longitud_cuerpo = snprintf(cuerpo, sizeof(cuerpo),
    "<html><body><h1>%d %s</h1></body></html>",
    codigo, mensaje);

    char headers[512]; 
    int longitud_headers = snprintf(headers, sizeof(headers),
    "HTTP/1.1 %d %s\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n"
    "\r\n",
    codigo, mensaje, longitud_cuerpo);


    enviar_todo(descriptor_cliente,headers,longitud_headers);
    enviar_todo(descriptor_cliente,cuerpo,longitud_cuerpo);
    printf("Error %d: %s (descriptor %d)\n", codigo, mensaje, descriptor_cliente);
    printf("longitud_headers: %d\n", longitud_headers);
    printf("longitud_cuerpo: %d\n", longitud_cuerpo);
    printf("headers: [%s]\n", headers);
}