/* Group 2
* Project 2
* November 10, 2021
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

//system size info
#define BLOCK_SIZE 1024
#define FREE_ARRAY_SIZE 251 // free and inode array size
#define INODE_SIZE 64

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
	unsigned int size0; /* high of 32 bit size*/
	unsigned int size1; /* low int 32 bit size*/
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
int file_descriptor ; 


void openfs(char* file_name) {
	file_descriptor = open(file_name, O_RDWR);		//open file given
		
	if (file_descriptor == -1) {				//file is not in system
		file_descriptor = open(file_name, O_CREAT);	// create file
		printf("file created and opened\n");  
	}
	else {
		printf("file opened\n");			//file found
	}
	return ;
}

void allocateBlocks(void) {
	int blockIdx = superBlock.isize + 1;			//isize + superBlock
	int unallocatedBlocks = superBlock.fsize - blockIdx;	//total data blocks
	int nextFree[252] = { 0 };				//next free array
	int nextnFree = 0;					// nfree value
	int storageBlockIdx = 0;
	int nextStorageBlockIdx = 0;
	int totalBytes = 0; 

	if (unallocatedBlocks < FREE_ARRAY_SIZE) {		//number of blocks < free array size
		nextnFree = unallocatedBlocks;
		for (int i = 0; i < unallocatedBlocks; i++) {
			blockIdx += 1;
			superBlock.free[i] = blockIdx;
			
		}
		superBlock.nfree = nextnFree;
		return;
	}
	else {							//number of blocks < free array size
		nextnFree = FREE_ARRAY_SIZE;
		//write to free array to superblock
		for (int i = 0;  i < FREE_ARRAY_SIZE; i++) {
			blockIdx += 1;
			superBlock.free[i] = blockIdx;
			
		}

		superBlock.nfree = nextnFree;
		unallocatedBlocks = unallocatedBlocks - FREE_ARRAY_SIZE;
		storageBlockIdx = superBlock.free[0];

		while (unallocatedBlocks >= 0) {

			if (unallocatedBlocks > FREE_ARRAY_SIZE) {	//free blocks > 251
				nextFree[FREE_ARRAY_SIZE] = storageBlockIdx;
				for (int i = 1 ; i < FREE_ARRAY_SIZE; i++) {			
					blockIdx += 1;
					if (i == 1) {
						nextFree[i] = blockIdx;
						nextStorageBlockIdx = blockIdx;				
					}
					else {

						nextFree[i] = blockIdx;
						
					}
					

				}
				nextFree[0] = FREE_ARRAY_SIZE;		// set new nfree value
				storageBlockIdx = nextStorageBlockIdx;

				//write to storage block
				totalBytes = (storageBlockIdx * BLOCK_SIZE); //calculate block number
				lseek(file_descriptor, totalBytes, SEEK_SET);
				write(file_descriptor, nextFree, (FREE_ARRAY_SIZE + 1) * sizeof(int));

				//update unallocated blocks
				unallocatedBlocks = unallocatedBlocks - FREE_ARRAY_SIZE; 
			}
			else {
				nextFree[unallocatedBlocks] = storageBlockIdx;		//free blocks < 251
				for (int i = 1; i < unallocatedBlocks; i++) {
						blockIdx += 1;
						nextFree[i] = blockIdx;
						
						
				}
				nextFree[0] = unallocatedBlocks;		// set new nfree value

				//write to storage block
				totalBytes = (storageBlockIdx * BLOCK_SIZE); 	//calculate block number
				lseek(file_descriptor, totalBytes, SEEK_SET);
				write(file_descriptor, nextFree, (unallocatedBlocks + 1) * sizeof(int));

				unallocatedBlocks = -1; 
			}
			
		}

	}
	


}

int getFreeBlock(void) {

	superBlock.nfree -=1;		//get next free block from free array
	if (superBlock.nfree > 0) {
		if (superBlock.free[superBlock.nfree] == 0) { //system is full, return error 
			return -1; 
		}
		else { 
			return superBlock.free[superBlock.nfree];	//get free block from free array
		}
	}
	else{						//get new set of free blocks for free array
		int newBlock = superBlock.free[0];			
		int totalBytes = newBlock * BLOCK_SIZE;
		
		lseek(file_descriptor, totalBytes, SEEK_SET);				//find block from free array
		read(file_descriptor, superBlock.free, FREE_ARRAY_SIZE * sizeof(int)); //read in 251 blocks into free array
		
		superBlock.nfree = FREE_ARRAY_SIZE-1;					// update nfree
		return superBlock.free[superBlock.nfree];				// get free block from free array
	}


}

void createRootDirectory(void) {
	inode_type rootDir;
	dir_type direc[2];
	int freeBlock; 
	int totalBytes; 

	freeBlock = getFreeBlock();
	if (freeBlock == -1) {							// System was full
		printf("No Free Blocks Available\n");
		return;
	}
	
	direc[0].inode = 1;							//1st inode = root directory
	strcpy(direc[0].filename, ".");

	direc[1].inode = 1;							//1st node = parent
	strcpy(direc[1].filename, "..");
	
	// inode struct
	rootDir.flags = (INODE_ALLOC | DIREC_FILE); 		// I-node allocated + directory file 
	rootDir.nlinks = 1;
	rootDir.uid = 0;
	rootDir.gid = 0;
	rootDir.addr[0] = freeBlock;
	rootDir.actime = time(NULL);
	rootDir.modtime = time(NULL);
	
	// directory size
	rootDir.size0 = (int)((sizeof(direc) & 0xFFFFFFFF00000000) >> 8); 	// high 32 bit
	rootDir.size1 = (int)(sizeof(direc) & 0x00000000FFFFFFFF); 		// low 32 bit

	//write inode to inode block
	totalBytes = (2 * BLOCK_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	write(file_descriptor, rootDir, sizeof(inode_type));
}

void initfs (int total_blocks, int total_inode_blocks){
    int x;
	superBlock.isize = total_inode_blocks; 		// total number of blocks for inode
	superBlock.fsize = total_blocks;		// total blocks for system

    if (file_descriptor < 2)  //verify if a file is opened
    {
        printf("File was not opened \n");
        return;
    }
	// defining variables of the superblock  ??? Ask prof
	superBlock.nfree = 0; 
	superBlock.flock = 0;
	superBlock.ilock = 0;
	superBlock.fmod = 0;
	superBlock.time = time(NULL);
	
	//set all blocks as free
	allocateBlocks();

    // writing the super block
	lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);
	write(file_descriptor, &superBlock, BLOCK_SIZE);

    // create root directory for the first inode
    createRootDirectory();

	// allocate other inodes as free
	int num_inodes = (BLOCK_SIZE/INODE_SIZE)*superBlock.isize;
	int blockNo, inodeBytes, inodeOffset, totalBytes;
	for (x = 2; x <= num_inodes; x++) {
		inode_type nodeX; 
		nodeX.flags = INODE_FREE; // set inodes to free

		inodeBytes = (x-1) * INODE_SIZE;		// total bytes for inode
		totalBytes = (2 * BLOCK_SIZE) + inodeBytes;

		lseek(file_descriptor, totalBytes, SEEK_SET);
		write(file_descriptor, nodeX, sizeof(inode_type));
	}

	return; 
}


void quit() {
	exit(0); //exit system
}



void modv6cmds(char* command) {

	char* token;
	char* args[5] = { NULL };  // spliced arguments
	// command
	token = strtok(command, " ");
	args[0] = token;


	if (args[0] != NULL) {
		if ((strcmp(args[0], "openfs") == 0)) {
			args[1] = strtok(NULL, "\n"); 
			openfs(args[1]); //pass file 
		}
		else if ((strcmp(args[0], "initfs") == 0)) {
			args[1] = strtok(NULL, " ");	// file system size in # blocks
			args[2] = strtok(NULL, "\n");	// # nodes for i-nodes

			if ((args[1] != NULL) && (args[2] != NULL)) {
				char arg1[10];
				char arg2[10];
				int i; 
				strcpy(arg1, args[1]);
				strcpy(arg2, args[2]);
				int arg1Len = strlen(arg1); 
				for (i = 0; i < arg1Len; i++) { //checking all numeric inputs
					if (isdigit(arg1[i]) == 0) {	//input is not numeric
						printf("Input is not numeric\n");
						return;
					}
				}
				int arg2Len = strlen(arg2);
				for (i = 0; i < arg2Len; i++) { //checking all numeric inputs
					if (isdigit(arg2[i]) == 0) {	//input is not numeric
						printf("Input is not numeric\n");
						return;
					}
				}

				int n1 = atoi(args[1]);
				int n2 = atoi(args[2]);
				if (n1 <= 0 || n2 <= 0) {				// no blocks or inode blocks to initialize
					printf("Invalid size\n"); 
					return; 
				}
				
				if (n1 < n2) {							//number of inode blocks > system size
					printf("Invalid size\n");
					return; 
				}

				initfs(n1, n2); 
			}
			else {
				printf("Invalid Command\n");
			}

		}
		else if ((strcmp(args[0], "q\n") == 0)) {
			quit();
		}
		else {
			printf("Invalid Command\n");
		}
	}
	
	else {
		printf("Invalid Command\n");
	}

	return;
}

int main(void) {
	char command[512];
	file_descriptor = 0; 

	while (1) {
		printf("Enter command:");
		fgets(command, sizeof(command), stdin); //get command from console
		modv6cmds(command);			//find modv6 command

	}
	return 0;
}

