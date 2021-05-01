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

#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT 9090
#define VAL 7

//Recursif program execution
void recProgExec(int numprog);
//heartBeat son
void heartBeat(int frequency,int* pipe2,int* pipe3);
//recursifExecutor son
void recursifExecutor(int numprog, int* pipe1);
//Add file C
void addFileC();
//modify file C
void editFileC(int numprog);
//execute program
void executeProgam(int numprog);

int main(int argc, char **argv) {

    printf("Welcome !\n");

    char command = '>';
    int numprog = 0;

    while (command != 'q')
    {
        printf("What do you want to do ?\n");
        scanf("%d",&command);
        /*---------Add file C------------*/
        if (command == '+')
        {
           addFileC();
        }

        /*---------modify file C----------*/
        else if (command == '.')
        {
            printf("Give the num of the program you want to edit\n");
            scanf("%d",&numprog);

            editFileC(numprog);
        }

        /*--------HeartBeat program execution--------*/
        else if (command == '*')
        {
            printf("Give the num of the program you want to launch\n");
            scanf("%d",&numprog);

            //Recursif program execution
            void recProgExec(int numprog);
        }

        /*---------Execute program---------*/
        else if (command == '@')
        {
            printf("Give the num of the program you want to launch\n");
            scanf("%d",&numprog);

            executeProgam(numprog);
        }
        
        else
        {
            printf("This command is not available use '+' , '.' , '*' , '@' or 'q'\n");
        }
        printf("Bye");
    }
}

//Recursif program execution
void recProgExec(int numprog){
    //shared memory access
    int frequency = 0;
    int p1[2];
    spipe(p1);

    //start recusif execution
    //pid_t pidRecExec = fork_and_run2(recursifExecutor,numprog,&p1); /// au choix
    pid_t pidRecExe = sfork();
    if (pidRecExe == 0)
    {
        sclose(p1[0]);
        int p2[2];
        int p3[2];
        spipe(p2);
        spipe(p3);

        //start heart beat
        //pid_t pidHeartBeat = fork_and_run3(heartBeat,frequency,&p2,&p3); /// au choix
        pid_t pidHeartBeat = sfork();
        if (pidHeartBeat == 0)
        {
            sclose(p2[0]);
            sclose(p3[1]);
            int start = 1;
            int go = 1;
            
            while (start == 1)
            {
                sleep(frequency);
                //start process
                swrite(p2[1],&go,sizeof(int));
                //end process
                sread(p3[0],&go,sizeof(int));
            }
    
            sclose(p2[1]);
            sclose(p3[0]);
        }else
        {
            sclose(p2[1]);
            sclose(p3[0]);
            int start = 1;
            int exec = 1;
            int data = 666;

            while (start == 1)
            {
                //Execute the program once
                sread(p2[0],&exec,sizeof(int));
                //Send data to terminal
                swrite(p1[1],&data,sizeof(int));
                //End program once 
                swrite(p3[1],&exec,sizeof(int));
            }

            sclose(p2[0]);
            sclose(p3[1]);
        }
        sclose(p1[1]);
    }else
    {
        sclose(p1[1]);

        sclose(p1[0]);
    }
}
//heartBeat son
void heartBeat(int frequency,int* pipe2,int* pipe3){
    sclose(pipe2[0]);
    sclose(pipe3[1]);
    int start = 1;
    int go = 1;
    
    while (start == 1)
    {
        sleep(frequency);
        //start process
        swrite(pipe2[1],&go,sizeof(int));
        //end process
        sread(pipe3[0],&go,sizeof(int));
    }

    sclose(pipe2[1]);
    sclose(pipe3[0]);
}
//recursifExecutor son
void recursifExecutor(int numprog, int* pipe1){
    int frequency = 0;
    sclose(pipe1[0]);
    int p2[2];
    int p3[2];
    spipe(p2);
    spipe(p3);

    //start heart beat
    pid_t pidHeartBeat = fork_and_run3(heartBeat,frequency,&p2,&p3);
    
    sclose(p2[1]);
    sclose(p3[0]);
    int start = 1;
    int exec = 1;
    int data = 666;

    while (start == 1)
    {
        //Execute message
        sread(p2[0],&exec,sizeof(int));
        //Execute program once
        executeProgam(numprog);
        //End program once 
        swrite(p3[1],&exec,sizeof(int));
    }

    sclose(p2[0]);
    sclose(p3[1]);
    
    sclose(pipe1[1]);

}
//Add file C
void addFileC(){

}
//modify file C
void editFileC(int numprog){

}
//execute program
void executeProgam(int numprog){ /// Les message ici doivent etre modifer en char ou int je n'aime pas les tableau
    char* message = "Do it! ... Just do it!";
    // socket creation
    int sockfd = ssocket();
    // socket connection
    sconnect(LOCAL_HOST, SERVER_PORT, sockfd);
    // send execute message to serveur
    swrite(sockfd,&message,sizeof(char)*22);
    // response from serveur (ok / ko)
    sread(sockfd,&message,sizeof(char)*2);
    if (message == "ko"){printf("Error in the execution");}
    if (message == "ok"){
        printf("Progam executing ...");
        while (message != "end")
        {
            sread(sockfd,&message,sizeof(char)*256);    /// Il va y avoir une erreur de taille ici ? a corriger
        }
    }
    sclose(sockfd);
}
