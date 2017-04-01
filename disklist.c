/*
    CSC360 Assignnment #3 - Disklist.c
    Jack Belford
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

typedef struct {
    unsigned char* fileName;
    int attribute;
    int startingCluster;
    int fileSize;
    int creationTime;
    int fineTime;
    int creationDate;
} entry;

unsigned char* seekSData(FILE *fp, int offset, int bytes);
int seekIData(FILE *fp, int offset, int bytes);
void printRootEntries(FILE *fp, int startOfRoot);
void printCenter(char* word, int maxChar);
void printDate(int creationDate);
void printTime(int fineTime, int creationTime);


void main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Missing file argument! Correct input: $ ./disklist diskImage.IMA\n");
        exit(0);
    }
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Could not read disk! Exiting program..\n");
        exit(0);
    }

    int bytesPerSect  = seekIData(fp, 0x0B, 2);
    int reserved      = seekIData(fp, 0x0E, 2);
    int numFAT        = seekIData(fp, 0x10, 1);
    int sectPerFAT    = seekIData(fp, 0x16, 2);
    int startOfRoot  = bytesPerSect*(reserved + numFAT*sectPerFAT);

    printRootEntries(fp, startOfRoot);

    fclose(fp);
}

//Return the string value of the data found at the offset location
unsigned char* seekSData(FILE *fp, int offset, int bytes) {
    unsigned char* buff = malloc(sizeof(unsigned char)*bytes);
    fseek(fp, offset, SEEK_SET);
    fread(buff, 1, bytes, fp);
    rewind(fp);
    return buff;
}

//Return the integer value of hex data found at the offset location
int seekIData(FILE *fp, int offset, int bytes) {
    unsigned char* buff = seekSData(fp, offset, bytes);
    int i, value = 0;
    for (i = 0; i < bytes; i++) {
        value += (unsigned int)(unsigned char)buff[i] << 8*i;
    }
    return value;
}

//Iterates through files in root directory and prints their information
void printRootEntries(FILE *fp, int startOfRoot) {
    int totalRoot = seekIData(fp, 0x11, 2);
    int i;
    for (i = 0; i < totalRoot; i++) {
        int index = startOfRoot+i*32;
        int check = seekIData(fp, index, 1);
        if (check == 0) break;
        else if (check == 229) continue;
        entry newEntry = {
            seekSData(fp, index, 11),
            seekIData(fp, index+11, 1),
            seekIData(fp, index+26, 2),
            seekIData(fp, index+28, 4),
            seekIData(fp, index+14, 2),
            seekIData(fp, index+13, 1),
            seekIData(fp, index+16, 2)
        };
        if (newEntry.attribute == 0 && newEntry.startingCluster <= 0) continue; //If not a file/directory
        else if (CHECK_BIT(newEntry.attribute, 3)) continue; //If volume label

        printf("%c ",CHECK_BIT(newEntry.attribute, 4) ? 'D' : 'F');
        char* sizeBuff = malloc(sizeof(char)*10);
        snprintf(sizeBuff, 10, "%d", newEntry.fileSize);
        printCenter(sizeBuff, 10);
        printCenter(newEntry.fileName, 20);
        printDate(newEntry.creationDate);
        printTime(newEntry.fineTime, newEntry.creationTime);
        printf("\n");
    }
}

//Prints a word by centering within a limited # of characters
void printCenter(char* word, int maxChar) {
    int len = maxChar - strlen(word);
    if (len <= 0) {
        printf("%s ", word);
        return;
    }
    int lenL = ceil(len/2.0);
    int lenR = floor(len/2.0);
    char spaceL[lenL];
    char spaceR[lenR];
    int i;
    for (i = 0 ; i < lenR; i++) {
        spaceL[i] = ' ';
        spaceR[i] = ' ';
    }
    if (len%2 != 0) spaceL[i] = ' ';
    printf("%s%s%s ", spaceL, word, spaceR);
}

//Converts creationDate information to year-month-day format & prints
void printDate(int creationDate) {
    //0000 0000 0001 1111
    int day = creationDate & 0x1f;
    //0000 0001 1110 0000
    int month = (creationDate >> 5) & 0xf;
    //1111 1110 0000 0000
    int year = 1980+((creationDate >> 9) & 0x7f);
    printf("%d-%d-%d ", year, month, day);
}

//Converts creationTime information to HH:MM:SS format & prints
void printTime(int fineTime, int creationTime) {
    int sec = (int)round(2.0*(creationTime & 0x1f) + fineTime*0.01);
    //0000 0111 1110 0000
    int min = (creationTime >> 5) & 0x3f;
    //1111 1000 0000 0000
    int hour = (creationTime >> 11) & 0x1f;
    printf("%02d:%02d:%02d", hour, min, sec);
}
