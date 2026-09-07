#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen, n;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	portno = 1200;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port =  htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
   	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	//the senario
	
	close(newsockfd);
	
	close(sockfd);


 }