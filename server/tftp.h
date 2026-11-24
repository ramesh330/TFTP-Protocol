#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 6969
#define DATA_SIZE 512
#define PACKET_SIZE 516
#define TIMEOUT_SEC 5

// Global mode state
extern char mode_state[10];

// TFTP Opcodes
typedef enum
{
    RRQ = 1,
    WRQ = 2,
    DATA = 3,
    ACK = 4,
    ERROR = 5
} tftp_opcode;

// TFTP packet (unified structure)
typedef struct
{
    uint16_t opcode;
    union
    {
        // For RRQ/WRQ: filename|0|mode|0
        struct
        {
            char filename_mode[DATA_SIZE];
            char mode[10];
        } request;
        // For DATA: block|data
        struct
        {
            uint16_t block_number;
            char data[DATA_SIZE];
        } data;
        // For ACK: block
        struct
        {
            uint16_t block_number;
        } ack;
        // For ERROR: code|msg|0
        struct
        {
            uint16_t error_code;
            char error_msg[DATA_SIZE];
        } error;
    } body;
} tftp_packet;

// APIs
void send_file(int sockfd, struct sockaddr_in client, socklen_t len, char *filename);
void receive_file(int sockfd, struct sockaddr_in client, socklen_t len, char *filename);
void convert_to_netascii(char *data, int *data_size);
void netascii_local(char *data, int *data_size);
#endif