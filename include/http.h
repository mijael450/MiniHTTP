#ifndef HTTP_H
#define HTTP_H
int  http_handle(int descriptor_cliente, char *buffer_solicitud, int bytes_leidos);
void http_send_error(int descriptor_cliente, int codigo, const char *mensaje);
#define MAX_URI 2048
#endif