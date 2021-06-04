/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsVolume.c
*
* Description: Sets up the file system's volume on the disk.
*
**************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mfs.h"
#include "fsVCB.h"

int main (int argc, char* argv[]) 
{
    // Needs volumeName, volumeSize, and blockSize in arguments
    if (argc < 4) 
    {
        printf("Missing arguments. Command: make run fsVolume.c volumeName volumeSize blockSize\n");
        return 0;
    }

    // Process the args into variables
    char volumeName[MAX_FILENAME_SIZE];
    strcpy(volumeName, argv[1]);

    uint64_t volumeSize;
    volumeSize = atoi(argv[2]);

    uint64_t blockSize;
    blockSize = atoi(argv[3]);
    

    int createVolumeCode = createVolume(volumeName, volumeSize, blockSize);
    if (createVolumeCode < 0)
    {
        printf("Call for createVolume function failed.\n");
        return 0;
    }

    openVolume(volumeName);
    closeVolume();
    return 0;
}