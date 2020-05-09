#include "packet.h"

int window = 8;

int packetDrop(double pdr)
{
    return ((rand()/(double)RAND_MAX) < pdr);
}

void rcv_from_clientLOGS(SNDPKT p, int pd)
{
    time_t ltime; 
    ltime = time(NULL);
    if(pd)
    printf(" RELAY 1 | D | %ld | %d | DATA  | %d | CLIENT | RELAY 1 \n", ltime, HEADER_SIZE+p.pay_size, p.seq_no);
    else
    printf(" RELAY 1 | R | %ld | %d | DATA  | %d | CLIENT | RELAY 1 \n", ltime, HEADER_SIZE+p.pay_size, p.seq_no);
}

void send_to_clientLOGS(ACKPKT ap)
{
    time_t ltime; 
    ltime=time(NULL);
    printf(" RELAY 1 | S | %ld | %d | ACK | %d | RELAY 1  | CLIENT \n", ltime,ACKPKT_SIZE, ap.seq_no);
}

void rcv_from_serverLOGS(ACKPKT ap)
{
    time_t ltime; 
    ltime = time(NULL);
    printf(" RELAY 1 | R | %ld | %d | ACK  | %d | SERVER | RELAY 1 \n", ltime, ACKPKT_SIZE , ap.seq_no);
}

void send_to_serverLOGS(SNDPKT p)
{
    time_t ltime; 
    ltime=time(NULL);
    printf(" RELAY 1 | S | %ld | %d | DATA | %d | RELAY 1 | SERVER \n", ltime,HEADER_SIZE+p.pay_size , p.seq_no);
}

void sendDATA(SNDPKT p, int sockfd, struct sockaddr_in* recv, int r1_len)
{
    send_to_serverLOGS(p);
    sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr*)recv, r1_len);
}

int createSocket()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) 
	{
		printf ("Error while server socketcreation");
		 exit (0); 
	}
    return sock;
}

void ConfigureConnection(struct sockaddr_in* addr, int port)
{
    memset(addr, '0', sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port); // port
    addr->sin_addr.s_addr = inet_addr("127.0.0.1");
}

int main()
{
    int sockfd = createSocket();

    struct sockaddr_in sender;
    struct sockaddr_in receiver;
    ConfigureConnection(&sender,7001);
    ConfigureConnection(&receiver,8001);

    struct sockaddr_in thisOne;
    ConfigureConnection(&thisOne,6002);

    if( bind(sockfd , (struct sockaddr*)&thisOne, sizeof(thisOne) ) == -1)
    {
        printf ("Error Occured while binding\n");
		exit (0);
    }
    printf("RELAY 1 is UP\n");
  
    DATAPKT* front=NULL;
    DATAPKT* back=NULL;

    struct sockaddr_in temp;
    int len = sizeof(temp);

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
  
    int timeout=-1;

    while(1)
    {
        poll(fds, 1, timeout);
        //printf("reached here\n");
        SNDPKT rcvd_pkt;
        for(int i=0; i<window/2 ; i++)
        {
            poll(fds, 1, 200);
            int pd = packetDrop(PDR);
            if((fds[0].revents & POLLIN) && pd==0)
            {
               // printf("packet received\n");
                memset(&rcvd_pkt,0,sizeof(SNDPKT));
                if(recvfrom(sockfd, &rcvd_pkt, PACKET_SIZE, 0, (struct sockaddr*) &temp, &len) >0 )
                {
                    rcv_from_clientLOGS(rcvd_pkt, pd);
                    sendDATA(rcvd_pkt, sockfd, &receiver, len);
                }
            }
            if(pd)
            {
                rcv_from_clientLOGS(rcvd_pkt, pd);
            }
            fds[0].revents=0;
        }

        poll(fds, 1, timeout);                       // listening for acknowledgement packets
        for(int i=0; i<window/2 ; i++)
        {
            ACKPKT rcvd_pkt;
            poll(fds, 1, 200);
            if(fds[0].revents & POLLIN)
            {
                memset(&rcvd_pkt,0,sizeof(ACKPKT));
                if(recvfrom(sockfd, &rcvd_pkt, 4, 0, (struct sockaddr*) &temp, &len) >0)
                 {
                     rcv_from_serverLOGS(rcvd_pkt);
                     sendto(sockfd, &rcvd_pkt, sizeof(rcvd_pkt), 0, (struct sockaddr*) &sender, len);
                     send_to_clientLOGS(rcvd_pkt);
                 }
            }
            fds[0].revents=0;
        }             
        
    }
    return 0;
}