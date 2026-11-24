/*
Name - Ramesh Pandhare
Date - 24/11/26
description - Developed a UDP-based TFTP client and server in C following RFC-1350,
              supporting file upload/download with proper DATA–ACK sequencing and error handling.
              Implemented timeout-based retransmission and RFC-compliant client behavior to prevent
              file creation on server errors.
*/
#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6969

// Global variables (same structure as before)

int fd = -1;

int main()
{
    char command[256];
    tftp_client_t client;

    memset(&client, 0, sizeof(client)); // Initialize client structure
    client.sockfd = -1;                 // Not connected initially
    strcpy(client.mode, mode_state);

    // Main loop for command-line interface
    while (1)
    {
        printf("tftp> ");
        fgets(command, sizeof(command), stdin);

        // Remove newline character
        command[strcspn(command, "\n")] = 0;

        // Process the command
        process_command(&client, command);
    }

    return 0;
}

// Function to process commands
void process_command(tftp_client_t *client, char *command)
{
    if (strncmp(command, "connect ", 8) == 0)
    {
        char *ip = command + 8;
        char *port_str = strchr(ip, ' ');
        if (port_str)
        {
            *port_str = '\0';
            port_str++;
            int port = atoi(port_str);
            connect_to_server(client, ip, port);
        }
        else
        {
            // printf("Usage: connect <ip> <port>\n");
            int port = 6969;
            connect_to_server(client, ip, port);
        }
    }
    else if (strncmp(command, "put ", 4) == 0)
    {
        if (client->sockfd == -1)
        {
            printf("Not connected to server. Use 'connect <ip> <port>' first\n");
            return;
        }
        char *filename = command + 4;
        put_file(client, filename);
    }
    else if (strncmp(command, "get ", 4) == 0)
    {
        if (client->sockfd == -1)
        {
            printf("Not connected to server. Use 'connect <ip> <port>' first\n");
            return;
        }
        char *filename = command + 4;
        get_file(client, filename);
    }
    else if (strncmp(command, "mode ", 5) == 0)
    {
        char *mode = command + 5;
        set_mode(client, mode);
    }
    else if (strcmp(command, "exit") == 0 ||
             strcmp(command, "Exit") == 0 ||
             strcmp(command, "bye") == 0 ||
             strcmp(command, "quit") == 0)
    {
        disconnect(client);
        exit(0);
    }
    else if (strlen(command) > 0)
    {
        printf("Unknown command. Available: connect, put, get, mode, exit\n");
    }
}

void set_mode(tftp_client_t *client, char *mode)
{
    if (strcmp(mode, "Normal") == 0 || strcmp(mode, "Octet") == 0 || strcmp(mode, "Netascii") == 0)
    {
        strncpy(client->mode, mode, sizeof(client->mode) - 1);
        client->mode[sizeof(client->mode) - 1] = '\0';
        printf("Mode set to %s\n", client->mode);
        // mode_state = client->mode;
        strcpy(mode_state, client->mode);
    }
    else
    {
        printf("Invalid mode. Available modes: Normal, Octet, Netascii\n");
    }
}

// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port)
{
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return;
    }

    // Set socket timeout option
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Set up server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_aton(ip, &server_addr.sin_addr) == 0)
    {
        printf("Invalid IP address: %s\n", ip);
        close(sockfd);
        return;
    }

    client->sockfd = sockfd;
    client->server_addr = server_addr;
    client->server_len = sizeof(server_addr);
    strncpy(client->server_ip, ip, INET_ADDRSTRLEN - 1);
    client->server_ip[INET_ADDRSTRLEN - 1] = '\0';

    printf("Connected to server %s:%d\n", client->server_ip, port);
}

void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    // Implement sending request
    tftp_packet packet;
    memset(&packet, 0, sizeof(packet));
    packet.opcode = htons(opcode);
    strcpy(packet.body.request.filename, filename);
    strcpy(packet.body.request.mode, mode_state);

    printf("Sending %s request for file: %s with mode: %s\n",
           (opcode == RRQ) ? "RRQ" : "WRQ", filename, mode_state);

    // Calculate size: opcode(2) + filename + null + mode + null
    int packet_size = 2 + strlen(filename) + 1 + strlen(mode_state) + 1;
    sendto(sockfd, &packet, packet_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
}

void put_file(tftp_client_t *client, char *filename)
{
    // Send WRQ request and send file
    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("File open error");
        return;
    }

    // send WRQ request to server
    send_request(client->sockfd, client->server_addr, filename, WRQ);

    // receive ACK from server
    tftp_packet packet;
    socklen_t server_addr_len = sizeof(client->server_addr);
    int n = recvfrom(client->sockfd, &packet, sizeof(packet), 0,
                     (struct sockaddr *)&client->server_addr, &server_addr_len);

    if (n < 0)
    {
        perror("Receive ACK error");
        close(fd);
        return;
    }

    uint16_t opcode = ntohs(packet.opcode);

    if (opcode == ERROR)
    {
        printf("Server error: %s\n", packet.body.error.error_msg);
        close(fd);
        return;
    }

    if (opcode != ACK || ntohs(packet.body.ack.block_number) != 0)
    {
        printf("Did not receive ACK for WRQ\n");
        close(fd);
        return;
    }

    printf("WRQ acknowledged, starting file transfer...\n");

    // send file to server
    send_file(client->sockfd, client->server_addr, client->server_len, filename);
    close(fd);
    printf("File upload completed\n");
}

void get_file(tftp_client_t *client, char *filename)
{
    // Send RRQ and receive file

    // send RRQ request to server
    send_request(client->sockfd, client->server_addr, filename, RRQ);

    printf("RRQ sent, waiting for file data...\n");

    // receive file from server
    receive_file(client->sockfd, client->server_addr, client->server_len, filename);

    close(fd);
    if(file_created)
    printf("File download completed\n");
}

void disconnect(tftp_client_t *client)
{
    // close socket
    if (client->sockfd >= 0)
    {
        close(client->sockfd);
        client->sockfd = -1;
    }
    printf("Disconnected\n");
}

void receive_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    // Implement receiving request
    tftp_packet packet;
    memset(&packet, 0, sizeof(packet));
    socklen_t server_len = sizeof(server_addr);

    recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, &server_len);

    if (ntohs(packet.opcode) == opcode)
    {
        printf("Received request for file: %s\n", packet.body.request.filename);
        strcpy(filename, packet.body.request.filename);
        strcpy(mode_state, packet.body.request.mode);
        printf("Filename: %s\n", filename);
    }
    else
    {
        printf("Received invalid request\n");
    }
}