#ifndef SERVER_H
#define SERVER_H

#define MAX_EVENTOS          64
#define BUFFER_SIZE          8192
#define MAX_URI              2048
#define MAX_BYTES_SOLICITUD  16384

int server_run(int descriptor_socket_servidor);

#endif