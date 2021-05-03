#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../utils_v10.h"
#include "ipc_conf.h"

int main(int argc, char const *argv[])
{
    char *badInputMessage = "Usage : ./maint type [opt]\nOù \"type\" est\n\t1. il crée les ressources partagées.\n\t2. il détruit les ressources partagées.\n\t3. il réserve de façon exclusive le répertoire de code partagé.\n\t   [opt] est le temps, en secondes, pendant lequel le répertoire sera réservé.\n\n";

    if (argc < 2 || (strcmp(argv[1], "1") != 0 && strcmp(argv[1], "2") != 0 && strcmp(argv[1], "3") != 0))
    {
        printf("%s", badInputMessage);
        exit(1);
    }

    int opt = atoi(argv[1]);
    int shm_id;
    int sem_id;
    switch (opt)
    {
    case 1:
        /* code */
        printf("Création ressources partagées\n");
        shm_id = sshmget(SHM_KEY, sizeof(Programmes), IPC_CREAT | IPC_EXCL | PERM);
        sem_create(SEM_KEY, 1, PERM, 1);

        // TODO Développement uniquement
        /* début développement */
        Programmes *s = sshmat(shm_id);
        Programme programme;

        strcpy(programme.nom, "HelloWorld.c");
        s->qte = 1;
        programme.erreur = false;
        programme.nbrExec = 10;
        programme.num = 0;
        programme.totalExec = 64;

        s->programmes[0] = programme;

        sshmdt(s);
        /* fin développement */

        break;

    case 2:
        printf("Destruction ressources partagées\n"); //TODO pas demandé
        shm_id = sshmget(SHM_KEY, sizeof(Programmes), 0);
        sem_id = sem_get(SEM_KEY, 1);

        sshmdelete(shm_id);
        sem_delete(sem_id);
        break;

    case 3:
        printf("Réservation ressources partagées\n"); //TODO pas demandé
        if (argc != 3){
            printf("%s", badInputMessage);
            exit(1);
        }
        int val = atoi(argv[2]);

        sem_id = sem_get(SEM_KEY, 1);

        printf("down\n"); //TODO Développement uniquement
        sem_down0(sem_id);
        

        sleep(val);

        printf("up\n"); // TODO Développement uniquement
        sem_up0(sem_id);

        break;
    }
}
