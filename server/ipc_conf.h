#ifndef _IPC_CONF_H_
#define _IPC_CONF_H_

#include <stdbool.h>

#define SEM_KEY 123
#define SHM_KEY 456
#define PERM 0666

typedef struct
{
    char nom[256];
    int num;
    bool erreur;
    int nbrExec;
    long totalExec;
} Programme;

typedef struct
{
    int qte;
    Programme programmes[1000];
} Programmes;



#endif  // _IPC_CONF_H_