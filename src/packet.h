#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 2048

void print_packet(unsigned char *buffer, int size);

#endif // PACKET_H
