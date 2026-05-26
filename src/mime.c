#include <string.h>
#include "mime.h"

typedef struct {
    const char *extension;
    const char *tipo_contenido;
} mime_entrada_t;

static mime_entrada_t tabla_mime[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".ico",  "image/x-icon"},
    {NULL, NULL}
};

const char *mime_obtener(const char *ruta_archivo) {

    const char *extension = strrchr(ruta_archivo, '.');

    if (extension == NULL)
        return "application/octet-stream"; 

    for (int i = 0; tabla_mime[i].extension != NULL; i++) {
        if (strcmp(extension, tabla_mime[i].extension) == 0)
            return tabla_mime[i].tipo_contenido;
    }

    return "application/octet-stream";  /* extensión no reconocida */
}