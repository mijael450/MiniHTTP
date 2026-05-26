# MiniHTTPd

Servidor HTTP/1.1 básico implementado en C para Linux, sin bibliotecas HTTP externas.

**Autor:** Olguer Molina  
**Repositorio:** https://github.com/mijael450/MiniHTTP.git

---

## Características

- Sirve archivos estáticos: HTML, CSS, JavaScript, imágenes (PNG, JPG)
- Procesa el método GET y analiza encabezados básicos (`Host`, `Connection`, `User-Agent`)
- Soporta múltiples clientes mediante `fork()` y `epoll`
- Conexiones persistentes (`Connection: keep-alive`)
- Tipos MIME automáticos según la extensión del archivo
- Protección contra directory traversal con `realpath()`
- Responde con códigos de estado HTTP: 200, 400, 403, 404, 405, 500

---

## Estructura del proyecto

```
minihttpd/
├── Makefile
├── README.md
├── include/
│   ├── http.h
│   ├── server.h
│   ├── mime.h
│   └── files.h
├── src/
│   ├── main.c
│   ├── server.c
│   ├── http.c
│   ├── mime.c
│   └── files.c
└── www/
    ├── index.html
    ├── style.css
    └── image.png
```

---

## Requisitos

- Linux (Ubuntu / Kali / Debian)
- GCC
- Make

---

## Compilar

```bash
make
```

Para limpiar y recompilar:

```bash
make clean && make
```

---

## Ejecutar

```bash
./minihttpd
```

El servidor escucha en el puerto **8080**. Abrir en el navegador:

```
http://localhost:8080/
```

---

## Probar

Solicitud básica:
```bash
curl http://localhost:8080/
```

Verificar códigos de estado:
```bash
curl http://localhost:8080/noexiste.html        # 404 Not Found
curl -X POST http://localhost:8080/             # 405 Method Not Allowed
curl --path-as-is http://localhost:8080/../../etc/passwd  # 403 Forbidden
```

Prueba de concurrencia:
```bash
ab -n 1000 -c 50 http://localhost:8080/
```

---

## Módulos

| Archivo | Responsabilidad |
|---|---|
| `main.c` | Crea el socket pasivo e inicia el servidor |
| `server.c` | Bucle epoll, accept y fork por cliente |
| `http.c` | Parseo del request GET y generación de respuesta |
| `files.c` | Lectura de archivos estáticos y validación de rutas |
| `mime.c` | Detección del Content-Type según extensión |
