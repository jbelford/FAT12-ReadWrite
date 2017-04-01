/*
    CSC360 Assignnment #3 - Diskget.c
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
} entry;

unsigned char* seekSData(FILE *fp, int offset, int bytes);
int seekIData(FILE *fp, int offset, int bytes);
unsigned char* convertFileName(char* fileName);
entry searchRootEntries(FILE *fp, int startOfRoot, unsigned char* filename);
void writeFile(FILE *fp, char* fileName, int startCluster, int bytesPerSect, int fileSize);
int getNextCluster(FILE *fp, int bytesPerSect, int logicalClust);

void main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Missing file argument! Correct input: $ ./diskget diskImage.IMA nameOfFile.ext\n");
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

    unsigned char* fileName = convertFileName(argv[2]);
    entry foundEntry = searchRootEntries(fp, startOfRoot, fileName);

    writeFile(fp, argv[2], foundEntry.startingCluster, bytesPerSect, foundEntry.fileSize);

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

//Convert input filename to format stored in FAT12
unsigned char* convertFileName(char* fileName) {
    unsigned char* converted = (unsigned char*)malloc(sizeof(unsigned char)*11);
    int i, j = 0;
    for(i = 0; i < strlen(fileName); i++) {
        if (fileName[i] == '.') {
            for (j = i; j < 8; j++) {
                converted[j] = ' ';
            }
            continue;
        }
        converted[j++] = toupper(fileName[i]);
    }
    while (j < 11) converted[j++] = ' ';

    return converted;
}

//Searches the entries in the root directory for the one to be copied
entry searchRootEntries(FILE *fp, int startOfRoot, unsigned char* filename) {
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
            seekIData(fp, index+28, 4)
        };
        if (newEntry.attribute == 0 && newEntry.startingCluster <= 0) continue; //If not a file/directory
        else if (CHECK_BIT(newEntry.attribute, 3)) continue; //If volume label

        if (strcmp(filename, newEntry.fileName) == 0) {
            return newEntry;
        }
    }
    printf("File not found.\n");
    exit(0);
}

//Reads the data from the disk image and writes to a file in current directory
void writeFile(FILE *fp, char* fileName, int startCluster, int bytesPerSect, int bytesLeft) {
    int bytesPerClust = seekIData(fp, 0x0D, 1)*bytesPerSect;
    FILE *wf = fopen(fileName, "w");
    if (wf == NULL) {
        printf("Error writing file!\n");
        exit(1);
    }
    int currCluster = startCluster;
    while (currCluster != 0 && currCluster < 0xFF8) {
        int numElements = bytesPerClust < bytesLeft ? bytesPerClust : bytesLeft;
        int startOfData = 33*bytesPerSect + bytesPerClust*(currCluster - 2);
        fwrite(seekSData(fp, startOfData, numElements), 1, numElements, wf);
        bytesLeft -= bytesPerClust;
        currCluster = getNextCluster(fp, bytesPerSect, currCluster);
    }

    printf("File copied to directory!\n");
    fclose(wf);
}

//Reads an entry number in the FAT and returns number of next cluster
int getNextCluster(FILE *fp, int bytesPerSect, int logicalClust) {
    int offset = floor(logicalClust*3.0/2.0);
    int bit16  = seekIData(fp, bytesPerSect + offset, 2);
    int bit12 = logicalClust%2 == 0 ? bit16 & 0xfff : bit16 >> 4;
    return bit12;
}
