#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <setjmp.h>

#define MAX_TIMEOUTS 3
#define ALARM_TIME 2


int timeout = 0;
jmp_buf timeoutbuf, endbuf;

int time_out(int sig) {
	switch(sig) {
		case SIGALRM: {
			timeout++;
			if (timeout >= MAX_TIMEOUTS) {
				exit(0);
			}
			longjmp(timeoutbuf, sig);
		} break;
		//case SIGINT: {
		//	timeout = 0;
		//	alarm(0);
			// longjmp(endbuf, sig);
		//} break;
		default: break;
	}
}

/*
 * ARGUMENTS
 * 1 --> "-r" or "-w" to indicate read/write
 * 2 --> Filename
 */
int main(int argc , char *argv[]) {
	unsigned short PORT_NUMBER = 6111;
	int DATA_LENGTH = 512;
	int ACK_LENGTH  = 4;
	char *file_name = argv[2];

	struct sockaddr_in myaddr, serv_addr;
	struct hostent *hp;
	unsigned char buffer[DATA_LENGTH + 4];

	// Create a UDP IPv4 socket
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 ) {
		perror("Failed to create socket\n");
		exit(2);
	}

	if(file_name == NULL) {
		perror("Please provide the name of the file.");
		exit(2);
	}

	// Populate hostent hp by the desired give name (in this case it's localhost)
	hp = gethostbyname("localhost");
	// Copy Internet address that was retrieved via gethostnameby() to serv_addr struct
	memcpy( (void *) &serv_addr.sin_addr, hp->h_addr_list[0], hp->h_length );

	/* Maybe not necessary (changed from myaddr.something to serv_addr.something) */
	/// memset( (char *) &myaddr, 0, sizeof(myaddr) );
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_port 			= htons(PORT_NUMBER);
	serv_addr.sin_addr.s_addr  	= htonl(INADDR_ANY);

	// More variable declarations
	FILE *output = NULL;
	const int REQUEST_LENGTH = (int)( 2 + strlen(file_name) + 1 + 5 + 1 );
	unsigned short block_number;
	size_t recv_count  = 0;
	int block_count = 0;

	if( argv[1] == "-r" ) {
		/* Send Request to Server */
		// Request the file from the server
		buffer[0] = 0;
		buffer[1] = 1;
		strcpy( &buffer[2], file_name );
		strcpy( &buffer[2 + strlen(file_name) + 1], "octet" );
		
		// Attach the signal handler to timer method
		signal(SIGALRM, timer);

		/* Receive data after request from server */
		while(1)
		{
			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);	  
			
			// Send RRQ to server
			size_t send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if( send_status == -1 ) { perror("Read Request failed\n"); }

			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);
			
			// Fetch server's response, and stores the length of the message (data) to variable recv_count.
			recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if( recv_count == -1 ) { perror("Receive failed!"); }

			// Acquire Block Number of Data. Block number is to distinguish between duplicate data.
			// TODO: Figure this out.
			block_number = (buffer[2] << 8) | (buffer[3] & 0x00FF);

			// Opens up file specified by file_name to be WRITTEN.
	        output = fopen( file_name, "w" );
	        if( output == NULL ) {
	            printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
	        }

			// If no error occured during receive,
			if( (recv_count > 4) && (block_number == block_count + 1) )
			{
	            block_count++;
	            // Write data from buffer to output.
	            fwrite( &buffer[4], 1, recv_count - 4, output );
			}

			/* Send ACK/NAK to server */
			// Set ACK OpCode
			buffer[0] = 0;
			buffer[1] = 4;

			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);
			
			// Send ACK to indicate client received the file.
			send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
				(const struct sockaddr *) &serv_addr, sizeof(serv_addr) );
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if (send_status == -1)  { perror("Acknowledgement failure"); }

			// Checks if this is the last packet. If so, break out of the while loop.
			if( recv_count - 4 < 512 )  break;
		}
	} else if ( argv[1] == "-w" ) {
		// Initiate header for write
		buffer[0] = 0;
		buffer[1] = 2;
		strcpy( &buffer[2], file_name );
		strcpy( &buffer[2 + strlen(file_name) + 1], "octet" );
		
		// Attach the signal handler to timer method
		signal(SIGALRM, timer);

		// Set jump for timeout
		sigsetjmp(timeoutbuf, 1); 
		// Start the alarm
		alarm(ALARM_TIME);
		
		// Send Write Request
		size_t send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
			(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
		
		// Reset the alarm and timeout count
		timeout = 0;
		alarm(0);
		
		if( send_status == -1 ) { perror("Write Request failed\n"); }

		// Opens up file specified by file_name to be READ.
        output = fopen( file_name, "r" );
        if( output == NULL ) {
            printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
        }

		while(1) {
			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);
			
			// Received ACK to write from server
			// Might not be the correct implementation
			recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if( recv_count == -1 ) { perror("Receive failed!"); break; }
		
			block_number = (buffer[2] << 8) | (buffer[3] & 0x00FF);

			// If no error occured, and block_number is valid,
			if( (recv_count > 4) && (block_number == block_count + 1) )
			{
	            block_count++;
	            // Proceed to transfer chunk(s) of file into buffer
	            fread( &buffer[4], 1, DATA_LENGTH, output );
			}

			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);
			
			// Send chunk to server.
			send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
				(const struct sockaddr *) &serv_addr, sizeof(serv_addr) );
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if( send_status == -1 ) { perror("Failed to send data\n"); } // Not sure if necessary

			// If length of the file is < 512, break.
			if(strlen((const char *) buffer[4]) < 512) break;
		}
	} else {
		// If option is invalid (i.e. -t), print out Invalid Option with list of valid options.
		printf("\nInvalid Option. Type -r to read file or -w to write file\n");
	}

	// Closing the output file.
	if( output != NULL ) {
        printf( "\n" );
        fclose( output );
	}

	// Close the socket after the loop is over.
	// https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket
	shutdown(fd, SHUT_RDWR);
}
