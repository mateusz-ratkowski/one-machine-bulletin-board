#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <errno.h>


//zmienne globalne
struct post {
    char name[64];
    char content[1024];
    int clen;
    unsigned short int likes;
};

int server_capacity;

key_t IPC_key_mem, IPC_key_sem;
int shmid, semid;
struct post *shared_data;
int request_fd;

const char *request_fifo = "/tmp/request_fifo";
const char *response_fifo = "/tmp/response_fifo";

//handler dla Ctrl^C niszczący obiekty IPC
void sigint_handler() {
    printf("\nSprzątanie...\n");
    if (shmdt(shared_data) == 0)
        printf(" (odłączenie pamięci współdzielonej: OK)\n");
    else
        printf(" (błąd przy odłączeniu pamięci współdzielonej)\n");

    if (shmctl(shmid, IPC_RMID, 0) == 0)
        printf(" (usunięcie pamięci współdzielonej: OK)\n");
    else
        printf(" (błąd przy usuwaniu pamięci współdzielonej)\n");

    if (semctl(semid, 0, IPC_RMID) == 0)
        printf(" (usunięcie semaforów: OK)\n");
    else
        printf(" (błąd przy usuwaniu semaforów)\n");

    close(request_fd);
    unlink(request_fifo);
    unlink(response_fifo);
    exit(0);
}

//handler dla Ctr^Z wypisujący treść pamięci
void sigtstp_handler() {
    printf("\n____________Twitter 2.0____________\n");
    for (int i = 0; i < server_capacity; i++) {
        if(shared_data[i].clen > 0)
            printf("[%s]: %s [Polubienia: %d]\n", shared_data[i].name, shared_data[i].content, shared_data[i].likes);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Za mało argumentów!\n");
        exit(1);
    }

    char file_name[64];
    strcpy(file_name, argv[1]);
    server_capacity = atoi(argv[2]);

    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    //generowanie różnych kluczy dla pamięci i semaforów
    IPC_key_mem = ftok(file_name, 1);
    IPC_key_sem = ftok(file_name, 2);
    if (IPC_key_mem == -1 || IPC_key_sem == -1) {
        printf("Błąd ftok\n");
        exit(1);
    }

    //tworzenie id pamięci współdzielonej
    shmid = shmget(IPC_key_mem, sizeof(struct post) * server_capacity, 0600 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1) {
        printf("Błąd shmget\n");
        exit(1);
    }

    //tworzenie pamięci współdzielonej
    shared_data = shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        printf("Błąd shmat\n");
        sigint_handler();
        exit(1);
    }

    //inicjalizacja clen i like'ów
    for (int i = 0; i < server_capacity; i++) {
        shared_data[i].clen = 0;
        shared_data[i].likes = 0;
    }


    //tworzenie semaforów
    semid = semget(IPC_key_sem, server_capacity, 0600 | IPC_CREAT);
    if (semid == -1) {
        printf("Błąd semget\n");
        sigint_handler();
        exit(1);
    }


    for (int i = 0; i < server_capacity; i++) {
        if (semctl(semid, i, SETVAL, 1) == -1) {
            printf("Błąd semctl\n");
            sigint_handler();
            exit(1);
        }
    }


    printf("Serwer nasluchuje...\n");

    //tworzenie FIFO
    if (mkfifo(request_fifo, 0666) == -1 || mkfifo(response_fifo, 0666) == -1) {
        printf("Błąd mkfifo\n");
        sigint_handler();
        exit(1);
    }

    request_fd = open(request_fifo, O_RDONLY);
    if (request_fd == -1) {
        printf("Błąd open request_fifo\n");
        sigint_handler();
        exit(1);
    }


    //pętla wychwytująca klientów
    char request_buffer[1];
    while (1) {
        if (read(request_fd, request_buffer, sizeof(request_buffer)) > 0) {
            printf("Serwer: Otrzymano żądanie od klienta.\n");
            int response_fd = open(response_fifo, O_WRONLY);
            if (response_fd == -1) {
                printf("Błąd open response_fifo\n");
                sigint_handler();
                exit(1);
            }

            if (write(response_fd, &server_capacity, sizeof(server_capacity)) == -1) {
                printf("Błąd write response_fifo\n");
                sigint_handler();
                exit(1);
            }

            close(response_fd);
        }
    }

    return 0;
}
