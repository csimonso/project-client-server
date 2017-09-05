#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

#define MAX_TIMEOUTS 10

// TODO:
// 1. Duplicate Messages (beefy I think)
// 2. Timeout for server (related to point #1)
// 3. Simple filename

int timeout = 0;

int time_out(int sig) {
	/* switch(sig) {
		case SIGALRM: {
			timeout++;
			if (timeout >= MAX_TIMEOUTS) {
				timeout = 0;
				alarm(0);
				// longjmp(endbuf, sig);
			}
			// longjmp(timeoutbuf, sig);
		} break;
		case SIGINT: {
			timeout = 0;
			alarm(0);
			// longjmp(endbuf, sig);
		} break;
		default: break;
	} */
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

	// TODO: Populate client_addr.
	struct sockaddr_in client_addr, serv_addr;
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

	/* Maybe not necessary (changed from myaddr.something to serv_addr.something) */
	/// memset( (char *) &myaddr, 0, sizeof(myaddr) );
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_port 			= htons(PORT_NUMBER);
	serv_addr.sin_addr.s_addr  	= htonl(INADDR_ANY);

	// More variable declarations
	FILE *output = NULL;
	// const int REQUEST_LENGTH = (int)( 2 + strlen(file_name) + 1 + 5 + 1 );
	unsigned short block_number = 0;
	unsigned short opcode;
	size_t recv_count  = 0;
	int block_count = 1;
	char file_name[512];
	int first_time_write = 1;
	size_t send_status;

	while(1) {
		// Receive Requests, ACK (in case of Read) or Data (in case of Write) from Client.
		recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in) );
		if( recv_count == -1 ) { perror("Receive failed!"); }

		// Extract opcode to determine if buffer contains Requests/ACK/Data (refer desc. above)
		opcode = (buffer[0] << 8) | (buffer[1] & 0x00FF);

		// If opcode is RRQ.
		if( opcode == 1 || opcode == 3 ) {
			// If RRQ has JUST been received, open up the file.
			if( opcode == 1 ) {
				// Extract filename from buffer.
				for(int i = 0; buffer[2 + i] != 0; i++) {
					// TODO: Make file_name simple (?)
					// Might be problematic: casting the buffer for strcat
					strcat(file_name, buffer[2 + i]);
				}

				// Open the file specified by file_name
				output = fopen( file_name, "r" );
				// Takes care of opening failure (prolly because file does not exist?)
		        if( output == NULL ) {
		            printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
		        }
		    }

	        // Make buffer all zero
	        memset(buffer, 516, sizeof(buffer));

	        // Populates buffer with header for Data (including opcode & block #)
	        buffer[0] = 0;
	        buffer[1] = 3;
	        if(buffer[3] != 0xF) {
	        	buffer[2] = buffer[2] | 0x1;
	        } else {
	        	buffer[3] = buffer[3] | 0x1;
	        }

	        // Copies 512 bytes chunks of entries from file (in server) to buffer
	        fread( &buffer[4], 1, 512, output );

	        // Sends data to client
	        // TODO: Client address
	        send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in) );
	        if( send_status == -1 ) { perror("Failed to send data\n"); }

	        // TODO: Possible terminating condition.
		} else if( opcode == 2 || opcode == 4 ) {
			// If Client just sent a WRQ, simply sends ACK #0
			if( first_time_write == 1 ) {
				first_time_write = 0;
				// Set up ACK #0 to be sent.
				buffer[0] = 0;
		        buffer[1] = 4;
		        if(buffer[3] != 0xF) {
		        	buffer[2] = buffer[2] & 0x0;
		        } else {
		        	buffer[3] = buffer[3] & 0x0;
		        }

		        // Send ACK #0 to Client to let it know, it can start transmitting file.
		        send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
					(struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in) );
		        if( send_status == -1 ) { perror("Failed to send ACK 0\n"); }
			} else {
		        // Opens up a file name to "save" whatever Client wishes to store to Server.
		        // If file does not exist, fopen will create one (since mode = w).
		        output = fopen( file_name, "w" );
		        if( output == NULL ) {
		            printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
		        }

		        // Write entries/data in buffer to the file that was opened earlier.
		        fwrite( &buffer[4], 1, 512, output );
		        
		        // Zero out the buffer before proceeding.
		        memset(buffer, 516, sizeof(buffer));

		        // Send ACK number accordingly.
		        buffer[0] = 0;
		        buffer[1] = 3;
		        if(buffer[3] != 0xF) {
		        	buffer[2] = buffer[2] | 0x1;
		        } else {
		        	buffer[3] = buffer[3] | 0x1;
		        }

		        // Send ACK after data is written to "output" file.
		        send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
					(struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in) );
		        if( send_status == -1 ) { perror("Failed to send data\n"); }
		    }

	        // TODO: Possible terminating condition.
		} else {
			perror("Unrecognized Opcode\n");
			break;
		}
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