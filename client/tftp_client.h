#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include <netinet/in.h>


typedef struct
{
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_len;
    char server_ip[INET_ADDRSTRLEN];
    char mode[10]; // TFTP mode 
} tftp_client_t;

// Global variables declaration
// extern char *mode_state;
extern int fd;

// Function prototypes
void connect_to_server(tftp_client_t *client, char *ip, int port);
void put_file(tftp_client_t *client, char *filename);
void get_file(tftp_client_t *client, char *filename);
void disconnect(tftp_client_t *client);
void process_command(tftp_client_t *client, char *command);
void set_mode(tftp_client_t *client, char *mode);

void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode);
void receive_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode);

#endif