/* udp_server.c */
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2021
 * Modified by:
 * Justin Henke
 * Eli Haberman-Rivera
 * 10/20/2021 */

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <arpa/inet.h>	    /* for htons and such */
#include <time.h>	    /* to seed rng generator */
#define STRING_SIZE 1024

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 8421

struct RequestPacket {
	unsigned short int ID;
	unsigned short int count;
}request_packet;

struct ResponsePacket {
	unsigned short int ID;
	unsigned short int seqNum;
	unsigned short int Last;
	unsigned short int count;
	unsigned long  int payload[25];
}response_packet;
	
int main(void) {

   srand(time(NULL)); /* seed rand function */

   int sock_server;  /* Socket on which server listens to clients */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   unsigned short server_port;  /* Port number used by server (local port) */

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;  /* Length of client address structure */

   int bytes_sent, packets_sent, bytes_recd; /* number of bytes sent or received */
   unsigned int i; /* temporary loop variable */
   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Server: can't open datagram socket\n");
      exit(1);
   }

   /* initialize server address information */

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_UDP_PORT; /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *) &server_addr,
                                    sizeof (server_addr)) < 0) {
      perror("Server: can't bind to local address\n");
      close(sock_server);
      exit(1);
   }

   /* wait for incoming messages in an indefinite loop */

   printf("Waiting for incoming messages on port %hu\n\n", 
                           server_port);

   client_addr_len = sizeof (client_addr);


   //initialize vars
   struct RequestPacket requestPacket;		//requestPacket struct
   struct ResponsePacket responsePacket;	//responsePacket struct
   responsePacket.Last = htons(0);		//set Last to 0
   int sentRandCount;				//how many rand numbers have we sent?
   int lastPacket = 0;				//local variable so we don't have to ntohs response.Last
   unsigned short int totalOfSeqNums;		//checksum for seqnums
   bytes_sent = 0;				//how many bytes have we sent?
   unsigned long  int totalPayloadChecksum;	//checksum for payload
   for(;;) {
      //reset variables for next response
      bytes_sent = 0;
      bytes_recd = 0;
      totalOfSeqNums = 0;
      totalPayloadChecksum = 0;
      sentRandCount = 0;
      packets_sent = 0;

      //wait for and receive request from client
      int temp_bytes_recd = recvfrom(sock_server, &requestPacket, sizeof(requestPacket), 0,
                     (struct sockaddr *) &client_addr, &client_addr_len);
      if (temp_bytes_recd < 1){ //ERROR, throw out packet
	      printf("ERROR: bad packet reception.");
	      continue;//loop back around to next request
      }
      //else continue
      bytes_recd += temp_bytes_recd;
	
      //set response ID to request ID
      responsePacket.ID = htons(ntohs(requestPacket.ID));

      do {
	      packets_sent++; //increment sent packet counter
	      /*load up payload with randomly generated longs*/
	      for (i=0; i<25 && sentRandCount < ntohs(requestPacket.count); i++){
		      //this function generates a random 32bit number, as rand isn't guaranteed to be 32 bit
			unsigned long int random = 
			(unsigned long int)(((uint32_t) rand() <<  0) & 0x000000000000FFFFull) |
			                   (((uint32_t) rand() << 16) & 0x00000000FFFF0000ull);

        	 responsePacket.payload[i] = htonl(random);
		 //printf("%d: %lu\n",i, responsePacket.payload[i]);
		 totalPayloadChecksum += random;
		 sentRandCount++;
	      }	
	      //flush buffer payload
	      if (i < 25){
		      for (int k = i;k < 25;k++)
			      responsePacket.payload[k] = htonl(0);
	      }
	      
	      //decide if last packet or not
	      if (sentRandCount < ntohs(requestPacket.count))
		 responsePacket.Last = htons(0);
	      else {
	      	 responsePacket.Last = htons(1);
		 lastPacket = 1;
	      }
	      //set count and seqNum
 	      responsePacket.seqNum = htons(packets_sent);
	      responsePacket.count = htons(i);
	      totalOfSeqNums+=packets_sent;
	      int temp_bytes_sent; //temp var for setting sent data
	      temp_bytes_sent = sendto(sock_server, &responsePacket, sizeof(responsePacket), 0,
                	(struct sockaddr*) &client_addr, client_addr_len);

	      if (temp_bytes_sent < 1) //error! don't count the data as being sent
	      	printf("ERROR: error in sending packet.");
	      else {
		bytes_sent+=temp_bytes_sent; //save sent byte count to counter var
	      }

      }while(sentRandCount < ntohs(requestPacket.count) || lastPacket == 0);//while not last packet essentially

      //print summary data
      printf( "All packet(s) received.\n"
	      "———————Data Summary———————\n"
	      "Request ID: %u\n"
	      "Request count: %u\n"
	      "Total number of packets transmitted: %d\n"
	      "Total number of bytes transmitted: %d\n"
	      "Sum of Sequence Numbers: %u\n"
	      "Sum of Checksums: %lu\n\n",
	      ntohs(requestPacket.ID),
	      ntohs(requestPacket.count),
	      packets_sent, bytes_sent,
	      totalOfSeqNums, totalPayloadChecksum);

   }
}
