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
#define CODE_PATH "./code"

//void addProgram(clientMessage msg, int newsockfd);

int initSocketServer(int port)
{
  int sockfd = ssocket();
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

void executeProgram (int pgmNumber, int newsockfd) {
  serverMessage* resp = smalloc(sizeof(serverMessage));
  struct timeval stop, start;
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
    strcpy(resp->output, buffer); // TO VERIFY
    return;
  }

  swaitpid(cpid_compilation, NULL, 0); // Wait for the compilation to be done
  

  // Now we can execute the pgm
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

  //DEBUG
  printf("end status : %d\n",resp->endStatus);
  printf("time : %d\n",resp->execTime);
  printf("Ret code: %d\n",resp->returnCode);
  printf("stdout: %s\n",resp->output);
  printf("%ld\n", strlen(resp->output));


  swrite(newsockfd, resp, sizeof(resp));
  free(resp);

}

  


void addProgram(clientMessage req, int newsockfd)
{ 
  serverMessage* resp = smalloc(sizeof(serverMessage));
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
  strcpy(programme.nom, req.name);
  programme.num = num;
  resp->pgmNum = num;

  /* Création et récupération des données du fichier */

  char path[30]; //TODO bof, voir avec constante
  sprintf(path, "%s/%d.c", CODE_PATH, num);

  int fd = sopen(path, O_WRONLY | O_TRUNC| O_CREAT, 0644);

  void* file = smalloc(req.filesize);
  sread(newsockfd, file, req.filesize);
  swrite(fd, file, req.filesize);
  free(file);
  sclose(fd);


  /* Compilation du programme */

  int pipefd[2];
  spipe(pipefd);

  pid_t cpid_compilation = fork_and_run2(&compilation_handler, pipefd, &num);
  close(pipefd[1]);

  char buffer[MAX_CHAR];
  while (sread(pipefd[0], buffer, sizeof(buffer)) != 0)
  {
    // Compilation error
    programme.erreur = true;
    resp->endStatus = COMPILE_KO;
    strcpy(resp->output, buffer);
  }

  swaitpid(cpid_compilation, NULL, 0); // Wait for the compilation to be done 

  /* Création des données dans la mémoire partagée */

  sem_down0(sem_id);
  (programmes->programmes)[num] = programme;
  programmes->taille = programmes->taille + 1;

  sem_up0(sem_id);

  swrite(newsockfd, resp, sizeof(resp));
  free(resp);
}


void editProgram (clientMessage req, int newsockfd)
{
  serverMessage* resp = smalloc(sizeof(serverMessage));
  int shm_id = sshmget(SHM_KEY, sizeof(Programmes), 0);
  int sem_id = sem_get(SEM_KEY, 1);

  int num = req.pgmNum;

  if(num < 0 || num > 999){
    resp->endStatus = PGM_NOT_FOUND;
    return;
  }

  Programmes *programmes = sshmat(shm_id);
  Programme p = (programmes->programmes)[num];

  // TO VERIFY
  if( (p.nom)[0] == '\0') {
    resp->endStatus = PGM_NOT_FOUND;
    return;
  }

  Programme programme;
  strcpy(programme.nom, req.name);
  programme.num = num;
  resp->pgmNum = num;

  /* récupération des données du fichier */

  char path[30]; //TODO bof, voir avec constante
  sprintf(path, "%s/%d.c", CODE_PATH, num);

  int fd = sopen(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);

  char* file = smalloc(req.filesize*sizeof(char));
  int nbChar = sread(newsockfd, file, req.filesize);
  if( nbChar != req.filesize) {
    printf("DEBUG: error reading file");
  }
  swrite(fd, file, nbChar);
  free(file);
  sclose(fd);

  /* Compilation du programme */

  int pipefd[2];
  spipe(pipefd);

  pid_t cpid_compilation = fork_and_run2(&compilation_handler, pipefd, &num);
  close(pipefd[1]);

  char buffer[MAX_CHAR];
  while (sread(pipefd[0], buffer, sizeof(buffer)) != 0)
  {
    // Compilation error
    programme.erreur = true;
    resp->endStatus = COMPILE_KO;
    strcpy(resp->output, buffer);
  }

  swaitpid(cpid_compilation, NULL, 0); // Wait for the compilation to be done

    

  /* Modification des données dans la mémoire partagée */

  sem_down0(sem_id);
  (programmes->programmes)[num] = programme;
  sem_up0(sem_id);
}




int main(int argc, char const *argv[])
{
  int sockfd, newsockfd;
  clientMessage msg;
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
    sread(newsockfd, &msg, sizeof(msg));

    if (msg.code == ADD_PGM)
    { 
      printf("client request : ADD NEW PROGRAM\n");
      addProgram(msg, newsockfd);
    }
    else if (msg.code == EXEC_PGM)
    {
      printf("client request : EXECUTE PROGRAM N°%d\n", msg.pgmNum);
      executeProgram(msg.pgmNum, newsockfd);
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