/* udp_client.c */ 
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2021
 * Modified by:
 * Justin Henke
 * Eli Haberman-Rivera
 * 10/20/2021 */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, sendto, and recvfrom */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */
#include <arpa/inet.h>      /* for htons and such */
#include <time.h>           /* to seed rng generator */

#define STRING_SIZE 1024

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

   int sock_client;  /* Socket used by client */ 

   struct sockaddr_in client_addr;  /* Internet address structure that
                                        stores client address */
   unsigned short client_port;  /* Port number used by client (local port) */

   struct sockaddr_in server_addr;  /* Internet address structure that
                                        stores server address */
   struct hostent * server_hp;      /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]; /* Server's hostname */
   unsigned short server_port;  /* Port number used by server (remote port) */

   int bytes_sent=0, bytes_recd=0, packets_recd; /* number of bytes sent or received */
  
   /* open a socket */

   if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
      perror("Client: can't open datagram socket\n");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port.
            The local address initialization and binding is done automatically
            when the sendto function is called later, if the socket has not
            already been bound. 
            The code below illustrates how to initialize and bind to a
            specific local port, if that is desired. */

   /* initialize client address information */

   client_port = 0;   /* This allows choice of any available local port */

   /* Uncomment the lines below if you want to specify a particular 
             local port: */
   /*
   printf("Enter port number for client: ");
   scanf("%hu", &client_port);
   */

   /* clear client address structure and initialize with client address */
   memset(&client_addr, 0, sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
                                        any host interface, if more than one 
                                        are present */
   client_addr.sin_port = htons(client_port);

   /* bind the socket to the local client port */

   if (bind(sock_client, (struct sockaddr *) &client_addr,
                                    sizeof (client_addr)) < 0) {
      perror("Client: can't bind to local address\n");
      close(sock_client);
      exit(1);
   }

   /* end of local address initialization and binding */

   /* initialize server address information */
   do {

   	printf("Enter hostname of server: ");
   	scanf("%s", server_hostname);
	   if ((server_hp = gethostbyname(server_hostname)) == NULL) {
      		perror("Client: invalid server hostname\n");
   	   }
   }while((server_hp = gethostbyname(server_hostname)) == NULL); //invalid hostname

   long int temp_server_port; //temp var for checking server port validity
   do {
	
   	printf("Enter port number for server: ");
   	scanf("%lu", &temp_server_port);
	if (temp_server_port <= 1 || temp_server_port >65535)
	printf("ERROR: Enter a valid port number between 1 and 65,535\n");
   }while(temp_server_port <= 1 || temp_server_port >65535);//if port number is out of range, prompt user again

   server_port = temp_server_port;
   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
                                    server_hp->h_length);
   server_addr.sin_port = htons(server_port);

   //initialize vars
   int confirm = 1;				//loop for more requests
   struct RequestPacket requestPacket;		//initialize requestPacket
   struct ResponsePacket responsePacket;	//initialize responsePacket
   unsigned short int totalOfSeqNums;		//sequence checksum
   unsigned long  int totalPayloadChecksum;	//payload checksum
   char answer[STRING_SIZE];			//for user input at end of loop
   requestPacket.ID = 1;			//set ID of first request to 1
   do {
	//reset vars to default values for new request
	totalOfSeqNums = 0;
	totalPayloadChecksum = 0;
	packets_recd = 0;
	bytes_recd = 0;
	bytes_sent = 0;
	long int tempUserInput= 0;	
	/* check user input for count of numbers*/
	do {
   		printf("How many randomly generated numbers would you like to receive from the server?"
		       "\nEnter number between 1 and 65,535: ");
		scanf("%ld", &tempUserInput);
		fflush(stdin);
	}while (tempUserInput < 1 || tempUserInput > 65535); //while user input is garbage

	//set request packet information in network safe htons form 
	requestPacket.count = htons(tempUserInput);
	int tempID = requestPacket.ID;
	requestPacket.ID = htons(tempID);
  	int temp_bytes_sent;

	//attempt to send the packet
   	temp_bytes_sent = sendto(sock_client, &requestPacket, sizeof(requestPacket), 0,
            (struct sockaddr *) &server_addr, sizeof (server_addr));
	if (temp_bytes_sent < 1){ //error, don't save bytes sent, jump to new request
		printf("ERROR in sending packet\n");
		continue;
	}
	bytes_sent += temp_bytes_sent; //success!

   	/* get response from server */
  	int temp_bytes_recd = 0;
   	printf("Waiting for response from server...\n");
	do {
		//accept packets while last!=1
   		temp_bytes_recd = recvfrom(sock_client, &responsePacket, sizeof(responsePacket), 0,
        		(struct sockaddr *) 0, (int *) 0);
		if (temp_bytes_recd < 1){//error
			printf("ERROR in received packet\n");
			continue;
		}
		//throw out packet that doesn't match ID of request
		if(ntohs(responsePacket.ID) != ntohs(requestPacket.ID))
			continue;
		else{
			bytes_recd+=temp_bytes_recd; //Only count data tied to successful request
			packets_recd++;
			for(int i = 0; i < ntohs(responsePacket.count); i++){ //extract payload
				totalPayloadChecksum+=ntohl(responsePacket.payload[i]); //compute checksum
				//printf("%d: %lu\n",i, responsePacket.payload[i]);
			}
			totalOfSeqNums+=ntohs(responsePacket.seqNum); //compute checksum of seq nums
		}

	}while(ntohs(responsePacket.Last) == 0); //loop back around while not last packet
	//Fully aware that this can and does cause a livelock with large request sizes, 
	//where the last packet is lost and the client will forever wait for the
	//mystical 'Last' packet that never will arrive.
	//This will be fixed in the TCP version in the future!

	//print summary information
	printf( "All packet(s) received.\n"
		"———————Data Summary———————\n"
		"Request ID: %u\n"
		"Request count: %u\n"
		"Total number of packets received: %d\n"
		"Total number of bytes received: %d\n"
		"Sum of Sequence Numbers: %u\n"
		"Sum of Checksums: %lu\n\n",
		ntohs(requestPacket.ID),
		ntohs(requestPacket.count),
		packets_recd, bytes_recd,
		totalOfSeqNums, totalPayloadChecksum);

   	do { //ask user if they would like to send another request to the server

    		printf("Would you like to send another request to the server?"
	  	       "\nEnter 'y' or 'yes' to confirm, 'n' or 'no' to exit: ");
       
    		scanf("%s", answer);
		fflush(stdin);
   	}while(strcmp(answer, "y") != 0 && 
			strcmp(answer, "n") != 0 && 
			strcmp(answer, "yes") != 0 && 
			strcmp(answer, "no") != 0);

	//if user wants to exit
	if (strcmp(answer, "n") == 0 || strcmp(answer, "no") == 0)
		confirm = 0;
	else {
		//continue!
		unsigned short int temp = requestPacket.ID;
		requestPacket.ID = ntohs(temp);
		requestPacket.ID++;//increment requestPacket's ID in host byte order (will fix on next loop)
	}

   }while(confirm ==1);

   close (sock_client);//close socket when finished
}
