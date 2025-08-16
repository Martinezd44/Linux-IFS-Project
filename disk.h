
#ifndef DISK_DOT_H
#define DISK_DOT_H
int createDisk(const char *disk, int bcount);
int openDisk(const char *disk);
int diskClose();
int blockWrite(int block, const void *buffer);
int blockRead(int block, void *buf);
int writeToFile( char *text, int inode[]);
int truncateBlock(int block);
#endif /*DISK_DOT_H*/