#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



//system size info
#define BLOCK_SIZE 1024
#define FREE_ARRAY_SIZE 251 // free and inode array size
#define INODE_SIZE 64
#define MAX_NFREE 250
#define DIR_SIZE 32

typedef struct {
	int isize; // number of blocks dedicated to i-list
	int fsize; // first block not potentially avaialble for allocation to a file
	int nfree; //free array index
	unsigned int free[FREE_ARRAY_SIZE]; // array of free blocks
	int flock;
	int ilock;
	unsigned int fmod;
	unsigned int time;
} superblock_type; //1024 bytes


/* i-node structure: 16 i-nodes/block */
typedef struct {
	unsigned short flags;
	unsigned short nlinks;
	unsigned int uid;
	unsigned int gid;
	unsigned int size0; /* high of 64 bit size*/
	unsigned int size1; /* low int 64 bit size*/
	unsigned int addr[9];
	unsigned int actime;
	unsigned int modtime;
} inode_type;// 64 bytes

/* directory struct */
typedef struct {
	unsigned int inode;
	unsigned char filename[28];
}dir_type; //32 bytes


// flag bit comparisons
#define INODE_ALLOC 0x8000 //A = 1 : I-node is allocated 
#define INODE_FREE 0x0000  // A = 0: I-node is free
#define BLOCK_FILE 0x6000 // BC = 11: block file
#define DIREC_FILE 0x4000 // BC = 10: directory file
#define CHAR_FILE 0x2000 // BC = 01: char special file
#define PLAIN_FILE 0x0000 // BC = 00: plain file
#define SMALL_FILE 0x0000		// DE = 00
#define MEDIUM_FILE 0x0800		// DE = 01
#define LONG_FILE 0x1000		// DE = 10
#define SUPERLONG_FILE 0x1800	//DE = 11
#define SETUID_EXEC 0x0400		//F = 1
#define SETGID_EXEC 0x0200		//G = 1

// global variables
superblock_type superBlock;
extern int file_descriptor;
extern int currentInode;

/* mkdir.c functions*/
extern int getFreeInode(void);
void updateInodeEntry(int addBytes, int inodeNum, inode_type newNode);
void mkdirv6(char* dir_name);
int addNewFileDirectoryEntry(int parentInode, dir_type newDir);
void changeDirectoryV6(char* dir_name); 
int findDirectory(char* dir_name, int parentNode);
int cpin (char* externalFile, char* internalFile);
void cpout(char* sourceFile, char* destinationFile);
void rm(char* filename);
void addFreeBlock (int block);
inode_type getInode(int inodeNumber);