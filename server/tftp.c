/* Common file for server & client */

#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

char mode_state[10];
void convert_to_netascii(char *data, int *data_size)
{
    int i, j = 0;
    char temp[1024];

    for (i = 0; i < *data_size; i++)
    {
        if (data[i] == '\n')
        {
            temp[j++] = '\r';
            temp[j++] = '\n';
        }
        else if (data[i] == '\r')
        {
            temp[j++] = '\r';
            temp[j++] = '\0';
        }
        else
        {
            temp[j++] = data[i];
        }
    }

    memcpy(data, temp, j);
    *data_size = j;
}
void netascii_local(char *data, int *data_size)
{
    int i = 0, j = 0;
    char temp[1024];

    while (i < *data_size)
    {
        if (data[i] == '\r' && (i + 1 < *data_size))
        {
            if (data[i + 1] == '\n')
            {
                temp[j++] = '\n';
                i += 2;
                continue;
            }
            else if (data[i + 1] == '\0')
            {
                temp[j++] = '\r';
                i += 2;
                continue;
            }
        }

        temp[j++] = data[i++];
    }

    memcpy(data, temp, j);
    *data_size = j;
}
void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)
{
    // send file to client
    int is_netascii = 0;
    if (strcmp(mode_state, "Netascii") == 0)
        is_netascii = 1;
    char buffer[1024];
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        tftp_packet err;
        memset(&err, 0, sizeof(err));
        err.opcode = htons(ERROR);
        err.body.error.error_code = htons(1); // File not found
        strcpy(err.body.error.error_msg, "File not found");

        sendto(sockfd, &err,
               4 + strlen(err.body.error.error_msg) + 1,
               0,
               (struct sockaddr *)&client_addr,
               client_len);
        return; // NO ACK, NO RETRY
    }

    uint16_t block_number = 1; // DATA packets start from 1
    int bytes_read;
    tftp_packet packet;

    while (1)
    {
        bytes_read = read(fd, buffer, 512);
        if (bytes_read == -1)
        {
            perror("File read error");
            close(fd);
            return;
        }

        if (bytes_read == 0) // End of file
        {
            break;
        }
        if (is_netascii)
        {
            // Convert to netascii
            convert_to_netascii(buffer, &bytes_read);
        }

        // Create DATA packet
        memset(&packet, 0, sizeof(packet));
        packet.opcode = htons(DATA);
        memcpy(packet.body.data.data, buffer, bytes_read);
        packet.body.data.block_number = htons(block_number);

        // Send DATA packet
        int packet_size = 4 + bytes_read; // 2 bytes opcode + 2 bytes block + data
        printf("Sending DATA block %d, size %d bytes\n", block_number, bytes_read);

        if (sendto(sockfd, &packet, packet_size, 0, (struct sockaddr *)&client_addr, client_len) == -1)
        {
            perror("Send error");
            close(fd);
            return;
        }

        // Wait for ACK
        tftp_packet ack_packet;
        socklen_t len = client_len;
        int n = recvfrom(sockfd, &ack_packet, sizeof(ack_packet), 0,
                         (struct sockaddr *)&client_addr, &len);

        if (n < 0)
        {
            perror("ACK receive error");
            close(fd);
            return;
        }

        // Check if ACK is correct
        uint16_t ack_opcode = ntohs(ack_packet.opcode);
        uint16_t ack_block = ntohs(ack_packet.body.ack.block_number);

        if (ack_opcode != ACK)
        {
            printf("Expected ACK, got opcode: %d\n", ack_opcode);
            close(fd);
            return;
        }

        if (ack_block != block_number)
        {
            printf("Wrong ACK block: got %d, expected %d\n", ack_block, block_number);
            close(fd);
            return;
        }

        printf("Received ACK for block %d\n", block_number);
        block_number++;

        // If last packet (less than 512 bytes), we're done
        if (bytes_read < 512)
        {
            printf("Last packet sent (size: %d bytes)\n", bytes_read);
            break;
        }
    }

    close(fd);
    printf("File transfer completed\n");
}

void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)
{
    int is_netascii = 0;
    if (strcmp(mode_state, "Netascii") == 0)
        is_netascii = 1;
    tftp_packet packet;
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        perror("File open error");
        return;
    }

    uint16_t expected_block = 1;
    int bytes_received;
    socklen_t len = client_len;

    while (1)
    {
        memset(&packet, 0, sizeof(packet));
        bytes_received = recvfrom(sockfd, &packet, sizeof(packet), 0,
                                  (struct sockaddr *)&client_addr, &len);

        if (bytes_received <= 0)
        {
            perror("Receive error");
            close(fd);
            return;
        }

        uint16_t opcode = ntohs(packet.opcode);

        if (opcode == ERROR)
        {
            printf("Error from server: %s\n", packet.body.error.error_msg);
            close(fd);
            return;
        }

        if (opcode != DATA)
        {
            printf("Unexpected packet type: %d\n", opcode);
            close(fd);
            return;
        }

        uint16_t block_num = ntohs(packet.body.data.block_number);
        int data_size = bytes_received - 4; // Minus opcode and block number

        printf("Received DATA block %d, size %d bytes\n", block_num, data_size);

        if (block_num != expected_block)
        {
            printf("Wrong block number: got %d, expected %d\n", block_num, expected_block);
            close(fd);
            return;
        }

        // if netascii
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, packet.body.data.data, data_size);

        if (is_netascii)
        {
            netascii_local(buffer, &data_size);
        }

        // Write data to file
        if (write(fd, buffer, data_size) == -1)
        {
            perror("File write error");
            close(fd);
            return;
        }

        // Send ACK
        tftp_packet ack_packet;
        memset(&ack_packet, 0, sizeof(ack_packet));
        ack_packet.opcode = htons(ACK);
        ack_packet.body.ack.block_number = htons(block_num);

        printf("Sending ACK for block %d\n", block_num);

        if (sendto(sockfd, &ack_packet, 4, 0,
                   (struct sockaddr *)&client_addr, len) == -1)
        {
            perror("ACK send error");
            close(fd);
            return;
        }

        expected_block++;

        // If last packet (less than 512 bytes), we're done
        if (data_size < 512)
        {
            printf("Received last packet (size: %d bytes)\n", data_size);
            break;
        }
    }

    close(fd);
    printf("File received successfully\n");
}