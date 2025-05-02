#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define FIFO_NAME "/tmp/treasureFIFO"

pid_t monitor_pid = -1;

void create_fifo() {
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("mkfifo");
    }
}

void start_monitor(){
    if (monitor_pid > 0) {
        printf("Monitor already running.\n");
        return;
    }
    if((monitor_pid = fork()) < 0){
        perror("eroare fork");
        exit(-1);
    }
    if(monitor_pid == 0){
        execlp("./monitor","monitor",NULL);
        perror("eroare execlp");
        exit(-1);
    }
}

void stop_monitor(){
    if (monitor_pid <= 0) {
        printf("Monitor is not running.\n");
        return;
    }

    kill(monitor_pid, SIGTERM);
    int status;
    waitpid(monitor_pid, &status, 0); 
    printf("Monitor exited with status %d\n", status);
    monitor_pid = -1;
}

void send_signal(int sig) {
    if (monitor_pid <= 0) {
        printf("Monitor is not running.\n");
        return;
    }
    
    char input[256] = {0};

    if (sig == SIGRTMIN) {
        char hunt[128], treasure[128];
        printf("Enter hunt name: ");
        scanf("%127s", hunt);
        getchar();
        printf("Enter treasure name: ");
        scanf("%127s", treasure);
        getchar();
        snprintf(input, sizeof(input), "%s %s", hunt, treasure);
    } else {
        printf("Enter hunt name: ");
        scanf("%255s", input);
        getchar();
    }
   
    if (kill(monitor_pid, sig) == -1) {
        perror("kill (signal to monitor failed)");
        return;
    }

    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open FIFO");
        exit(-1);
    }

    if (write(fd, input, strlen(input) + 1) == -1) {
        printf("no");
        perror("write to FIFO");
    } 
    close(fd);
}

int main(){
    create_fifo();
    char command[256];
    printf("Enter command:\n(start_monitor, list_hunts, list_treasures, view_treasures, stop_monitor, exit)\n");
    while (1) {
        if (!fgets(command, sizeof(command), stdin))
            break;
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(command, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(command, "list_hunts") == 0) {
            if (kill(monitor_pid, SIGUSR1) == -1) {
                perror("kill (signal to monitor failed)");
            }
        } else if (strcmp(command, "list_treasures") == 0) {
            send_signal(SIGUSR2);
        }else if (strcmp(command,"view_treasure") == 0){
            send_signal(SIGRTMIN);
        } else if (strcmp(command, "exit") == 0) {
            if ( monitor_pid > 0) {
                printf("Error: Monitor still running. Stop it first.\n");
            } else {
                break;
            }
        } else {
            printf("Unknown command.\n");
        }
    }
    unlink(FIFO_NAME);
    return 0;
}