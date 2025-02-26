#include "utils.h"

int energy(int n1, int n2)
{
    int max_atomic = (n1 > n2) ? n1 : n2;
    return (n1 * n2) - max_atomic;
}

// APIs to use semaphores

int sem_set_val(int sem_id, int sem_num, int sem_val)
{
    return semctl(sem_id, sem_num, SETVAL, sem_val);
}

int sem_reserve(int sem_id, int sem_num)
{
    struct sembuf sops;

    sops.sem_num = sem_num;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(sem_id, &sops, 1);
}

int sem_release(int sem_id, int sem_num)
{
    struct sembuf sops;

    sops.sem_num = sem_num;
    sops.sem_op = 1;
    sops.sem_flg = 0;

    return semop(sem_id, &sops, 1);
}

int sem_wait_until_zero(int sem_id, int sem_num)
{
    struct sembuf sops;

    sops.sem_num = sem_num;
    sops.sem_op = 0;
    sops.sem_flg = 0;

    return semop(sem_id, &sops, 1);
}