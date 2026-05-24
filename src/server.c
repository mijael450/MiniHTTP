#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <fcntl.h>
#include "server.h"
#include "http.h"

/* 
 * Pone un socket en modo no-bloqueante.
 */
static int poner_socket_no_bloqueante(int descriptor_socket) {
    int banderas_actuales = fcntl(descriptor_socket, F_GETFL, 0);
    if (banderas_actuales < 0) return -1;
    return fcntl(descriptor_socket, F_SETFL, banderas_actuales | O_NONBLOCK);
}

/*
 * Registra un descriptor de socket en la instancia epoll.
 */
static int registrar_socket_en_epoll(int descriptor_epoll, int descriptor_socket) {
    struct epoll_event configuracion_evento;
    configuracion_evento.events  = EPOLLIN | EPOLLET;
    configuracion_evento.data.fd = descriptor_socket;
    return epoll_ctl(descriptor_epoll, EPOLL_CTL_ADD, descriptor_socket, &configuracion_evento);
}

/*
 * Lee la solicitud HTTP de un cliente y genera la respuesta.
 */
static int atender_cliente(int descriptor_cliente) {
    char buffer_solicitud[BUFFER_SIZE];

    ssize_t bytes_leidos = read(descriptor_cliente, buffer_solicitud, sizeof(buffer_solicitud) - 1);

    if (bytes_leidos == 0) {
        /* El cliente cerro la conexion desde su lado */
        return -1;
    }

    if (bytes_leidos < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* No hay mas datos para leer pero la conexion debe seguir viva*/
            return 0;
        }
        /* Error de lectura */
        perror("read");
        return -1;
    }

    buffer_solicitud[bytes_leidos] = '\0';

    /* Rechazar solicitudes que excedan el limite */
    if (bytes_leidos >= MAX_BYTES_SOLICITUD) {
        http_send_error(descriptor_cliente, 400, "Request Too Large");
        return -1;
    }

    int cliente_pidio_keep_alive = http_handle(descriptor_cliente, buffer_solicitud, bytes_leidos);

    return cliente_pidio_keep_alive ? 0 : -1;
}

/*
 * Bucle principal del servidor.
 * Recibe el socket pasivo ya creado y vinculado.
 */
int server_run(int descriptor_socket_servidor) {

    /* Poner el socket del servidor en modo no bloqueante */
    if (poner_socket_no_bloqueante(descriptor_socket_servidor) < 0) {
        perror("poner_socket_no_bloqueante");
        return -1;
    }

    /*Crear la instancia epoll*/
    int descriptor_epoll = epoll_create1(0);
    if (descriptor_epoll < 0) {
        perror("epoll_create1");
        return -1;
    }

    /* Registrar el socket del servidor con epoll */
    if (registrar_socket_en_epoll(descriptor_epoll, descriptor_socket_servidor) < 0) {
        perror("registrar_socket_en_epoll");
        close(descriptor_epoll);
        return -1;
    }

    struct epoll_event eventos_listos[MAX_EVENTOS];

    printf("Servidor listo. Esperando conexiones en el puerto 8080...\n");

    while (1) {

        int cantidad_eventos_listos = epoll_wait(
            descriptor_epoll,
            eventos_listos,
            MAX_EVENTOS,
            -1
        );

        if (cantidad_eventos_listos < 0) {
            if (errno == EINTR) {
                /* Una señal del sistema interrumpio la espera normal).*/
                continue;
            }
            perror("epoll_wait");
            break;
        }

        /* Procesar cada evento que reportado como listo*/
        for (int indice = 0; indice < cantidad_eventos_listos; indice++) {

            int descriptor_con_actividad = eventos_listos[indice].data.fd;

            if (descriptor_con_actividad == descriptor_socket_servidor) {
                /*
                 Un cliente nuevo quiere conectarse.
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
                 * Un cliente ya existente tiene actividad:
                 * llegaron datos de una solicitud HTTP.
                 */
                int resultado = atender_cliente(descriptor_con_actividad);

                if (resultado < 0) {

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