/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: bitMap.c
*
* Description: Manages the bits in the passed bitmap.
*
**************************************************************/
#include "bitMap.h"

void setBit(int freeMap[], int blockNumber)
{
    freeMap[blockNumber / 32] |= 1 << (blockNumber % 32);
}

int findBit(int freeMap[], int blockNumber)
{
    return ((freeMap[blockNumber / 32] & (1 << (blockNumber % 32))) != 0);
}

void clearBit(int freeMap[], int blockNumber)
{
    freeMap[blockNumber / 32] &= ~(1 << (blockNumber % 32));
}
