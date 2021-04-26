#include "../utils_v10.h"

int main(int argc, char const *argv[])
{
    if (argc < 2)
        printf("Usage : maint type [opt]\nOù \"type\" est\n\t1, il crée les ressources partagées\n\t2, il détruit les ressources partagées\n\t3, il réserve de façon exclusive le répertoire de code partagé");
    return 0;
}
