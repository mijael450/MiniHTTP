#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>   /* accept(), sockaddr_storage */
#include <netinet/in.h>   /* sockaddr_in */
#include <fcntl.h>
#include "server.h"
#include "http.h"

/* 
 * Pone un socket en modo no-bloqueante.
 * Con epoll edge-triggered (EPOLLET) esto es obligatorio:
 * el kernel solo avisa UNA vez cuando llegan datos, así que
 * el socket no puede quedarse bloqueado esperando más bytes.
 */
static int poner_socket_no_bloqueante(int descriptor_socket) {
    int banderas_actuales = fcntl(descriptor_socket, F_GETFL, 0);
    if (banderas_actuales < 0) return -1;
    return fcntl(descriptor_socket, F_SETFL, banderas_actuales | O_NONBLOCK);
}

/*
 * Registra un descriptor de socket en la instancia epoll,
 * para que epoll lo vigile y nos avise cuando tenga datos listos.
 *
 * EPOLLIN  = avisar cuando haya datos para leer
 * EPOLLET  = modo edge-triggered (avisar solo cuando cambia el estado,
 *             no repetidamente mientras haya datos pendientes)
 */
static int registrar_socket_en_epoll(int descriptor_epoll, int descriptor_socket) {
    struct epoll_event configuracion_evento;
    configuracion_evento.events  = EPOLLIN | EPOLLET;
    configuracion_evento.data.fd = descriptor_socket;
    return epoll_ctl(descriptor_epoll, EPOLL_CTL_ADD, descriptor_socket, &configuracion_evento);
}

/*
 * Lee la solicitud HTTP de un cliente y genera la respuesta.
 * Retorna  0 si la conexión debe mantenerse abierta (keep-alive).
 * Retorna -1 si la conexión debe cerrarse.
 */
static int atender_cliente(int descriptor_cliente) {
    char buffer_solicitud[BUFFER_SIZE];

    ssize_t bytes_leidos = read(descriptor_cliente, buffer_solicitud, sizeof(buffer_solicitud) - 1);

    if (bytes_leidos == 0) {
        /* El cliente cerró la conexión desde su lado */
        return -1;
    }

    if (bytes_leidos < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* No hay más datos por ahora, pero la conexión sigue viva.
             * Esto es normal en modo no-bloqueante. */
            return 0;
        }
        /* Error real de lectura */
        perror("read");
        return -1;
    }

    /* Terminar el buffer con '\0' para poder usar funciones de string */
    buffer_solicitud[bytes_leidos] = '\0';

    /* Rechazar solicitudes que excedan el límite de tamaño (→ 400) */
    if (bytes_leidos >= MAX_BYTES_SOLICITUD) {
        http_send_error(descriptor_cliente, 400, "Request Too Large");
        return -1;
    }

    /* 
     * Pasar la solicitud a http.c para que la parsee y responda.
     * http_handle devuelve 1 si el cliente pidió keep-alive, 0 si no.
     */
    int cliente_pidio_keep_alive = http_handle(descriptor_cliente, buffer_solicitud, bytes_leidos);

    return cliente_pidio_keep_alive ? 0 : -1;
}

/*
 * Bucle principal del servidor.
 * Recibe el socket pasivo ya creado y vinculado en main.c,
 * y se queda en un loop infinito atendiendo conexiones.
 */
int server_run(int descriptor_socket_servidor) {

    /* Paso 1: poner el socket del servidor en modo no-bloqueante */
    if (poner_socket_no_bloqueante(descriptor_socket_servidor) < 0) {
        perror("poner_socket_no_bloqueante");
        return -1;
    }

    /* Paso 2: crear la instancia epoll (el "monitor" de descriptores) */
    int descriptor_epoll = epoll_create1(0);
    if (descriptor_epoll < 0) {
        perror("epoll_create1");
        return -1;
    }

    /* Paso 3: registrar el socket del servidor en epoll,
     * para que nos avise cuando llegue un cliente nuevo */
    if (registrar_socket_en_epoll(descriptor_epoll, descriptor_socket_servidor) < 0) {
        perror("registrar_socket_en_epoll");
        close(descriptor_epoll);
        return -1;
    }

    /* Array donde epoll deposita los eventos listos en cada iteración */
    struct epoll_event eventos_listos[MAX_EVENTOS];

    printf("Servidor listo. Esperando conexiones en el puerto 8080...\n");

    /* Bucle infinito: esperar eventos, procesarlos, repetir */
    while (1) {

        /* epoll_wait se bloquea hasta que haya al menos un evento listo.
         * El -1 como timeout significa "esperar indefinidamente". */
        int cantidad_eventos_listos = epoll_wait(
            descriptor_epoll,
            eventos_listos,
            MAX_EVENTOS,
            -1
        );

        if (cantidad_eventos_listos < 0) {
            if (errno == EINTR) {
                /* Una señal del sistema interrumpió la espera (normal).
                 * Simplemente volvemos a esperar. */
                continue;
            }
            perror("epoll_wait");
            break;
        }

        /* Procesar cada evento que epoll reportó como listo */
        for (int indice = 0; indice < cantidad_eventos_listos; indice++) {

            int descriptor_con_actividad = eventos_listos[indice].data.fd;

            if (descriptor_con_actividad == descriptor_socket_servidor) {
                /*
                 * El socket del SERVIDOR tiene actividad:
                 * significa que un cliente nuevo quiere conectarse.
                 */
                struct sockaddr_storage direccion_cliente;
                socklen_t longitud_direccion_cliente = sizeof(direccion_cliente);

                int descriptor_cliente_nuevo = accept(
                    descriptor_socket_servidor,
                    (struct sockaddr *)&direccion_cliente,
                    &longitud_direccion_cliente
                );

                if (descriptor_cliente_nuevo < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        perror("accept");
                    continue;
                }

                /* Preparar el socket del cliente para epoll y registrarlo */
                poner_socket_no_bloqueante(descriptor_cliente_nuevo);
                registrar_socket_en_epoll(descriptor_epoll, descriptor_cliente_nuevo);

                printf("Cliente conectado (descriptor %d)\n", descriptor_cliente_nuevo);

            } else {
                /*
                 * Un socket de CLIENTE ya existente tiene actividad:
                 * llegaron datos de una solicitud HTTP.
                 */
                int resultado = atender_cliente(descriptor_con_actividad);

                if (resultado < 0) {
                    /* La conexión debe cerrarse: eliminar de epoll y cerrar */
                    epoll_ctl(descriptor_epoll, EPOLL_CTL_DEL,
                              descriptor_con_actividad, NULL);
                    close(descriptor_con_actividad);
                    printf("Cliente desconectado (descriptor %d)\n", descriptor_con_actividad);
                }
            }
        }
    }

    close(descriptor_epoll);
    return 0;
}