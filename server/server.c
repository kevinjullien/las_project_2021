#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../utils_v10.h"
#include "../messages.h"

#define BACKLOG 5 // TO VERIFY
#define MAX_CLIENT 5 // TO VERIFY

int initSocketServer(int port) 
{
  int sockfd  = ssocket();
  sbind(port, sockfd);
  slisten(sockfd, BACKLOG);
  return sockfd;
}

int main(int argc, char const *argv[])
{
  int sockfd, newsockfd;
  clientMessage msg;
  //struct pollfd fds[MAX_CLIENT];

  if (argc != 2) {
		printf("%s\n", "Usage: ./server port");
		exit(1);
	}

  sockfd = initSocketServer(atoi(argv[1]));
  printf("server listening on port : %i \n", atoi(argv[1]));

  while(1)
  {
    newsockfd = saccept(sockfd);
    sread(newsockfd, &msg, sizeof(msg));

    if (msg.code == ADD_PGM) {

    } else if (msg.code == EXEC_PGM) {

    } else {
      // MODIFY PGM
      // check if pgm exists
      // 

    }

  }

  return 0;
}