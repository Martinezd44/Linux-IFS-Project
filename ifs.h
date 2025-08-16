#define IFS_DOT_H
void usageMessage();
int findFreeInode();
int findFreeInodeInIL();
int createDirectory(const char *filename, int parent);
void createSuperBlock();
void create_fs(int userID);
int createFile(const char *filename, int parent);
int appendToFile(int inode, char *text);
int truncateFile(int inode);
int printFile(int inode);