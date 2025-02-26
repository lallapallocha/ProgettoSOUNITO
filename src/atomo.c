#include "utils.h"

void sigusr1_handler();

int atomic_number;
int new_atomic_number;
int sem_id;
int shm_id;
struct shared_data *my_data;
int msgqueue_id;
struct msg my_message;

int main(int argc, char const *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sa, NULL);

    if ((sem_id = semget(KEY, 3, 0666)) == -1)
        TEST_ERROR;

    if ((shm_id = shmget(KEY, sizeof(struct shared_data), 0666)) == -1)
        TEST_ERROR;

    if ((msgqueue_id = msgget(KEY, 0666)) == -1)
        TEST_ERROR;

    my_data = shmat(shm_id, NULL, 0);

    srand(getpid());

    atomic_number = rand() % (my_data->N_ATOM_MAX - 1) + 1;
#ifdef DEBUG
    printf("(ATOMO) PID: %d, PPID: %d atomic number: %d\n", getpid(), getppid(), atomic_number);
#endif
    if (!strcmp(argv[1], "prima_attivazione") && argc > 0) // prima sincronizzazione
    {
        // primo semaforo
        sem_reserve(sem_id, 0);
        TEST_ERROR;

        // secondo semaforo
        sem_wait_until_zero(sem_id, 1);
        TEST_ERROR;
    }

    while (my_data->stato_simulazione)
        sleep(1);
#ifdef DEBUG
    TEST_ERROR;
#endif

    exit(EXIT_SUCCESS);
}

void sigusr1_handler()
{
    if (atomic_number <= my_data->MIN_N_ATOMICO)
    {
        if (sem_reserve(sem_id, 2) == -1)
            TEST_ERROR;
        my_data->qty_scorie_prodotte++;
        if (sem_release(sem_id, 2) == -1)
            TEST_ERROR;
    }
    else
    {
        atomic_number = atomic_number / 2;
        switch (fork())
        {
        case -1:
            TEST_ERROR;
            if (sem_reserve(sem_id, 2) == -1)
                TEST_ERROR;
            my_data->stato_simulazione = 0;
            my_data->reason_of_end_of_simulation = "MELTDOWN";
            if (sem_release(sem_id, 2) == -1)
                TEST_ERROR;
            exit(EXIT_FAILURE);
            break;
        case 0:
#ifdef DEBUG
            printf("Hello i am an ATOMO after a split, my PID is %d, my PPID is %d my atomic_number is %d\n", getpid(), getppid(), atomic_number);
#endif
            my_message.mtype = 1;
            my_message.pid = getpid();
            if (msgsnd(msgqueue_id, &my_message, LEN, 0) == -1)
                TEST_ERROR;
            break;
        default:
            my_message.mtype = 1;
            my_message.pid = getpid();
            if (msgsnd(msgqueue_id, &my_message, LEN, 0) == -1)
                TEST_ERROR;
            if (sem_reserve(sem_id, 2) == -1)
                TEST_ERROR;
            my_data->n_scissioni_last_sec++;
            my_data->qty_energia_prodotta_last_sec = my_data->qty_energia_prodotta_last_sec + energy(atomic_number, atomic_number);
            if (sem_release(sem_id, 2) == -1)
                TEST_ERROR;
            break;
        }
    }
}
