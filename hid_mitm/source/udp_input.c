#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <errno.h>

#include <switch.h>
#include "udp_input.h"

#define PORT     8080 
#define MAXLINE 1024 

static int sockfd = -1;

struct sockaddr_in servaddr, cliaddr; 
    
void setup_socket()
{
    if(sockfd != -1) {
        close(sockfd);
    }
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;

    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);


    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
    
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
}

static u32 curIP = NULL;
static int failed = 30;
static struct input_msg cached_msg = {0};
int poll_udp_input(struct input_msg* buf) {
    // Reduce the poll frequency to lower load.
    // If we're not "connected" right now we only check around every second
    if(failed > 30 && failed % 60 != 0) {
        failed++;
        return -1;
    }

    if(curIP != gethostid()) {
        setup_socket();
        curIP = gethostid();
    }

    socklen_t len;
    int n; 
    struct input_msg tmp_msg;
    n = recvfrom(sockfd, &tmp_msg, sizeof(struct input_msg),  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len); 
    if(n <= 0 || tmp_msg.magic != INPUT_MSG_MAGIC) {
        failed++;
    } else {
        failed = 0;
        cached_msg = tmp_msg;
    }
    *buf = cached_msg;

    if(failed >= 30) {
        return -1;
    }

    return 0;
}