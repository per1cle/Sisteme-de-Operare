
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "treasure.h"
#define _OPEN_SYS
#include <sys/wait.h>
#define FIFO_NAME "/tmp/treasureFIFO"

void handle_list_hunts(int sig) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char type[260];
    if((dir = opendir(".")) == NULL){
        perror("eroare deschidere director");
        return;
    }
    while((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(type, sizeof(type), "./%s", entry->d_name);
        if (stat(type, &st) == -1) {
            perror("stat");
            continue;
        }
        if (S_ISDIR(st.st_mode) && entry->d_name[0] != '.') {
            DIR *hunt;
            struct dirent *hunt_entry;
            if((hunt = opendir(entry->d_name)) == NULL){
                perror("eroare deschidere director");
                closedir(hunt);
                exit(-1);
            }
            printf("%s - ",entry->d_name);
            char str[1024],filename[1024];
            sprintf(filename,"%s/treasures.dat",entry->d_name);
            while((hunt_entry = readdir(hunt)) != NULL){
                sprintf(str, "%s/%s",entry->d_name,hunt_entry->d_name);
                if(strcmp(filename,str) == 0){
                    int fd = open(str,O_RDONLY);
                    if(fd == -1){
                        perror("eroare deschidere fisier");
                        close(fd);
                        closedir(hunt);
                        exit(-1);
                    }
                    struct treasure data;
                    int c=0;
                    while(read(fd,&data,sizeof(data)) == sizeof(data)){
                         c++;
                    }
                    printf("Number of treasures: %d\n", c);
                    fflush(stdout);
                    if(close(fd) == -1){
                        perror("eroare inchidere fisier");
                        exit(-1);
                    }
                    break;
                }
            }
            if(closedir(hunt) == -1){
                perror("eroare inchidere director");
                exit(-1);
            }
        }
    }
    if(closedir(dir) == -1){
        perror("eroare inchidere director");
        exit(-1);
    }
}

void handle_list_treasures(int sig) {
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open FIFO in monitor");
        return;
    }

    char hunt_name[256];
    if (read(fd, hunt_name, sizeof(hunt_name)) > 0) {
        char *args[] = { "./treasure_manager", "--list", hunt_name, NULL };
        pid_t pid = fork();
        if (pid == 0) {
            execv("./treasure_manager", args);
            perror("execv failed"); 
            exit(1);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork failed");
        }
    }

    close(fd);
}

void handle_view_treasures(int sig){
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open FIFO in monitor");
        return;
    }

    char buffer[256];
    if (read(fd, buffer, sizeof(buffer)) > 0) {
        buffer[strcspn(buffer, "\n")] = 0; 

        char *hunt = strtok(buffer, " ");
        char *treasure = strtok(NULL, " ");

        if (!hunt || !treasure) {
            fprintf(stderr, "[Monitor] Invalid input: expected hunt and treasure name\n");
            close(fd);
            return;
        }

        char *args[] = { "./treasure_manager", "--view", hunt, treasure, NULL };

        pid_t pid = fork();
        if (pid == 0) {
            execv("./treasure_manager", args);
            perror("execv failed");
            exit(1);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
        } else {
            perror("fork failed");
        }
    }

    close(fd);
}

void handle_term(int sig) {
    printf("[Monitor] Termination signal received. Delaying exit...\n");
    usleep(3000000); 
    printf("[Monitor] Exiting now.\n");
    exit(0);
}

void setup_signal(int sig, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    if (sigaction(sig, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

int main() {
    printf("[Monitor] Running... PID: %d\n", getpid());

    setup_signal(SIGUSR1, handle_list_hunts);
    setup_signal(SIGUSR2, handle_list_treasures);
    setup_signal(SIGRTMIN, handle_view_treasures);
    setup_signal(SIGTERM, handle_term);

    while (1) {
        pause(); 
    }
    return 0;
}
