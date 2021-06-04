/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsVolume.h
*
* Description: Header file for fsVolume.c
*
**************************************************************/
#include <stdio.h>
#include "mfs.h"
#include "fsLow.h"

#define DATA_BLOCKS_PER_INODE 4 // Allocated blocks for each Inode
#define VCB_START_BLOCK 0

// Volume control block
typedef struct 
{
  char header[16];
  uint64_t volumeSize;
  uint64_t blockSize;
  uint64_t diskSizeBlocks;
  uint64_t vcbStartBlock;
  uint64_t totalVCBBlocks;

  uint64_t inodeStartBlock;
  uint64_t totalInodes;
  uint64_t totalInodeBlocks;
  uint64_t freeMapSize;
  uint32_t freeMap[];
} fs_VCB;

void initialize(uint64_t _volumeSize, uint64_t _blockSize);
int allocateVCB(fs_VCB** vcb);
void initializeVCB();
void initializeInodes();
fs_VCB* getVCB();
uint64_t readVCB();
uint64_t writeVCB();
uint64_t getFreeBlock();
void printVCB();
uint64_t divUp(u_int64_t a, u_int64_t b);

int createVolume(char* volumeName, uint64_t _volumeSize, uint64_t _blockSize);
void openVolume(char* volumeName);
void closeVolume();