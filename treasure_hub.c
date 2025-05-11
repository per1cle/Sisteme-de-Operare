#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
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

#define MAX_HUNTS 100
#define MAX_USERS 100
#define MAX_OUTPUT 8192

typedef struct {
    char username[64];
    int total_score;
} UserScore;

UserScore user_scores[MAX_USERS];
int user_count = 0;

int starts_with(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

void add_score(const char* username, int score) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user_scores[i].username, username) == 0) {
            user_scores[i].total_score += score;
            return;
        }
    }
    // Not found, add new
    if (user_count < MAX_USERS) {
        strncpy(user_scores[user_count].username, username, sizeof(user_scores[user_count].username));
        user_scores[user_count].total_score = score;
        user_count++;
    }
}

void calculate_score() {
    DIR *d = opendir(".");
    if (!d) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    int pipes[MAX_HUNTS][2];
    pid_t pids[MAX_HUNTS];
    char hunt_names[MAX_HUNTS][256];
    int hunt_count = 0;
    struct stat st;

    while ((entry = readdir(d)) && hunt_count < MAX_HUNTS) {
        if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode) && strncmp(entry->d_name, "HUNT", strlen("HUNT")) == 0) {
            if (pipe(pipes[hunt_count]) < 0) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                continue;
            } else if (pid == 0) {
    
                close(pipes[hunt_count][0]);
                dup2(pipes[hunt_count][1], STDOUT_FILENO);
                close(pipes[hunt_count][1]);

                execl("./score", "score", entry->d_name, (char *)NULL);
                perror("execl failed");
                exit(1);
            } else {
                
                close(pipes[hunt_count][1]);
                strncpy(hunt_names[hunt_count], entry->d_name, sizeof(hunt_names[hunt_count]));
                pids[hunt_count] = pid;
                hunt_count++;
            }
        }
    }
    closedir(d);

    for (int i = 0; i < hunt_count; i++) {
        char buffer[256];
        FILE *fp = fdopen(pipes[i][0], "r");
        if (!fp) {
            perror("fdopen");
            continue;
        }

        while (fgets(buffer, sizeof(buffer), fp)) {
            char user[64];
            int score;
            if (sscanf(buffer, "%63[^:]: %d", user, &score) == 2) {
                add_score(user, score);
            }
        }

        fclose(fp);
        waitpid(pids[i], NULL, 0);
    }

    printf("User scores: \n");
    for (int i = 0; i < user_count; i++) {
        printf("%s: %d\n", user_scores[i].username, user_scores[i].total_score);
    }
}

int main(){
    create_fifo();
    char command[256];
    printf("Enter command:\n(start_monitor, list_hunts, list_treasures, view_treasures, stop_monitor, calculate_score, exit)\n");
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
        }else if(strcmp(command, "calculate_score") == 0){
            calculate_score();
        } else {
            printf("Unknown command.\n");
        }
    }
    unlink(FIFO_NAME);
    return 0;
}