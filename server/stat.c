#include "../utils_v10.h"
#include "ipc_conf.h"

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./stat n\nOù n est le numéro associé à un programme\n");
        exit(1);
    }

    int shm_id = sshmget(SHM_KEY, sizeof(Programmes), 0);
    int sem_id = sem_get(SEM_KEY, 1);

    Programmes *programmes = sshmat(shm_id);

    sem_down0(sem_id);

    printf("Nbr total de programmes %d\n", programmes->taille);

    int numProg = atoi(argv[1]);
    if (numProg < 0 || numProg >= programmes->taille)
    {
        printf("Ce programme n'existe pas\n");
        sem_up0(sem_id);
        sshmdt(programmes);
        exit(1);
    }
    Programme prog = (programmes->programmes)[numProg];
    printf("%d\n%s\n%s\n%d\n%ld\n", prog.num, prog.nom, prog.erreur ? "true" : "false", prog.nbrExec, prog.totalExec);

    sem_up0(sem_id);
    sshmdt(programmes);
    exit(0);
}
