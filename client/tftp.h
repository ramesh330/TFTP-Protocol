/* Common file for server & client*/

#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 6969
#define BUFFER_SIZE 516 // TFTP data packet size (512 bytes data + 4 bytes header)
#define TIMEOUT_SEC 5   // Timeout in seconds
extern char mode_state[10];
extern int file_created ;
// TFTP OpCodes
typedef enum
{
    RRQ = 1,  // Read Request
    WRQ = 2,  // Write Request
    DATA = 3, // Data Packet
    ACK = 4,  // Acknowledgment
    ERROR = 5 // Error Packet
} tftp_opcode;

typedef enum
{
    Normal,
    Octet,
    Netascii
} tftp_mode;

// TFTP Packet Structure
typedef struct
{
    uint16_t opcode; // Operation code (RRQ/WRQ/DATA/ACK/ERROR)
    union
    {
        struct
        {
            char filename[256];
            char mode[10]; // Typically "octet" or "netascii"
        } request;         // RRQ and WRQ

        struct
        {
            uint16_t block_number;
            char data[512];
        } data; // DATA
        struct
        {
            uint16_t block_number;
        } ack; // ACK
        struct
        {
            uint16_t error_code;
            char error_msg[512];
        } error; // ERROR
    } body;
} tftp_packet;

// 4 PARAMETER VERSION (no mode parameter)
void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);
void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);
void convert_to_netascii(char *data, int *data_size);
void netascii_local(char *data, int *data_size);

#endif