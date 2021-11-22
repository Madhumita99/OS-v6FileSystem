#include "mod-v6.h"


/*
* Check to see if file system is initialized
* output: -1: system is not initialized
*		   1: file is open & system is 
*/
int fileSystemCheck(void) {

	if (file_descriptor < 2)  //verify if a file is opened
	{
		printf("Error: File is not opened \n");
		return -1; 
	}
	if (superBlock.fsize < 1 || superBlock.isize < 1) {
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
			return x;
		}
	}
	return -1; // did not find free inode

}

void updateInodeEntry(int addBytes, int inodeNum, inode_type newNode) {

	unsigned long   size = ((newNode.size0 << 32) | newNode.size1); //find directory size
	int totalBytes = (2 * BLOCK_SIZE) + ((inodeNum - 1) * INODE_SIZE);
	int writeBytes =  size + addBytes;
	newNode.size0 = (int)((writeBytes & 0xFFFFFFFF00000000) >> 32); 	// high 64 bit
	newNode.size1 = (int)(writeBytes & 0x00000000FFFFFFFF); 		// low 64 bit
	newNode.actime = time(NULL);
	newNode.modtime = time(NULL);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	write(file_descriptor, &newNode, INODE_SIZE);


	/* TEST */
	inode_type buffer[16];
	totalBytes = (2 * BLOCK_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	read(file_descriptor, buffer, BLOCK_SIZE);
	/* END TEST*/
}
void mkdirv6(char* dir_name) {

	inode_type v6dir;
	dir_type v6direc[16];
	dir_type newFile; 
	int freeBlock;
	int totalBytes;
	int writeBytes;
	int parentInode = 1;
	int fp = 0;
	char* folder = strtok(dir_name, "/\n");
	int newEntry = 0; //new entry needed fla

	/* Test */
	dir_type test[16];
	/* End Test*/
	//check if file is opened
	int check = fileSystemCheck();
	if (check == -1)  //issue with file system
	{
		return;
	}


	//check how many folders in filepath

	if (*dir_name != '/') {
		printf("Error: Incorrect input, must start at root\n");
		return;
	}


	//check if directory is already there

	while(folder != NULL){
		fp = findDirectory(folder, parentInode);
		if (fp < 0) {
			newEntry = 1; 
			break;
		}
		else if (fp == 0) {
			printf("Error: File is not a directory file\n");
			break; 
		}
		else {
			parentInode = fp;
			folder = strtok(NULL, "/\n");
			
		}

	}

	if (parentInode > 0 && newEntry == 0) {
		printf("Error: Directory already created\n");
		return;
	}

	//if not there, make a new directory entry
	//check to see if it is a parent directory 
	freeBlock = getFreeBlock();
	if (freeBlock == -1) {							// System was full
		printf("Error: No Free Blocks Available. Directory not created.\n");
		return;
	}

	int freeInode = getFreeInode();
	if (freeInode == -1) {
		printf("Error: No Inodes available. \n");
		return;
	}

	newFile.inode = freeInode; 
	strcpy(&newFile.filename, folder);

	/*** only if new directory entry **/
	//find the parent inode if it is not the parent directory
	v6direc[0].inode = parentInode;							//root directory
	strcpy(v6direc[0].filename, ".");

	v6direc[1].inode = 1;							//root directory
	strcpy(v6direc[1].filename, "..");

	for (int i = 2; i < 16; i++) {				//set rest of nodes to free
		v6direc[i].inode = 0;
	}

	//write directory to data block
	totalBytes = (freeBlock * BLOCK_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	write(file_descriptor, v6direc, BLOCK_SIZE);



	/*** go to parent directory to write parent directory entry ***/
	//find next free inode spot in root directory at parent inode
	check = addNewFileDirectoryEntry(v6direc[0].inode, newFile);
	if (check < 0) {		//error occurred
		return; 
	}
	

	/* Update Inode Struct*/

	// inode struct
	v6dir.flags = (INODE_ALLOC | DIREC_FILE); 		// I-node allocated + directory file 
	v6dir.nlinks = 1;
	v6dir.uid = 0;
	v6dir.gid = 0;
	v6dir.addr[0] = freeBlock;
	v6dir.actime = time(NULL);
	v6dir.modtime = time(NULL);

	// directory size
	writeBytes = DIR_SIZE * 2;
	v6dir.size0 = (int)((writeBytes & 0xFFFFFFFF00000000) >> 32); 	// high 32 bit
	v6dir.size1 = (int)(writeBytes & 0x00000000FFFFFFFF); 		// low 32 bit

	//write inode to inode block
	updateInodeEntry(0, freeInode, v6dir);



	return;
}

/*
* Description: Add new file or folder entry to directory
* Input:
*	- dir_name = folder/file name
*	- parentNode = starting Inode
*
* Output:
*	- fpFound = file inode number
*
*/

int addNewFileDirectoryEntry(int parentInodeNum, dir_type newDir) {
	inode_type parentNode;

	//get file inode struct
	int totalBytes = (2 * BLOCK_SIZE) + ((parentInodeNum - 1) * INODE_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	read(file_descriptor, &parentNode, INODE_SIZE);


	int check = findDirectory(newDir.filename, parentInodeNum); 
	if (check > 0) {
		printf("Error: File is already in directory\n"); 
		return -1;
	}
	if (check == -1) {
		printf("Error: Not accessing directory file\n");
		return -1; 
	}
	

	// go to size and see what is size
	unsigned long   size = ((parentNode.size0 << 32) | parentNode.size1); //find directory size

	// go to addr[] and get blocks
	int readBlocks[8];
	dir_type buffer[DIR_SIZE];
	int totalBlocks = size / BLOCK_SIZE; // total blocks in file
	int dirNum = -1;
	int writeflag = 0; 
	while (totalBlocks > -1 && writeflag == 0) {

		//load block to temp buffer
		totalBytes = parentNode.addr[totalBlocks] * BLOCK_SIZE;
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, buffer, BLOCK_SIZE);

		// search through inodes to find directory value
		for (int i = 0; i < DIR_SIZE; i++) {
			if (buffer[i].inode == 0) {
				dirNum = i;

				totalBytes = (parentNode.addr[totalBlocks] * BLOCK_SIZE) + (dirNum * DIR_SIZE);
				lseek(file_descriptor, totalBytes, SEEK_SET);
				write(file_descriptor, &newDir, DIR_SIZE);
				writeflag = 1; 
				break;
			}
		}

		

		if (dirNum < 0) {			//go to next block to find empty

			totalBlocks--;
			
		}
		else {
			
			break;
		}

	}
	if (writeflag == 0) {
		//if directory is full find a new block
		//need to add new block to inode
		int freeBlock = getFreeBlock();
		if (freeBlock == -1) {							// System was full
			printf("Error: No Free Blocks Available. Directory not created. \n");
			return -1;
		}

		int nextBlock = size / BLOCK_SIZE; 

		if (nextBlock < 7) {
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
	/* Test */
	totalBytes = parentNode.addr[totalBlocks] * BLOCK_SIZE;
	lseek(file_descriptor, totalBytes, SEEK_SET);
	read(file_descriptor, buffer, BLOCK_SIZE);

	/* end test*/

	// update inode entry
	updateInodeEntry(DIR_SIZE, parentInodeNum, parentNode);

	return 1;

}

/*
*
* Change Directory
* Description: change currentInode (global) to input path file inode
* Input:  dir_name = file path
* Output: print out file path 
*
*/
void changeDirectoryV6(char* dir_name) {
    //TODO test
    int testCurrentInodeBefore = currentInode;

    // search through directory entries and then change current inode to last directory entry value

	int freeBlock;
	int totalBytes;
	int writeBytes;
	int parentInode = 1;

	char* newfile = '/'; 

	//check if file is opened
	int check = fileSystemCheck();
	if (check == -1)  //issue with file system
	{
		return;
	}


	//check how many folders in filepath

	if (*dir_name != '/') {
		printf("Error: Incorrect input, must start at root\n");
		return;
	}


	//check if directory is already there
	int fp = 0;
	char* folder = strtok(dir_name, "/\n");
	int fileNotFound = 0; //path is not available
	
	while (folder != NULL) {
		
		fp = findDirectory(folder, parentInode);
		if (fp == 0) {
			fileNotFound = 1;
			break;
		}
		else if (fp == -1) {
			printf("Error: File is not a directory file\n");
		}
		else {
			parentInode = fp;
			folder = strtok(NULL, "/\n");

		}

	}

	if (parentInode > 0 && fileNotFound == 0) {
		printf("Directory changed: ");
		printf("%s\n", dir_name);
		inode_type curNode;
		currentInode = parentInode;		//change current inode to new inode


		//find inode for path
		totalBytes = (2 * BLOCK_SIZE) + INODE_SIZE * (currentInode - 1);	//read inode from block
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, &curNode, INODE_SIZE);

		//find starting address block 
		totalBytes = curNode.addr[0] * BLOCK_SIZE; 
		lseek(file_descriptor, totalBytes, SEEK_SET);
	}
	else {
		printf("Error: Directory not found\n");
	}

    //TODO test
    int testCurrentInodeAfter = currentInode;

    return;
	
}


/*
* 
* Find Directory
* Description: Find if directory is already in file system
* Input:  dir_name = folder/file name, parentNode = starting Inode
* Output: fpFound = file inode number
* 
*/

int findDirectory(char* dir_name, int parentNode) {


	// find parent folder directory
	inode_type rootNode;
	int totalBytes = (2 * BLOCK_SIZE) + ((parentNode - 1) * INODE_SIZE);
	lseek(file_descriptor, totalBytes, SEEK_SET);
	read(file_descriptor, &rootNode, INODE_SIZE);

	// check to see if inode is a directory file 
	if ((rootNode.flags & DIREC_FILE) != DIREC_FILE) { //not a directory file
		printf("File is not a directory file\n"); 
		return 0;
	}

	// go to size and see what is size
	unsigned long size = ((rootNode.size0 << 32) | rootNode.size1); //find directory size

	// go to addr[]
	int readBlocks[8];
	dir_type buffer [DIR_SIZE];
	int totalBlocks = size / BLOCK_SIZE; // total blocks in file
	int fpFound = -1; // folder Inode 

	while (totalBlocks > -1) {

		//load block to temp buffer
		totalBytes = rootNode.addr[totalBlocks] * BLOCK_SIZE;
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, buffer, BLOCK_SIZE);

		//read through list to find directory value
		for (int i = 0; i < DIR_SIZE; i++) {
			if (strcmp(buffer[i].filename, dir_name) == 0) {
				fpFound = buffer[i].inode;
				return fpFound;
			}
		}

		totalBlocks--;
		// search through inodes to find directory value
	}

	return fpFound;


}
