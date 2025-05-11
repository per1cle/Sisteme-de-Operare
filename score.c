#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FILENAME "treasures.dat"

typedef struct {
    char id[20];
    char username[50];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

typedef struct UserScore {
    char username[50];
    int score;
    struct UserScore* next;
} UserScore;

UserScore* find_or_add_user(UserScore** head, const char* username) {
    UserScore* u = *head;
    while (u) {
        if (strcmp(u->username, username) == 0) 
            return u;
        u = u->next;
    }
   
    UserScore* new_node = malloc(sizeof(UserScore));
    strncpy(new_node->username, username, 50);
    new_node->score = 0;
    new_node->next = *head;
    *head = new_node;
    return new_node;
}

void free_scores(UserScore* head) {
    while (head) {
        UserScore* tmp = head;
        head = head->next;
        free(tmp);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        perror("eroare argumente");
        return 1;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", argv[1], FILENAME);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("eroare fopen");
        return 1;
    }

    Treasure t;
    UserScore* score_list = NULL;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        UserScore* user = find_or_add_user(&score_list, t.username);
        user->score += t.value;
    }

    close(fd);

    UserScore* curr = score_list;
    while (curr) {
        printf("%s: %d\n", curr->username, curr->score);
        curr = curr->next;
    }

    free_scores(score_list);
    return 0;
}
