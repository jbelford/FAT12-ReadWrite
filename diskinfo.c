/*
    CSC360 Assignnment #3 - Diskinfo.c
    Jack Belford
*/

#include <stdio.h>
#include <stdlib.h>

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

typedef struct {
    unsigned char* fileName;
    int attribute;
    int startingCluster;
    int fileSize;
} entry;

unsigned char* seekSData(FILE *fp, int offset, int bytes);
int seekIData(FILE *fp, int offset, int bytes);
unsigned char* findVolumeLabelETC(FILE *fp, int startOfRoot, int bytesPerClust, int* usedSpace, int* rootEntries);

void main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Missing file argument! Correct input: $ ./diskinfo diskImage.IMA\n");
        exit(0);
    }
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("Could not read disk! Exiting program..\n");
        exit(0);
    }

    int bytesPerSect  = seekIData(fp, 0x0B, 2);
    int bytesPerClust = seekIData(fp, 0x0D, 1)*bytesPerSect;
    int reserved      = seekIData(fp, 0x0E, 2);
    int numSects      = seekIData(fp, 0x13, 2);
    if (numSects == 0) numSects = seekIData(fp, 0x20, 4);
    int totalSize     = bytesPerSect*numSects;

    int numFAT        = seekIData(fp, 0x10, 1);
    int sectPerFAT    = seekIData(fp, 0x16, 2);
    int startOfRoot  = bytesPerSect*(reserved + numFAT*sectPerFAT);
    int usedSpace    = 0;
    int rootEntries  = 0;

    unsigned char* volumeLabel = findVolumeLabelETC(fp, startOfRoot, bytesPerClust, &usedSpace, &rootEntries);

    printf("OS Name: %s\n", seekSData(fp, 0x03, 8));
    printf("Label of the disk: %s\n", volumeLabel);
    printf("Total size of the disk: %d bytes\n", totalSize);
    printf("Free size of the disk: %d bytes\n", totalSize - usedSpace);
    printf("========================\n");
    printf("Number of files in root directory: %d\n", rootEntries);
    printf("========================\n");
    printf("Number of FAT copies: %d\n", numFAT);
    printf("Sectors per FAT: %d\n", sectPerFAT);

    fclose(fp);
}

//Return the string value of the data found at the offset location
unsigned char* seekSData(FILE *fp, int offset, int bytes) {
    unsigned char* buff = (unsigned char*)malloc(sizeof(unsigned char)*bytes);
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

//Scans files in root folder. The volume label is located here so it will return that.
//This function also stores:
//  - the total space used by the files in 'usedSpace' address (this includes the slack space due to cluster size)
//  - the total number of non-empty files in the root directory in 'rootEntries' address
unsigned char* findVolumeLabelETC(FILE *fp, int startOfRoot, int bytesPerClust, int* usedSpace, int* rootEntries) {
    int totalRoot = seekIData(fp, 0x11, 2);
    unsigned char* volumeLabel = (unsigned char*)malloc(sizeof(unsigned char*)*11);
    volumeLabel = "NO NAME";
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
        if (newEntry.attribute == 0 && newEntry.startingCluster <= 0) continue;
        else if (CHECK_BIT(newEntry.attribute, 4)) continue;
        else if (newEntry.attribute == 8) {
            volumeLabel = newEntry.fileName;
            continue;
        }
        *rootEntries += newEntry.attribute == 0 ? 1 : 0;
        *usedSpace += newEntry.fileSize + bytesPerClust - newEntry.fileSize%bytesPerClust;
    }
    return volumeLabel;
}
