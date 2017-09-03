#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT_NUMBER 6111

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5

#define MAXSIZE 512
#define BUFSIZE 1024
#define NAMESIZE 128

int main(int argc, char *argv[]){
    
    int nBytes, socketfd;
    char buffer[BUFSIZE], filename[NAMESIZE], mode[NAMESIZE], opcode, *bufindex;
    struct sockaddr_in myaddr, serveraddr;
    
    /* Create a UDP socket, return the File Descriptor */
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(socketfd < 0) {
        perror("ERROR");
        exit();
    }
    
    /* From Discussion Session */
    memset((char*)&serveraddr,0,sizeof(serveraddr));//Discussion
    serveraddr.sin_family = AF_INET;//Discussion
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//Discussion
    serveraddr.sin_port = htons(PORT_NUMBER);//Discussion
    
    /* Assigns a local socket address to the socket FD */
    int x = bind(socketfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));//Discussion
    
    memset(buffer,0,BUFSIZE);
    
    /* Loop in case of other requests */
    while(1) {
        
        /* Receives the message request */
        nBytes = RecvFrom(socketfd,buffer,BUFSIZE,0,&clientAddr,addr_size);
        
        /* Set the buffer index */
        bufindex = buffer;
        /* Increment the index to begin parsing */
        bufindex++;
        
        /* Get the opcode */
        //op_code = (buffer[0] << 8) | buffer[1];
        opcode = *bufindex++;
        
        /* Get the filename */
        strncpy(filename,bufindex,sizeof(filename)-1);
        
        /* Get the mode */
        strncpy(mode,bufindex,sizeof(mode)-1);
        
        if(op_code == OP_RRQ) {
            send(filename,mode, myaddr);
        }    
    }
    return 0;
}
/* STILL NEED TO FIGURE OUT HOW TO MAKE THIS FUNCTION */
void send (char* filename, char* mode, struct sockaddr_in myaddr) {
    
}
