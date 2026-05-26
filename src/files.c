#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "files.h"

resultado_archivo_t leer_archivo(const char *ruta_archivo) {
    resultado_archivo_t resultado;
    resultado.contenido = NULL;
    resultado.longitud  = 0;
    resultado.error     = ARCHIVO_OK;

    /* Verificar que la ruta no salga de www/ */
    char ruta_www[PATH_MAX];
    if (realpath("www", ruta_www) == NULL) {
        resultado.error = ARCHIVO_ERROR_INTERNO;
        return resultado;
    }

    char ruta_resuelta[PATH_MAX];
    if (realpath(ruta_archivo, ruta_resuelta) == NULL) {
        resultado.error = ARCHIVO_NO_ENCONTRADO;
        return resultado;
    }

    if (strncmp(ruta_resuelta, ruta_www, strlen(ruta_www)) != 0) {
        resultado.error = ARCHIVO_FORBIDDEN;
        return resultado;
    }

    /* Abrir y leer el archivo */
    FILE *archivo = fopen(ruta_archivo, "rb");
    if (archivo == NULL) {
        resultado.error = ARCHIVO_NO_ENCONTRADO;
        return resultado;
    }

    fseek(archivo, 0, SEEK_END);
    resultado.longitud = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);

    resultado.contenido = malloc(resultado.longitud);
    if (resultado.contenido == NULL) {
        fclose(archivo);
        resultado.error = ARCHIVO_ERROR_INTERNO;
        return resultado;
    }

    fread(resultado.contenido, 1, resultado.longitud, archivo);
    fclose(archivo);

    return resultado;
}