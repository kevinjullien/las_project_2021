#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "../utils_v10.h"
#include "../messages.h"
#include "ipc_conf.h"

#define BACKLOG 5    // TO VERIFY
#define MAX_CLIENT 5 // TO VERIFY
#define CODE_PATH "./code/"

int initSocketServer(int port);

void execution_handler(void *arg1, void *arg2);

void compilation_handler(void *arg1, void *arg2);

void executeProgram(clientMessage *req, int *newsockfd);

void addProgram(clientMessage *req, int *newsockfd);

void client_connection_handler(void *arg1);


int main(int argc, char const *argv[])
{
  int sockfd, newsockfd;
  //struct pollfd fds[MAX_CLIENT];

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
    if (newsockfd > 0)
    {
      fork_and_run1(&client_connection_handler, &newsockfd);
    }
  }

  return 0;
}

/**
 * Initialize a server socket with the given port, then return the file descriptor.
 * 
 * @param port the given port
 * @return int the file descriptor of the listened socket
 */
int initSocketServer(int port)
{
  int sockfd = ssocket();
  sbind(port, sockfd);
  slisten(sockfd, BACKLOG);
  /*To be able to reuse the port immediately in case of abrupt stop of the process*/
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    perror("setsockoption(SO_REUSEADDR) failed");
  return sockfd;
}

/**
 * Child program used to execute a program.
 * It uses a pipe connected to the father program.
 * This pipe is used to send the server's response to the father.
 * 
 * @param arg1 the pipefd pointer
 * @param arg2 the program number pointer
 */
void execution_handler(void *arg1, void *arg2)
{
  int *pipefd = arg1;
  int *pgmNumber = arg2;

  sclose(pipefd[0]); // close read end of pipe for child

  dup2(pipefd[1], 1); // send stdout to the pipe
  dup2(pipefd[1], 2); // send stderr to the pipe

  sclose(pipefd[1]); // finally close write end of pipe

  char path[10]; // eg: "./code/999"
  sprintf(path, "%s%d", CODE_PATH, *pgmNumber);
  char pgm[3];
  sprintf(pgm, "%d", *pgmNumber);

  sexecl(path, pgm, NULL);
}

/**
 * Child program used to compile a program.
 * It uses a pipe connected to the father program.
 * This pipe is used to send the server's response to the father.
 * 
 * @param arg1 the pipefd pointer
 * @param arg2 the program number pointer
 */
void compilation_handler(void *arg1, void *arg2)
{
  int *pipefd = arg1;
  int *pgmNumber = arg2;

  sclose(pipefd[0]); // close read end of pipe for child

  dup2(pipefd[1], 1); // send stdout to the pipe
  dup2(pipefd[1], 2); // send stderr to the pipe

  sclose(pipefd[1]); // finally close write end of pipe

  char output_file[10]; // ex: ./code/999
  char input_file[11];  // ex: ./code/999.c
  sprintf(output_file, "%s%d", CODE_PATH, *pgmNumber);
  sprintf(input_file, "%s%d.c", CODE_PATH, *pgmNumber);

  sexecl("/usr/bin/gcc", "gcc", "-o", output_file, input_file, NULL);
}

/**
 * Run a program present in the ipcs with the given program number.
 * 
 * @param req the clientMessage, containing at least a valid program number
 * @param newsockfd the client's socket fd pointer.
 */
void executeProgram(clientMessage *req, int *newsockfd)
{
  serverMessage *resp = smalloc(sizeof(serverMessage));
  int pgmNumber = req->pgmNum;
  struct timeval stop, start;
  int status;
  int pipefd[2];
  char buffer[MAX_CHAR] = {0};

  resp->pgmNum = pgmNumber;
  resp->execTime = 0;

  if (pgmNumber < 0 || pgmNumber > 999)
  {
    resp->endStatus = PGM_NOT_FOUND;
    return;
  }

  // GET SEMAPHORE
  int sem_id = sem_get(SEM_KEY, 1);
  // GET SHARED MEMORY
  int shm_id = sshmget(SHM_KEY, sizeof(Programmes), 0);
  Programmes *s = sshmat(shm_id);

  // DOWN MUTEX
  sem_down0(sem_id);

  Programme *p = &(s->programmes)[pgmNumber];
  // TO VERIFY
  if ((p->nom)[0] == '\0')
  {
    resp->endStatus = PGM_NOT_FOUND;
    return;
  }

  spipe(pipefd);

  gettimeofday(&start, NULL); // Start Timer
  pid_t cpid_execution = fork_and_run2(&execution_handler, pipefd, &pgmNumber);
  close(pipefd[1]);
  while (sread(pipefd[0], buffer, sizeof(buffer)) != 0)
  {
    strcpy(resp->output, buffer); // TO VERIFY

    // Stop Timer
    gettimeofday(&stop, NULL);
    resp->execTime = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
    (s->programmes)[pgmNumber].totalExec += +resp->execTime;
  }

  // Wait for execution_handler to finish execution
  swaitpid(cpid_execution, &status, 0);
  if (WIFEXITED(status))
  {
    int code = WEXITSTATUS(status);
    resp->returnCode = code;

    // TO VERIFY
    if (code == 0)
    {
      resp->endStatus = PGM_STATUS_OK;
    }
    else
    {
      resp->endStatus = PGM_STATUS_KO;
    }
  }

  // UPDATE PGM IN SHARED MEMORY
  p->nbrExec = p->nbrExec + 1;
  p->erreur = false;
  p->totalExec += resp->execTime; //TO VERIFY

  // UP MUTEX
  sem_up0(sem_id);
  sshmdt(s);

  swrite(*newsockfd, resp, sizeof(*resp));
  free(resp);
}

/**
 * A child that add or edit a program in the ipcs and in the CODE_PATH folder.
 * The program will be compiled and a serverMessage will be sent through the socket with the data completed.
 * 
 * @param req a clientMessage containing at least the filesize, the file name, the code (which is -1 if the program need to be added, >=0 for edition)
 * @param newsockfd the socket file descriptor
 */
void addProgram(clientMessage *req, int *newsockfd)
{
  serverMessage *resp = smalloc(sizeof(serverMessage));
  /* récupération des données nécessaires */
  int shm_id = sshmget(SHM_KEY, sizeof(Programmes), 0);
  int sem_id = sem_get(SEM_KEY, 1);

  Programmes *programmes = sshmat(shm_id);
  int num = req->code;

  sem_down0(sem_id);

  if (num < 0)
  {
    num = programmes->taille;
    programmes->taille = programmes->taille + 1;
  }

  Programme *programme = &(programmes->programmes)[num];
  strcpy(programme->nom, req->name);
  programme->num = num;
  programme->totalExec = 0;
  programme->nbrExec = 0;
  programme->erreur = false;
  resp->pgmNum = num;

  /* Création et récupération des données du fichier */

  char path[MAX_CHAR];
  sprintf(path, "%s%d.c", CODE_PATH, num);

  int fd = sopen(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);

  void *file = smalloc(req->filesize);
  sread(*newsockfd, file, req->filesize);
  swrite(fd, file, req->filesize);
  free(file);
  sclose(fd);

  /* Compilation du programme */

  int pipefd[2];
  spipe(pipefd);

  pid_t cpid_compilation = fork_and_run2(&compilation_handler, pipefd, &num);
  close(pipefd[1]);

  programme->erreur = false;
  resp->endStatus = COMPILE_OK;

  char buffer[MAX_CHAR];
  while (sread(pipefd[0], buffer, sizeof(buffer)) != 0)
  {
    // Compilation error
    programme->erreur = true;
    resp->endStatus = COMPILE_KO;
    strcpy(resp->output, buffer);
  }

  swaitpid(cpid_compilation, NULL, 0); // Wait for the compilation to be done

  sem_up0(sem_id);

  swrite(*newsockfd, resp, sizeof(*resp));
  free(resp);
}

/**
 * A child that handle a client connection and communication.
 * It will wait to receive the request from the client and will
 * uses the functions depending on the request's code
 * 
 * @param arg1 the client's socket fd
 */
void client_connection_handler(void *arg1)
{
  int *newsockfd = arg1;
  clientMessage req;

  while (sread(*newsockfd, &req, sizeof(req)) != 0)
  {
    if (req.code == ADD_PGM)
    {
      printf("client request : ADD NEW PROGRAM\n");
      addProgram(&req, newsockfd);
    }
    else if (req.code == EXEC_PGM)
    {
      printf("client request : EXECUTE PROGRAM N°%d\n", req.pgmNum);
      executeProgram(&req, newsockfd);
    }
    else
    {
      printf("client request : EDIT PROGRAM N°%d\n", req.code);
      addProgram(&req, newsockfd);
    }
  }

  printf("close connection with client\n");
  sclose(*newsockfd);
}

/* SCENARIO DE TEST (DEV ONLY) */
/*
int main(int argc, char const *argv[]){
  serverMessage* resp = malloc(sizeof(serverMessage));

  clientMessage* req = malloc(sizeof(clientMessage));
  strcpy(req->name,"HelloWord");
  req->nameLength = 9;
  char* file = "#include<stdio.h>\n#include<stdlib.h>\n#include<unistd.h>\nint main(){sleep(3);printf(\"Hello World SLEEP 3 sec\");}";
  strcpy(req->file, file); 


  printf("---- ADD PROGRAM ----\n");

  addProgram(*req, resp);
  printf("PGM NUM         : %d\n",resp->pgmNum);
  printf("PGM COMPILATION : %d\n",resp->compileFlag);
  printf("PGM OUTPUT      : \n>>> %s\n", resp->output);


  printf("---- EXEC PROGRAM ----\n");
  executeProgram(1, resp);
  printf("PGM NUM         : %d\n",resp->pgmNum);
  printf("PGM END STATUS  : %d\n",resp->endStatus);
  printf("PGM EXEC TIME   : %d\n",resp->execTime);
  printf("PGM RETURN CODE : %d\n",resp->returnCode);
  printf("PGM OUTPUT      : \n>>> %s\n",resp->output);

  printf("---- EDIT PROGRAM ----\n");

  strcpy(resp->output,"");
  req->pgmNum = 1;
  file = "#include<stdio.h>\n#include<stdlib.h>\n#include<unistd.h>\nint main(){sleep(5);printf(\"Hello World SLEEP 5 sec\");}";
  strcpy(req->file, file); 

  editProgram(*req, resp);
  printf("PGM NUM         : %d\n",resp->pgmNum);
  printf("PGM COMPILATION : %d\n",resp->compileFlag);
  printf("PGM OUTPUT      : \n>>> %s\n", resp->output);


  printf("---- EXEC PROGRAM ----\n");
  executeProgram(1, resp);
  printf("PGM NUM         : %d\n",resp->pgmNum);
  printf("PGM END STATUS  : %d\n",resp->endStatus);
  printf("PGM EXEC TIME   : %d\n",resp->execTime);
  printf("PGM RETURN CODE : %d\n",resp->returnCode);
  printf("PGM OUTPUT      : \n>>> %s\n",resp->output);

  free(resp);
}

*/
