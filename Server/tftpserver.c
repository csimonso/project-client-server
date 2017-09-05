                                                                                                                                                                               1,2           Top
      #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT_NUMBER 54321

#define OP_RRQ 1
#define OP_WRQ 2
#define OP_DATA 3
#define OP_ACK 4
#define OP_ERROR 5

#define MAXSIZE 512
#define BUFSIZE 1024
#define NAMESIZE 128

erverFiles/", "r");
    /* Check For Errors */
    if(file == NULL) {
        printf("ERROR Opening File");
        exit(1);
    }

    /* Fill the buffer with zeros */
    memset(fbuffer,0,sizeof(fbuffer));

    while(1){
        /* Read contents of file to the buffer */
        fsize = fread(fbuffer,sizeof(char),MAXSIZE,file);

        memcpy((char *) fbuffer + 4, fbuffer,fsize);

        /* Send File */
        sendto(socketfd, fbuffer,BUFSIZE, 0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

        memset(fbuffer,0,BUFSIZE);

        recvfrom(socketfd,fbuffer,BUFSIZE,0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

    }
}

/* NOT FINISHED YET, NEED TO CHECK REC AND SEND, AS WELL AS ACK */
void receive_file (char* filename, char* mode, struct sockaddr_in serveraddr) {

    char fbuffer[BUFSIZE];
    int ack = 0;
    int total = 0;
    int fsize;

    int socketfd = socket(PF_INET,SOCK_DGRAM,0);

    /* Fill the buffer with zeros */
    memset(fbuffer,0,sizeof(fbuffer));

    while(1){

        recvfrom(socketfd,fbuffer,BUFSIZE,0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

        sendto(socketfd, fbuffer,BUFSIZE, 0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

    }
}
int main(int argc, char *argv[]){

    int nBytes, socketfd;
    char buffer[BUFSIZE], filename[NAMESIZE], mode[NAMESIZE], opcode, *bufindex;
    struct sockaddr_in clientaddr, serveraddr;

    /* Create a UDP socket, return the File Descriptor */
    socketfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Check for Errors */
    if(socketfd < 0) {
        printf("Socket Error");
        exit(1);
    }

    /* From Discussion Session */
        /* Method used to allocate pages, must be done in the kernel */
        public static int allocatePage(int vpn, VMProcess process, boolean readOnly){
                //Initialize ppn to -1 since we have not determined it yet
                int ppn = -1;
                //Initialize our clock variable
                int clock = 0;
                //Initialize page table index variable
                int indexPageTable = -1;
                //Acquire physical pages Lock
                physicalLock.acquire();
                //Chekc if this is the first physical page
                if (physicalPages.size() != 0) {
                        //Get the ppn of the first page
                        ppn = physicalPages.pollFirst();
                }
                //Release the physical pages lock
                physicalLock.release();
                //Acquire kernel page table lock
                kernelptLock.acquire();
                //Set our clock variable
                clock = clockTracker;
                while (ppn < 0) {
                        //Update our clock, used formula found online
                        clockTracker = (clockTracker + 1) % VMKernel.kernelPageTable.length;
                        //Acquire physical lock
                        physicalLock.acquire();
                        //Check that our physical pages is not empty
                        if (physicalPages.size() != 0) {
                                //Set the ppn to the first physical page
                                ppn = physicalPages.pollFirst();
                                //Release physical lock
                                physicalLock.release();
                                break;
                        }
                        //Release physical lock
                        physicalLock.release();
                        //Set our meta data
                        VMKernel.MetaData data = VMKernel.kernelPageTable[clockTracker];
                        //Check if data is pinned
                        if(!data.pinned){
                                //Get the entry of the meta data
                                TranslationEntry entry = data.getEntry();
                                //Loop through the TLB
                                for (int i = 0; i < Machine.processor().getTLBSize(); i++) {
                                        //Read in each TLB entry
                                        TranslationEntry e = Machine.processor().readTLBEntry(i);
                                        //Check if the entry vpn matches meta datas
                                        if(e.vpn == entry.vpn) {
                                                //Check if entry is valid
                                                if(e.valid) {
    memset((char*)&serveraddr,0,sizeof(serveraddr));//Discussion
    serveraddr.sin_family = AF_INET;//Discussion
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//Discussion
    serveraddr.sin_port = htons(PORT_NUMBER);//Discussion

    /* Assigns a local socket address to the socket FD */
    int x = bind(socketfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));//Discussion

    /* Checks for errors */
    if(x < 0){
        printf("Binding Error");
        exit(1);
    }

    /* Loop in case of other requests */
    while(1) {

        /* Receives the message request */
        nBytes = recvfrom(socketfd,buffer,BUFSIZE,0,(struct sockaddr *)&clientaddr,sizeof(clientaddr));

        /* Check for Errors */
        if(nBytes < 0){
            printf("Recvfrom ERROR");
            exit(1);
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

        if(opcode == OP_RRQ) {
            send_file(filename,mode, serveraddr);
        }
        if(opcode == OP_WRQ){
            receive_file(filename,mode,serveraddr);
        }

    }
    return 0;
}

void send_file (char* filename, char* mode, struct sockaddr_in serveraddr) {

    char fbuffer[BUFSIZE];
    int ack = 0;
    int total = 0;
    int fsize;

    int socketfd = socket(PF_INET,SOCK_DGRAM,0);

    /* Open File */
    FILE* file = fopen("/serverFiles/", "r");
    /* Check For Errors */
    if(file == NULL) {
        printf("ERROR Opening File");
        exit(1);
    }

    /* Fill the buffer with zeros */
    memset(fbuffer,0,sizeof(fbuffer));

    while(1){
        /* Read contents of file to the buffer */
        fsize = fread(fbuffer,sizeof(char),MAXSIZE,file);

        memcpy((char *) fbuffer + 4, fbuffer,fsize);

        /* Send File */
        sendto(socketfd, fbuffer,BUFSIZE, 0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

        memset(fbuffer,0,BUFSIZE);

        recvfrom(socketfd,fbuffer,BUFSIZE,0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

    }
}
    /* NOT FINISHED YET, NEED TO CHECK REC AND SEND, AS WELL AS ACK */
void receive_file (char* filename, char* mode, struct sockaddr_in serveraddr) {

    char fbuffer[BUFSIZE];
    int ack = 0;
    int total = 0;
    int fsize;

    int socketfd = socket(PF_INET,SOCK_DGRAM,0);

    /* Fill the buffer with zeros */
    memset(fbuffer,0,sizeof(fbuffer));

    while(1){

        recvfrom(socketfd,fbuffer,BUFSIZE,0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

        sendto(socketfd, fbuffer,BUFSIZE, 0,(struct sockaddr *) &serveraddr, sizeof(serveraddr));

    }
}
                                                                                                                                      1,2           Top
