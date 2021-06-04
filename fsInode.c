/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsInode.c
*
* Description: Handles the Inode array. Holds information about each directory.
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsInode.h"
#include "fsVCB.h"

fs_dir* inodes;

int initInodeArray()
{
    printf("totalInodeBlocks %ld, blockSize %ld\n", getVCB()->totalInodeBlocks, getVCB()->blockSize);
    inodes = calloc(getVCB()->totalInodeBlocks, getVCB()->blockSize);
    printf("Inodes allocated at %p.\n", inodes);

    uint64_t blocksRead = LBAread(inodes, getVCB()->totalInodeBlocks, getVCB()->inodeStartBlock);
    printf("%ld inode blocks were read.\n", blocksRead);

    // Return failed if not enough blocks read
    if (blocksRead != getVCB()->totalInodeBlocks)
    {
        printf("fs_init: Failed to read all inode blocks.\n");
        return 0;
    }
    return 1;
}

void closeInodeArray()
{
    free(inodes);
}

fs_dir* createInode(InodeType type, const char* path)
{
    fs_dir * inode;
    char parentPath[MAX_FILEPATH_SIZE];
    fs_dir* parentNode;

    // Set the current creation time
    time_t currentTime;
    currentTime = time(NULL);

    // Find and assign the parent to the new inode
    inode = getFreeInode();
    getParentPath(parentPath, path);
    parentNode = getInode(parentPath);

    // Set inode info
    inode->type = type;
    strcpy(inode->name , getPathName());
    sprintf(inode->path, "%s/%s", parentPath, inode->name);
    inode->lastAccessTime = currentTime;
    inode->lastModificationTime = currentTime;

    // Set the inode's parent
    if (!setParent(parentNode, inode)) 
    {
        freeInode(inode);
        // printf("Failed to set parent.\n");
        return NULL;
    }

    //printf("Sucessfully created inode for path '%s'.\n", path);       
    return inode;
}

// Get inode with specified path
fs_dir* getInode(const char *path)
{
    //printf("Searching for path: '%s'\n", path);
    for (int i = 0; i < getVCB()->totalInodes; i++) 
    {
        //printf("Inode: (%s)\n", inodes[i].path);
        if (strcmp(inodes[i].path, path) == 0) 
        {
            //printf("Inode found! (%s)\n", path);
            return &inodes[i];
        }
    }
    //printf("Failed to find inode. (%s)\n", path);
    return NULL;
}

// Return first open inode in the array; return null if none found
fs_dir* getFreeInode()
{
    fs_dir* freeInode;

    for (size_t i = 0; i < getVCB()->totalInodes; i++) 
    {
        if (inodes[i].inUse == 0) 
        {
            inodes[i].inUse = 1;
            freeInode = &inodes[i];
            return freeInode;
        }
    }

    printf("No free inode found.\n");
    return NULL;
}

int removeFromParent(fs_dir* parent, fs_dir* child) 
{
    // Find matching requested child from parent list
    for (int i = 0; i < parent->numChildren; i++) 
    {
        if (!strcmp(parent->children[i], child->name)) 
        {
            // Clear the child from the list and update parent size
            strcpy(parent->children[i], "");
            parent->numChildren--;
            parent->sizeInBlocks -= child->sizeInBlocks;
            parent->sizeInBytes -= child->sizeInBytes;
            return 1;
        }
    }
    
    printf("Could not find child '%s' in parent '%s'.\n", child->name, parent->path);
    return 0;
}

void writeInodes() 
{
    LBAwrite(inodes, getVCB()->totalInodeBlocks, getVCB()->inodeStartBlock);
}

int writeBufferToInode(fs_dir * inode, char* buffer, size_t bufSizeBytes, uint64_t blockNumber) 
{
    // Get free index from the data block pointer list
    int freeIndex = -1;
    for (int i = 0; i < MAX_DATABLOCK_POINTERS; i++) 
    {
        if (inode->directBlockPointers[i] == -1) 
        {
            freeIndex = i;
            break;
        }
    }

    // If no free index to insert dataBlock, return 0 
    if (freeIndex == -1) 
    {
        return 0;
    }

    // Write buffer data into the data block
    LBAwrite(buffer, 1, blockNumber);
    setBit(getVCB()->freeMap, blockNumber);
    writeVCB();

    // Update inode info
    inode->directBlockPointers[freeIndex] = blockNumber;
    inode->numDirectBlockPointers++;
    inode->sizeInBlocks++;
    inode->sizeInBytes += bufSizeBytes;
    inode->lastAccessTime = time(0);
    inode->lastModificationTime = time(0);

    writeInodes();
    return 1;
}

void freeInode(fs_dir * inode)
{
    inode->inUse = 0;
    inode->type = I_UNUSED;
    strcpy(inode->name, "\0");
    strcpy(inode->path, "\0");
    strcpy(inode->parent, "\0");
    inode->sizeInBlocks = 0;
    inode->sizeInBytes = 0;
    inode->lastAccessTime = 0;
    inode->lastModificationTime = 0;
    inode->fd = -1;

    if (inode->numChildren > 0)
    {
        for (int i = 0; i < inode->numChildren; i++)
        {
            strcpy(inode->children[i], "");
            inode->numChildren--;
        }
    }
    
    // If the inode is a file, free all related data block pointers
    if (inode->type == I_FILE)
    {
        for (int i = 0; i < inode->numDirectBlockPointers; i++) 
        {
            int blockPointer = inode->directBlockPointers[i];
            clearBit(getVCB()->freeMap, blockPointer);
        }
    }

    // Write inode changes to disk
    writeInodes();
}