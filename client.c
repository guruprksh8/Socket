#include "packet.h"

int count=0;
int window = 8;
int BytesAcked=0;
int offset=0;

void rcvLOGS(ACKPKT ap, int relay)
{
    time_t ltime; 
    ltime = time(NULL);
    printf(" CLIENT | R | %ld | %d | ACK  | %d | RELAY %d | SERVER \n", ltime, ACKPKT_SIZE , ap.seq_no, relay);
}

void sendLOGS(SNDPKT p, int relay, int retransmit)
{
    time_t ltime; 
    ltime=time(NULL);
    if(retransmit)
    {
      printf(" CLIENT | RE | %ld | %d | DATA | %d | CLIENT   | RELAY %d \n", ltime,HEADER_SIZE+p.pay_size , p.seq_no, relay);
    }
    else
    {
      printf(" CLIENT | S | %ld | %d | DATA | %d | CLIENT   | RELAY %d \n", ltime,HEADER_SIZE+p.pay_size , p.seq_no, relay);
    } 
    
}

DATAPKT* createPKT(FILE** fp)
{
    DATAPKT* dp = (DATAPKT*)malloc(sizeof(DATAPKT));
    dp->next = NULL;
    dp->isACKED=0;
    dp->pkt_no = count;
    count++;
    char BUFF[PAYLOAD_SIZE];
    fseek(*fp, offset, SEEK_SET);
    dp->seq_no = ftell(*fp);
    int nread = fread(BUFF, sizeof(char), PAYLOAD_SIZE, *fp);
    if(nread > 0)
    {
        dp->payload = (char*) malloc(sizeof(char)*PAYLOAD_SIZE);
        memcpy(dp->payload, BUFF, nread);
    }
    dp->pay_size = nread;
    offset = dp->seq_no + nread;
    fseek(*fp, offset , SEEK_SET);
    if(fgetc(*fp)==EOF || nread < PAYLOAD_SIZE)
    dp->isLast = 1;
    else
    dp->isLast=0;
    return dp;
}

void sendDATA(DATAPKT* dp, int sockfd, struct sockaddr_in *relay1, struct sockaddr_in *relay2, int r1_len, int retransmit)
{
    //printf("sending seq no %d \n", dp->seq_no);
    SNDPKT p;                
    p.pay_size = dp->pay_size;
    p.seq_no = dp->seq_no;
    p.isLast = dp->isLast;
    strncpy(p.payload, dp->payload, dp->pay_size);
  
    if(dp->pkt_no%2==1)
    sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr *)relay1, r1_len);
    else
    sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr *)relay2, r1_len);
    
    sendLOGS(p, dp->pkt_no%2, retransmit);
}

void push(DATAPKT** front, DATAPKT** back, DATAPKT** dp)
{
    if(*back == NULL)
    {
        *front = *dp;
        *back = *dp;
    }
    else
    {
        (*back)->next = *dp;
        (*back) = *dp;
    }
}

void pop(DATAPKT** front, DATAPKT** back)
{
    if(*front==NULL)
    return ;

    DATAPKT* temp = *front;
    (*front) = (*front)->next;
    if((*front)==NULL)
      (*back)=NULL;
    
    BytesAcked += temp->pay_size; 
    free(temp);
    //printf("Element Popped\n");
}

void createFileptr(FILE** ptr)
{
    *ptr = fopen("input.txt", "rb"); 
  
    if(*ptr==NULL)
    {
        printf("Error opening file");
        exit(0);
    }
    fseek(*ptr, 0, SEEK_SET);
}

void markAck(DATAPKT* front, ACKPKT ap)
{
    //printf("markAck\n");
    while(front)
    { 
        if(front->seq_no == ap.seq_no)
        {
            front->isACKED=1;
            //printf("ACKS MARKED corresponding to %d\n", ap.seq_no);
            break;
        }
        front = front->next;
    }  
    rcvLOGS(ap, rand()%2);
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
    
    // create file pointer for reading file
    FILE* fp;
    createFileptr(&fp);

    struct sockaddr_in r1;
    struct sockaddr_in r2;
    ConfigureConnection(&r1,6001);
    ConfigureConnection(&r2,6002);
  
    int r_len = sizeof(r1);

    struct sockaddr_in thisOne;
    ConfigureConnection(&thisOne,7001);
    //ConfigureConnection(&thisOne);

    if( bind(sockfd , (struct sockaddr*)&thisOne, sizeof(thisOne) ) == -1)
    {
        printf ("Error Occured while binding\n");
		    exit (0);
    }
    printf("Binding Complete\n");
  
    DATAPKT* front=NULL;
    DATAPKT* back=NULL;

    int lastpktsent=0;    

    for(int i=0; i<window ; i++)
    {   
        DATAPKT* pkt = createPKT(&fp);
        push(&front, &back, &pkt);
        //if(packetDrop(0.40)==0)
        //{
            sendDATA(pkt, sockfd, &r1, &r2, r_len, 0);
        //}
        if(pkt->isLast)
        {
            lastpktsent=1;
            break;
        }
    }
  
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
  
    int infi=-1, timeout=2000;
  
    DATAPKT* temp = front;

    struct sockaddr_in recv_addr;  
    int recv_addr_len = sizeof(recv_addr);
  
    while(1)
    {
        poll(fds, 1, infi);
        for(int i=0 ;i<window ;i++)
        {
            poll(fds, 1, timeout);
            if(fds[0].revents & POLLIN)
            {
                ACKPKT rcvd_pkt;
                memset(&rcvd_pkt,0,sizeof(ACKPKT));
                if(recvfrom(sockfd, &rcvd_pkt, 4, 0, (struct sockaddr *)&recv_addr, &recv_addr_len) >0 )
                  markAck(front, rcvd_pkt);
            }
        }
        
        while(front!=NULL && front->isACKED)
        {
            pop(&front, &back);
        }
        
        if(BytesAcked == offset && lastpktsent)
        break;
        
        DATAPKT* temp = front;
        for(int i=0; i<window ;i++)
        {
            if(temp==NULL)
            {
                DATAPKT* pkt = createPKT(&fp);
                sendDATA(pkt, sockfd, &r1, &r2, r_len, 0);
                push(&front, &back, &pkt);
                if(pkt->isLast)
                {
                     lastpktsent=1;
                     break;
                }
            }
            else if(temp->isACKED==0)
            {
                sendDATA(temp, sockfd, &r1, &r2, r_len, 1);
                temp = temp->next;
                if(temp->isLast)
                {
                     lastpktsent=1;
                     break;
                }
                
            }
            else if(temp->isACKED==1)
              temp = temp->next;            
        }
        
        if(BytesAcked == offset && lastpktsent)
        break;
    }
    printf("Complete File Transferred\n");
    return 0;
}