#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
//#define _POSIX_SOURCE
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "treasure.h"

struct stringValue words[] = {
    {"--add", add},
    {"--list", list},
    {"--view", view},
    {"--remove_treasure", rt},
    {"--remove_hunt", rh},
    {NULL, not}
};

enum strval stringToEnum(struct stringValue *words, char *str){
    for(int i=0;words[i].value!=not;i++){
        if(strcmp(words[i].str,str) == 0)
            return words[i].value;
    }
    return not;
}

void printTreasure(struct treasure t){
    printf("Treasure ID: %s - ", t.id); 
    printf("User name: %s - ", t.user); 
    printf("Latitude: %f - ", t.lat); 
    printf("Longitude: %f - ", t.lon); 
    printf("Clue: %s - ", t.clue); 
    printf("Value: %d\n", t.val); 
}

void updateLog(char *id, char *up){
    char file[30];
    strcpy(file,id);
    strcat(file,"/logged_hunt.txt");
    FILE *lh = fopen(file,"a");
    if(lh == NULL){
        perror("eroare deschidere fisier");
        fclose(lh);
        return ;
    }
    fprintf(lh,up);
    fclose(lh);
}

void addHunt(char *id){
    DIR *hunt = opendir(id);
    if(errno == ENOENT){
        if(mkdir(id,0744)){
            hunt = opendir(id);
            if(hunt == NULL){
                perror("eroare");
                exit(-1);
            }

        }
    }else if(hunt == NULL){
        perror("eroare deschidere");
        closedir(hunt);
        exit(-1);
    }
    char name[20];
    strcpy(name,id);
    strcat(name,"/treasures.dat");
    int fd = open(name, O_CREAT|O_WRONLY|O_APPEND,S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH );
    if(fd == -1){
        perror("eroare deschidere fisier");
        close(fd);
        closedir(hunt);
        exit(-1);
    }
    char ID[20],user[50],clue[100];
    int val;
    float lat,lon;
    printf("Treasure ID: "); scanf("%19s",ID); 
    printf("User name: "); scanf("%49s", user); 
    printf("Latitude: "); scanf("%f",&lat);
    printf("Longitude: "); scanf("%f",&lon);
    printf("Clue: "); scanf("%99s", clue);
    printf("Value: "); scanf("%d",&val);
    struct treasure data; strcpy(data.id,ID); strcpy(data.user,user); data.lat=lat; data.lon=lon; strcpy(data.clue,clue); data.val=val;
    if(write(fd,&data,sizeof(data)) == -1){
        perror("eroare scriere");
        close(fd);
        closedir(hunt);
        exit(-1);
    }
    char up[100];
    sprintf(up,"Added treasure to %s\n",id);
    updateLog(id,up);
    close(fd);
    closedir(hunt);
}

void listHunt(char *id){
    DIR *hunt;
    if((hunt = opendir(id)) == NULL){
        perror("eroare deschidere director");
        closedir(hunt);
        exit(-1);
    }
    printf("Hunt name: %s\n", id);
    char str[260],filename[260];
    struct dirent *entry;
    sprintf(filename,"%s/treasures.dat",id);
    while((entry = readdir(hunt))!= NULL){
        sprintf(str, "%s/%s", id, entry->d_name);
        if(strcmp(filename,str) == 0){
            struct stat statbuff;
            if(stat(str,&statbuff) == -1){
                perror("eroare stat");
                closedir(hunt);
                exit(-1);
            }
            printf("File size: %ld\n", statbuff.st_size);
            printf("Last modification time: %ld\n", statbuff.st_mtime);
            int fd = open(str,O_RDONLY);
            if(fd == -1){
                perror("eroare deschidere fisier");
                close(fd);
                closedir(hunt);
                exit(-1);
            }
            struct treasure data;
            while(read(fd,&data,sizeof(data)) == sizeof(data)){
                printTreasure(data);
            }
            if(close(fd) == -1){
                perror("eroare inchidere fisier");
                exit(-1);
            }
            break;
        }
    }
    char up[50];
    sprintf(up,"Listed %s\n",id);
    updateLog(id,up);
    if(closedir(hunt) == -1){
        perror("eroare inchidere director");
        exit(-1);
    }
}

void viewTreasure(char *id, char *t){
    DIR *hunt;
    if((hunt = opendir(id)) == NULL){
        perror("eroare deschidere director");
        closedir(hunt);
        exit(-1);
    }
    char str[260],filename[260];
    struct dirent *entry;
    sprintf(filename,"%s/treasures.dat",id);
    while((entry = readdir(hunt))!= NULL){
        sprintf(str, "%s/%s", id, entry->d_name);
        if(strcmp(filename,str) == 0){
            int fd = open(str,O_RDONLY);
            if(fd == -1){
                perror("eroare deschidere fisier");
                close(fd);
                closedir(hunt);
                exit(-1);
            }
            struct treasure data;
            char name[20];
            while(read(fd,name,sizeof(data.id)) == sizeof(data.id)){
                if(strcmp(name,t) == 0){
                    lseek(fd,-sizeof(data.id),SEEK_CUR);
                    read(fd,&data,sizeof(data));
                    printTreasure(data);
                    char up[50];
                    sprintf(up,"Viewed %s from %s\n",t,id);
                    updateLog(id,up);
                    close(fd);
                    closedir(hunt); 
                    return;             
                }
                else{
                    lseek(fd,-sizeof(data.id),SEEK_CUR);
                    lseek(fd,sizeof(data),SEEK_CUR);
                }
            }
            printf("Treasure does not exist\n");
            close(fd);
        }
    }
    closedir(hunt);
}

#define MAX 1000

void deleteTreasure(int fd, char *t, char *id, char *str){
    struct treasure treasures[MAX];
    int nr = 0;
    ssize_t n;
    while((n = read(fd,&treasures[nr],sizeof(struct treasure))) > 0){
        nr++;
    }
    int index = -1;
    for(int i=0;i<nr;i++){
        if(strcmp(treasures[i].id,t) == 0){
            index = i;
            break;
        }
    }
    if(index == -1){
        printf("Treasure %s not found\n",t);
        return ;
    }
    for(int i=index;i<nr-1;i++){
        treasures[i] = treasures[i+1];
    }

    fd = open(str, O_RDWR | O_TRUNC);
    if (fd == -1) {
        perror("Error opening file for writing");
        return;
    }

    lseek(fd,0,SEEK_SET);
    for(int i=0;i<nr-1;i++){
        if(write(fd,&treasures[i],sizeof(struct treasure)) == -1){
            perror("eroare scriere in fisier");
            exit(-1);
        }
    }
    char up[50];
    sprintf(up,"Deleted %s from %s\n",t,id);
    updateLog(id,up);
    //close(fd);
}

void removeTreasure(char *id, char *t){
    DIR *hunt;
    if((hunt = opendir(id)) == NULL){
        perror("eroare deschidere director");
        closedir(hunt);
        exit(-1);
    }

    char str[260],filename[260];
    struct dirent *entry;
    sprintf(filename,"%s/treasures.dat",id);
    while((entry = readdir(hunt))!= NULL){
        sprintf(str, "%s/%s", id, entry->d_name);
        if(strcmp(filename,str) == 0){
            int fd = open(str,O_RDWR);
            if(fd == -1){
                perror("eroare deschidere fisier");
                close(fd);
                closedir(hunt);
                exit(-1);
            }
            deleteTreasure(fd,t,id,str);
            close(fd);
        }
    }
    closedir(hunt);
}

void removeHunt(char *id){
    DIR *hunt;
    if((hunt = opendir(id)) == NULL){
        perror("eroare deschidere director");
        closedir(hunt);
        exit(-1);
    }
    struct dirent *entry;
    char str[260];
    while((entry = readdir(hunt))!= NULL){
        sprintf(str, "%s/%s", id, entry->d_name);
        remove(str);
    }
    closedir(hunt);
    if(rmdir(id) == -1){
        perror("eroare stergere fisier");
        exit(-1);
    }
}