#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>


#include "../utils_v10.h"
#include "../messages.h"

char* local_host = "";
int server_port = 0;

//heartBeat son
void heartBeat(void* frequency,void* pipe);
//recurrentExecutor son
void recurrentExecutor(void* pipe);
//Add file C
void addFileC(int* sockfd);
//modify file C
void editFileC(int* numprog, int * sockfd);
//execute program
void executeProgam(int* numprog);

int main(int argc, char **argv) {

    if (argc != 4)
    {
        printf("ERROR\nYou need to enter 3 args to create a client :\n./client local_host(string) server_port(int) DELAY(int)\n");
        exit(1);
    }
    
    
    local_host = argv[1];
    server_port = atoi(argv[2]);
    int delay = atoi(argv[3]);

    int pipe[2];
    spipe(pipe);

    // son creation
    fork_and_run2(heartBeat,&delay,pipe);
    fork_and_run1(recurrentExecutor,pipe);

    // socket creation
    int sockfd = ssocket();
    // socket connection
    sconnect(local_host, server_port, sockfd);
    
    printf("Welcome !\n");

    char command = '>';
    int numprog = 0;

    while (command != 'q')
    {
        printf("What do you want to do ?\n");
        scanf(" %c",&command);

        /*---------Add file C------------*/
        if (command == '+')
        {
           addFileC(&sockfd);
        }

        /*---------modify file C----------*/
        else if (command == '.')
        {
            printf("Give the num of the program you want to edit\n");
            scanf("%d",&numprog);

            editFileC(&numprog,&sockfd);
        }

        /*--------HeartBeat program execution--------*/
        else if (command == '*')
        {
            printf("Give the num of the program you want to launch\n");
            scanf("%d",&numprog);

            //Message to add a new program to execute in the recurrent son
            swrite(pipe[1],&numprog,sizeof(int));
        }

        /*---------Execute program---------*/
        else if (command == '@')
        {
            printf("Give the num of the program you want to launch\n");
            scanf("%d",&numprog);

            executeProgam(&numprog);
        }

        else if (command == 'q')
        {
            printf("bye\n");
            sclose(sockfd);
        }
        
        else
        {
            printf("This command is not available use '+' , '.' , '*' , '@' or 'q'\n");
        }
    }
}


//heartBeat son
void heartBeat(void* frequency,void* pipe){
    
    int* freq = frequency;
    
    int* p = pipe;
    sclose(p[0]);
    
    int go = -1;
    
    while (true)
    {
        sleep(*freq);
        //start process message 
        swrite(p[1],&go,sizeof(int));
    }

    sclose(p[1]);
}
//recurrentExecutor son
void recurrentExecutor(void* pipe){

    int programs[50];
    int pointeur = 0;

    int* p = pipe;
    sclose(p[1]);
    
    int exec = -1;

    while (true)
    {
        //Recive heartBeat or a new program to launch
        sread(p[0],&exec,sizeof(int));

        //New program to add in the launch list 
        if (exec != -1)
        {
            programs[pointeur] = exec;
            exec = -1;
            pointeur++;
        }

        for (int i = 0; i < pointeur; i++)
        {
            //Execute program once
            executeProgam(&programs[0]);
        }
    }
    sclose(p[0]);
}
//Add file C
void addFileC(int* sockfd){
    char file [MAX_CHAR];
    char name [MAX_NAME];

    printf("Give me the file location\n");
    scanf("%s",file);
    printf("Give me name of the file\n");
    scanf("%s",name);

    serverMessage serverMessage;
    clientMessage clientMessage;
    
    clientMessage.code = -1;
    clientMessage.nameLength = strlen(name);
    strcpy(clientMessage.name,name);
    // Size of the file
    int fd = sopen(file, O_RDONLY, 0644);
    clientMessage.filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    // Copy of the file 
    char* content = smalloc(clientMessage.filesize);
    int res = sread(fd, content, clientMessage.filesize);
    if (res != clientMessage.filesize){
        perror("ERROR READ");
        return;
    }
    // Give the message and the file content to the server  
    swrite(*sockfd,&clientMessage,sizeof(clientMessage));
    swrite(*sockfd,content, clientMessage.filesize);
    // Answer from the server
    res = sread(*sockfd,&serverMessage,sizeof(serverMessage));
    if (res != sizeof(serverMessage)){
        perror("ERROR READ");
        return;
    }

    if (serverMessage.endStatus != COMPILE_OK)
    {
        printf("The program n°%d doesn't compile.\n",serverMessage.pgmNum);
        printf("Error message : %s\n",serverMessage.output);        
    }else
    {
        printf("The program n°%d compile correctly.\n",serverMessage.pgmNum);
        printf("%s\n", serverMessage.output);
    }
    
}
//modify file C
void editFileC(int* numprog, int* sockfd){
    char file [MAX_CHAR];
    char name [MAX_NAME];

    printf("Give me the file location\n");
    scanf("%s",file);
    printf("Give me name of the file\n");
    scanf("%s",name);

    serverMessage serverMessage;
    clientMessage clientMessage;
    
    clientMessage.code = *numprog;
    clientMessage.nameLength = strlen(name);
    strcpy(clientMessage.name,name);

    // Size of the file
    int fd = sopen(file, O_RDONLY, 0644);
    clientMessage.filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    // Copy of the file 
    char* content = smalloc(clientMessage.filesize);
    int res = sread(fd, content, clientMessage.filesize);
    if (res != clientMessage.filesize){
        perror("ERROR READ1");
        return;
    }
    // Give the message and the file content to the server  
    swrite(*sockfd,&clientMessage,sizeof(clientMessage));
    swrite(*sockfd,content, clientMessage.filesize);
    // Answer from the server
    res = sread(*sockfd,&serverMessage,sizeof(serverMessage));
    if (res != sizeof(serverMessage)){
        perror("ERROR READ2");
        return;
    }

   if (serverMessage.endStatus != COMPILE_OK)
    {
        printf("The program n°%d doesn't compile.\n",serverMessage.pgmNum);
        printf("Error message : %s\n",serverMessage.output);        
    }else
    {
        printf("The program n°%d compile correctly.\n",serverMessage.pgmNum);
        printf("%s\n", serverMessage.output);
    }
    
}
//execute program
void executeProgam(int* numprog){ 
    // socket creation
    int sockfd = ssocket();
    // socket connection
    sconnect(local_host, server_port, sockfd);

    serverMessage serverMessage;
    clientMessage clientMessage;
    clientMessage.pgmNum = *numprog;
    clientMessage.code = EXEC_PGM;

    swrite(sockfd,&clientMessage,sizeof(clientMessage));

    int res = sread(sockfd,&serverMessage,sizeof(serverMessage));
    if (res != sizeof(serverMessage)){
        perror("ERROR READ");
        return;
    }

    if (serverMessage.endStatus == PGM_NOT_FOUND)
    {
        printf("The program n°%d doesn't exist.\n",*numprog);
    }else if (serverMessage.endStatus == COMPILE_KO)
    {
        printf("The program n°%d doesn't compile.\n",*numprog);
    }else if (serverMessage.endStatus == PGM_STATUS_KO)
    {
        printf("The program n°%d has an unexpected behaviour :\n",*numprog);
    }else if (serverMessage.endStatus == PGM_STATUS_OK)
    {
        printf("The program n°%d ended safely.\n", *numprog);
        printf("Time : %d\n",serverMessage.execTime);
        printf("Code : %d\n",serverMessage.returnCode);
        printf("stdout : %s\n",serverMessage.output);
    }
    sclose(sockfd);
}
