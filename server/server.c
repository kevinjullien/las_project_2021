#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#include "../utils_v10.h"
#include "ipc_conf.h"
#include "../messages.h"

#define BACKLOG 5 // TO VERIFY
#define MAX_CLIENT 5 // TO VERIFY
#define CODE_PATH "./code"

int initSocketServer(int port) 
{
  int sockfd  = ssocket();
  sbind(port, sockfd);
  slisten(sockfd, BACKLOG);
  return sockfd;
}

void execution_handler (void* arg1, void* arg2) {
  int *pipefd = arg1;
  int *pgmNumber = arg2;

  sclose(pipefd[0]);    // close read end of pipe for child

  dup2(pipefd[1], 1);   // send stdout to the pipe
  dup2(pipefd[1], 2);   // send stderr to the pipe

  sclose(pipefd[1]);    // finally close write end of pipe

  char path[10]; // eg: "./code/999"
  sprintf(path, "%s/%d", CODE_PATH, *pgmNumber);
  char pgm[3];
  sprintf(pgm, "%d", *pgmNumber);

  sexecl(path, pgm, NULL);

}


void compilation_handler (void* arg1, void* arg2) {
  int *pipefd = arg1;
  int *pgmNumber = arg2;

  sclose(pipefd[0]);    // close read end of pipe for child

  dup2(pipefd[1], 1);   // send stdout to the pipe
  dup2(pipefd[1], 2);   // send stderr to the pipe

  sclose(pipefd[1]);    // finally close write end of pipe

  char output_file[10]; // ex: ./code/999
  char input_file[11];        // ex: ./code/999.c
  sprintf(output_file, "%s/%d", CODE_PATH, *pgmNumber);
  sprintf(input_file, "%s/%d.c", CODE_PATH, *pgmNumber);

  sexecl("/usr/bin/gcc", "gcc", "-o", output_file, input_file, NULL);

}

void execute_program (int pgmNumber, serverMessage* resp) {
  time_t start, end;
  int status;
  int pipefd[2];
  char buffer[MAX_CHAR];

  resp->pgmNum = pgmNumber;
  resp->execTime = 0;

  if (pgmNumber < 0 || pgmNumber > 999){
    resp->endStatus = PGM_NOT_FOUND;
    return;
  }

  // GET SEMAPHORE
  int sem_id = sem_get(SEM_KEY, 1);
  // GET SHARED MEMORY
  int shm_id = sshmget(SHM_KEY, sizeof(Programmes), 0);
  Programmes* s = sshmat(shm_id);
  
  // DOWN MUTEX
  sem_down0(sem_id);

  Programme p = (s->programmes)[pgmNumber];
  // TO VERIFY
  if( (p.nom)[0] == '\0') {
    resp->endStatus = PGM_NOT_FOUND;
    return;
  }

  spipe(pipefd);

  pid_t cpid_compilation = fork_and_run2(&compilation_handler, pipefd, &pgmNumber);

  close(pipefd[1]);  // close the write end of the pipe in the parent

  while (sread(pipefd[0], buffer, sizeof(buffer)) != 0)
  {
    // COMPILATION FAILED
    p.erreur = true;
    resp->endStatus = COMPILE_KO;
    resp->returnCode = -1;        // TO VERIFY
    strcpy(resp->output, buffer); // TO VERIFY
    return;
  }

  swaitpid(cpid_compilation, NULL, 0); // Wait for the compilation to be done
  

  // Now we can execute the pgm
  spipe(pipefd);
  time(&start); // Start Timer
  pid_t cpid_execution = fork_and_run2(&execution_handler, pipefd, &pgmNumber);
  close(pipefd[1]); 
  while (sread(pipefd[0], buffer, sizeof(buffer)) != 0)
  {
    //sleep(2);
    resp->endStatus = COMPILE_OK;
    strcpy(resp->output, buffer); // TO VERIFY

    // Stop Timer
    time(&end);
    resp->execTime = (int) difftime(end, start);
  }

  // Wait for execution_handler to finish execution
  swaitpid(cpid_execution, &status, 0);
  if ( WIFEXITED(status) ) {
      int code = WEXITSTATUS(status);
      resp->returnCode = code;  

      // TO VERIFY
      if(code == 0){
        resp->endStatus = PGM_STATUS_OK; 
      } else {
        resp->endStatus = PGM_STATUS_KO;
      }
  }

  // UPDATE PGM IN SHARED MEMORY
  p.nbrExec = p.nbrExec + 1;
  p.erreur = false;
  p.totalExec = p.totalExec + resp->execTime; //TO VERIFY
 
  // UP MUTEX
  sem_up0(sem_id);
  sshmdt(s);

}

/*
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
      // créer un fils pour chaque execution d'un pgm
      printf("Demande d'execution du programmes %d", msg.pgmNum);
      serverMessage* resp = malloc(sizeof(serverMessage));
      execute_program(msg.pgmNum, resp);

      //int cpid = fork_and_run1(&child_handler, msg.pgmNum);
    } else {
      // MODIFY PGM
      // check if pgm exists
      // 

    }

  }

  return 0;
}*/


/* FOR DEBUGGING PURPOSES */
int main(int argc, char const *argv[]){
  serverMessage* resp = malloc(sizeof(serverMessage));
  execute_program(1, resp);
  printf("PGM NUM         : %d\n",resp->pgmNum);
  printf("PGM END STATUS  : %d\n",resp->endStatus);
  printf("PGM EXEC TIME   : %d\n",resp->execTime);
  printf("PGM RETURN CODE : %d\n",resp->returnCode);
  printf("PGM OUTPUT      : \n>>> %s\n",resp->output);
  free(resp);
}
  

