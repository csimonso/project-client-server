#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT_NUMBER 12345
//#define PORT_NUMBER 61111

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5

#define MAX_DATA 512
#define BUFSIZE 1024
#define NAMESIZE 128
#define TIMEOUT_NUMBER 3

int prev_seq(int curr_seq){
    if (curr_seq == 0){
        return '1';
    }
    return curr_seq--;
}

/* Function to send acknowledgement packet */
void send_ACK(int socketfd, struct sockaddr* addr, socklen_t * addrLen, char seq_num){
    
    char buffer[4];
    
    buffer[0] = '0';
    buffer[1] = '4';
    buffer[2] = '0';
    
    if('0' == seq_num) buffer[3] = '0';
    else buffer[3] = '1';
    
    sendto(socketfd,buffer,4,0,(struct sockaddr *) addr, *addrLen);
    fprintf(stdout, "ACK SENT\n");
}

/* Function to handle write */
void write_handler(int socketfd, char* buffer, struct sockaddr* senderAddr, socklen_t * addrLen, FILE* file ){
    
    int recvlen, try = 0;
    int curr_seq = 0;
    char fbuf[MAX_DATA];
    
    while (try < TIMEOUT_NUMBER){
        
        recvlen = 0;
        bzero(buffer, BUFSIZE);
        
        recvlen = recvfrom(socketfd,buffer,BUFSIZE,0,(struct sockaddr*)senderAddr,addrLen);
	fprintf(stdout, "WRITE HANDLER RECVLEN = %i\n", recvlen);        
        if(recvlen > 0){
            try = 0;
            
            if(buffer[1] == '3'){
                try = 0;
                memcpy(fbuf, buffer+4, recvlen-4);
                fwrite(fbuf, 1, recvlen-4, file);
		fprintf(stdout, "FINISHED FWRITE INSIDE WRITE HANDLER\n");
                if(buffer[3] == '0'){
                    curr_seq = 0;
                }
                else{
                    curr_seq = 1;
                }
		fprintf(stdout, "SEND ACK FROM INSIDE WRITE HANDLER DATA OPCODE\n");
                send_ACK(socketfd, senderAddr, addrLen, '0'+ curr_seq);
                
                if(curr_seq == '1') {
                    curr_seq = 0;
                }
                else {
                    curr_seq++;
                }
            }
        }
        
        if(recvlen < (MAX_DATA + 4) && buffer[1] == '3' && recvlen > 0){
            break;
        }
        
        if(recvlen <= 0){
	    fprintf(stdout, "SENDING ACK INSIDE WRITE HANDLER\n");
            send_ACK(socketfd, senderAddr, addrLen, (char)prev_seq(curr_seq));
            try++;
        }
    }
}

void request_handler(int socketfd, char* recbuffer, char* sendbuffer, struct sockaddr* senderAddr, socklen_t * addrLen, FILE* file ){
    
    char fbuf[MAX_DATA];
    int recvlen, try, ack, nBytes, curr_seq = 0;
    int read = MAX_DATA;
    
    while(read == MAX_DATA) {
        
        bzero(sendbuffer, BUFSIZE);
        read = fread(sendbuffer + 4, 1, MAX_DATA, file);
        
        if (read < 1) {
            break;
        }
        
        try = 0;
        ack = 0;
        
        while (try < TIMEOUT_NUMBER && ack == 0) {
            
            sendbuffer[0] = '0';
            sendbuffer[1] = '3';
            sendbuffer[2] = '0';
            sendbuffer[3] = '0' + curr_seq;
            
            nBytes = sendto(socketfd,sendbuffer,read+4, 0, (struct sockaddr *) senderAddr, *addrLen);
    	    fprintf(stdout, "SENDTO REQUEST = %i\n", nBytes);        
            recvlen = 0;
            bzero(recbuffer, BUFSIZE);
            
            recvlen = recvfrom(socketfd, recbuffer, BUFSIZE, 0, (struct sockaddr *) senderAddr, addrLen);
            fprintf(stdout, "RECVFROM REQUEST = %i\n", recvlen);
            if (recvlen > 0) {
                
                try = 0;
                
                if(recbuffer[1] == '4') {
                    ack = 1;
                    if(curr_seq == '1'){
                        curr_seq = 0;
                    }
                    else {
                        curr_seq++;
                    }
                }
            }
            else{
                try++;
            }
        }
    }
}

int main(int argc, char *argv[]){

    fprintf(stdout, "ENTERING MAIN\n");
    
    int read,socketfd,nBytes,recvlen,request;
    
    char fileName[NAMESIZE];
    //char *fileName;
    char recbuff[BUFSIZE];
    char sendbuff[BUFSIZE];
    
    FILE *file;
    
    struct sockaddr_in clientAddress, serverAddress;
    struct hostent *hp;
    socklen_t addrLen = sizeof(clientAddress);
    
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    fprintf(stdout, "SOCKETFD = %i\n", socketfd);
    
    hp = gethostbyname("localhost");
    memcpy((void *)&serverAddress.sin_addr, hp->h_addr_list[0], hp->h_length);
    //memset((char *) &serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    //serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT_NUMBER);

    
    nBytes = bind(socketfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    fprintf(stdout, "BIND = %i\n", nBytes);
    while(1){
       	fprintf(stdout, "ENTER LOOP\n");
        bzero(recbuff, BUFSIZE);
        bzero(sendbuff, BUFSIZE);
        
        recvlen = 0;
        recvlen = recvfrom(socketfd, recbuff, BUFSIZE, 0, (struct sockaddr *) &clientAddress, &addrLen);
        fprintf(stdout, "REC LENGTH = %i\n", recvlen);
        if(recvlen > 0){
            
            if(recbuff[1] == '1'){
		fprintf(stdout, "INSIDE IF FOR R Request\n");
                sprintf(fileName, "/serverFiles/%s", (recbuff+2));
                //fileName = recbuff+2;
                file = fopen(fileName, "rb");
                request_handler(socketfd, recbuff, sendbuff, (struct sockaddr*)&clientAddress, &addrLen, file);
                fclose(file);
		break;
            }
            else if(recbuff[1] == '2'){
		fprintf(stdout, "INSIDE IF FOR W REQUEST\n");
                sprintf(fileName, "/serverFiles/%s", (recbuff+2));
                //fileName = recbuff+2;
                file = fopen(fileName, "w");
		fprintf(stdout, "SENDING ACK FOR W REQUEST\n");
                send_ACK(socketfd, (struct sockaddr*)&clientAddress, &addrLen, '0');
		fprintf(stdout, "ENTERING WRITE HANDLER FOR W REQUEST\n");
                write_handler(socketfd, recbuff,(struct sockaddr*)&clientAddress, &addrLen, file);
                fclose(file);
		break;
            }
        }
    }
    return 0;
}
                                                                                                                                   
