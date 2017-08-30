#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define PORT_NUMBER = 6111
#define DATA_LENGTH  = 512

int timeout() {}

int main(int argc , char *argv[]) {
	struct sockaddr_in myaddr, serv_addr;
	struct hostent *hp;
	unsigned char buffer[REQ_LENGTH + 4];

	// Create a UDP IPv4 socket
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 ) {
		perror("Failed to create socket\n");
		exit();
	}

	// Populate hostent hp by the desired give name (in this case it's localhost)
	hp = gethostnameby("localhost");
	// Copy Internet address that was retrieved via gethostnameby() to serv_addr struct
	memcpy( (void *) &serv_addr.sin_addr, hp->h_addr_list[0], hp->h_length );

	memset( (char *) &myaddr, 0, sizeof(myaddr) );
	myaddr.sin_family = AF_INET
	myaddr.sin_port   = htonl(INADDR_ANY);
	myaddr.sin_addr   = htons(PORT_NUMBER);

	/* Send Request to Server */
	// Connect socket to address specified by the second parameter (Server?)
	int connect_status = connect( fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) );
	if( connect_status == -1 ){ perror("Error on connecting\n"); }

	FILE *from_server;
	char *file_name; // TODO: Specify the name of the file.
	const int REQUEST_LENGTH = (int)( 2 + strlen(file_name) + 1 + 5 + 1 );

	// Request the file from the server
	buffer[0] = 0;
	buffer[1] = 1;
	strcpy( &buffer[2], file_name );
	strcpy( &buffer[2 + strlen(file_name) + 1], "octet" );

	// char TEMP_HEADER[1000] = "TODO: Make the request header for RRQ (Read Request) & WRQ (Write Request)"
	// TODO: Timer
	size_t send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
		(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
	if( send_status == -1 ) { perror("Request failed\n"); }

	/* Receive data after request from server */
	size_t recv_status = 0; // Initialize receive status to 0
	while(1)
	{
		// 
		recv_status = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
			(struct sockaddr *) &serv_addr, sizeof(serv_addr));
		if( recv_status == -1 ) { perror("Receive failed!"); }
	}

	/* Send ACK/NAK to server */
}