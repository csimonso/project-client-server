#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>

#define MAX_TIMEOUTS 10


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
	unsigned short opcode;
	size_t recv_count  = 0;
	int block_count = 0;

	while(1) {
		recv_count = recvfrom( fd, buffer, DATA_LENGTH + 4, 0,
				(struct sockaddr *) &serv_addr, sizeof(serv_addr) );
		// Error handling

		opcode = (buffer[0] << 8) | (buffer[1] & 0x00FF);
		block_number = (buffer[2] << 8) | (buffer[3] & 0x00FF);

		if( block_number == 01 ) {
			// Read
		} else if( block_number == 02 ) {
			// Write
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