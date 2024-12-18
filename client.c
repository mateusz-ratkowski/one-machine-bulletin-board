#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <errno.h>

struct post {
    char name[64];
    char content[1024];
    int clen;
    unsigned short int likes;
};

key_t IPC_key_mem, IPC_key_sem;
int shmid, semid;
struct post *shared_data;
int server_capacity;

void connect_memory(char file_name[]) {
    // creating key for shared memory
    IPC_key_mem = ftok(file_name, 1);
    if (IPC_key_mem == -1) {
        printf("Błąd ftok\n");
        exit(1);
    }

    // geting id of shared memory
    shmid = shmget(IPC_key_mem, 0, 0600);
    if (shmid == -1) {
        printf("Błąd shmget\n");
        exit(1);
    }

    // connecting shared memory
    shared_data = shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        printf("Błąd shmat\n");
        exit(1);
    }
}

void connect_semaphores(char file_name[]) {
    // creating key for semaphores
    IPC_key_sem = ftok(file_name, 2);
    if (IPC_key_sem == -1) {
        printf("Błąd ftok\n");
        exit(1);
    }

    // getting id of semaphores
    semid = semget(IPC_key_sem, 0, 0600);
    if (semid == -1) {
        printf("Błąd semget\n");
        exit(1);
    }
}

int check_slots() {
    // searching for free slot by semaphores
    for (int i = 0; i < server_capacity; i++) {
        int semval = semctl(semid, i, GETVAL);
        if (semval == -1) {
            printf("Błąd semctl GETVAL\n");
            exit(1);
        }
        if (semval == 1) {
            printf("Slot %d jest dostępny.\n", i);
            return i;
        }
    }
    printf("Brak dostępnych slotów.\n");
    exit(0);
}

void occupy_slot(int slot) {
    // capture of free slot
    struct sembuf sb = {slot, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        printf("Błąd semop\n");
        exit(1);
    }
    printf("Slot %d został zajęty.\n", slot);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Za mało argumentów! Podaj nazwę pliku jako pierwszy argument.\n");
        exit(1);
    }

    char file_name[64], user_name[64];
    strcpy(file_name, argv[1]);

    // FIFO comunication - request for server_capacity
    const char *request_fifo = "/tmp/request_fifo";
    const char *response_fifo = "/tmp/response_fifo";

    int request_fd = open(request_fifo, O_WRONLY);
    if (request_fd == -1) {
        printf("Błąd open request_fifo\n");
        exit(1);
    }

    char request_message = '1';
    if (write(request_fd, &request_message, sizeof(request_message)) == -1) {
        printf("Błąd write request_fifo\n");
        close(request_fd);
        exit(1);
    }
    close(request_fd);

    int response_fd = open(response_fifo, O_RDONLY);
    if (response_fd == -1) {
        printf("Błąd open response_fifo\n");
        exit(1);
    }

    if (read(response_fd, &server_capacity, sizeof(server_capacity)) == -1) {
        printf("Błąd read response_fifo\n");
        close(response_fd);
        exit(1);
    }
    close(response_fd);

    // action of connecting semaphores and memory
    connect_memory(file_name);
    connect_semaphores(file_name);

    if (argc == 3 && strcmp(argv[2], "P") == 0) {
        int like_index = server_capacity + 1;
        int there_no_posts = 1;

        // P variant
        printf("\n____________Twitter 2.0____________\n");
        for (int i = 0; i < server_capacity; i++) {
            if(shared_data[i].clen > 0) {
                there_no_posts = 0;
                printf("%d. [%s]: %s [Polubienia: %d]\n", i + 1, shared_data[i].name, shared_data[i].content, shared_data[i].likes);
            }   else {
                printf("%d. ____WOLNY SLOT____\n", i + 1);
            }
        }

        if (there_no_posts) {
            printf("Na serwisie nie ma obecnie rzadnych postow\n");
            exit(0);
        }

        //menu of posts to like
        while (1) {
            printf("Który wpis chcesz polubic? (wybierz)\n");
            if (scanf("%d", &like_index) != 1 || like_index >= server_capacity || shared_data[like_index - 1].clen == 0) {
                printf("Podano niepoprawne dane !\n\n");
                while (getchar() != '\n');
                like_index = server_capacity + 1;
            }   else {
                break;
            }
        }


        shared_data[like_index - 1].likes += 1;
        printf("Dziekujemy za skorzystanie z Twitter 2.0\n");

    } else if (argc == 4 && strcmp(argv[2], "N") == 0) {

        /*
            N variant
        */

        strcpy(user_name, argv[3]);

        int free_slot = check_slots();

        occupy_slot(free_slot);

        char content[1024];
        printf("Podaj treść posta: ");
        fgets(content, sizeof(content), stdin);

        //remove new line symbol
        content[strcspn(content, "\n")] = '\0';

        strcpy(shared_data[free_slot].name, user_name);
        strcpy(shared_data[free_slot].content, content);
        shared_data[free_slot].clen = strlen(content);
        shared_data[free_slot].likes = 0;

        printf("Post zapisany w slocie %d.\n", free_slot);
    }   else {
        printf("forma argumentow nie jest poprawna\n");
    }

    //memory detach
    if (shmdt(shared_data) == -1) {
        printf("Błąd shmdt\n");
        exit(1);
    }

    return 0;
}
