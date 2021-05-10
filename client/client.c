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
//recursifExecutor son
void recursifExecutor(void* numprog, void* pipe);
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
    
    fork_and_run2(heartBeat,&delay,pipe);
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
           //addFileC(&sockfd);
        }

        /*---------modify file C----------*/
        else if (command == '.')
        {
            printf("Give the num of the program you want to edit\n");
            scanf("%d",&numprog);

            //editFileC(&numprog,&sockfd);
        }

        /*--------HeartBeat program execution--------*/
        else if (command == '*')
        {
            printf("Give the num of the program you want to launch\n");
            scanf("%d",&numprog);

            //Recursif program execution
            fork_and_run2(recursifExecutor,&numprog,pipe);
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
            //sclose(sockfd);
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
    int go = 1;
    while (true)
    {
        sleep(*freq);
        //start process
        swrite(p[1],&go,sizeof(int));
    }

    sclose(p[1]);
}
//recursifExecutor son
void recursifExecutor(void* numprog, void* pipe){
    int* numprogram = numprog;
    int* p = pipe;

    sclose(p[1]);
    
    int exec = 1;

    while (true)
    {
        //Execute message
        sread(p[0],&exec,sizeof(int));
        //Execute program once
        executeProgam(numprogram);
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

    swrite(*sockfd,&clientMessage,sizeof(clientMessage));
    sread(*sockfd,&serverMessage,sizeof(serverMessage));

    if (serverMessage.endStatus != 0)
    {
        printf("The program n°%d don't compile.\n",serverMessage.pgmNum);
        printf("Error message : %s\n",serverMessage.compileError);        
    }else
    {
        printf("The program n°%d compile correctly.\n",serverMessage.pgmNum);
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
    clientMessage.pgmNum = *numprog;
    clientMessage.nameLength = strlen(name);
    strcpy(clientMessage.name,name);

    swrite(*sockfd,&clientMessage,sizeof(clientMessage));
    sread(*sockfd,&serverMessage,sizeof(serverMessage));

    if (serverMessage.endStatus != 0)
    {
        printf("The program n°%d don't compile.\n",*numprog);
        printf("Error message : %s\n",serverMessage.compileError);        
    }else
    {
        printf("The program n°%d compile correctly.\n",*numprog);
    }
    
}
//execute program
void executeProgam(int* numprog){ /// Les message ici doivent etre modifer en char ou int je n'aime pas les tableau
    // socket creation
    int sockfd = ssocket();
    // socket connection
    sconnect(local_host, server_port, sockfd);

    serverMessage serverMessage;
    clientMessage clientMessage;
    clientMessage.pgmNum = *numprog;
    clientMessage.code = -2;

    swrite(sockfd,&clientMessage,sizeof(clientMessage));
    sread(sockfd,&serverMessage,sizeof(serverMessage));

    if (serverMessage.endStatus == -2)
    {
        printf("The program n°%d don't exist.\n",*numprog);
    }else if (serverMessage.endStatus == -1)
    {
        printf("The program n°%d don't compile.\n",*numprog);
    }else if (serverMessage.endStatus == 0)
    {
        printf("The program n°%d has an unexpected comportement :\n",*numprog);
    }else if (serverMessage.endStatus == 1)
    {
        printf("The program n°%d ended safely.\n", *numprog);
        printf("Time : %d\n",serverMessage.execTime);
        printf("Code : %d\n",serverMessage.returnCode);
        printf("stdout : %s\n",serverMessage.output);
    }
    sclose(sockfd);
}
