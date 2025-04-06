#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "treasure.h"

int main(int argc, char **argv){
    if(argc < 2){
        printf("Wrong number of arguments\n");
        return 0;
    }
    switch(stringToEnum(words,argv[1])){
        case add:{
            if(argc == 3)
                addHunt(argv[2]);
            else{
                printf("Wrong number of arguments\n");
            }
            break;
        }
        case list:{
            if(argc == 3)
                listHunt(argv[2]);
            else{
                printf("Wrong number of arguments\n");
            }
            break;
        }
        case view:{
            if(argc == 4)
                viewTreasure(argv[2],argv[3]);
            else{
                printf("Wrong number of arguments\n");
            }
            break;
        }
        case rt:{
            break;
        }
        case rh:{
            break;
        }
        case not:{
            printf("Invalid operation");
            break;
        }
    }
    return 0;
}