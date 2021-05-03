#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "../utils_v10.h"
#include "../messages.h"
#include "ipc_conf.h"

#define BACKLOG 5    // TO VERIFY
#define MAX_CLIENT 5 // TO VERIFY

void addProgram(clientMessage msg, int newsockfd);

int initSocketServer(int port)
{
  int sockfd = ssocket();
  sbind(port, sockfd);
  slisten(sockfd, BACKLOG);
  return sockfd;
}

int main(int argc, char const *argv[])
{
  int sockfd, newsockfd;
  clientMessage msg;
  struct pollfd fds[MAX_CLIENT];

  if (argc != 2)
  {
    printf("%s\n", "Usage: ./server port");
    exit(1);
  }

  sockfd = initSocketServer(atoi(argv[1]));
  printf("server listening on port : %i \n", atoi(argv[1]));

  while (1)
  {
    newsockfd = saccept(sockfd);
    sread(newsockfd, &msg, sizeof(msg));

    if (msg.code == ADD_PGM)
    { 
       addProgram(msg, newsockfd);
    }
    else if (msg.code == EXEC_PGM)
    {
    }
    else
    {
      // MODIFY PGM
      // check if pgm exists
      //
    }
  }

  return 0;
}

void addProgram(clientMessage msg, int newsockfd)
{ 
  /* récupération des données nécessaires */
  int shm_id = sshmget(SHM_KEY, sizeof(Programmes), 0);
  int sem_id = sem_get(SEM_KEY, 1);

  Programmes *programmes = sshmat(shm_id);
  int num = programmes->taille;

  if (num >= 1000){
    //TODO message d'erreur
    return;
  }

  Programme programme;
  strcpy(programme.nom, msg.name);
  programme.num = num;

  /* Création et récupération des données fu fichier */
  char* root = "./code/"; //TODO utiliser constante
  char path[10]; //TODO bof, voir avec constante
  sprintf(path, "%s%d", root, num);

  int fd = sopen(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
      
  char data[MAX_CHAR];
  int dataRead = sread(newsockfd, &data, MAX_CHAR);
  while (dataRead != 0){
    int res = write(fd, data, dataRead);
    checkCond(res != dataRead, "error writing");
    sread(newsockfd, &msg, sizeof(msg));
  }
  sclose(fd);

/* Création des données dans la mémoire partagée */

  sem_down0(sem_id);
  (programmes->programmes)[num] = programme;
  programmes->taille = programmes->taille + 1;

  sem_up0(sem_id);
}