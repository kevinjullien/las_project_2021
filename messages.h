#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#define MAX_NAME 256
#define MAX_CHAR 1024 // TO VERIFY

#define ADD_PGM -1
#define EXEC_PGM -2
// Le pgm compile
#define COMPILE_OK 0
// Le pgm ne compile pas
#define COMPILE_KO -1
// Le pgm n'existe pas
#define PGM_NOT_FOUND -2
// Le pgm s'est terminé normalement
#define PGM_STATUS_OK 1
// Le pgm ne s'est pas terminé normalement
#define PGM_STATUS_KO 0


typedef struct {
    int code; // -1 (add) or -2 (execute) or the pgm number (modif)
    int pgmNum;
    int nameLength;
    char name[MAX_NAME];
    char file[MAX_CHAR];
} clientMessage;

typedef struct {
    int pgmNum;
    int compileFlag;
    char compileError[MAX_CHAR];
    int endStatus; 
    int execTime;
    int returnCode;
    char output[MAX_CHAR];
} serverMessage;

#endif
