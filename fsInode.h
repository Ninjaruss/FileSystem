/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsInode.h
*
* Description: Header file for fsInode.c
*
**************************************************************/
#include "mfs.h"
#include "bitMap.h"

int initInodeArray();
void closeInodeArray();
fs_dir* createInode(InodeType type, const char* path);
fs_dir* getInode(const char *pathname);
fs_dir* getFreeInode();
int removeFromParent(fs_dir* parent, fs_dir* child);
void writeInodes();
int writeBufferToInode(fs_dir * inode, char* buffer, size_t bufSizeBytes, uint64_t blockNumber);
void freeInode(fs_dir * node);