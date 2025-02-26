#ifndef __UTILS_H__
#define __UTILS_H__
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/msg.h>
#include <stddef.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#define TEST_ERROR                                \
    if (errno)                                    \
    {                                             \
        fprintf(stderr,                           \
                "%s:%d: PID=%d: Error %d (%s)\n", \
                __FILE__,                         \
                __LINE__,                         \
                getpid(),                         \
                errno,                            \
                strerror(errno));                 \
    }

#define KEY 0x123456

#define LEN sizeof(sizeof(struct msg) - sizeof(long))

struct msg
{
    long mtype;
    pid_t pid;
};

struct shared_data
{
    int N_ATOMI_INIT;
    int N_ATOM_MAX;
    int STEP_ALIMENTAZIONE;
    int N_NUOVI_ATOMI;
    int MIN_N_ATOMICO;
    int SIM_DURATION;
    int ENERGY_DEMAND;
    int ENERGY_EXPLODE_THRESHOLD;
    int STEP_ATTIVATORE;
    int n_attivazioni;
    int n_attivazioni_last_sec;
    int n_scissioni;
    int n_scissioni_last_sec;
    int qty_energia_prodotta;
    int qty_energia_prodotta_last_sec;
    int qty_energia_consumata;
    int qty_scorie_prodotte;
    int qty_scorie_prodotte_last_sec;
    int qty_energia_totale;
    int stato_simulazione;
    char *reason_of_end_of_simulation;
};

// computes freed energy
int energy(int n1, int n2);

// APIs to use semaphores

// set semaphore to a certain value
int sem_set_val(int sem_id, int sem_num, int sem_val);

// access resource by decreasing value of semaphore
int sem_reserve(int sem_id, int sem_num);

// release resource by increasing value of semaphore
int sem_release(int sem_id, int sem_num);

// sync all processes on a semaphore to wait for a certain process
int sem_wait_until_zero(int sem_id, int sem_num);

#endif