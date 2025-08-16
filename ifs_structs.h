#include "ifs_types.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#define IFS_BLOCK_SIZE 1024 /*block size 1024 bytes*/
//unsigned shorts 0 to 65,535
#pragma pack(1)

struct ifs_super_block{
    uint32_t num_blocks; /*total number of blocks*/
    uint32_t totalNumOfBlocks; /*total number of blocks used for inode list*/
    uint32_t root_inode; /*always inode 1*/
    uint32_t numberOfFiles; 
    char padding[IFS_BLOCK_SIZE - 3 * sizeof(uint32_t)]; /*pad out to an entire block*/
}__attribute__((packed));


struct inode{ 
    int freeInodes[10]; /*array of 10 disk-block addresses*/
    time_t i_accesstime; /*time of last access*/
    time_t i_changetime; /*date and time file was last changed*/
    unsigned short i_size; /* file size in bytes */
    unsigned short i_ino; //inode number of file 
    unsigned short i_type; /*type of file- regular file or directory*/
    unsigned short i_uid;  /* user id of owner */
    unsigned short i_gid; /* group id of owner */
    unsigned short i_mode; /* access permissions */
    
}__attribute__((packed));

struct directory{ //32 bytes 
    int d_dinode; /*inode number*/
    char d_dname[28]; /*filename of directory*/
};

