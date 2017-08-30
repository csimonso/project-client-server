#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define PORT_NUMBER = 6111

int send_file(const char * file_name, socket socket_handle, const struct sockaddr_in * serveraddr)
{
	FILE
}

int main(int argc , char *argv[])
{
	int fd = socket(AF_INET, OCK_DGRAM, 0);
	if ( fd < 0 ) {
		perror("Failed to create socket\n");
		exit();
	}
	
	struct sockaddr_in myaddr;
	struct hostent * hp;
	hp = gethostbyname("localhost");
	memcpy((void*) &serveraddr.sin_addr);

	// Binds socket address to the address & check if binding is successful
	if( bind(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 )
		perror("Binding failed\n");

	char msg[] = "Hello";
	int x = sendto(fd, msg, strlen(msg), 0, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
}