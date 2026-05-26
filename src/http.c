#include <stdio.h>
#include <unistd.h>
#include "http.h"
#include "mime.h"
#include "files.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

static int enviar_todo(int descriptor_cliente, const char *datos, size_t longitud) {
    size_t bytes_enviados = 0;

    while (bytes_enviados < longitud) {
        ssize_t resultado = write(
            descriptor_cliente,
            datos + bytes_enviados,
            longitud - bytes_enviados
        );

        if (resultado < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Buffer lleno, esperar un momento e intentar de nuevo */
                usleep(1000); /* esperar 1ms */
                continue;
            }
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

    int partes_encontradas = sscanf(buffer_solicitud, "%15s %2047s %15s", 
                                     metodo, uri, version);


    if (partes_encontradas != 3) {
        http_send_error(descriptor_cliente, 400, "Bad Request");
        return 0;
    }

    printf("Metodo:  [%s]\n", metodo);
    printf("URI:     [%s]\n", uri);
    printf("Version: [%s]\n", version);

    char *linea_connection = strstr(buffer_solicitud, "Connection:"); 
    int keep_alive = 0; 
    if(linea_connection != NULL){
        if(strstr(linea_connection, "keep-alive")){
            keep_alive = 1;
        }
    }

    /* Solo se acepta GET */
    if (strcmp(metodo, "GET") != 0) {
        http_send_error(descriptor_cliente, 405, "Method Not Allowed");
        return 0;
    }
    if (strstr(uri, "..") != NULL) {
        http_send_error(descriptor_cliente, 403, "Forbidden");
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

    resultado_archivo_t archivo = leer_archivo(ruta_archivo);

    if (archivo.error == ARCHIVO_NO_ENCONTRADO) {
        http_send_error(descriptor_cliente, 404, "Not Found");
        return 0;
    }
    if (archivo.error == ARCHIVO_FORBIDDEN) {
        http_send_error(descriptor_cliente, 403, "Forbidden");
        return 0;
    }
    if (archivo.error == ARCHIVO_ERROR_INTERNO) {
        http_send_error(descriptor_cliente, 500, "Internal Server Error");
        return 0;
    }

    const char *tipo_mime = mime_obtener(ruta_archivo);
    char headers[256];
    int longitud_headers = snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: %s\r\n"
        "\r\n",
        tipo_mime, archivo.longitud,
        keep_alive ? "keep-alive" : "close");

    enviar_todo(descriptor_cliente, headers, longitud_headers);
    enviar_todo(descriptor_cliente, archivo.contenido, archivo.longitud);

    free(archivo.contenido);
   
    return keep_alive;
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