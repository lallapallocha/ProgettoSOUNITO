#include "utils.h"

int main()
{
    char *PATH[] = {"./bin/atomo", NULL};
    int i;
    pid_t pid_child;
    struct msg my_message;
    int msgqueue_id;
    int shm_id;
    int sem_id;
    struct shared_data *my_data;

    setvbuf(stdout, NULL, _IONBF, 0);

    if ((sem_id = semget(KEY, 3, 0666)) == -1)
        TEST_ERROR;

    if ((msgqueue_id = msgget(KEY, 0666)) == -1)
        TEST_ERROR;

    if ((shm_id = shmget(KEY, sizeof(struct shared_data), 0666)) == -1)
        TEST_ERROR;

    my_data = shmat(shm_id, NULL, 0);

#ifdef DEBUG
    printf("(ALIMENTAZIONE) my PID is %d, my PPID is %d \n", getpid(), getppid());
#endif

    // semaforo 1
    sem_reserve(sem_id, 0);
    TEST_ERROR;

    // semaforo 2
    sem_wait_until_zero(sem_id, 1);
    TEST_ERROR;

    struct timespec my_time;
    my_time.tv_sec = 0;
    my_time.tv_nsec = my_data->STEP_ALIMENTAZIONE;

    while (my_data->stato_simulazione)
    {
        nanosleep(&my_time, NULL);
#ifdef DEBUG
        TEST_ERROR;
#endif
        for (i = 0; i < my_data->N_NUOVI_ATOMI; i++)
        {
            switch (fork())
            {
            case -1:
                TEST_ERROR;
                sem_reserve(sem_id, 2);
                my_data->stato_simulazione = 0;
                my_data->reason_of_end_of_simulation = "MELTDOWN";
                sem_release(sem_id, 2);
                exit(EXIT_FAILURE);
            case 0:
#ifdef DEBUG
                fprintf(stderr, "(CHILD PROCESSO ATTIVATORE) %d\n", getpid());
#endif
                my_message.mtype = 1;
                my_message.pid = getpid();
                if (msgsnd(msgqueue_id, &my_message, LEN, 0) == -1)
                    TEST_ERROR;
                if (execv("./bin/atomo", PATH) == -1)
                    TEST_ERROR;
                exit(EXIT_FAILURE);
            default:
                break;
            }
        }
    }

    while ((pid_child = wait(NULL)) != -1)
    {
#ifdef DEBUG
        fprintf(stderr, "(ALIMENTAZIONE) child terminato: %d\n", pid_child);
#endif
    }

    exit(EXIT_SUCCESS);
}