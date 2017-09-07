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

extern int errno;

int timer(int sig) {
	switch(sig) {
		case SIGALRM: {
			timeout++;
			if (timeout >= MAX_TIMEOUTS) {
				printf("Timeout!!!\n");
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
	int test = 0;
	unsigned short PORT_NUMBER = 6111; //54321;
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
		exit(1);
	}

	if(file_name == NULL) {
		perror("Please provide the name of the file.");
		exit(1);
	}

	memset( (char *) &serv_addr, 0, sizeof(serv_addr) );

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
	unsigned short block_number;
	size_t recv_count  = 0;
	int block_count = 0;
	socklen_t len = sizeof(serv_addr);

	if( strcmp(argv[1], "-r") == 0 ) {
		/* Send Request to Server */
		printf("Initiating Read Request to server\n");

		// Request the file from the server
		buffer[0] = 0;
		buffer[1] = 1;
		strcpy( &buffer[2], file_name );
		strcpy( &buffer[2 + strlen(file_name) + 1], "octet" );
		
		// Attach the signal handler to timer method
		signal(SIGALRM, timer);

		// Send RRQ to server
		size_t send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
			(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
		if( send_status == -1 ) { perror("Read Request failed\n"); }

		printf("Sending RRQ to server...\n");
		/*while( i < 20 ) {
			printf("%d", buffer[i]);
			i++;
		}*/

		// Opens up file specified by file_name to be WRITTEN.
		// Might be at the top.
        output = fopen( file_name, "w" );
        if( output == NULL ) {
            // printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
            printf( "Unable to open" );
            return;
        }
        printf("File is opened!\n");

		/* Receive data after request from server */
		while(1)
		{
			// Set jump for timeout
			// sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			// alarm(ALARM_TIME);	  
			
			// Reset the alarm and timeout count
			// timeout = 0;
			// alarm(0);
			// Set jump for timeout
			// sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			// alarm(ALARM_TIME);
			
			printf("Receiving Data from server...\n");
			// Fetch server's response, and stores the length of the message (data) to variable recv_count.
			recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, &len );
			// TODO: length needs to be readjusted to prevent infinite loop
			printf("Received Data from server! Size is %u\n", recv_count - 4);
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if( recv_count == -1 ) { perror("Receive failed!"); }

			// Acquire Block Number of Data. Block number is to distinguish between duplicate data.
			block_number = (buffer[2] << 8) | (buffer[3] & 0x00FF);
			printf("Block Number of data: %d\n", block_number);

			// If no error occured during receive,
			if( recv_count > 4 )
			{
	            block_count++;
	            // Write data from buffer to output.
	            printf("Writing to output file...\n");
	            fwrite( &buffer[4], 1, recv_count - 4, output );
			}

			/* Send ACK/NAK to server */
			// Set ACK OpCode
			buffer[0] = 0;
			buffer[1] = 4;
			if(buffer[3] != 0xF) {
	        	buffer[2] = buffer[2] | 0x1;
	        } else {
	        	buffer[3] = buffer[3] | 0x1;
	        }

			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);
			
			// Send ACK to indicate client received the file.
			send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
				(const struct sockaddr *) &serv_addr, sizeof(serv_addr) );
			printf("Sent ACK to server\n");
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if (send_status == -1)  { perror("Acknowledgement failure"); }

			// Checks if this is the last packet. If so, break out of the while loop.
			if( recv_count - 4 < 512 ) { printf("Done!\n"); break; }
		}
	} else if ( strcmp(argv[1], "-w") == 0 ) {
		bzero(buffer, 516);

		// Initiate header for write
		buffer[0] = 0;
		buffer[1] = 2;
		strcpy( &buffer[2], file_name );
		strcpy( &buffer[2 + strlen(file_name) + 1], "octet" );
		int j = 0;
		
		// Attach the signal handler to timer method
		signal(SIGALRM, timer);

		// Set jump for timeout
		sigsetjmp(timeoutbuf, 1); 
		// Start the alarm
		alarm(ALARM_TIME);
		
		// Send Write Request
		size_t send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
			(struct sockaddr *) &serv_addr, len );
		printf("Sent WRQ to server\n");
		bzero(buffer, 516);
		
		// Reset the alarm and timeout count
		timeout = 0;
		alarm(0);
		
		if( send_status == -1 ) { perror("Write Request failed\n"); }

		// Opens up file specified by file_name to be READ.
        output = fopen( file_name, "r" );
        if( output == NULL ) {
            printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
        }
        printf("Opened file\n");

		while(1) {
			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);
			
			// Received ACK to write from server
			// Might not be the correct implementation
			recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, &len );
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if( recv_count == -1 ) { perror("Receive failed!"); break; }
			printf("Received ACK from server!\n");
		
			block_number = (buffer[2] << 8) | (buffer[3] & 0x00FF);

			// If no error occured, and block_number is valid,
			// if( block_number == block_count + 1 )
			// {
	            block_count++;
	            //Proceed to transfer chunk(s) of file into buffer
	            fread( &buffer[4], 1, 512, output );
	            printf("fread()!\n");
			// }

			// Set jump for timeout
			sigsetjmp(timeoutbuf, 1); 
			// Start the alarm
			alarm(ALARM_TIME);

			buffer[0] = 0;
			buffer[1] = 3;
			if(buffer[3] != 0xF) {
	        	buffer[2] = buffer[2] | 0x1;
	        } else {
	        	buffer[3] = buffer[3] | 0x1;
	        }
			
			// Send chunk to server.
			send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
				(const struct sockaddr *) &serv_addr, len );
			
			// Reset the alarm and timeout count
			timeout = 0;
			alarm(0);
			
			if( send_status == -1 ) { perror("Failed to send data\n"); } // Not sure if necessary
			printf("Sent chunk\n");

			// If length of the file is < 512, break.
			if(send_status < 512) break; // causes segfault
		}
		printf("Exiting while(1) on write.\n");
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
