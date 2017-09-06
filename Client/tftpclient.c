#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT_NUMBER 54321

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5

#define MAX_DATA 512
#define BUFSIZE 1024
#define NAMESIZE 128
#define TIMEOUT_NUMBER 3

/* Function to Send Acknowledgment Packet */
void send_ACK(int socketfd, struct sockaddr* addr, socklen_t * addrLen, char seq_num){
    
    char buffer[4];
    
    buffer[0] = '0';
    buffer[1] = OP_ACK;
    buffer[2] = '0';
    
    if(seq_num == '0') buffer[3] = '0';
    else buffer[3] = '1';
    
    sendto(socketfd, buffer, 4, 0, (struct sockaddr *) addr, *addrLen);
}

int prev_seq(int curr_seq){
    if (curr_seq == 0){
        return '1';
    }
    return curr_seq--;
}

/* Function to handle a write request */
void write_handler(int socketfd, char* buffer, struct sockaddr* addr, socklen_t * addrLen, FILE* file ){
    
    /* Declare variables */
    int recvlen = 0;
    int try = 0;
    int curr_seq = 0;
    char fbuffer[MAX_DATA];
    
    /* Loop the max number of times */
    while (try < TIMEOUT_NUMBER){
        /* Reset Length */
        recvlen = 0;
        /* Zero out buffer */
        bzero(buffer, BUFSIZE);
        /* Receive from socket */
        recvlen = recvfrom(socketfd,buffer, BUFSIZE, 0,(struct sockaddr*)addr, addrLen);
        
        if(recvlen > 0) {
            /* Reset */
            try = 0;
            /* Checks what opcode is */
            if(buffer[1] == OP_DATA){
                /* Resets */
                try = 0;
                /* Copies input buffer to file buffer */
                memcpy(fbuffer, buffer+4, recvlen-4);
                /* Write to buffer */
                fwrite(fbuffer, 1, recvlen-4, file);
                
                /* Sets sequcence number */
                if(buffer[3] == '0') curr_seq = 0;
                else curr_seq = 1;
                
                /* Send packet acknowledgement */
                send_ACK(socketfd, addr, addrLen, '0'+ curr_seq);
                
                /* Checks if we reached the max sequence */
                if(curr_seq == '1') break;
                /* Increments Sequence Number */
                else curr_seq++;
            }
        }
        
        if(recvlen < (MAX_DATA + 4) && buffer[1] == OP_DATA && recvlen > 0) break;
        
        /* Send Acknowledgement and increment try variable */
        if(recvlen <= 0){
            send_ACK(socketfd,addr,addrLen,(char)prev_seq(curr_seq));
            try++;
        }
    }
}

/* Function to handle request */
void request_handler(int socketfd,char* recbuff,char* sendbuff,struct sockaddr* senderAddr,socklen_t * addrLen, FILE* file ){
    
    /* Declare Variables */
    char fbuffer[MAX_DATA];
    int recvlen, try, ack, nBytes, curr_seq = 0;
    int read = MAX_DATA;
    
    while(read == MAX_DATA) {
        /* Zero out buffer */
        bzero(sendbuff, BUFSIZE);
        /* Read File */
        read = fread(sendbuff + 4, 1, MAX_DATA, file);
        
        /* Break from loop */
        if (read < 1) break;
        
        /* Reset try and ack variables */
        try = 0;
        ack = 0;
        
        /* Loop until an ack */
        while (try < TIMEOUT_NUMBER && ack == 0) {
            /* Opcode */
            sendbuff[0] = '0';
            sendbuff[1] = OP_DATA;
            
            sendbuff[2] = '0';
            sendbuff[3] = '0' + curr_seq;
            
            /* Send to socket */
            nBytes = sendto(socketfd,sendbuff,read+4,0,(struct sockaddr *) senderAddr, *addrLen);
            
            /* Zero out buffer */
            bzero(recbuff, BUFSIZE);
            
            /* Reset length variable */
            recvlen = 0;
            /* Receive from socket */
            recvlen = recvfrom(socketfd,recbuff,BUFSIZE,0,(struct sockaddr *) senderAddr, addrLen);
            
            if (recvlen > 0) {
                /* Reset try variable */
                try = 0;
                /* Checks opcode */
                if(recbuff[1] == OP_ACK){
                    ack = 1;
                    if(curr_seq == '1') break;
                    else curr_seq++;
                }
            }
            else{
                try++;
            }
            
        }
        /* Breaks if max tries is reached */
        if(try == TIMEOUT_NUMBER) break;
    }
}

/* Main function to run program and make function calls */
int main(int argc, char *argv[]){
    
    /* Variable Declarations */
    char fileName[NAMESIZE], sendbuf[BUFSIZE], recbuf[BUFSIZE];
    
    FILE *file;
    int res,socketfd,request,nBytes,recvlen;
    struct sockaddr_in clientAddr, serverAddr;
    struct hostent *hp;
    socklen_t addrLen = sizeof(serverAddr);
    
    /* Create a new socket */
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    
    //memset((char *) &clientAddr, 0, sizeof(clientAddr));
    //clientAddr.sin_family = AF_INET;
    //clientddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //clientAddr.sin_port = htons(0);
    //if (bind(socketfd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
    //    perror("bind failed");
    //    exit (0);
    //}
    
    hp = gethostbyname("localhost");
    
    /* Populate server struct (From Discussion Session) */
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUMBER);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    memcpy((void *)&serverAddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    
    /* Checks if write mode */
    if(strcmp( argv[1], "-w") == 0){
        
        request = 0;
        
        /* Open file */
        file = fopen(fileName, "r");
        
        /* Cancatenate string for autograder submission */
        //fileName = strcat("/clientFiles/", argv[2]);
        sprintf(fileName, "/clientFiles/%s", argv[2]);
        
        /* Zero out the buffer */
        bzero(sendbuf, BUFSIZE);
        
        /* Opcode */
        sendbuf[0] = '0';
        sendbuf[1] = OP_WRQ;
        
        /* filename */
        strcpy(sendbuf+2, argv[2]);
        
        /* Send message on socket */
        nBytes = sendto(socketfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr*)&serverAddr, addrLen);
        
        /* Loop while request is zero */
        while (request == 0){
            
            recvlen = 0;
            
            /* Zero out the buffer */
            bzero(recbuf, BUFSIZE);
            
            /* Receive message from socket */
            recvlen = recvfrom(socketfd,recbuf,BUFSIZE,0,(struct sockaddr*)&serverAddr, &addrLen);
            
            /* Checks if acknowedge received */
            if(recvlen > 0 && recbuf[1] == OP_ACK) request = 1;
            
            /* Function call to handle the request */
            request_handler(socketfd, recbuf, sendbuf, (struct sockaddr*)&serverAddr, &addrLen, file);
            
            /* Close File */
            fclose(file);
        }
    }
    /* Checks if read mode */
    else if(strcmp(argv[1], "-r") == 0){
        
        /* Cancatenate string for autograder submission */
        //fileName = strcat("/clientFiles/", argv[2]);
        sprintf(fileName, "/clientFiles/%s", argv[2]);
        
        /* Open file */
        file = fopen(fileName, "w");
        
        /* Zero out the buffer */
        bzero(sendbuf, BUFSIZE);
        
        /* Opcode */
        sendbuf[0] = '0';
        sendbuf[1] = OP_RRQ;
        
        /* Move filename to buffer */
        strcpy(sendbuf+2, argv[2]);
        
        /* Send message on socket */
        nBytes = sendto(socketfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr*)&serverAddr, addrLen);
        
        /* Handle write request */
        write_handler(socketfd, recbuf, (struct sockaddr*)&serverAddr, &addrLen, file);
            
        /* Close File */
        fclose(file);
    }
    return 0;
}
