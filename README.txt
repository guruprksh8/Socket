Its an implementation of sliding window protocol and insures reliability over unreliable UDP channels. This small network comprises of a server, a client and two two relays.

INSTRUCTIONS FOR COMPILING :

Type ./r1, ./r2, ./s followed by ./client on terminal to run.
If it gives error, recompile using gcc -o name name_of_file. (did it on an online ubuntu 16 container so better to recompile locally)

*******************************************

Have used three types of packet structure : ACKPKT, DATAPKT, SNDPKT as DATAPKT contains pointers necessary for maintaining a queue which can be send over network (bcoz of pointers)
so they are converted into SNDPKT before sending over network. ACKPKT are acknowlegement packet.

CLIENT: 

Window size is kept at 8 (specified as macro). 
Used a queue for maintaining a sliding window. 
Packets are send and pushed into queue.
When acks arrive, corresponding packets are marked by isACKED integer in the queue itself.
If acked packets are at the front of queue, they are popped and hence window slides.
poll() is used to make recvfrom unblocking.

SERVER:

Window size is again kept at 8.
When packets arrive, they are pushed into queue. They are then sorted as per seq no. and written to file.

RELAY1 and RELAY2:

They simply forward packets, by giving delay and packet drop is implemented using rand() function. 
They sometimes receive empty packets and forwards them (couldn't fix it) and overwhelm client or server causing them to fail. 
  