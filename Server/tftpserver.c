#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT_NUMBER 61011

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
    struct sockaddr_in clientaddr, serveraddr;
    
    /* Create a UDP socket, return the File Descriptor */
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    /* Check for Errors */
    if(socketfd < 0) {
        perror("Socket Error");
        exit(errno);
    }
    
    /* From Discussion Session */
    memset((char*)&serveraddr,0,sizeof(serveraddr));//Discussion
    serveraddr.sin_family = AF_INET;//Discussion
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//Discussion
    serveraddr.sin_port = htons(PORT_NUMBER);//Discussion
    
    /* Assigns a local socket address to the socket FD */
    int x = bind(socketfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));//Discussion
    
    /* Checks for errors */
    if(x < 0){
        perro("Binding Error");
        exit(errno);
    }
    
    /* Loop in case of other requests */
    while(1) {
        
        /* Receives the message request */
        nBytes = recvfrom(socketfd,buffer,BUFSIZE,0,(struct sockaddr *)&clientaddr,sizeof(clientaddr));
        
        /* Check for Errors */
        if(nBytes < 0){
            perror("Recvfrom ERROR");
            exit(errno);
        }
        
        /* Set the buffer index */
        bufindex = buffer;
        /* Increment the index to begin parsing */
        bufindex++;
        
        /* Get the opcode */
        opcode = *bufindex++;
        
        /* Get the filename */
        strncpy(filename,bufindex,sizeof(filename)-1);
        
        /* Get the mode */
        strncpy(mode,bufindex,sizeof(mode)-1);
        
        if(op_code == OP_RRQ) {
            send(filename,mode, serveraddr);
        }
        if(op_code == OP_WRQ){
            receive(filename,mode,serveraddr);
        }
        
    }
    return 0;
}

void send (char* filename, char* mode, struct sockaddr_in serveraddr) {
    
    char fbuffer[BUFSIZE];
    int ack = 0;
    int total = 0;
    int fsize;
    
    int socketfd = socket(PF_INET,SOCK_DGRAM,0);
    
    /* Open File */
    FILE* file = fopen(filename, "r");
    /* Check For Errors */
    if(file == NULL) {
        perror("ERROR Opening File");
        exit(errno);
    }
    
    /* Fill the buffer with zeros */
    memset(fbuffer,0,sizeof(fbuffer));
    
    while(1){
        /* Read contents of file to the buffer */
        fsize = fread(fbuffer,sizeof(char),MAXSIZE,file);
        
        memcpy((char *) fbuffer + 4, fbuffer,fsize);
        
        /* Send File */
        sendto(socketfd, fbuffer,BUFFSIZE, 0,(struct sockaddr *) &myaddr, sizeof(myaddr));
        
        memset(fbuffer,0,BUFFSIZE);
        
        recvfrom(socketfd,fbuffer,BUFFSIZE,0,(struct sockaddr *) &myaddr, sizeof(myaddr));
        
    }
}

/* NOT FINISHED YET, NEED TO CHECK REC AND SEND, AS WELL AS ACK */
void receive (char* filename, char* mode, struct sockaddr_in serveraddr) {
    
    char fbuffer[BUFSIZE];
    int ack = 0;
    int total = 0;
    int fsize;
    
    int socketfd = socket(PF_INET,SOCK_DGRAM,0);
    
    /* Fill the buffer with zeros */
    memset(fbuffer,0,sizeof(fbuffer));
    
    while(1){
        
        recvfrom(socketfd,fbuffer,BUFFSIZE,0,(struct sockaddr *) &myaddr, sizeof(myaddr));
        
        sendto(socketfd, fbuffer,BUFFSIZE, 0,(struct sockaddr *) &myaddr, sizeof(myaddr));
        
    }
}
