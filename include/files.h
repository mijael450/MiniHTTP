#ifndef FILES_H
#define FILES_H
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef enum {
    ARCHIVO_OK,
    ARCHIVO_NO_ENCONTRADO,
    ARCHIVO_FORBIDDEN,
    ARCHIVO_ERROR_INTERNO
} error_archivo_t;

typedef struct {
    char          *contenido;
    long           longitud;
    error_archivo_t error;
} resultado_archivo_t;

resultado_archivo_t leer_archivo(const char *ruta_archivo);

#endif