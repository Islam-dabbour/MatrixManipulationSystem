#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

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

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    char buffer[1024];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(server_sock < 0){
        perror("Socket Error");
        exit(1);
    }

    printf("[+] Server socket created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_sock,
           (struct sockaddr*)&server_addr,
           sizeof(server_addr)) < 0)
    {
        perror("Bind Error");
        exit(1);
    }

    printf("[+] Bound to port %d\n", port);

    listen(server_sock, 5);

    printf("Listening...\n");

    addr_size = sizeof(client_addr);

    client_sock = accept(server_sock,(struct sockaddr*)&client_addr,&addr_size);//the senario
	
	if(client_sock < 0)
        error("Accept Error");

    printf("[+] Client Connected.\n");
    
    close(client_sock);
	
	close(server_sock);


 }