#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg){
    perror(msg);
    exit(0);
}

int main(int argc, char **argv){

    if(argc != 2){
        printf("You need to provide a port number\n");
        exit(0);
    }

    int port = atoi(argv[1]);

    int sockFD;

    struct sockaddr_in addr;

    char buffer[1024];

    sockFD = socket(AF_INET, SOCK_STREAM, 0);

    if(sockFD < 0){
        perror("Socket Error");
        exit(1);
    }

    printf("[+] Client socket created.\n");

    memset(&addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(connect(sockFD,
              (struct sockaddr*)&addr,
              sizeof(addr)) < 0)
    {
        error("Connection Error");
    }

    printf("[+] Connected to server.\n"); //senario




 close(sockFD);
   }