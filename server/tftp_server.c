#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet, int packet_len);

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket timeout option
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("TFTP Server listening on port %d...\n", PORT);

    // Main loop to handle incoming requests
    while (1)
    {
        memset(&packet, 0, sizeof(packet));
        memset(&client_addr, 0, sizeof(client_addr));
        client_len = sizeof(client_addr);

        int n = recvfrom(sockfd, &packet, sizeof(packet), 0,
                         (struct sockaddr *)&client_addr, &client_len);

        if (n < 0)
        {
            // perror("Receive failed or timeout occurred");
            continue;
        }

        handle_client(sockfd, client_addr, client_len, &packet, n);
    }

    close(sockfd);
    return 0;
}

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet, int packet_len)
{
    uint16_t opcode = ntohs(packet->opcode);
    char filename[256];
    char mode[10];
    char *ptr;

    if (opcode == RRQ || opcode == WRQ)
    {
        // Extract filename and mode from request packet
        // Format: filename\0mode\0
        strcpy(filename, packet->body.request.filename_mode);

        // Find mode (skip filename and null terminator)
        // ptr = packet->body.request.filename_mode + strlen(filename) + 1;
        // strcpy(mode, ptr);
        strcpy(mode, packet->body.request.mode);
        strcpy(mode_state, mode);

        printf("Request: %s for file: %s (mode: %s)\n",
               (opcode == RRQ) ? "RRQ" : "WRQ", filename, mode_state);

        if (opcode == RRQ)
        {
            // Read request - send file to client
            send_file(sockfd, client_addr, client_len, filename);
        }
        else if (opcode == WRQ)
        {
            // Write request - receive file from client
            // First send ACK for WRQ (block 0)
            tftp_packet ack_packet;
            memset(&ack_packet, 0, sizeof(ack_packet));
            ack_packet.opcode = htons(ACK);
            ack_packet.body.ack.block_number = htons(0);

            if (sendto(sockfd, &ack_packet, 4, 0,
                       (struct sockaddr *)&client_addr, client_len) == -1)
            {
                perror("ACK send failed");
                return;
            }

            printf("Sent initial ACK for WRQ\n");
            receive_file(sockfd, client_addr, client_len, filename);
        }
    }
    else
    {
        printf("Unexpected opcode: %d\n", opcode);
    }
}