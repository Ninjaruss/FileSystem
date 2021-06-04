/**************************************************************
* Class:  CSC-415
* Name: Henry Kang
* Student ID: 920765115
* Project: Basic File System
*
* File: b_io.c
*
* Description: 
*	This is the implementation of functions defined in b_io.h
**************************************************************/
#include "b_io.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "fsLow.h"
#include "fsInode.h"
#include "mfs.h"
#include "fsVCB.h"

//initialize global constants
#define MAXFCBS 20
#define BUFSIZE getVCB()->blockSize

//structure to store fcb information
typedef struct b_fcb
{
    fs_dir* file;       //the opened file
    int block;          //the block index
    char * buf;         //the open file buffer
    int offset;         //current position in the buffer
    int buflen;         //number of bytes in the buffer
    int flag;           //the flag of the file
}b_fcb;

//open file table
b_fcb fcbArray[MAXFCBS];

//indicator for initialization: 0 for uninitialized
int initialized = 0;

//function to initialize fcbArray
void b_init() {
    for(int i = 0; i < MAXFCBS; i++) {
        fcbArray[i].file = NULL;
    }

    initialized = 1;
}

//function to obtain free element
int b_getFCB() {
    for(int i = 0; i < MAXFCBS; i++) {
        if(fcbArray[i].file == NULL) {
            return i;
        }
    }

    return -1; // return -1 if there is no free element
}

//opens the file with flags and returns fd
int b_open(char * filename, int flags) {
    //initialize fcbArray if not
    if(initialized == 0) {
        b_init();
    }
    
    // Parse the path into a tokenized array of path levels
    parseFilePath(filename);

    // Combine tokens into a single char string; gets the parent of the called level
    char fullPath[256] = "";
    for (int i = 0; i < requestFilePathArraySize; i++)
    {
        // Append a '/' before each token
        strcat(fullPath, "/");
        strcat(fullPath, requestFilePathArray[i]);
    }

    fs_dir* inode = getInode(fullPath);
    
    //if directory entry does not exist, create one and initialize if create flag is set
    if(inode == NULL) {
        // printf("inodeName: %s | inUse: %d\n", inode->name, inode->inUse);
        if(flags & O_CREAT) {
            //printf("createInode with flags\n");
            inode = createInode(I_FILE, filename);
            if (inode == NULL)
            {
                printf("Failed to create inode.\n");
                return -1;
            }

            /*
            // Add the parent to the inode
            char parentPath[MAX_FILENAME_SIZE];
			getParentPath(parentPath, filename);
			fs_dir* parentInode = getInode(parentInode);
			setParent(parentInode, inode);
            */
            
            
            
            // Write changes to disk
			// writeInodeArray();
            inode->sizeInBytes = 0;
        } 
        /*
        else 
        {
            return -1;
        }
        */
    }

    //if flag has trancate, reinitialize the file size to 0
    if(flags & O_TRUNC) {
        inode->sizeInBytes = 0;
        inode->numDirectBlockPointers = 0;
        inode->sizeInBlocks = 0;
    }

    //find empty fcb index
    int index = b_getFCB();
    if(index == -1) {
        printf("The open file list is full.");
        return -1;
    }

    //initialize and return index
    fcbArray[index].file = inode;
    fcbArray[index].buf = malloc(BUFSIZE);
    fcbArray[index].block = 0;
    fcbArray[index].buflen = 0;
    fcbArray[index].flag = flags;
    fcbArray[index].offset = 0;

    return index;
}

//reads file's content into buffer and returns bytes read
int b_read(int fd, char * buffer, int count) {
    int bytesRead;              //what we are reading
    int bytesReturned;          //what we are returning
    int part1, part2, part3;    //holding copy lengths

    //initialize fcbArray if not
    if(initialized == 0) {
        b_init();
    }

    //check if fd is between 0 and MAXFCBS-1
    if((fd < 0) || (fd >= MAXFCBS)) {
        return -1;
    }

    //check if the file is opened
    if(fcbArray[fd].file == NULL) {
        return -1;
    }

    //obtain number of bytes available to copy from the buffer
    int remain = fcbArray[fd].buflen - fcbArray[fd].offset;
    part3 = 0;

    //we have enough in buffer
    if(remain >= count) {
        part1 = count;
        part2 = 0;
    } else {
        //read remaining first
        part1 = remain;
        part2 = count - remain;
    }

    //memcpy what's in part1
    if(part1 > 0) {
        memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].offset, part1);
        fcbArray[fd].offset += part1;
    }

    //we have more to read
    if(part2 > 0) {
        //case where count is greater than block size
        if(part2 > BUFSIZE) {
            //obtain number of blocks needed
            int blocks = part2 / BUFSIZE;
            bytesRead = LBAread(buffer + part1, blocks, fcbArray[fd].file->directBlockPointers[fcbArray[fd].block]) * BUFSIZE;
            part3 = bytesRead;
            part2 -= part3;
            fcbArray[fd].block += blocks;
        }

        //try to read block size into our buffer
        bytesRead = LBAread(fcbArray[fd].buf, 1, fcbArray[fd].file->directBlockPointers[fcbArray[fd].block]) * BUFSIZE;

        //error handling if read fails
        if(bytesRead == 0) {
            return -1;
        }

        //set index, buflen, and lbaPosition
        fcbArray[fd].offset = 0;
        fcbArray[fd].buflen = bytesRead;
        fcbArray[fd].block++;

        //not enough to satisfy read
        if(bytesRead < part2) {
            part2 = bytesRead;
        }

        //memcpy rest
        if(part2 > 0) {
            memcpy(buffer + part1 + part3, fcbArray[fd].buf + fcbArray[fd].offset, part2);
            fcbArray[fd].offset += part2;
        }
    }

    bytesReturned = part1 + part2 + part3;
    return bytesReturned;
}

//writes buffer's content into file and returns bytes written
int b_write(int fd, char * buffer, int count) {
    int bytesWritten;              //what we are writing
    int bytesReturned;          //what we are returning
    int part1, part2, part3;    //holding copy lengths

    //initialize fcbArray if not
    if(initialized == 0) {
        b_init();
    }

    //check if fd is between 0 and MAXFCBS-1
    if((fd < 0) || (fd >= MAXFCBS)) {
        return -1;
    }

    //check if the file is opened
    if(fcbArray[fd].file == NULL) {
        return -1;
    }

    //available space in buffer
    int remain = BUFSIZE - fcbArray[fd].buflen;

    //only used if count > BUFSIZE
    part3 = 0;

    //we have enough space in buffer
    if(remain >= count) {
        part1 = count;
        part2 = 0;
    } else {
        //we copy part of it first
        part1 = remain;
        part2 = count - remain;
    }

    //memcpy part1
    if(part1 > 0) {
        memcpy(fcbArray[fd].buf + fcbArray[fd].offset, buffer, part1);
        fcbArray[fd].offset += part1;
        fcbArray[fd].buflen += part1;
    }

    //take care of remaining bytes to write
    if(part2 > 0) {
        //write what is in our buffer
        int freeBlock = getFreeBlock();
        bytesWritten = writeBufferToInode(fcbArray[fd].file, fcbArray[fd].buf, BUFSIZE, freeBlock) * BUFSIZE;
        fcbArray[fd].block++;
        
        //error handling
        if(bytesWritten == 0) {
            return -1;
        }

        //reset offset and buflen and increment lbaPosition
        fcbArray[fd].offset = 0;
        fcbArray[fd].buflen = 0;

        //handle special case where writing more than block size
        if(part2 > BUFSIZE) {
            //obtain number of blocks needed
            int blocks = part2 / BUFSIZE;

            //write to blocks
            int blockWritten = 0;

            for(int i = 0; i < blocks; i++) {
                freeBlock = getFreeBlock();
                blockWritten += writeBufferToInode(fcbArray[fd].file, buffer + part1, BUFSIZE, freeBlock);
                fcbArray[fd].block++;
            }
            bytesWritten = blockWritten * BUFSIZE;

            //error handling
            if(bytesWritten == 0) {
                return -1;
            }

            //written amount to part3
            part3 += bytesWritten;
        }

        //copy remaining into buffer
        part2 -= part3;
        if(part2 > 0) {
            memcpy(fcbArray[fd].buf + fcbArray[fd].offset, buffer + part1 + part3, part2);
            fcbArray[fd].offset = part2;
            fcbArray[fd].buflen = part2;
        }
    }

    //return bytes written
    bytesReturned = part1 + part2 + part3;
    return bytesReturned;
}

//moves offset based on whence and returns the resulting offset
int b_seek(int fd, off_t offset, int whence) {
    //set num to return based on offset and whence
    int num;
    if(whence == SEEK_SET) {
        num = offset;
    } else if(whence == SEEK_CUR) {
        num = offset + ((fcbArray[fd].block - 1) * BUFSIZE);
    } else if(whence == SEEK_END) {
        num = offset + fcbArray[fd].file->sizeInBytes;
    }

    //move the file offset
    int block = num / BUFSIZE;
    int off = num % BUFSIZE;
    int index = fcbArray[fd].file->directBlockPointers[block - 1];

    LBAread(fcbArray[fd].buf, 1, index);
    fcbArray[fd].offset = off;

    return num;
}

//write remaining buffer, close the file, and clean up
void b_close(int fd) {
    //if the file is for write, write the rest to the file
    if(fcbArray[fd].flag & O_WRONLY) {
        int freeBlock = getFreeBlock();
        int written = writeBufferToInode(fcbArray[fd].file, fcbArray[fd].buf, fcbArray[fd].buflen, freeBlock) * BUFSIZE;
        fcbArray[fd].block++;
        fcbArray[fd].file->numDirectBlockPointers = fcbArray[fd].block;
        //error handling
        if(written == 0) {
           // printf("Error writing\n");
        }
    }
    
    //clean up
    fcbArray[fd].offset = 0;
    fcbArray[fd].buflen = 0;
    fcbArray[fd].flag = 0;
    fcbArray[fd].block = 0;
    free(fcbArray[fd].buf);
    fcbArray[fd].buf = NULL;
    fcbArray[fd].file = NULL;
}