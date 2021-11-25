/* Project 2 - Part 2
*  November 24, 2021
*
* Group 2 - Part 2 contributions:
* Jessica Nguyen: getFreeInode, updateInodeEntry, mkdirv6, addNewFileDirectoryEntry,
				changeDirectoryV6, findDirectory, fileSystemCheck, & debugging
* Madhumita Ramesh: getInode, addFreeBlock, cpout, rm, README
* Micah Steinbrecher: cpin, rm, debugging & integrating modules
*
* How to compile & run:
* See README.md
*
*
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
#define SMALL_FILE 0x0000// DE = 00
#define MEDIUM_FILE 0x0800// DE = 01
#define LONG_FILE 0x1000// DE = 10
#define SUPERLONG_FILE 0x1800//DE = 11
#define SETUID_EXEC 0x0400//F = 1
#define SETGID_EXEC 0x0200//G = 1



// global variables
superblock_type superBlock;
int file_descriptor;
int currentInode;
char currentFilepath[200]; 

void openfs(char* file_name) {
	file_descriptor = open(file_name, O_RDWR);//open file given


	if (file_descriptor == -1) {//file is not in system

		file_descriptor = open(file_name, O_CREAT | O_RDWR, 0600);// create file with R&W
		printf("File created and opened\n");
	}
	else {
		printf("File opened\n");//file found

		//read in superblock
		lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);
		read(file_descriptor, &superBlock, BLOCK_SIZE);

	}
	return;
}


int fileSystemCheck(void) {

	if (file_descriptor < 2)  //verify if a file is opened
	{
		printf("Error: File is not opened \n");
		return -1;
	}
	if (superBlock.fsize < 1 || superBlock.isize < 1) { // check if file was initialized
		printf("Error: File not initialized\n");
		return -1;
	}
	return 1;
}

int getFreeInode(void) {
	int x;
	int num_inodes = (BLOCK_SIZE / INODE_SIZE) * superBlock.isize;
	int totalBytes;
	inode_type freeNode;

	totalBytes = (2 * BLOCK_SIZE);	//read inode from block
	lseek(file_descriptor, totalBytes, SEEK_SET);
	for (x = 1; x <= num_inodes; x++) {				//find next free inode

		read(file_descriptor, &freeNode, INODE_SIZE);

		if ((freeNode.flags & INODE_ALLOC) == 0) {			//check if inode is free
			return x;										//return next free inode value
		}
	}
	return -1; // did not find free inode

}


int findDirectory(char* dir_name, int parentNode) {
	// find parent folder directory
	inode_type rootNode;
	int totalBytes = (2 * BLOCK_SIZE) + ((parentNode - 1) * INODE_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	read(file_descriptor, &rootNode, INODE_SIZE);

	// check to see if inode is a directory file
	if ((rootNode.flags & DIREC_FILE) != DIREC_FILE) { //not a directory file
		printf("Error: File is not a directory file\n");
		return -1;
	}

	// go to size and see what is size
	unsigned long long size = ((rootNode.size0 << 32) | rootNode.size1); //find directory size

	// go to addr[]
	dir_type buffer[DIR_SIZE];
	int totalBlocks = size / BLOCK_SIZE; // total blocks in file
	int fpFound = 0; // folder Inode value

	while (totalBlocks > -1) {
		//load block to temp buffer
		totalBytes = rootNode.addr[totalBlocks] * BLOCK_SIZE;
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, buffer, BLOCK_SIZE);

		//read through list to find directory value
		int i = 0;
		for (i = 0; i < DIR_SIZE; i++) {
			if (strcmp(buffer[i].filename, dir_name) == 0) {
				fpFound = buffer[i].inode;
				return fpFound;				//return inode if found
			}
		}
		totalBlocks--;
		// search through inodes to find directory value
	}
	return fpFound;

}

void updateInodeEntry(int addBytes, int inodeNum, inode_type newNode) {

	unsigned long long size = ((newNode.size0 << 32) | newNode.size1); //find directory size
	int totalBytes = (2 * BLOCK_SIZE) + ((inodeNum - 1) * INODE_SIZE);
	unsigned long long writeBytes = size + addBytes;			//get total new bytes of file
	newNode.size0 = (int)((writeBytes & 0xFFFFFFFF00000000) >> 32); // high 64 bit
	newNode.size1 = (int)(writeBytes & 0x00000000FFFFFFFF); // low 64 bit
	newNode.actime = time(NULL);
	newNode.modtime = time(NULL);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	write(file_descriptor, &newNode, INODE_SIZE);

}

int cpin(char* externalFile, char* internalFile) {

	//get length of external file
	unsigned long long file_size;
	inode_type inode;
	int offset;
	unsigned long long inode_size;


	//check if file system is opened & valid
	int check = fileSystemCheck();
	if (check == -1)  //issue with file system
	{
		return -1;
	}

	//check external file is open
	int source_fd = open(externalFile, O_RDONLY);
	if (source_fd == -1) {
		printf("Error: File not found\n");
		return -1;
	}

	file_size = lseek(source_fd, 0, SEEK_END); // seek to end of file
	lseek(source_fd, 0, SEEK_SET); // seek back to beginning of file


	//get blocks
	int num_of_blocks = file_size / 1024 + (file_size % 1024 != 0);
	int nfree_reset = superBlock.nfree;
	int free_reset[251];
	memcpy(free_reset, superBlock.free, sizeof(free_reset));



	//get data blocks
	int blocks[num_of_blocks]; 
	int i = 0;
	for (i = 0; i < num_of_blocks; i++) {
		int free_block = getFreeBlock();
		if (free_block == -1) {
			printf("Error: Not enough system memory\n");

			//reset superblock
			superBlock.nfree = nfree_reset;
			memcpy(superBlock.free, free_reset, sizeof(free_reset));

			//rewrite superblock
			lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);//find superblock
			write(file_descriptor, &superBlock, BLOCK_SIZE);//update nfree in superblock
			return -1;
		}
		blocks[i] = free_block;
	}

	//write to data blocks
	for (i = 0; i < num_of_blocks; i++) {
		int bytes = 1024;
		if (i == num_of_blocks - 1) {			//last block get the offset
			bytes = file_size - (i * 1024);
		}
		char buffer[bytes];		 
		offset = blocks[i] * 1024;
		lseek(file_descriptor, offset, SEEK_SET);
		read(source_fd, buffer, bytes);				//get block from source file
		write(file_descriptor, buffer, bytes);		// write block to v6 system

	}

	//get inode for new file
	int inode_idx = getFreeInode();

	//set fields except address
	inode.flags = (INODE_ALLOC | PLAIN_FILE); // I-node allocated + directory file
	inode.nlinks = 1;
	inode.uid = 0;
	inode.gid = 0;
	inode.size0 = (int)((file_size & 0xFFFFFFFF00000000) >> 32);
	inode.size1 = (int)(file_size & 0x00000000FFFFFFFF);
	inode.actime = time(NULL);
	inode.modtime = time(NULL);
	//if small file:
	if (num_of_blocks < 9) {
		for (i = 0; i < num_of_blocks; i++)
			inode.addr[i] = blocks[i];
	}
	//if large file:
	else {
		for (i = 0; i < num_of_blocks; i++) {
			//todo
		}
	}
	updateInodeEntry(0, inode_idx, inode);

	//create new directory entry
	dir_type dir_entry;
	dir_entry.inode = inode_idx;
	strcpy(dir_entry.filename, internalFile);
	check = addNewFileDirectoryEntry(currentInode, dir_entry);
	if (check < 0) {//error occurred
		return -1;
	}

	return 1;
}

inode_type getInode(unsigned int inodeNumber) {

	inode_type inode;
	int totalBytes = (2 * BLOCK_SIZE) + (INODE_SIZE * (inodeNumber - 1));    // calculates the where the inode is present
	lseek(file_descriptor, totalBytes, SEEK_SET);
	read(file_descriptor, &inode, INODE_SIZE);                  // reads the inode values into inode variable
	return inode;
}

void addFreeBlock(unsigned int block) {
	if (superBlock.nfree == MAX_NFREE) {                   // if the free array is full, store the free blocks in a new block
		lseek(file_descriptor, BLOCK_SIZE * block, SEEK_SET);
		write(file_descriptor, superBlock.nfree, sizeof(int));
		write(file_descriptor, superBlock.free, (FREE_ARRAY_SIZE) * sizeof(int));
		superBlock.nfree = 0;
	}
	superBlock.free[superBlock.nfree] = block;                  // updating the free array with the new block
	superBlock.nfree++;
	lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);//find superblock
	write(file_descriptor, &superBlock, BLOCK_SIZE);//update nfree in superblock
}

void cpout(char* sourceFile, char* destinationFile) {
	inode_type node;
	int i, j;
	unsigned int block;
	unsigned long long inode_size;
	char buffer[BLOCK_SIZE] = { 0 };                              // temperoray storage to read the contents of block into
	dir_type directory[100];

	//check source file is open
	int check = fileSystemCheck();
	if (check == -1)  //issue with file system
	{
		return;
	}

	// external file is opened
	int destination = open(destinationFile, O_RDWR | O_CREAT, 0600);
	if (destination == -1) {
		printf("File error\n");
		return;
	}

	//check if source file is in directory
	check = findDirectory(sourceFile, currentInode);
	if (check == 0) {
		printf("Error: File is not in directory\n");
		return;
	}

	// gets the inode of the current directory
	node = getInode(currentInode);

	// total size of the current directory
	inode_size = (node.size0 << 32) | (node.size1);

	int inode_blocks = (inode_size / BLOCK_SIZE) + (inode_size % BLOCK_SIZE != 0);
	int x = 0;
	int writeFlag = 0;		// flag - file written to system
	dir_type temp[DIR_SIZE];
	for (i = 0; i < inode_blocks; i++) {
		block = node.addr[i];                                       // returns the first block stored in the addr of the inode
		lseek(file_descriptor, (block * BLOCK_SIZE), SEEK_SET);      // finding the address of the block of the inode
		read(file_descriptor, temp, BLOCK_SIZE);              // read the contents of the blocks of addr into buffer data structure
		for (j = 0; j < DIR_SIZE; j++) {
			directory[x] = temp[j];
			x++;
		}
	}
	// checks through each directory entry
	for (i = 0; i < (inode_size / DIR_SIZE); i++) {

		if ((strcmp(sourceFile, directory[i].filename) == 0) && (directory[i].inode > 0)) {    // if the file to be copied to external file has been found

			inode_type fileInode = getInode(directory[i].inode);

			if (fileInode.flags == (INODE_ALLOC | PLAIN_FILE)) {                   // if the inode is allocated & regular file
		   // calculates the size of file
				unsigned long long fileInode_size = (fileInode.size0 << 32) | (fileInode.size1);

				// iterates through every block in the addr and stores the content in buffer
				lseek(destination, 0, SEEK_SET);

				for (j = 0; j < (fileInode_size / BLOCK_SIZE); j++) {

					block = fileInode.addr[j];
					lseek(file_descriptor, (block * BLOCK_SIZE), SEEK_SET);
					read(file_descriptor, buffer, BLOCK_SIZE);
					write(destination, buffer, BLOCK_SIZE);
				}

				// stores the offset of the last block into the buffer

				if (fileInode_size % BLOCK_SIZE != 0) {
					block = fileInode.addr[j];
					lseek(file_descriptor, (block * BLOCK_SIZE), SEEK_SET);
					read(file_descriptor, buffer, fileInode_size % BLOCK_SIZE);
					write(destination, buffer, fileInode_size % BLOCK_SIZE);
				}
				node.actime = time(NULL);

				// writing the updated inode into the block at it's correct position
				int totalBytes = (2 * BLOCK_SIZE) + (INODE_SIZE * (currentInode - 1));
				lseek(file_descriptor, totalBytes, SEEK_SET);
				read(file_descriptor, node, INODE_SIZE);
				writeFlag = 1;						//file written to system
				printf("File written to system\n");
			}
			else {                                               // if the inode is unallocated, there is no file present
				printf("Error: File not found\n");
				return;
			}
			
		}
		}
	if (writeFlag == 0) {
		printf("Error: File Transfer Failed\n");
		return;
	}


}

void rm(char* filename) {

	int i, j;
	unsigned long long directory_size;
	dir_type directory[200];

	//check if file is opened
	int check = fileSystemCheck();
	if (check == -1)  //issue with file system
	{
		return;
	}
	// get the current inode
	inode_type currentDirectoryInode = getInode(currentInode);

	// total size of the current directory
	directory_size = (currentDirectoryInode.size0 << 32) | (currentDirectoryInode.size1);
	int inode_blocks = (directory_size / BLOCK_SIZE) + (directory_size % BLOCK_SIZE != 0);
	int x = 0;
	dir_type temp[DIR_SIZE];
	int removeFlag = 0; // file has been removed
	for (x = 0; x < inode_blocks; x++) {
		int block = currentDirectoryInode.addr[x];                                       // returns the first block stored in the addr of the inode
		lseek(file_descriptor, (block * BLOCK_SIZE), SEEK_SET);      // finding the address of the block of the inode
		read(file_descriptor, directory, BLOCK_SIZE);              // read the contents of the blocks of addr into buffer data structure


		 // checks through each directory entry
		for (i = 0; i < DIR_SIZE; i++) {


			if ((strcmp(filename, directory[i].filename) == 0) && (directory[i].inode > 0)) {      // if the file to be deleted has been found

				inode_type fileInode = getInode(directory[i].inode);
				unsigned int* fileBlocks = fileInode.addr;          // storing the block numbers of the inode of the filename to be copied

				if (fileInode.flags ==	(INODE_ALLOC | PLAIN_FILE)) {                     // if the inode is allocated & regular file
				   // calculates the size of file
					unsigned long long fileInode_size = (fileInode.size0 << 32) | (fileInode.size1);

					// iterates through every block in the addr and add the blocks to the free array
					for (j = 0; j < (fileInode_size / BLOCK_SIZE); j++) {
						int block = fileInode.addr[j];

						// add the block number to the free array
						addFreeBlock(block);
					}
					if ((fileInode_size % BLOCK_SIZE) > 0) {
						int block = fileInode.addr[j];

						// add the block number to the free array
						addFreeBlock(block);
					}
					inode_type file_inode = getInode(directory[i].inode);  //get inode of file 
					int file_inode_number = directory[i].inode;

					directory[i].inode = 0;                // this directory entry no longer exists so making inode 0
					strcpy(directory[i].filename, "");		// remove file entry 


					file_inode.flags = INODE_FREE;
					file_inode.modtime = time(NULL);                      // updating the modifying time
					file_inode.actime = time(NULL);                       // updating the accessing time
					file_inode.nlinks = 0;
					directory_size -= DIR_SIZE;

					currentDirectoryInode.size0 = (int)((directory_size & 0xFFFFFFFF00000000) >> 32);        // high 64 bit
					currentDirectoryInode.size1 = (int)(directory_size & 0x00000000FFFFFFFF);                 // low 64 bit

				  // writing the updated directory into the block at it's correct position
					lseek(file_descriptor, (BLOCK_SIZE * currentDirectoryInode.addr[x]), SEEK_SET);
					write(file_descriptor, directory, BLOCK_SIZE);

					// writing the updated file inode into the block at it's correct position
					int inode_block = (file_inode_number * INODE_SIZE) / BLOCK_SIZE;
					int offset = ((file_inode_number - 1) * INODE_SIZE) % BLOCK_SIZE;
					lseek(file_descriptor, (inode_block * BLOCK_SIZE) + offset, SEEK_SET);
					write(file_descriptor, &file_inode, INODE_SIZE);
					removeFlag = 1; 
					printf("File removed\n"); 
					return;
				}
				else {
					printf("Error: File not found\n");
					return;
				}

			}

		}

	}

	if (removeFlag == 0) {
		printf("Error: File not found\n");
		return;
	}

}


int addNewFileDirectoryEntry(int parentInodeNum, dir_type newDir) {
	inode_type parentNode;

	//get file inode struct
	int totalBytes = (2 * BLOCK_SIZE) + ((parentInodeNum - 1) * INODE_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	read(file_descriptor, &parentNode, INODE_SIZE);

	int check = findDirectory(newDir.filename, parentInodeNum); 	//check if file is in directory
	if (check > 0) {
		printf("Error: File is already in directory\n");
		return -1;
	}
	if (check == -1) {
		printf("Error: Not accessing directory file\n");
		return -1;
	}


	// go to size and see what is size
	unsigned long long size = ((parentNode.size0 << 32) | parentNode.size1); //find directory size

	// go to addr[] and get blocks
	dir_type buffer[DIR_SIZE];
	int totalBlocks = size / BLOCK_SIZE; // total blocks in file
	int dirNum = -1;
	int writeflag = 0;		//flag - file entry added
	while (totalBlocks > -1 && writeflag == 0) {

		//load block to temp buffer
		totalBytes = parentNode.addr[totalBlocks] * BLOCK_SIZE;
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, buffer, BLOCK_SIZE);

		// search through inodes to find directory value
		int i = 0;
		for (i = 0; i < DIR_SIZE; i++) {
			if (buffer[i].inode == 0) {			//find empty directory location and put in new entry
				dirNum = i;					//location in directory

				totalBytes = (parentNode.addr[totalBlocks] * BLOCK_SIZE) + (dirNum * DIR_SIZE);
				lseek(file_descriptor, totalBytes, SEEK_SET);
				write(file_descriptor, &newDir, DIR_SIZE);	
				writeflag = 1;
				break;
			}
		}
		if (dirNum < 0) {//go to next block to find empty

			totalBlocks--;
		}
		else { break; }
	}

	if (writeflag == 0) {
		//if directory is full find a new block
		//need to add new block to inode
		int freeBlock = getFreeBlock();
		if (freeBlock == -1) {// System was full
			printf("Error: No Free Blocks Available. Directory not created. \n");
			return -1;
		}
		int nextBlock = size / BLOCK_SIZE;				//find how many blocks to write to 
		if (nextBlock < 7) {							// for small file
			parentNode.addr[nextBlock + 1] = freeBlock;
			totalBytes = parentNode.addr[freeBlock] * BLOCK_SIZE;
			lseek(file_descriptor, totalBytes, SEEK_SET);
			write(file_descriptor, &newDir, DIR_SIZE);


			totalBlocks = totalBlocks + 1;


			writeflag = 1;
		}
		else { //need to convert to large file

		}

	}

	// update inode entry
	updateInodeEntry(DIR_SIZE, parentInodeNum, parentNode);

	return 1;			//update success

}

void mkdirv6(char* dir_name) {

	inode_type v6dir;
	dir_type v6direc[16];
	dir_type newFile;
	int freeBlock;
	int totalBytes;
	int writeBytes;
	int parentInode = currentInode;//start from current Inode
	char* root = "/";
	char directoryPath[100] = { 0 };
	char* newFolder;
	int fp = 0;
	int newEntry = 0; //new entry needed flag

	//check if file is opened
	int check = fileSystemCheck();
	if (check == -1)  //issue with file system
	{
		return;
	}


	//check to see if started with absolute path

	if (*dir_name != '/') {
		printf("Error: Incorrect input, must be a folder\n");
		return;
	}

	//check if directory is already there

	char* fullfilename = strtok(dir_name, "\n");

	if ((strcmp(fullfilename, root)) == 0) { 	//check for root node
		printf("No folder created\n");
		return;
	}

	
	char* folder = strtok(dir_name, "/\n"); //go through file path and find new location
	while (folder != NULL) {
		strcat(directoryPath, root);
		fp = findDirectory(folder, parentInode);
		if (fp == 0) {
			newEntry = 1;
			strcat(directoryPath, folder);  //add file to pathname
			newFolder = folder;
			folder = strtok(NULL, "/\n");
			if (folder != NULL) {
				printf("Error: Parent Folder Does Not Exist\n"); 
				return; 
			}
			else {
				break;
			}
		}
		else if (fp == -1) {
			printf("Error: File is not a directory file\n");
			break;
		}
		else {
			parentInode = fp;				//update parent inode value
			strcat(directoryPath, folder);  //add file to pathname
			folder = strtok(NULL, "/\n");

		}
	}
	
	if (parentInode > 0 && newEntry == 0) {		//entry already created
		printf("Error: Directory already created\n");
		return;
	}

	/*** only if new directory entry **/
	//if not there, make a new directory entry
	freeBlock = getFreeBlock();  //get a new free block
	if (freeBlock == -1) {// System was full
		printf("Error: No Free Blocks Available. Directory not created.\n");
		return;
	}

	int freeInode = getFreeInode();  // get new inode
	if (freeInode == -1) {
		printf("Error: No Inodes available. \n");
		return;
	}

	//write new folder entry directory file
	v6direc[0].inode = parentInode;  //parent directory
	strcpy(v6direc[0].filename, ".");

	v6direc[1].inode = 1;//root directory
	strcpy(v6direc[1].filename, "..");

	int i;
	for (i = 2; i < 16; i++) {//set rest of directory to free
		v6direc[i].inode = 0;
	}

	//write directory file to data block
	totalBytes = (freeBlock * BLOCK_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	write(file_descriptor, v6direc, BLOCK_SIZE);

	/* Update Inode Block with new Inode*/
	// inode struct
	v6dir.flags = (INODE_ALLOC | DIREC_FILE); // I-node allocated + directory file
	v6dir.nlinks = 1;
	v6dir.uid = 0;
	v6dir.gid = 0;
	v6dir.addr[0] = freeBlock;
	v6dir.actime = time(NULL);
	v6dir.modtime = time(NULL);

	// directory size
	writeBytes = DIR_SIZE * 2;
	v6dir.size0 = (int)((writeBytes & 0xFFFFFFFF00000000) >> 32); // high 32 bit
	v6dir.size1 = (int)(writeBytes & 0x00000000FFFFFFFF); // low 32 bit

	//Create new inode entry
	updateInodeEntry(0, freeInode, v6dir);

	/* go to parent directory to write parent directory entry */
	//directory entry 
	newFile.inode = freeInode;
	strcpy(newFile.filename, newFolder);

	//find next free inode spot in root directory at parent inode
	check = addNewFileDirectoryEntry(parentInode, newFile);
	if (check < 0) {//error occurred
		return;
	}

	printf("Directory Created: %s\n", directoryPath);
	return;
}


void changeDirectoryV6(char* dir_name) {
	// search through directory entries and then change current inode to last directory entry value

	int parentInode = currentInode; 		//start from root directory
	char filename[100] = { 0 };

	char* root = "/";
	char* parentFolder = "."; 
	char* rootFolder = ".."; 

	char* childFolder;
	int filelen = 0;
	int pathlen = 0;

	//check if file is opened
	int check = fileSystemCheck();
	if (check == -1)  //issue with file system
	{
		return;
	}


	//check if directory is  there
	int fp = 0;
	char* fullfilename = strtok(dir_name, "\n");
	int fileNotFound = 0; //flag - path is not available

	if ((strcmp(fullfilename, root)) == 0) {  //check current folder
		printf("Error: No Folder Specified\n"); 
		parentInode = currentInode;
	}
	else {
		char* folder = strtok(dir_name, "/\n");  //find other directory
		if (strcmp(folder, parentFolder) == 0) {	// go back to parent folder
			fp = findDirectory(folder, parentInode);
			parentInode = fp; 
			childFolder = strrchr(currentFilepath, '/'); //remove last folder from filepath
			filelen = strlen(childFolder);
			pathlen = strlen(currentFilepath); 
			pathlen = pathlen - filelen; 
			memcpy(filename, currentFilepath, pathlen);	 //new path name

		}
		else if ((strcmp(folder, rootFolder) == 0)) { //go to root
			fp = findDirectory(folder, parentInode);
			parentInode = fp;
			
		}
		else {									//find next directory folder 
			if ((*dir_name != '/')) {			//check to see if started with absolute path
				printf("Error: Incorrect input, must be a folder\n");
				return;
			}

			while (folder != NULL) {		
				strcat(filename, root);
				fp = findDirectory(folder, parentInode);
				
				if (fp == 0) {					//folder not in directory
					fileNotFound = 1;
					break;
				}
				else if (fp == -1) {			//file is another type of file
					printf("Error: File is not a directory file\n");
				}
				else {
					parentInode = fp;			//go to next path value
					strcat(filename, folder);
					folder = strtok(NULL, "/\n");

				}
			}
		}
	}

	

	if (parentInode > 0 && fileNotFound == 0) {

		inode_type newNode;
		int totalBytes = (2 * BLOCK_SIZE) + ((parentInode - 1) * INODE_SIZE);
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, &newNode, INODE_SIZE);

		// check to see if inode is a directory file
		if ((newNode.flags & DIREC_FILE) != DIREC_FILE) { //not a directory file
			printf("Error: File is not a directory file\n");
			return;
		}

		printf("Current Directory: ");
		if (parentInode == 1) {						//specify root folder
			strcpy(currentFilepath, root); 
		}
		else if(filelen>0){							//specify new file path
			strcpy(currentFilepath, filename); 
		}
		else {
			pathlen = strlen(currentFilepath);  //add new folders to filepath
			if (pathlen == 1) {
				strcpy(currentFilepath, filename);  //file start from root
			}
			else {
				strcat(currentFilepath, filename); 
			}
			
			
		}
		printf("%s\n", currentFilepath);
		currentInode = parentInode;//change current inode to new inode
	}
	else {
		printf("Error: Directory not found\n");
	}

	return;
}




void allocateBlocks(void) {
	unsigned int blockIdx = superBlock.isize + 2;//isize + superBlock + Block0
	int unallocatedBlocks = superBlock.fsize - blockIdx;//total data blocks
	unsigned int nextFree[252] = { 0 };//next free array
	unsigned int nextnFree = 0;// nfree value
	unsigned int storageBlockIdx = 0;
	unsigned int nextStorageBlockIdx = 0;
	unsigned int totalBytes = 0;
	unsigned int i;


	if (unallocatedBlocks < FREE_ARRAY_SIZE) {//number of blocks < free array size
		nextnFree = unallocatedBlocks;
		for (i = 0; i < unallocatedBlocks; i++) {//write to free array
			superBlock.free[i] = blockIdx;
			blockIdx += 1;
		}
		superBlock.nfree = nextnFree;

		storageBlockIdx = superBlock.free[0];


		//write last storage block
		nextFree[2] = storageBlockIdx;
		nextFree[1] = 0;
		nextFree[0] = 0;
		nextnFree = 2;
		totalBytes = (storageBlockIdx * BLOCK_SIZE); //calculate block number
		lseek(file_descriptor, totalBytes, SEEK_SET);
		write(file_descriptor, &nextnFree, sizeof(int));//store nfree value in indirect block
		write(file_descriptor, nextFree, sizeof(int) * FREE_ARRAY_SIZE);
		return;
	}
	else {//number of blocks < free array size
		nextnFree = MAX_NFREE;
		//write to free array to superblock
		for (i = 0; i < FREE_ARRAY_SIZE; i++) {
			superBlock.free[i] = blockIdx;
			blockIdx += 1;
		}

		superBlock.nfree = nextnFree;//set nfree
		unallocatedBlocks = unallocatedBlocks - FREE_ARRAY_SIZE;//find number of unallocated blocks
		storageBlockIdx = superBlock.free[0];//next storage block value

		while (blockIdx < superBlock.fsize) {


			if (unallocatedBlocks > FREE_ARRAY_SIZE) {//free blocks > 251
				nextFree[MAX_NFREE] = storageBlockIdx;

				for (i = 0; i < MAX_NFREE; i++) {//create array of free blocks
					if (i == 0) {
						nextFree[i] = blockIdx;
						nextStorageBlockIdx = blockIdx;//store next storage block value
					}
					else {
						nextFree[i] = blockIdx;
					}
					blockIdx += 1;
				}
				nextnFree = MAX_NFREE;// set new nfree value

				//write to storage block
				totalBytes = (storageBlockIdx * BLOCK_SIZE); //calculate block number
				lseek(file_descriptor, totalBytes, SEEK_SET);//find block
				write(file_descriptor, &nextnFree, sizeof(int));//store nfree value in indirect block
				write(file_descriptor, nextFree, (FREE_ARRAY_SIZE) * sizeof(int)); //store free blocks in indirect block

				//update unallocated blocks
				storageBlockIdx = nextStorageBlockIdx;//keep track of next storage block
				unallocatedBlocks = unallocatedBlocks - MAX_NFREE; //find number of blocks to allocate
			}
			else {
				nextFree[unallocatedBlocks] = storageBlockIdx;//free blocks < 251
				for (i = 0; i < unallocatedBlocks; i++) {//create array of free blocks
					if (i == 0) {
						nextStorageBlockIdx = blockIdx;//store next storage block value
					}

					nextFree[i] = blockIdx;
					blockIdx += 1;
				}
				nextnFree = unallocatedBlocks;// set new nfree value
				//write to storage block
				totalBytes = (storageBlockIdx * BLOCK_SIZE); //calculate block number
				lseek(file_descriptor, totalBytes, SEEK_SET);
				write(file_descriptor, &nextnFree, sizeof(int));//store nfree value in indirect block
				write(file_descriptor, nextFree, (unallocatedBlocks + 1) * sizeof(int)); //store free blocks in indirect block + indirect block #

				//last storage block
				storageBlockIdx = nextStorageBlockIdx;
			}

		}

		//write last storage block
		nextFree[2] = storageBlockIdx;
		nextFree[1] = 0;
		nextFree[0] = 0;
		nextnFree = 2;
		totalBytes = (storageBlockIdx * BLOCK_SIZE); //calculate block number
		lseek(file_descriptor, totalBytes, SEEK_SET);
		write(file_descriptor, &nextnFree, sizeof(int));//store nfree value in indirect block
		write(file_descriptor, nextFree, sizeof(int) * FREE_ARRAY_SIZE);

		return;
	}

}


int getFreeBlock(void) {
	superBlock.nfree -= 1;//get next free block from free array

	if (superBlock.nfree > 0) {
		if (superBlock.free[superBlock.nfree] == 0) { //system is full, return error

			superBlock.nfree++;
			lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);//find superblock
			write(file_descriptor, &superBlock, BLOCK_SIZE);//update nfree in superblock
			return -1;
		}
		else {
			lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);//find superblock
			write(file_descriptor, &superBlock, BLOCK_SIZE);//update nfree in superblock
			return superBlock.free[superBlock.nfree];//get free block from free array

		}
	}
	else {//get new set of free blocks for free array
		int newBlock = superBlock.free[0];
		int totalBytes = (newBlock * BLOCK_SIZE);

		lseek(file_descriptor, totalBytes, SEEK_SET);//find block from free array
		read(file_descriptor, &superBlock.nfree, sizeof(int)); //read in nfree into super block
		read(file_descriptor, superBlock.free, (FREE_ARRAY_SIZE) * sizeof(int)); // read 251 bytes to free array

		//write free array to superBlock
		lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);//find superblock
		write(file_descriptor, &superBlock, BLOCK_SIZE);//update nfree in superblock
		return superBlock.free[superBlock.nfree];// get free block from free array
	}

}


void createRootDirectory(void) {
	inode_type rootDir;
	dir_type direc[16];
	int freeBlock;
	int totalBytes;
	unsigned long long writeBytes;
	int i;
	freeBlock = getFreeBlock();
	if (freeBlock == -1) {// System was full
		printf("Error: No Free Blocks Available\n");
		return;
	}

	direc[0].inode = 1;//1st inode = root directory
	strcpy(direc[0].filename, ".");

	direc[1].inode = 1;//1st node = parent
	strcpy(direc[1].filename, "..");



	for (i = 2; i < 16; i++) {//set rest of nodes to free
		direc[i].inode = 0;
	}


	// inode struct
	rootDir.flags = (INODE_ALLOC | DIREC_FILE); // I-node allocated + directory file
	rootDir.nlinks = 1;
	rootDir.uid = 0;
	rootDir.gid = 0;
	rootDir.addr[0] = freeBlock;
	rootDir.actime = time(NULL);
	rootDir.modtime = time(NULL);

	// directory size
	writeBytes = DIR_SIZE * 2;
	rootDir.size0 = (int)((writeBytes & 0xFFFFFFFF00000000) >> 32); // high 64 bit
	rootDir.size1 = (int)(writeBytes & 0x00000000FFFFFFFF); // low 64 bit

	//write inode to inode block
	totalBytes = (2 * BLOCK_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	write(file_descriptor, &rootDir, INODE_SIZE);

	//write root directory to data block
	totalBytes = (freeBlock * BLOCK_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	write(file_descriptor, direc, BLOCK_SIZE);


}


void initfs(int total_blocks, int total_inode_blocks) {
	int x;
	superBlock.isize = total_inode_blocks; // total number of blocks for inode
	superBlock.fsize = total_blocks;// total blocks for system

	if (file_descriptor < 2)  //verify if a file is opened
	{
		printf("Error: File is not opened \n");
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

	// find number of inodes in system
	int num_inodes = (BLOCK_SIZE / INODE_SIZE) * superBlock.isize;
	int totalBytes;
	// set other inodes to free
	totalBytes = (2 * BLOCK_SIZE) + INODE_SIZE; //start from INODE 2
	lseek(file_descriptor, totalBytes, SEEK_SET);

	for (x = 2; x <= num_inodes; x++) {
		inode_type nodeX;
		nodeX.flags = INODE_FREE; // set inodes to free
		write(file_descriptor, &nodeX, INODE_SIZE);//write inode to block
	}

	return;
}

void quit(void) {
	if (superBlock.fsize > 0 && superBlock.isize > 0) {
		superBlock.time = time(NULL);//update super block access time
		lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);
		write(file_descriptor, &superBlock, BLOCK_SIZE);
	}

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
			args[1] = strtok(NULL, " ");// file system size in # blocks
			args[2] = strtok(NULL, "\n");// # nodes for i-nodes

			if ((args[1] != NULL) && (args[2] != NULL)) {
				char arg1[10];
				char arg2[10];
				int i;
				strcpy(arg1, args[1]);
				strcpy(arg2, args[2]);
				int arg1Len = strlen(arg1);
				for (i = 0; i < arg1Len; i++) { //checking all numeric inputs
					if (isdigit(arg1[i]) == 0) {//input is not numeric
						printf("Input is not numeric\n");
						return;
					}
				}
				int arg2Len = strlen(arg2);
				for (i = 0; i < arg2Len; i++) { //checking all numeric inputs
					if (isdigit(arg2[i]) == 0) {//input is not numeric
						printf("Input is not numeric\n");
						return;
					}
				}

				int n1 = atoi(args[1]);
				int n2 = atoi(args[2]);
				if (n1 <= 0 || n2 <= 0) {// no blocks or inode blocks to initialize
					printf("Invalid size\n");
					return;
				}

				if ((n1 < (n2 + 2))) {//number of blocks system size < inode blocks
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
		else if ((strcmp(args[0], "cpin") == 0)) {
			args[1] = strtok(NULL, " ");// system external file
			args[2] = strtok(NULL, "\n");// file system v6 file
			if (args[2] == NULL || args[1] == NULL) {
				printf("Invalid Command\n");
			}
			else {
			int check = cpin(args[1], args[2]);
			if (check > 0) {
				printf("File transfer success\n");
			}
			else {
				printf("File transfer failed\n");
			}
		 }
		}
		else if ((strcmp(args[0], "cpout") == 0)) {
			args[1] = strtok(NULL, " ");// system external file
			args[2] = strtok(NULL, "\n");// file system v6 file
			
			if (args[2] == NULL || args[1] == NULL) {
				printf("Invalid Command\n"); 
			}
			else {
				cpout(args[1], args[2]);
			}
			
		}
		else if ((strcmp(args[0], "rm") == 0)) {
			args[1] = strtok(NULL, "\n");// file system v6 file
			if (args[1] == NULL) {
				printf("Invalid Command\n");
			}
			else {
				rm(args[1]);
			}
		}
		else if ((strcmp(args[0], "mkdir") == 0)) {
			args[1] = strtok(NULL, "");// file system v6 directory
			char* enter = "\n";
			if (args[1] == NULL || (strcmp(args[1], enter) == 0)) {
				printf("Invalid Command\n");
			}
			else {
				mkdirv6(args[1]);
			}
		}
		else if ((strcmp(args[0], "cd") == 0)) {
			args[1] = strtok(NULL, "");// # nodes for i-nodes
			char* enter = "\n"; 
			if (args[1] == NULL || (strcmp(args[1], enter)==0)) {
				printf("Invalid Command\n");
			}
			else {
				changeDirectoryV6(args[1]);
			}
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
	file_descriptor = -1;
	currentInode = 1; // start at the root directory;

	while (1) {
		printf("Enter command:");
		fgets(command, sizeof(command), stdin); //get command from console
		modv6cmds(command);//find modv6 command

	}

	return 0;

}

