#include "packet.h"

int totalBytesRead=0, lastByte=-1;
int window = 8;
int savedUPTO=0;

void rcvLOGS(SNDPKT p, int relay)
{
    time_t ltime; 
    ltime = time(NULL);
    printf(" SERVER | R | %ld | %d | DATA  | %d | RELAY %d | SERVER \n", ltime, HEADER_SIZE+p.pay_size, p.seq_no, relay);
}

void sendLOGS(ACKPKT ap,int relay)
{
    time_t ltime; 
    ltime=time(NULL);
    printf(" SERVER | S | %ld | %d | ACK | %d | CLIENT  | RELAY %d \n", ltime,ACKPKT_SIZE, ap.seq_no, relay);
}

void sendAck(DATAPKT* dp, int sockfd, struct sockaddr_in *r1, struct sockaddr_in *r2, int rcv_len)
{
    ACKPKT ap;
    ap.seq_no = dp->seq_no;
    int channel = rand()%2;
    if(channel==1)
    sendto(sockfd, &ap, ACKPKT_SIZE, 0, (struct sockaddr *)r1, rcv_len);
    else
    sendto(sockfd, &ap, ACKPKT_SIZE, 0, (struct sockaddr *)r2, rcv_len);
  
   // printf("ACK sent corresponding to %d \n", ap.seq_no);
    sendLOGS(ap, channel);
}

void savePkt(FILE** fp, DATAPKT* pkt)
{
   // printf("seq_no %d----------------------------------------------------------BYTES received \n", pkt->seq_no);
    fseek(*fp, pkt->seq_no, SEEK_SET);
    //printf("%s\n", pkt.Payload);
    fwrite(pkt->payload, 1, pkt->pay_size, *fp);
    //printf("file written successful\n");
    totalBytesRead+=pkt->pay_size;
    if(pkt->isLast)
    {
        lastByte = pkt->seq_no + pkt->pay_size;
    }
    return;
}

void write_to_file(int start, int end, char BUFF[], FILE** fp)
{
    fseek(*fp, start, SEEK_SET);
    //printf("%s\n", pkt.Payload);
    fwrite(BUFF, 1, end-start , *fp);
    //printf("file written successful\n");
    totalBytesRead+=end-start;
   
    return;
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

void deleteNode(DATAPKT** front, DATAPKT** back, int SEQ)
{
    if(front==NULL)
      return;
    DATAPKT* prev;
    DATAPKT* temp = *front;
    if((*front)->seq_no == SEQ)
    {
        (*front) = (*front)->next;
        if((*front)==NULL)
        (*back)=NULL;
        return;
    }
  
    while(temp && temp->seq_no!=SEQ)
    {
        prev = temp;
        temp=temp->next;
    }
  
    if(temp==NULL)
      return;
    prev->next = temp->next;
    free(temp);    
}

void pop(DATAPKT** front, DATAPKT** back)
{
    if(*front==NULL)
    return ;

    DATAPKT* temp = *front;
    (*front) = (*front)->next;
    if((*front)==NULL)
      (*back)=NULL;
  
    free(temp);
    //printf("Element Popped\n");
}

void swapNodes(DATAPKT** head_ref, DATAPKT* currX, DATAPKT* currY, DATAPKT* prevY) 
{ 
    *head_ref = currY; 
    prevY->next = currX; 
  
    DATAPKT* temp = currY->next; 
    currY->next = currX->next; 
    currX->next = temp; 
} 

DATAPKT*recurSelectionSort(DATAPKT* head) 
{ 
    if (head->next == NULL) 
        return head; 
  
    DATAPKT* min = head; 

    DATAPKT* beforeMin = NULL; 
    DATAPKT* ptr; 
  
    for (ptr = head; ptr->next != NULL; ptr = ptr->next) 
    { 
          if (ptr->next->seq_no < min->seq_no) 
          { 
              min = ptr->next; 
              beforeMin = ptr; 
          } 
    } 
    if (min != head) 
        swapNodes(&head, head, min, beforeMin); 
  
    head->next = recurSelectionSort(head->next); 
  
    return head; 
} 
  
void sort(DATAPKT** head_ref) 
{ 
    if ((*head_ref) == NULL) 
        return; 
    *head_ref = recurSelectionSort(*head_ref); 
}


DATAPKT* convertPKT(SNDPKT p)
{
    DATAPKT* dp = (DATAPKT*)malloc(sizeof(DATAPKT));
    dp->next = NULL;
    dp->isACKED=0;
    dp->pkt_no = -1;
    dp->pay_size = p.pay_size;
    dp->seq_no = p.seq_no;
    dp->isLast = p.isLast;
    dp->payload = (char*) malloc(p.pay_size * sizeof(char));
    strncpy(dp->payload, p.payload, p.pay_size);
    return dp;
}

void createFileptr(FILE** ptr)
{
    *ptr = fopen("output.txt", "ab"); 
    fseek(*ptr, 0, SEEK_SET);
  
    if(*ptr==NULL)
    {
        printf("Error opening file");
        exit(0);
    }
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

    struct sockaddr_in rcv;
    int rcv_len = sizeof(rcv);
  
    struct sockaddr_in r1;
    struct sockaddr_in r2;
    int ret_len = sizeof(r1);
  
    ConfigureConnection(&r1, 6001);
    ConfigureConnection(&r2, 6002);
  
    struct sockaddr_in thisOne;

    ConfigureConnection(&thisOne, 8001);

    if( bind(sockfd , (struct sockaddr*)&thisOne, sizeof(thisOne) ) == -1)
    {
        printf ("Error Occured while binding\n");
		    exit (0);
    }
  
    DATAPKT* front=NULL;
    DATAPKT* back=NULL;
    
    printf("Listening\n");
    
    FILE* fp;
    createFileptr(&fp);
  
    //char BUFF[PAYLOAD_SIZE*100];
  
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    
    int infi = -1, timeout = 500;
    
        // Waits Indefinitely for First Input
    while(1)
    {
        SNDPKT p;
        poll(fds, 1, infi);
        for(int i=0; i<window ; i++)
        {
            int count=0;
            poll(fds, 1, timeout);
            if((fds[0].revents & POLLIN))
            {
                memset(&p,0,sizeof(SNDPKT));
                if(recvfrom(sockfd, &p, PACKET_SIZE, 0, (struct sockaddr *)&rcv, &rcv_len) >0 )
                {
                    rcvLOGS(p, -1);
                    //printf("receiving seq_no: %d \n", p.seq_no);
                    DATAPKT* dp = convertPKT(p);
                  //strncpy(BUFF+dp->seq_no-savedUPTO, dp->payload, dp->pay_size);  
                    if(dp->isLast)
                    {
                        lastByte = dp->seq_no + dp->pay_size;
                    }
                    push(&front, &back, &dp);  
                    count++; 
                    sort(&front);
                }
            }
        }
      
        int start = savedUPTO;
          
        DATAPKT* temp = front;
        while(temp)
        {          
            if(temp->isACKED==0)
            {
                sendAck(temp, sockfd, &r1, &r2, rcv_len);
                temp->isACKED = 1;
            }
            
            savePkt(&fp, temp);
            pop(&front, &back);
          temp = temp->next;
        }
        if(lastByte == totalBytesRead)
        break;
    }
    printf("Complete File Received");
    return 0;
}