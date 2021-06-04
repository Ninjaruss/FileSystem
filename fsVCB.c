/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsVCB.c
*
* Description: Manages the volume control block for the file system.
*
**************************************************************/
#include "fsVCB.h"
#include "bitMap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Info for the volume control block
char header[16] = "[File System]";
uint64_t volumeSize;
uint64_t blockSize;
uint64_t diskSizeBlocks;
uint32_t vcbStartBlock;
uint32_t vcbTotal;
uint32_t inodeStartBlock;
uint32_t inodeTotal;
uint32_t inodeBlockTotal;
uint32_t freeMapSize;

int volumeInitialized = 0;
fs_VCB* openVCB;

// Initializes the info for the volume control block and allocates to memory
void initialize(uint64_t _volumeSize, uint64_t _blockSize) 
{
    volumeSize = _volumeSize;
    blockSize = _blockSize;
    diskSizeBlocks = divUp(volumeSize, blockSize);
    freeMapSize = diskSizeBlocks <= sizeof(uint32_t) * 8 ? 1 : diskSizeBlocks / sizeof(uint32_t) / 8;
    vcbTotal = divUp(sizeof(fs_VCB) + sizeof(uint32_t[freeMapSize]), blockSize);
    inodeStartBlock = VCB_START_BLOCK + vcbTotal;
    inodeTotal = (diskSizeBlocks - inodeStartBlock) / (DATA_BLOCKS_PER_INODE + divUp(sizeof(fs_dir), blockSize));
    inodeBlockTotal = divUp(inodeTotal * sizeof(fs_dir), blockSize);

    // Allocate space for the VCB
    int vcbSize = allocateVCB(&openVCB);
    printf("Allocated %d blocks for VCB.\n", vcbSize);

    volumeInitialized = 1;
}

// Rounded up division of integers
uint64_t divUp(uint64_t a, uint64_t b) 
{
    uint64_t result = (a + b - 1) / b;
    return result;
}

int allocateVCB(fs_VCB** vcb) 
{
    *vcb = calloc(vcbTotal, blockSize);
    return vcbTotal;
}

void initializeVCB() 
{
    if (!volumeInitialized) 
    {
        printf("VCB system not initialized.\n");
        return;
    }

    sprintf(openVCB->header, "%s", header); 

    // Set all block info
    openVCB->volumeSize = volumeSize;
    openVCB->blockSize = blockSize;
    openVCB->diskSizeBlocks = diskSizeBlocks;
    openVCB->vcbStartBlock = VCB_START_BLOCK;
    openVCB->totalVCBBlocks = vcbTotal;
    openVCB->inodeStartBlock = inodeStartBlock;
    openVCB->totalInodes = inodeTotal;
    openVCB->totalInodeBlocks = inodeBlockTotal;
    openVCB->freeMapSize = freeMapSize;
    printf("initVCB: totalInodeBlocks %ld\n", openVCB->totalInodeBlocks);
    printf("initVCB: inodeStartBlock %ld\n", openVCB->inodeStartBlock);

    // Set block free space map to available
    for (int i = 0; i < freeMapSize; i++) 
    {
        openVCB->freeMap[i] = 0;
    }

    // Set bits for inodes into free space map
    for (int i = 0; i < inodeStartBlock + inodeBlockTotal; i++) 
    {
        setBit(openVCB->freeMap, i);
    }

    printVCB();
    writeVCB();
}

void initializeInodes() 
{
    if (!volumeInitialized) 
    {
        printf("initializeInodes: System not initialized.\n");
        return;
    }

    printf("Total disk blocks: %ld, total inodes: %d, total inode blocks: %d\n", diskSizeBlocks, inodeTotal, inodeBlockTotal);

    // Initialize the root inode
    fs_dir* inodes = calloc(inodeBlockTotal, blockSize);
    strcpy(inodes[0].name, "root");
    strcpy(inodes[0].path, "/root");

    inodes[0].inUse = 1;
    inodes[0].fd = -1;
    inodes[0].inodeIndex = 0;
    inodes[0].type = I_DIR;

    inodes[0].lastAccessTime = time(0);
    inodes[0].lastModificationTime = time(0);
    inodes[0].numDirectBlockPointers = 0;

    // Initialize rest of inodes
    for (int i = 1; i < inodeTotal; i++) 
    {
        strcpy(inodes[i].parent, "");
        strcpy(inodes[i].name, "");

        inodes[i].inUse = 0;
        inodes[0].fd = -1;
        inodes[i].inodeIndex = i;
        inodes[i].type = I_UNUSED;
        
        inodes[i].lastAccessTime = 0;
        inodes[i].lastModificationTime = 0;

        // Set each inode datablock to invalid (not initialized)
        for (int j = 0; j < MAX_DATABLOCK_POINTERS; j++) 
        {
            inodes[i].directBlockPointers[j] = -1;
        }
        inodes[i].numDirectBlockPointers = 0;
    }

    // Write inode changes to disk
    char* char_p = (char*) inodes;
    uint64_t blocksWritten = LBAwrite(char_p, inodeBlockTotal, inodeStartBlock);
    printf("Wrote inodes to %ld blocks.\n", blocksWritten);
    printf("Wrote %d inodes of size %ld bytes each starting at block %d.\n", inodeTotal, sizeof(fs_dir), inodeStartBlock);
    free(inodes);
}

fs_VCB* getVCB() 
{
    return openVCB;
}

// Reads the VCB from the disk
uint64_t readVCB()
{
    if (!volumeInitialized) 
    {
        printf("readVCB: VCB system not initialized.\n");
        return 0;
    }

    uint64_t blocksRead = LBAread(openVCB, vcbTotal, VCB_START_BLOCK);
    printf("Read VCB in %d blocks starting at block %d.\n", vcbTotal, VCB_START_BLOCK);
    return blocksRead;
}

// Write all changes on VCB to the disk
uint64_t writeVCB() 
{
    if (!volumeInitialized) 
    {
        printf("writeVCB: VCB system not initialized.\n");
        return 0;
    }

    uint64_t blocksWritten = LBAwrite(openVCB, vcbTotal, VCB_START_BLOCK);
    printf("writeVCB: %ld\n", blocksWritten);
    printf("Wrote VCB in %d blocks starting at block %d.\n", vcbTotal, VCB_START_BLOCK);
    return blocksWritten;
}

// Returns the index of a free block
uint64_t getFreeBlock()
{
    for (int index = 0; index < diskSizeBlocks; index++)
    {
        if (findBit(openVCB->freeMap, index) == 0) 
        {
            return index;
        }
    }
    return -1;
}

void printVCB()
{
    int size = openVCB->totalVCBBlocks * (openVCB->blockSize);
    int width = 16;
    char* char_p = (char*)openVCB;
    char ascii[width+1];
    sprintf(ascii, "%s", "................");
    for (int i = 0; i < size; i++) 
    {
        printf("%02x ", char_p[i] & 0xff);
        if (char_p[i]) 
        {
            ascii[i%width] = char_p[i];
        }
        if ((i+1)%width==0&&i>0) 
        {
            ascii[i%width+1] = '\0';
            printf("%s\n", ascii);
            sprintf(ascii, "%s", "................");
        } 
        else if (i==size-1) 
        {
            for (int j=0; j<width-(i%(width-1)); j++) 
            {
                printf("   ");
            }
            ascii[i%width+1] = '\0';
            printf("%s\n", ascii);
            sprintf(ascii, "%s", "................");
        }
    }
    printf("Printed VCB Size: %d bytes\n", size);
}

int createVolume(char* volumeName, uint64_t _volumeSize, uint64_t _blockSize) 
{
    // Check if volume already exists
    if (access(volumeName, F_OK) != -1) 
    {
        printf("Cannot create volume '%s'. Volume already exists.\n", volumeName);
        return -1;
    }

    uint64_t existingVolumeSize = _volumeSize;
    uint64_t existingBlockSize = _blockSize;

    // Initialize the volume on disk
    int retVal = startPartitionSystem(volumeName, &existingVolumeSize, &existingBlockSize);

    printf("Opened %s, Volume Size: %llu;  BlockSize: %llu; Return %d\n", volumeName, (ull_t)existingVolumeSize, (ull_t)existingBlockSize, retVal);

    // Initalize the volume control block
    if (retVal == 0) 
    {
        initialize(_volumeSize, _blockSize);
        printf("Done initialization\n");
        initializeVCB();
        printf("Done initialization of VCB\n");
        initializeInodes();
        printf("Done initialization of Inodes\n");
    }

    closeVolume();
    printf("Close volume\n");
    return retVal;
}

void openVolume(char* volumeName) 
{
    if (!volumeInitialized) 
    {
        uint64_t existingVolumeSize;
        uint64_t existingBlockSize;

        int retVal = startPartitionSystem(volumeName, &existingVolumeSize, &existingBlockSize);
        if (!retVal) 
        {
            initialize(existingVolumeSize, existingBlockSize);
            readVCB();
            printVCB();
        }
    }
    else 
    {
        printf("Failed to open volume '%s'. Another volume is already open.\n", volumeName);
    }
}

void closeVolume() 
{
    if (volumeInitialized) 
    {
        closePartitionSystem();
        free(openVCB);
        volumeInitialized = 0;
    } 
    else 
    {
        printf("Can't close volume. Volume not open.\n");
    }
}

