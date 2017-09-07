#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <setjmp.h>

#define PORT_NUMBER 54321
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
#define MAX_TIMEOUTS 3
#define ALARM_TIME 2

int volatile timeout = 0;
jmp_buf timeoutbuf;

//void time_out(int sig) {
//	fprintf(stdout, "timed out\n");
//	signal(SIGALRM, time_out);
//        timeout++;
//        if(timeout >= MAX_TIMEOUTS) {
//            exit(1);
//        }
//	else {
//	    alarm(0);
//	    longjmp(timeoutbuf, 3);	
//        }
//}


/* Function to Send Acknowledgment Packet */
void send_ACK(int socketfd, struct sockaddr* addr, socklen_t * addrLen, char seq_num){
    
    char buffer[4];
    
    buffer[0] = '0';
    buffer[1] = '4';
    buffer[2] = '0';
    
    if(seq_num == '0') buffer[3] = '0';
    else buffer[3] = '1';
    
    sendto(socketfd, buffer, 4, 0, (struct sockaddr *) addr, *addrLen);
    fprintf(stdout, "ACK SENT\n");
}

int prev_seq(int curr_seq){
    if (curr_seq == 0){
        return '1';
    }
    return curr_seq--;
}

/* Function to handle a write request */
void write_handler(int socketfd, char* buffer, struct sockaddr* addr, socklen_t * addrLen, FILE* file ){
    fprintf(stdout, "IN WRITE HANDLER\n"); 
    /* Declare variables */
    int recvlen = 0;
    int try = 0;
    int curr_seq = 0;
    char fbuffer[MAX_DATA];
    struct timeval time_out;
    time_out.tv_sec = 3;
    time_out.tv_usec = 0;
    /* Loop the max number of times */
    while (try < TIMEOUT_NUMBER){
	fprintf(stdout, "ENTERED LOOP\n");
        /* Reset Length */
        recvlen = 0;
        /* Zero out buffer */
        bzero(buffer, BUFSIZE);
	
	
	setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,&time_out,sizeof(time_out));
        /* Receive from socket */
        recvlen = recvfrom(socketfd,buffer, BUFSIZE, 0,(struct sockaddr*)addr, addrLen);

	fprintf(stdout, "Receive pass\n");

	if(recvlen == -1) {
	     fprintf(stdout, "Failed send\n");

	 }

        //alarm(0);

        fprintf(stdout,"REV LENGTH = %i\n", recvlen);
        if(recvlen > 0) {
            /* Reset */
            try = 0;
            /* Checks what opcode is */
            if(buffer[1] == '3'){
		fprintf(stdout, "OPCODE is 3\n");
                /* Resets */
                try = 0;
                /* Copies input buffer to file buffer */
                memcpy(fbuffer, buffer+4, recvlen-4);
                /* Write to buffer */
                fwrite(fbuffer, 1, recvlen-4, file);
                fprintf(stdout, "WROTE TO File Buffer\n");
                /* Sets sequcence number */
                if(buffer[3] == '0') curr_seq = 0;
                else curr_seq = 1;
                fprintf(stdout, "SENDING ACK\n");
                /* Send packet acknowledgement */
                send_ACK(socketfd, addr, addrLen, '0'+ curr_seq);
                
                /* Checks if we reached the max sequence */
                if(curr_seq == '1') break;
                /* Increments Sequence Number */
                else curr_seq++;
            }
        }
        
        if(recvlen < (MAX_DATA + 4) && buffer[1] == '3' && recvlen > 0) break;
        
        /* Send Acknowledgement and increment try variable */
        if(recvlen <= 0){
            send_ACK(socketfd,addr,addrLen,(char)prev_seq(curr_seq));
            try++;
        }
    }
}

/* Function to handle request */
void request_handler(int socketfd,char* recbuff,char* sendbuff,struct sockaddr* senderAddr,socklen_t * addrLen, FILE* file ){
    fprintf(stdout, "IN REQEUST HANDLER\n");    
    /* Declare Variables */
    char fbuffer[MAX_DATA];
    int recvlen, try, ack, nBytes, curr_seq = 0;
    int read = MAX_DATA;
    
    while(read == MAX_DATA) {
	fprintf(stdout, "ENTERING OUTER PROBLEM LOOP\n");
        /* Zero out buffer */
        bzero(sendbuff, BUFSIZE);
        /* Read File */
        read = fread(sendbuff + 4, 1, MAX_DATA, file);
        fprintf(stdout, "FINISHED READING\n");
        /* Break from loop */
        if (read < 1) break;
        
        /* Reset try and ack variables */
        try = 0;
        ack = 0;
        
        /* Loop until an ack */
        while (try < TIMEOUT_NUMBER && ack == 0) {
	    fprintf(stdout, "%i\n", ack);
            /* Opcode */
            sendbuff[0] = '0';
            sendbuff[1] = '3';
            
            sendbuff[2] = '0';
            sendbuff[3] = '0' + curr_seq;
            
            /* Send to socket */
            nBytes = sendto(socketfd,sendbuff,read+4,0,(struct sockaddr *) senderAddr, *addrLen);
            fprintf(stdout, "SEND TO = %i\n", nBytes);
            /* Zero out buffer */
            bzero(recbuff, BUFSIZE);
            
            /* Reset length variable */
            recvlen = 0;
            /* Receive from socket */
            recvlen = recvfrom(socketfd,recbuff,BUFSIZE,0,(struct sockaddr *) senderAddr, addrLen);
            fprintf(stdout, "RECFROM = %i\n",recvlen);
            if (recvlen > 0) {
                /* Reset try variable */
                try = 0;
                /* Checks opcode */
                if(recbuff[1] == '4'){
		    fprintf(stdout, "ACK RECEIVED\n");
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
        //if(try == TIMEOUT_NUMBER) break;
    }
}

/* Main function to run program and make function calls */
int main(int argc, char *argv[]){
    
    fprintf(stdout, "ENTERING MAIN\n");

    /* Variable Declarations */
    char fileName[NAMESIZE];
    //char *fileName = argv[2];
    char sendbuf[BUFSIZE], recbuf[BUFSIZE];
    
    FILE *file;
    int res,socketfd,request,nBytes,recvlen;
    struct sockaddr_in clientAddr, serverAddr;
    struct hostent *hp;
    socklen_t addrLen = sizeof(serverAddr);
    
    /* Create a new socket */
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketfd < 0 ) {
 	fprintf(stderr, "SOCKET ERROR\n");
    }    
    fprintf(stdout, "SOCKETFD = %i\n", socketfd);
    
    hp = gethostbyname("localhost");
    
    /* Populate server struct (From Discussion Session) */
    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUMBER);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    memcpy((void *)&serverAddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    
    /* Checks if write mode */
    if(strcmp( argv[1], "-w") == 0){
       	fprintf(stdout, "WRITE REQUEST\n");     
        request = 0;
        
        /* Open file */
        file = fopen(fileName, "r");
        if(file == NULL) {
		fprintf(stdout, "ERROR OPENING FILE\n");
	}
	fprintf(stdout, "FILE OPENED\n");
        /* Cancatenate string for autograder submission */
        //fileName = strcat("/clientFiles/", argv[2]);
        sprintf(fileName, "/clientFiles/%s", argv[2]);
        
        /* Zero out the buffer */
        bzero(sendbuf, BUFSIZE);
        
        /* Opcode */
        sendbuf[0] = '0';
        sendbuf[1] = '2';
        
        /* filename */
        strcpy(sendbuf+2, argv[2]);
        
        /* Send message on socket */
        nBytes = sendto(socketfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr*)&serverAddr, addrLen);
        fprintf(stdout, "SENDTO MAIN = %i\n", nBytes);
        /* Loop while request is zero */
        while (request == 0){
           
            recvlen = 0;
            
            /* Zero out the buffer */
            bzero(recbuf, BUFSIZE);
            
            /* Receive message from socket */
            recvlen = recvfrom(socketfd,recbuf,BUFSIZE,0,(struct sockaddr*)&serverAddr, &addrLen);

            fprintf(stdout, "RECVFROM MAIN = %i\n", recvlen);
            /* Checks if acknowedge received */
            if(recvlen > 0 && recbuf[1] == '4') request = 1;
            fprintf(stdout, "ENTERING REQUEST HANDLER\n");
            /* Function call to handle the request */
            request_handler(socketfd, recbuf, sendbuf, (struct sockaddr*)&serverAddr, &addrLen, file);
            
            /* Close File */
            fclose(file);
        }
    }
    /* Checks if read mode */
    else if(strcmp(argv[1], "-r") == 0){

        fprintf(stdout, "STARTING r REQUEST\n");

        /* Cancatenate string for autograder submission */
        //fileName = strcat("/clientFiles/", argv[2]);
        sprintf(fileName, "/clientFiles/%s", argv[2]);
        
        /* Open file */
        file = fopen(fileName, "w");
	if(file == NULL) {
		fprintf(stdout, "NULL FILE\n");
	}
        
        /* Zero out the buffer */
        bzero(sendbuf, BUFSIZE);
        
        /* Opcode */
        sendbuf[0] = '0';
        sendbuf[1] = '1';
        
        /* Move filename to buffer */
        strcpy(sendbuf+2, argv[2]);
	int i = 0;
	if(sendbuf[1] == '1'){
		fprintf(stdout, "OPCODE IS 1\n");
	} 

	

        /* Send message on socket */
        nBytes = sendto(socketfd, sendbuf, strlen(sendbuf), 0, (struct sockaddr*)&serverAddr, addrLen);
         
	if(nBytes == -1) {
	     fprintf(stdout, "Failed send\n");
	     timeout++;
             if(timeout >= MAX_TIMEOUTS) {
                exit(1);
             }
	     else {
	         longjmp(timeoutbuf, 1);	
             }
	 }

        alarm(0);

        //fprintf(stdout, "SENDTO RETURN VAL = %i\n", nBytes);
    	fprintf(stdout, "ENTERING WRITE HANDLER\n");
        /* Handle write request */
        write_handler(socketfd, recbuf, (struct sockaddr*)&serverAddr, &addrLen, file);
            
        /* Close File */
        fclose(file);
    }
    return 0;
}
