/**
 * @file client.c
 * @authors Furnelle-Jullien-Valbuena
 * @date 2021-05-16
 * 
 */
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../utils_v10.h"
#include "../messages.h"

char *local_host = "";
int server_port = 0;

//heartBeat child
void heartBeat(void *frequency, void *pipe);
//recurrentExecutor child
void recurrentExecutor(void *pipe);
//Add file C
void addFileC(int *sockfd, char *path);
//modify file C
void editFileC(int *sockfd, int *numprog, char *path);
//execute program
void executeProgam(int *numprog);

int main(int argc, char **argv)
{

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
    fork_and_run2(heartBeat, &delay, pipe);
    fork_and_run1(recurrentExecutor, pipe);

    // socket creation
    int sockfd = ssocket();
    // socket connection
    sconnect(local_host, server_port, sockfd);

    printf("Welcome !\n");

    char buff[MAX_CHAR];
    char command = '>';
    int numprog = 0;
    char path[MAX_CHAR];

    while (1)
    {
        printf("What do you want to do ?\n");
        sread(0, buff, MAX_CHAR);
        command = buff[0];

        if (command == 'q')
        {
            printf("bye\n");
            break;
        }

        /*---------Parsing received string------------*/
        char *ptr = strtok(buff, " ");

        if (command == '*' || command == '@' || command == '.')
        {
            ptr = strtok(NULL, " ");
            numprog = atoi(ptr);
        }
        else
        {
            ptr = strtok(NULL, "\n");
            strcpy(path, ptr);
        }
        if (command == '.')
        {
            ptr = strtok(NULL, "\n");
            strcpy(path, ptr);
        }

        /*---------Add file C------------*/
        if (command == '+')
        {
            addFileC(&sockfd, path);
        }

        /*---------edit file C----------*/
        else if (command == '.')
        {
            editFileC(&sockfd, &numprog, path);
        }

        /*--------Recurrent program execution--------*/
        else if (command == '*')
        {
            //Message to add a new program to execute in the recurrent son
            swrite(pipe[1], &numprog, sizeof(int));
        }

        /*---------Execute program---------*/
        else if (command == '@')
        {
            executeProgam(&numprog);
        }

        else
        {
            printf("This command is not available use '+' , '.' , '*' , '@' or 'q'\n");
        }
    }
    sclose(sockfd);
}

/**
 * This program will simulate a heartbeat at a regular rate.
 * Every heartbeat will send '-1' in the pipe.
 * 
 * @param frequency the frequency in seconds
 * @param pipe the pipe to send the data
 */
void heartBeat(void *frequency, void *pipe)
{
    int *freq = frequency;

    int *p = pipe;
    sclose(p[0]);

    int go = -1;

    while (true)
    {
        sleep(*freq);
        //start process message
        swrite(p[1], &go, sizeof(int));
    }

    sclose(p[1]);
}

/**
 * The program will receive, via a pipe, a heartbeat and new programs to run.
 * Every heartbeat, the program will runs the programs in its list, max 50.
 * If a number between 0 and 99 is sent, it will be considered as a new program to run.
 * 
 * @param pipe the pipe to receive communication from the client and the heartbeat.
 */
void recurrentExecutor(void *pipe)
{
    int programs[50];
    int pointeur = 0;

    int *p = pipe;
    sclose(p[1]);

    int exec = -1;

    while (true)
    {
        //Receive heartBeat or a new program to launch
        sread(p[0], &exec, sizeof(int));

        //New program to add in the launch list
        if (exec != -1)
        {
            if (pointeur < 50)
            {
                programs[pointeur++] = exec;
            }
        }

        else
        {
            for (int i = 0; i < pointeur; i++)
            {
                //Execute program once
                //executeProgam(&programs[i]);
                executeProgam(&programs[i]);
            }
        }
    }
    sclose(p[0]);
}

/**
 * Add a program on the server. 
 * 
 * @param sockfd the socket fd
 */
void addFileC(int *sockfd, char *path)
{
    serverMessage serverMessage;
    clientMessage clientMessage;
    char *progname = basename(path);

    clientMessage.code = ADD_PGM;
    clientMessage.nameLength = strlen(progname);
    strcpy(clientMessage.name, progname);

    int fd = sopen(path, O_RDONLY, 0644);
    // Size of the file
    struct stat stat;
    fstat(fd, &stat);
    clientMessage.filesize = stat.st_size;
    // Copy of the file
    char *content = smalloc(clientMessage.filesize);
    int res = sread(fd, content, clientMessage.filesize);
    if (res != clientMessage.filesize)
    {
        perror("ERROR READ");
        return;
    }

    // Give the message and the file content to the server
    swrite(*sockfd, &clientMessage, sizeof(clientMessage));
    swrite(*sockfd, content, clientMessage.filesize);
    free(content);
    // Answer from the server
    res = sread(*sockfd, &serverMessage, sizeof(serverMessage));
    if (res != sizeof(serverMessage))
    {
        perror("ERROR READ");
        return;
    }

    if (serverMessage.endStatus != COMPILE_OK)
    {
        printf("The program n°%d doesn't compile.\n", serverMessage.pgmNum);
        printf("Error message : %s\n", serverMessage.output);
    }
    else
    {
        printf("The program n°%d compile correctly.\n", serverMessage.pgmNum);
        printf("%s\n", serverMessage.output);
    }
}

/**
 * Edit a program existing on the server with new data. 
 * 
 * @param numprog the program number
 * @param sockfd the socket fd
 */
void editFileC(int *sockfd, int *numprog, char *path)
{
    serverMessage serverMessage;
    clientMessage clientMessage;
    char *progname = basename(path);

    clientMessage.code = *numprog;
    clientMessage.nameLength = strlen(progname);
    strcpy(clientMessage.name, progname);

    // Size of the file
    int fd = sopen(path, O_RDONLY, 0644);
    clientMessage.filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    // Copy of the file
    char *content = smalloc(clientMessage.filesize);
    int res = sread(fd, content, clientMessage.filesize);
    if (res != clientMessage.filesize)
    {
        perror("ERROR READ");
        return;
    }
    // Give the message and the file content to the server
    swrite(*sockfd, &clientMessage, sizeof(clientMessage));
    swrite(*sockfd, content, clientMessage.filesize);
    // Answer from the server
    res = sread(*sockfd, &serverMessage, sizeof(serverMessage));
    if (res != sizeof(serverMessage))
    {
        perror("ERROR READ");
        return;
    }

    if (serverMessage.endStatus != COMPILE_OK)
    {
        printf("The program n°%d doesn't compile.\n", serverMessage.pgmNum);
        printf("Error message : %s\n", serverMessage.output);
    }
    else
    {
        printf("The program n°%d compile correctly.\n", serverMessage.pgmNum);
        printf("%s\n", serverMessage.output);
    }
}

/**
 * Execute program on server, with a new connection.
 * 
 * @param numprog the program number
 */
void executeProgam(int *numprog)
{
    // socket creation
    int sockfd = ssocket();
    // socket connection
    sconnect(local_host, server_port, sockfd);

    serverMessage serverMessage;
    clientMessage clientMessage;
    clientMessage.pgmNum = *numprog;
    clientMessage.code = EXEC_PGM;

    swrite(sockfd, &clientMessage, sizeof(clientMessage));

    int res = sread(sockfd, &serverMessage, sizeof(serverMessage));
    if (res != sizeof(serverMessage))
    {
        perror("ERROR READ");
        return;
    }

    if (serverMessage.endStatus == PGM_NOT_FOUND)
    {
        printf("The program n°%d doesn't exist.\n", *numprog);
    }
    else if (serverMessage.endStatus == COMPILE_KO)
    {
        printf("The program n°%d doesn't compile.\n", *numprog);
    }
    else if (serverMessage.endStatus == PGM_STATUS_KO)
    {
        printf("The program n°%d has an unexpected behaviour\n", *numprog);
        printf("Code : %d\n", serverMessage.returnCode);
        printf("stdout : %s\n", serverMessage.output);
    }
    else if (serverMessage.endStatus == PGM_STATUS_OK)
    {
        printf("The program n°%d ended safely.\n", *numprog);
        printf("Time : %d\n", serverMessage.execTime);
        printf("Code : %d\n", serverMessage.returnCode);
        printf("stdout : %s\n", serverMessage.output);
    }
    sclose(sockfd);
}