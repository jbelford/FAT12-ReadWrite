/*
    CSC360 Assignnment #3 - Diskput.c
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
    unsigned int *clusters;
} entry;

unsigned char* seekSData(FILE *fp, int offset, int bytes);
int seekIData(FILE *fp, int offset, int bytes);
void overwriteData(FILE *fp, unsigned char* buffInsert, int offset, int bytes);
entry readFileInput(char* fileName, int bytesPerClust);
unsigned char* convertFileName(char* fileName);
void getUsedSpace(FILE *fp, int startOfRoot, int totalRoot, int bytesPerClust, int* usedSpace, int* rootEntries);
void getListofClusters(FILE *fp, int clustersNeeded, int bytesPerSect, entry* writeData);
void writeToDataAndFAT(FILE *fp, int clustersNeeded, int bytesPerClust, int bytesPerSect, int numFAT, int sectPerFAT, entry writeData);
void writeToFAT(FILE *fp, int currCluster, int nextCluster, int bytesPerSect, int numFAT, int bytesPerFAT);
void writeToCluster(FILE *fp, int diskOffset, int fileOffset, int bytesToWrite);
void addToRoot(FILE *fp, int startOfRoot, int totalRoot, entry writeData);
unsigned char* bufferize(unsigned int valueToBuffer, int bytes);

char* diskName;
char* fileName;
int totalSize;

void main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Missing file argument! Correct input: $ ./diskput diskImage.IMA nameOfFile.ext\n");
        exit(0);
    }
    diskName = argv[1];
    fileName = argv[2];

    FILE *fp = fopen(diskName, "rb+");
    if (fp == NULL) {
        printf("Could not read disk! Exiting program..\n");
        exit(0);
    }

    int bytesPerSect  = seekIData(fp, 0x0B, 2);
    int bytesPerClust = seekIData(fp, 0x0D, 1)*bytesPerSect;
    int reserved      = seekIData(fp, 0x0E, 2);
    int numFAT        = seekIData(fp, 0x10, 1);
    int sectPerFAT    = seekIData(fp, 0x16, 2);
    int startOfRoot   = bytesPerSect*(reserved + numFAT*sectPerFAT);

    int numSects  = seekIData(fp, 0x13, 2);
    if (numSects == 0) numSects = seekIData(fp, 0x20, 4);
    int totalRoot = seekIData(fp, 0x11, 2);
    totalSize = bytesPerSect*numSects;

    int usedSpace    = 0;
    int rootEntries  = 0;

    entry writeData = readFileInput(fileName, bytesPerClust);
    getUsedSpace(fp, startOfRoot, totalRoot, bytesPerClust, &usedSpace, &rootEntries);
    if (rootEntries == totalRoot || totalSize - usedSpace - 33*bytesPerSect < writeData.fileSize) {
        printf("Not enough free space in the disk image!");
        exit(0);
    }

    int clustersNeeded = ceil(writeData.fileSize/((double)bytesPerClust));
    getListofClusters(fp, clustersNeeded, bytesPerSect, &writeData); //Get list of clusters we wish to write to
    writeToDataAndFAT(fp, clustersNeeded, bytesPerClust, bytesPerSect, numFAT, sectPerFAT, writeData); //Write data in those clusters as well as update FAT(s)
    addToRoot(fp, startOfRoot, totalRoot, writeData); //Finally, create an entry in the root directory for the file

    printf("Done writing into disk!\n");
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

//Inserts data to file by re-writing the file with new data included.
void overwriteData(FILE *fp, unsigned char* buffInsert, int offset, int bytes) {
    unsigned char* buff1 = seekSData(fp, 0, offset);
    unsigned char* buff2 = seekSData(fp, offset+bytes, totalSize);
    fclose(fp);
    fp = fopen(diskName, "wb+");
    if (fp == NULL) {
        printf("Something went wrong overwriting in file.(1)\n");
        exit(0);
    }
    fwrite(buff1, 1, offset, fp);
    fwrite(buffInsert, 1, bytes, fp);
    fwrite(buff2, 1, totalSize - (offset+bytes), fp);
    fclose(fp);
    fp = fopen(diskName, "rb+");
    if (fp == NULL) {
        printf("Something went wrong overwriting in file.(2)\n");
        exit(0);
    }
}

//Reads the input file and initializes struct for entry to be written
entry readFileInput(char* fileName, int bytesPerClust) {
    FILE *fp = fopen(fileName, "rb");
    if (fp == NULL) {
        printf("Could not read file! Exiting program..\n");
        exit(0);
    }
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    entry writeData = {
        convertFileName(fileName),
        0,
        0, //Decide this later
        size,
        (unsigned int*)malloc(sizeof(unsigned int)*(1+ceil(size/((double)bytesPerClust))))
    };
    return writeData;
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

//Gets the used space on the disk in bytes as well as the number of entries in the root directory
void getUsedSpace(FILE *fp, int startOfRoot, int totalRoot, int bytesPerClust, int* usedSpace, int* rootEntries) {
    int i;
    for (i = 0; i < totalRoot; i++) {
        int index = startOfRoot+i*32;
        int check = seekIData(fp, index, 1);
        if (check == 0) break;
        else if (check == 0xE5) continue;
        entry newEntry = {
            seekSData(fp, index, 11),
            seekIData(fp, index+11, 1),
            seekIData(fp, index+26, 2),
            seekIData(fp, index+28, 4),
            malloc(0)
        };
        *rootEntries += 1;

        if (newEntry.attribute == 0 && newEntry.startingCluster <= 0) continue; //If not a file/directory
        else if (CHECK_BIT(newEntry.attribute, 3)) continue; //If volume label

        *usedSpace   += newEntry.fileSize + bytesPerClust - newEntry.fileSize%bytesPerClust;
    }
}

//Selects clusters that data will be written to
void getListofClusters(FILE *fp, int clustersNeeded, int bytesPerSect, entry* writeData) {
    int i, j = 0;
    for (i = 2; j < clustersNeeded; i++) {
        int offset = floor(i*3.0/2.0);
        int bit16  = seekIData(fp, bytesPerSect + offset, 2);
        int bit12  = i%2 == 0 ? bit16 & 0xfff : bit16 >> 4;
        if (bit12 == 0) {
            if (j == 0) writeData->startingCluster = i;
            writeData->clusters[j++] = i;
        }
    }
    writeData->clusters[clustersNeeded] = 0xFFF;
}

//Writes data to each cluster and updates FAT table accordingly
void writeToDataAndFAT(FILE *fp, int clustersNeeded, int bytesPerClust, int bytesPerSect, int numFAT, int sectPerFAT, entry writeData) {
    int bytesPerFAT = bytesPerSect*sectPerFAT;
    int bytesLeft = writeData.fileSize;
    int i;
    for (i = 0; i < clustersNeeded; i++) {
        writeToFAT(fp, writeData.clusters[i], writeData.clusters[i+1], bytesPerSect, numFAT, bytesPerFAT);
        int bytesToWrite = bytesLeft < bytesPerClust ? writeData.fileSize : bytesPerClust;
        writeToCluster(fp, 33*bytesPerSect + (writeData.clusters[i]-2)*bytesPerClust, i*bytesPerClust, bytesToWrite);
        bytesLeft -= bytesPerClust;
    }
}

//Writes a value into the FAT(s) according to the cluster
void writeToFAT(FILE *fp, int currCluster, int nextCluster, int bytesPerSect, int numFAT, int bytesPerFAT) {
    int offset = floor(currCluster*3.0/2.0);
    int bit16 = seekIData(fp, bytesPerSect + offset, 2);
    unsigned int bit12  = currCluster%2 == 0 ? nextCluster + (bit16 & 0xf000) : (nextCluster << 4) + (bit16 & 0xf);
    unsigned char* buff = (unsigned char*)malloc(sizeof(unsigned char)*2);
    buff[0] = bit12 & 0xff;
    buff[1] = bit12 >> 8;
    //Update each FAT table
    int i;
    for (i = 0; i < numFAT; i++) {
        overwriteData(fp, buff, bytesPerSect + i*bytesPerFAT + offset, 2);
    }
}

//Reads some data from input file and writes into cluster
void writeToCluster(FILE *fp, int diskOffset, int fileOffset, int bytesToWrite) {
    FILE *dataFile = fopen(fileName, "rb");
    if (dataFile == NULL) {
        printf("Could not read file! Exiting program..\n");
        exit(0);
    }
    unsigned char* buff = seekSData(dataFile, fileOffset, bytesToWrite);
    overwriteData(fp, buff, diskOffset, bytesToWrite);
    fclose(dataFile);
}

//Inserts information about the new entry in the root directory
void addToRoot(FILE *fp, int startOfRoot, int totalRoot, entry writeData) {
    int i;
    for (i = 0; i < totalRoot; i++) {
        int index = startOfRoot+i*32;
        int check = seekIData(fp, index, 1);
        if (check != 0 && check != 0xE5) continue;

        overwriteData(fp, writeData.fileName, index, 11);
        overwriteData(fp, bufferize(writeData.startingCluster, 2), index+26, 2);
        overwriteData(fp, bufferize(writeData.fileSize, 4), index+28, 4);

        break;
    }
}

//Takes an integer value and returns a buffer of its bytes
unsigned char* bufferize(unsigned int valueToBuffer, int bytes) {
    unsigned char* buff = (unsigned char*)malloc(sizeof(unsigned char)*bytes);
    int i;
    for (i=0; i < bytes; i++) {
        buff[i] = (valueToBuffer >> i*8) & 0xff;
    }
    return buff;
}
