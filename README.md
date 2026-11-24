# TFTP Client–Server Project

## Abstract
Trivial File Transfer Protocol (TFTP) is a simple file transfer protocol defined in RFC 1350.  
It is mainly used in embedded and network environments for lightweight file transfer.  
TFTP supports limited features such as no authentication and no directory listing, focusing on simplicity and reliability over UDP.

The protocol supports two transfer modes:
- **octet** (binary mode)
- **netascii** (text mode)

---

## Requirements
- Follow RFC 1350
- Server runs continuously in a loop
- Client runs until the user types `bye` or `quit`
- Server handles client requests (forking and unique ports can be extended)
- Uses UDP socket programming

---

## Prerequisites
- Knowledge of C programming
- Understanding of socket programming
- Familiarity with UDP-based client–server communication

---

## Design Overview

### Client–Server Interaction
- Client sends **Read Request (RRQ)** or **Write Request (WRQ)** to the server
- Data is exchanged in **512-byte chunks**
- Each data block uses a **sequence (block) number**
- Reliability is ensured using **ACK packets**

---

## Packet Types

| Opcode | Packet Type | Description |
|------|------------|-------------|
| 1 | RRQ | Read Request |
| 2 | WRQ | Write Request |
| 3 | DATA | Data Packet |
| 4 | ACK | Acknowledgement |
| 5 | ERROR | Error Packet |

---

## Error Handling
The protocol supports the following error conditions:
- File not found
- Access violation
- Disk full
- Illegal operation
- Unknown transfer ID
- File already exists

---

## Data Transfer
- Data is sent in **512-byte blocks**
- Each DATA packet must be acknowledged
- Transfer ends when a DATA packet of size **less than 512 bytes** is received and acknowledged
- Basic timeout handling is implemented for reliability

---

## Supported Modes
- **octet**: Binary-safe file transfer
- **netascii**: Text mode with CR-LF (`\r\n`) conversion

---

## Sample Client Commands

```text
connect <ip>          Connect to server
get <filename>        Download file from server
put <filename>        Upload file to server
mode <octet/netascii> Set transfer mode
bye / quit            Exit client

