typedef struct { const char *ext; const char *type; } mime_entry_t;

static mime_entry_t mime_table[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {NULL, NULL}
};

const char *mime_get(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    for (int i = 0; mime_table[i].ext; i++)
        if (strcmp(dot, mime_table[i].ext) == 0)
            return mime_table[i].type;
    return "application/octet-stream";
}