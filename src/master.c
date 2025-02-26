#include "utils.h"

void clean_all();

void sig_alarm_handler();

int sem_id;
int msgqueue_id;
int shm_id;
struct shared_data *my_data;

int main(void)
{
    char *PATH[] = {"./bin/atomo", "./bin/attivatore", "./bin/alimentazione", NULL};
    char *ARGS[] = {"atomo", "prima_attivazione", NULL};
    int pgid;
    int end_status;
    int child_pid;
    struct msg my_message;
    struct sigaction sa;
    int i;
    FILE *my_file;

    setvbuf(stdout, NULL, _IONBF, 0);

    pgid = getpgid(getpid());

    bzero(&sa, sizeof(sa));
    sa.sa_handler = clean_all;
    if (sigaction(SIGINT, &sa, NULL) == -1)
        TEST_ERROR;
    sa.sa_handler = sig_alarm_handler;
    if (sigaction(SIGALRM, &sa, NULL) == -1)
        TEST_ERROR;

    if ((shm_id = shmget(KEY, sizeof(struct shared_data), IPC_CREAT | IPC_EXCL | 0666)) == -1)
        TEST_ERROR;

    my_data = shmat(shm_id, NULL, 0);
    my_data->n_attivazioni = 0;
    my_data->n_attivazioni_last_sec = 0;
    my_data->n_scissioni = 0;
    my_data->n_scissioni_last_sec = 0;
    my_data->qty_energia_consumata = 0;
    my_data->qty_energia_totale = 0;
    my_data->qty_energia_prodotta = 100;
    my_data->qty_energia_prodotta_last_sec = 0;
    my_data->qty_scorie_prodotte = 0;
    my_data->qty_energia_prodotta_last_sec = 0;
    my_data->stato_simulazione = 1;
    my_data->reason_of_end_of_simulation = 0;

    my_file = fopen("./src/parameters.txt", "r");
    TEST_ERROR;

    fscanf(my_file, "%*s %d %*s %d %*s %d %*s %d %*s %d %*s %d %*s %d %*s %d %*s %d", &my_data->N_ATOMI_INIT, &my_data->N_ATOM_MAX,
           &my_data->STEP_ALIMENTAZIONE, &my_data->N_NUOVI_ATOMI, &my_data->MIN_N_ATOMICO,
           &my_data->SIM_DURATION, &my_data->ENERGY_DEMAND, &my_data->ENERGY_EXPLODE_THRESHOLD,
           &my_data->STEP_ATTIVATORE);

    if ((msgqueue_id = msgget(KEY, IPC_CREAT | IPC_EXCL | 0666)) == -1)
        TEST_ERROR;

    if ((sem_id = semget(KEY, 3, IPC_CREAT | IPC_EXCL | 0666)) == -1)
        TEST_ERROR;

    // primo semaforo: processo master aspetta che gli altri processi siano inizializzati
    if (sem_set_val(sem_id, 0, my_data->N_ATOMI_INIT + 2) == -1)
        TEST_ERROR;

    // secondo semaforo: master fa partire la simulazione
    if (sem_set_val(sem_id, 1, 1) == -1)
        TEST_ERROR;

    // terzo semaforo: accedere a memoria condivisa
    if (sem_set_val(sem_id, 2, 1) == -1)
        TEST_ERROR;

    for (i = 0; i < my_data->N_ATOMI_INIT + 2; i++)
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
        case 0: // fork and exec N_ATOMI_INIT atomi 1 attivatore 1 alimentazione
            if (i < my_data->N_ATOMI_INIT)
            {
                my_message.mtype = 1;
                my_message.pid = getpid();
                msgsnd(msgqueue_id, &my_message, LEN, 0);
                execv(PATH[0], ARGS);
                TEST_ERROR;
                exit(EXIT_FAILURE);
            }
            else if (i == my_data->N_ATOMI_INIT)
            {
                execv(PATH[1], ARGS);
                TEST_ERROR;
                exit(EXIT_FAILURE);
            }
            else if (i == (my_data->N_ATOMI_INIT + 1))
            {
                execv(PATH[2], ARGS);
                TEST_ERROR;
                exit(EXIT_FAILURE);
            }
        default:
            break;
        }
    }

    // primo semaforo: processi hanno finito inizializzazione
    sem_wait_until_zero(sem_id, 0);

#ifdef DEBUG
    printf("Hello i am the MASTER my PID is %d, all the other processes should be initialized by now\n", getpid());

    printf("Let's start the simulation now!\n");

    fprintf(stderr, "(MASTER) ALERT ALERT MY GPID IS NOW %d\n", getpgid(getpid()));
#endif
    alarm(my_data->SIM_DURATION);

    // secondo semaforo = fai partire la simulazione
    sem_reserve(sem_id, 1);

    while (my_data->stato_simulazione)
    {
        sleep(1);

        if (sem_reserve(sem_id, 2) == -1)
            TEST_ERROR;

        my_data->n_attivazioni += my_data->n_attivazioni_last_sec;
        my_data->n_scissioni += my_data->n_scissioni_last_sec;
        my_data->qty_energia_prodotta += my_data->qty_energia_prodotta_last_sec;
        my_data->qty_energia_consumata += my_data->ENERGY_DEMAND;
        my_data->qty_scorie_prodotte += my_data->qty_scorie_prodotte_last_sec;
        my_data->qty_energia_totale = my_data->qty_energia_prodotta - my_data->qty_energia_consumata;

        printf("--------------------------------------------------------------------------------------------------\n");
        printf("Numero di attivazioni occorse ad opera del processo attivatore: %d (TOTALI), %d (ULTIMO SECONDO)\n",
               my_data->n_attivazioni, my_data->n_attivazioni_last_sec);
        printf("Numero di scissioni: %d (TOTALI), %d (ULTIMO SECONDO)\n", my_data->n_scissioni, my_data->n_scissioni_last_sec);
        printf("Quantità di energia prodotta: %d (TOTALI), %d (ULTIMO SECONDO)\n", my_data->qty_energia_prodotta,
               my_data->qty_energia_prodotta_last_sec);
        printf("Quantità di energia consumata: %d (TOTALI), %d (ULTIMO SECONDO)\n", my_data->qty_energia_consumata, my_data->ENERGY_DEMAND);
        printf("Quantità di scorie prodotte: %d (TOTALI), %d (ULTIMO SECONDO)\n", my_data->qty_scorie_prodotte,
               my_data->qty_scorie_prodotte_last_sec);
        printf("--------------------------------------------------------------------------------------------------\n");

        my_data->n_attivazioni_last_sec = 0;
        my_data->n_scissioni_last_sec = 0;
        my_data->qty_energia_prodotta_last_sec = 0;
        my_data->qty_scorie_prodotte_last_sec = 0;

        // BLACKOUT
        if (my_data->qty_energia_totale < 0)
        {
            my_data->stato_simulazione = 0;
            my_data->reason_of_end_of_simulation = "BLACKOUT";
            if (sem_release(sem_id, 2) == -1)
                TEST_ERROR;
            break;
        }

        // EXPLODE
        if (my_data->qty_energia_totale > my_data->ENERGY_EXPLODE_THRESHOLD)
        {
            my_data->stato_simulazione = 0;
            my_data->reason_of_end_of_simulation = "EXPLODE";
            if (sem_release(sem_id, 2) == -1)
                TEST_ERROR;
            break;
        }

        if (sem_release(sem_id, 2) == -1)
            TEST_ERROR;
    }

    printf("End of simulation because of %s\n", my_data->reason_of_end_of_simulation);

    // MASTER won't get the SIGTERM signal
    if (setpgid(0, 0) == -1)
        TEST_ERROR;

    if (kill(-pgid, SIGINT) == -1)
        TEST_ERROR;

    while ((child_pid = wait(&end_status)) != -1)
    {
#ifdef DEBUG
        printf("PID= %dd (PARENT):Got info of child with PID = %d,status = 0x %d \n ", getpid(), child_pid, end_status);

        if (WIFEXITED(end_status))
            printf("Child correctly exited with status %d\n", WEXITSTATUS(end_status));

        if (WIFSIGNALED(end_status))
            printf("Child terminated by the signal %d\n", WTERMSIG(end_status));
#endif
    }

    if (shmdt(my_data) == -1)
        TEST_ERROR;

    // CLEAN UP
    if (semctl(sem_id, 0, IPC_RMID) == -1)
        TEST_ERROR;
    if (msgctl(msgqueue_id, 0, IPC_RMID) == -1)
        TEST_ERROR;
    if (shmctl(shm_id, 0, IPC_RMID) == -1)
        TEST_ERROR;

    exit(EXIT_SUCCESS);
}

// using ctrl c if something doesn´t go as intended..
void clean_all()
{
    if (semctl(sem_id, 0, IPC_RMID) == -1)
        TEST_ERROR;
    if (msgctl(msgqueue_id, 0, IPC_RMID) == -1)
        TEST_ERROR;
    if (shmctl(shm_id, 0, IPC_RMID) == -1)
        TEST_ERROR;

    fprintf(stderr, " EXITING USING CLEAN ALL\n");
    exit(EXIT_FAILURE);
}

void sig_alarm_handler()
{
    if (sem_reserve(sem_id, 2) == -1)
        TEST_ERROR;
    my_data->stato_simulazione = 0;
    my_data->reason_of_end_of_simulation = "TIMEOUT";
    if (sem_release(sem_id, 2) == -1)
        TEST_ERROR;
}