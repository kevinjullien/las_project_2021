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

#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT 9090
#define VAL 7

//Recursif program execution
void recProgExec(int numprog);
//heartBeat son
void heartBeat(void* frequency,void* pipe2,void* pipe3);
//recursifExecutor son
void recursifExecutor(void* numprog, void* pipe1);
//Add file C
void addFileC(int* sockfd);
//modify file C
void editFileC(int* numprog, int * sockfd);
//execute program
void executeProgam(int* numprog);

int main(int argc, char **argv) {
    /*
    // socket creation
    int sockfd = ssocket();
    // socket connection
    sconnect(LOCAL_HOST, SERVER_PORT, sockfd);
    */
    printf("Welcome !\n");

    char command = '>';
    int numprog = 0;

    while (command != 'q')
    {
        printf("What do you want to do ?\n");
        scanf("%c",&command);
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
            recProgExec(numprog);
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

//Recursif program execution
void recProgExec(int numprog){
    int p1[2];
    spipe(p1);

    //start recusif execution
    fork_and_run2(recursifExecutor,&numprog,&p1); /// au choix
    
    sclose(p1[1]);

    sclose(p1[0]);

}
//heartBeat son
void heartBeat(void* frequency,void* pipe2,void* pipe3){
    int* freq = frequency;
    int* p2 = pipe2;
    int* p3 = pipe3;
    sclose(p2[0]);
    sclose(p3[1]);
    int start = 1;
    int go = 1;
    
    while (start == 1)
    {
        sleep(*freq);
        //start process
        swrite(p2[1],&go,sizeof(int));
        //end process
        sread(p3[0],&go,sizeof(int));
    }

    sclose(p2[1]);
    sclose(p3[0]);
}
//recursifExecutor son
void recursifExecutor(void* numprog, void* pipe1){
    int* numprogram = numprog;
    int* p1 = pipe1;
    int frequency = 0;

    sclose(p1[0]);
    int p2[2];
    int p3[2];
    spipe(p2);
    spipe(p3);

    //start heart beat
    fork_and_run3(heartBeat,&frequency,&p2,&p3);
    
    sclose(p2[1]);
    sclose(p3[0]);
    int start = 1;
    int exec = 1;

    while (start == 1)
    {
        //Execute message
        sread(p2[0],&exec,sizeof(int));
        //Execute program once
        executeProgam(numprogram);
        //End program once 
        swrite(p3[1],&exec,sizeof(int));
    }

    sclose(p2[0]);
    sclose(p3[1]);
    
    sclose(p1[1]);

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
    strcpy(clientMessage.file,file);
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
    strcpy(clientMessage.file,file);
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
    sconnect(LOCAL_HOST, SERVER_PORT, sockfd);

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
