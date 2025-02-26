#include "utils.h"

void sigalarm_handler();

int msgqueue_id;
int sem_id;
struct msg my_message;
struct shared_data *my_data;

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);

    int shm_id;
    struct sigaction sa;

    bzero(&sa, sizeof(sa));
    sa.sa_handler = sigalarm_handler;
    sigaction(SIGALRM, &sa, NULL);

    if ((shm_id = shmget(KEY, sizeof(struct shared_data), 0666)) == -1)
        TEST_ERROR;

    my_data = shmat(shm_id, NULL, 0);

    if ((msgqueue_id = msgget(KEY, 0666)) == -1)
        TEST_ERROR;

    if ((sem_id = semget(KEY, 3, 0666)) == -1)
        TEST_ERROR;

#ifdef DEBUG
    printf("(ATTIVATORE) PID: %d, PPID: %d \n", getpid(), getppid());
#endif
    sem_reserve(sem_id, 0);
    TEST_ERROR;

    // secondo semaforo
    sem_wait_until_zero(sem_id, 1);
    TEST_ERROR;

    while (my_data->stato_simulazione)
    {
        alarm(my_data->STEP_ATTIVATORE);
        pause();
#ifdef DEBUG
        TEST_ERROR;
#endif
    }

    exit(EXIT_SUCCESS);
}

void sigalarm_handler()
{
#ifdef DEBUG
    fprintf(stderr, "(ATTIVATORE) nel sigalarm handler \n");
#endif
    if (msgrcv(msgqueue_id, &my_message, LEN, 1, 0) == -1)
        TEST_ERROR;

#ifdef DEBUG
    printf("(ATTIVATORE): PID ricevuto da message queue %d\n", my_message.pid);
#endif

    if (kill(my_message.pid, SIGUSR1) == -1)
    {
        if (errno == ESRCH) // processo terminato
        {
            if (msgrcv(msgqueue_id, &my_message, LEN, 1, 0) == -1) // tentativo 2
                if (errno = ENOMSG)                                // no message of desired type: nessuno processo dentro message queue
                    exit(EXIT_FAILURE);
        }
        else if (errno = EINVAL) // message queue già cancellata, simulation_status = 0
            exit(EXIT_FAILURE);
    }

    if (sem_reserve(sem_id, 2) == -1)
        TEST_ERROR;
    my_data->n_attivazioni_last_sec++;
    if (sem_release(sem_id, 2) == -1)
        TEST_ERROR;
}
