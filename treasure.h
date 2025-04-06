#ifndef TREASURE_H
#define TREASURE_H

enum strval {add, list, view, rt, rh, not};

struct stringValue{
    char *str;
    enum strval value;
};

extern struct stringValue words[6];

struct treasure{
    char id[20];
    char user[50];
    float lat;
    float lon;
    char clue[100];
    int val;
};

enum strval stringToEnum(struct stringValue *words, char *str);

void addHunt(char *id);

void listHunt(char *id);

void viewTreasure(char *id, char *t);

#endif

