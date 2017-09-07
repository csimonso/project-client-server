#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>

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
 * Extract filename/file path:
 * ORIGINAL PATH: ~/home/document/proj/file_that_we_want.txt
 * AFTER CALL   : /file_that_we_want.txt
 * TODO: In main(), append return value with either client/server's test file directory.
 */
const char * simplify_name(const char * filepath) {
	const char ch = '/';
	return strrchr(filepath, ch);
}

/*
 * ARGUMENTS
 * 1 --> "-r" or "-w" to indicate read/write
 * 2 --> Filename
 */
int main(int argc , char *argv[]) {
	printf("Starting Server now!\n");

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

	memset(&serv_addr, 0, sizeof(serv_addr));

	// Populate hostent hp by the desired give name (in this case it's localhost)
	hp = gethostbyname("localhost");
	// Copy Internet address that was retrieved via gethostnameby() to serv_addr struct
	memcpy( (void *) &serv_addr.sin_addr, hp->h_addr_list[0], hp->h_length );

	/* Maybe not necessary (changed from myaddr.something to serv_addr.something) */
	/// memset( (char *) &myaddr, 0, sizeof(myaddr) );
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_port 			= htons(PORT_NUMBER);
	// Why replace memcpy???
	// serv_addr.sin_addr.s_addr  	= htonl(INADDR_ANY);

	// Bind socket with server's address.
	int bind_stat = bind( fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr) );
	if( bind_stat < 0 ) {
		perror("Binding Error!\n");
		exit(1);
	}

	// More variable declarations
	FILE *output = NULL;
	unsigned short block_number = 0;
	unsigned short opcode;
	size_t recv_count  = 0;
	int block_count = 1;
	char file_name[516] = "";
	int first_time_write = 1;
	size_t send_status;
	socklen_t len = sizeof client_addr;
	int filesize = -1;

	while(1) {
		// Receive Requests, ACK (in case of Read) or Data (in case of Write) from Client.
		// Note that recvfrom will populate client_addr.
		bzero(buffer, 516);
		recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &client_addr, &len );
		if( recv_count == -1 ) { printf("Error!\n"); perror("Receive failed!"); }
		printf("Request/packet received!\n");

		// Extract opcode to determine if buffer contains Requests/ACK/Data (refer desc. above)
		opcode = (buffer[0] << 8) | (buffer[1] & 0x00FF);

		// If opcode is RRQ.
		if( opcode == 1 || opcode == 4 ) {
			printf("Read request!\n");
			// If RRQ has JUST been received, open up the file.
			if( opcode == 1 ) {
				printf("RRQ, filename management!\n");
				// Extract filename from buffer.
				int index = 0;
				while(buffer[2 + index] != 0) {
					file_name[index] = buffer[2 + index];
					index++;
				}

				printf("Opening a file....");
				// Open the file specified by file_name
				output = fopen( file_name, "r" );
				// Takes care of opening failure (prolly because file does not exist?)
		        if( output == NULL ) {
		            printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
		        }
		        printf("File is open!\n");
		        // filesize = lseek(fd, 0, SEEK_END);
		        // lseek(fd, 0, SEEK_SET);
		    }

		    printf("memsetting!\n");
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

	        // printf("%u", filesize);
	        // Copies 512 bytes chunks of entries from file (in server) to buffer
	        // if(filesize > 512){
	        	fread( &buffer[4], 1, 512, output );
	        	// filesize = filesize - 512;
	    	// } else {
	    		// fread( &buffer[4], 1, filesize, output );
	    	// }

	        // Sends data to client
	        send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &client_addr, sizeof(struct sockaddr_in) );
	        if( send_status == -1 ) { perror("Failed to send data\n"); } // No need for terminating condition
		} else if( opcode == 2 || opcode == 3 ) {
			printf("Write request\n");
			// If Client just sent a WRQ, simply sends ACK #0
			if( first_time_write == 1 ) {
		        // Copy the file name from buffer to file_name
		        int index = 0;
				while(buffer[2 + index] != 0) {
					file_name[index] = buffer[2 + index];
					index++;
				}

				bzero(buffer, 516);
				first_time_write = 0;
				// Set up ACK #0 to be sent.
				buffer[0] = 0;
		        buffer[1] = 4;
		        if(buffer[3] != 0xF) {
		        	buffer[2] = buffer[2] & 0x0;
		        } else {
		        	buffer[3] = buffer[3] & 0x0;
		        }

		        // Opens up a file name to "save" whatever Client wishes to store to Server.
		        // If file does not exist, fopen will create one (since mode = w).
		        printf("Opening file!\n");
		        output = fopen( file_name, "w" );
		        if( output == NULL ) {
		            printf( "Unable to open %s (after receiving block #%u)", file_name, block_number );
		        }
		        printf("File opened!\n");

		        printf("Sending VERY FIRST ACK!\n");
		        // Send ACK #0 to Client to let it know, it can start transmitting file.
		        send_status = sendto( fd, buffer, DATA_LENGTH + 4, 0,
					(struct sockaddr *) &client_addr, sizeof(struct sockaddr_in) );
		        if( send_status == -1 ) { perror("Failed to send ACK 0\n"); }
			} else {
		        // Write entries/data in buffer to the file that was opened earlier.
		        printf("Writing buffer to file!\n");
		        fwrite( &buffer[4], 1, recv_count - 4, output );
		        printf("File written!\n");
		        
		        // Zero out the buffer before proceeding.
		        printf("Zeroing out buffer....\n");
		        memset(buffer, 516, sizeof(buffer));
		        printf("Zeroed!\n");

		        printf("Making ACK packet....\n");
		        // Send ACK number accordingly.
		        buffer[0] = 0;
		        buffer[1] = 4;
		        if(buffer[3] != 0xF) {
		        	buffer[2] = buffer[2] | 0x1;
		        } else {
		        	buffer[3] = buffer[3] | 0x1;
		        }
		        printf("ACK made\n");

		        printf("Sending ACK.....\n");
		        // Send ACK after data is written to "output" file.
		        send_status = sendto( fd, buffer, 4, 0,
					(struct sockaddr *) &client_addr, sizeof(struct sockaddr_in) );
		        if( send_status == -1 ) { perror("Failed to send data\n"); }
		        printf("ACK sent!!!\n");
		    } // No need for terminating condition.
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