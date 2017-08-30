#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

int timeout() {}

// Need to define file_name (probably do it in main's argument?)
int main(int argc , char *argv[]) {
	unsigned short PORT_NUMBER = 6111;
	int DATA_LENGTH = 512;
	int ACK_LENGTH  = 4;

	struct sockaddr_in myaddr, serv_addr;
	struct hostent *hp;
	unsigned char buffer[DATA_LENGTH + 4];

	// Create a UDP IPv4 socket
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 ) {
		perror("Failed to create socket\n");
		exit(2);
	}

	// Populate hostent hp by the desired give name (in this case it's localhost)
	hp = gethostbyname("localhost");
	// Copy Internet address that was retrieved via gethostnameby() to serv_addr struct
	memcpy( (void *) &serv_addr.sin_addr, hp->h_addr_list[0], hp->h_length );

	memset( (char *) &myaddr, 0, sizeof(myaddr) );
	myaddr.sin_family		= AF_INET;
	myaddr.sin_port 		= htons(PORT_NUMBER);
	myaddr.sin_addr.s_addr  = htonl(INADDR_ANY);

	/* Send Request to Server */
	// Connect socket to address specified by the second parameter (Server?)
	int connect_status = connect( fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr) );
	if( connect_status == -1 ){ perror("Error on connecting\n"); }

	// More variable declarations
	FILE *output = NULL;
	char *file_name; // TODO: Specify the name of the file.
	const int REQUEST_LENGTH = (int)( 2 + strlen(file_name) + 1 + 5 + 1 );
	const char *simple_file_name; 
	unsigned short block_number;
	size_t recv_count  = 0;
	int block_count = 0;
	int byte_count = 0;	// Not sure if this is necessary.

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
	while(1)
	{
		// Fetch server's response, and stores the length of the message (data) to variable recv_count.
		recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
			(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
		if( recv_count == -1 ) { perror("Receive failed!"); }

	   /* Not sure if this is necessary.
		* // Make sure it's data
		* op_code = (buffer[0] << 8) | buffer[1];
	    *     if( op_code == 5 ) {
	    *         printf( "Error from server: %s\n", &buffer[4] );
	    *         break;  // Do we really want to do this?
	    *     }
		*/

		// Acquire Block Number of Data. Block number is to distinguish between duplicate data.
		// TODO: Figure this out.
		block_number = (buffer[2] << 8) | (buffer[3] & 0x00FF);

		// Strip paths off file name. Be sure the output file is open.
	    if( output == NULL ) {
	        // TODO: This doesn't handle trailing slash characters very well but presumably
	        //       they would cause errors anyway.
	        simple_file_name = strrchr( file_name, '/' );
	        if( simple_file_name == NULL )
	            simple_file_name = file_name;
	        else
	            simple_file_name = simple_file_name + 1;

	        output = fopen( simple_file_name, "w" );
	        if( output == NULL ) {
	            printf( "Unable to open %s (after receiving block #%u)", simple_file_name, block_number );
	        }
		}

		// If there's any message, continually writes to "output"
		if( (recv_count > 4) && (block_number == block_count + 1) )
		{
            block_count++;
            byte_count += (recv_count - 4);
            fwrite( &buffer[4], 1, recv_count - 4, output );
		}

		/* Send ACK/NAK to server */
		buffer[0] = 0;
		buffer[4] = 4;

		send_status = sendto( fd, buffer, ACK_LENGTH, 0,
			(const struct sockaddr *) &serv_addr, sizeof(serv_addr) );
		if (send_status == -1)  { perror("Acknowledgement failure"); }

		// Checks if this is the last packet. If so, break out of the while loop.
		if( recv_count - 4 < 512 )  break;
	}

	// Closing the output file.
	if( output != NULL ) {
        printf( "\n" );
        fclose( output );
	}
}