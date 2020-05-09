#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

#define PACKET_SIZE 100
#define HEADER_SIZE 32
#define PAYLOAD_SIZE  (PACKET_SIZE - HEADER_SIZE)
#define TIMEOUT 4 
#define ACKPKT_SIZE 4
#define PDR 0.1

typedef struct packet1{
    int pay_size;
    int seq_no;
    int isLast;
    int isACKED;
    int pkt_no;
    char *payload;
    struct packet1* next;
} DATAPKT;

typedef struct packet2{
    int pay_size;
    int seq_no;
    int isLast;
    int isACKED;
    char payload[80];
} SNDPKT;

typedef struct packet3{
    int seq_no;
} ACKPKT;