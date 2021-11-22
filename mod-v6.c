/* Project 2 - Part 1
*  November 10, 2021
*
* Group 2
* Jessica Nguyen: main, modv6cmds, quit, flags & defines, & integrating modules
* Madhumita Ramesh: initfs, createRootDirectory, getFreeBlock
* Micah Steinbrecher: openfs, allocateBlocks
*
* How to compile:
* Open SSH connection to csgrads1.utdallas.edu UTD linux server
* Copy/Load mod-v6.c file to server directory
* Compile using the following command in the terminal: "gcc mod-v6.c -o modv6"
*
* How to run the program:
* In terminal, enter the following command to start the program: "./modv6"
* Program will prompt to "Enter command:"
* User can enter the following commands:
* 1. openfs X (X = file name)
* 2. initfs n1 n2 (n1 = total number of blocks, n2 = total blocks for inodes;  n1, n2 are numeric values)
* 3. q
*
* When user is done with program, can use "q" command to exit the program
*
*/

#include "mod-v6.h"

// global variables
superblock_type superBlock;
int file_descriptor;
char file_name[28];
int currentInode; 


void openfs(char* file_name) {
	file_descriptor = open(file_name, O_RDWR);		//open file given


	if (file_descriptor == -1) {				//file is not in system
	
		file_descriptor = open(file_name, O_CREAT | O_RDWR, 0600);	// create file with R&W
		printf("File created and opened\n");
	}
	else {
		printf("File opened\n");			//file found
		/* new */
		//read in superblock 
		lseek(file_descriptor, BLOCK_SIZE, SEEK_SET); 
		read(file_descriptor, &superBlock, BLOCK_SIZE); 

	}
	return;
}

void allocateBlocks(void) {
	unsigned int blockIdx = superBlock.isize + 2;			//isize + superBlock + Block0
	int unallocatedBlocks = superBlock.fsize - blockIdx;	//total data blocks
	unsigned int nextFree[252] = { 0 };				//next free array
	unsigned int nextnFree = 0;					// nfree value
	unsigned int storageBlockIdx = 0;
	unsigned int nextStorageBlockIdx = 0;
	unsigned int totalBytes = 0;
	unsigned int i;


	if (unallocatedBlocks < FREE_ARRAY_SIZE) {		//number of blocks < free array size
		nextnFree = unallocatedBlocks;
		for (i = 0; i < unallocatedBlocks; i++) {	//write to free array
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
		totalBytes = (storageBlockIdx * BLOCK_SIZE); 	//calculate block number
		lseek(file_descriptor, totalBytes, SEEK_SET);
		write(file_descriptor, &nextnFree, sizeof(int));	//store nfree value in indirect block
		write(file_descriptor, nextFree, sizeof(int)*FREE_ARRAY_SIZE);

		/*test */
		int testFree [10]; 
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, testFree, sizeof(int)*10); 
		/*end test*/
		return;
	}
	else {							//number of blocks < free array size
		nextnFree = MAX_NFREE;
		//write to free array to superblock
		for (i = 0; i < FREE_ARRAY_SIZE; i++) {

			superBlock.free[i] = blockIdx;
			blockIdx += 1;
		}

		superBlock.nfree = nextnFree;			//set nfree
		unallocatedBlocks = unallocatedBlocks - FREE_ARRAY_SIZE;	//find number of unallocated blocks
		storageBlockIdx = superBlock.free[0];		//next storage block value

		while (blockIdx < superBlock.fsize) {


			if (unallocatedBlocks > FREE_ARRAY_SIZE) {	//free blocks > 251
				nextFree[MAX_NFREE] = storageBlockIdx;

				for (i = 0; i < MAX_NFREE; i++) {		//create array of free blocks
					if (i == 0) {
						nextFree[i] = blockIdx;
						nextStorageBlockIdx = blockIdx;	//store next storage block value
					}
					else {
						nextFree[i] = blockIdx;
					}
					blockIdx += 1;

				}
				nextnFree = MAX_NFREE;	// set new nfree value

				//write to storage block
				totalBytes = (storageBlockIdx * BLOCK_SIZE); //calculate block number
				lseek(file_descriptor, totalBytes, SEEK_SET);		//find block
				write(file_descriptor, &nextnFree, sizeof(int));	//store nfree value in indirect block
				write(file_descriptor, nextFree, (FREE_ARRAY_SIZE) * sizeof(int)); //store free blocks in indirect block

				//update unallocated blocks
				storageBlockIdx = nextStorageBlockIdx;			//keep track of next storage block
				unallocatedBlocks = unallocatedBlocks - MAX_NFREE; //find number of blocks to allocate
			}
			else {
				nextFree[unallocatedBlocks] = storageBlockIdx;		//free blocks < 251
				for (i = 0; i < unallocatedBlocks; i++) {			//create array of free blocks
					if (i == 0) {
						nextStorageBlockIdx = blockIdx;	//store next storage block value
					}

					nextFree[i] = blockIdx;
					blockIdx += 1;
				}

				nextnFree = unallocatedBlocks;		// set new nfree value

				//write to storage block
				totalBytes = (storageBlockIdx * BLOCK_SIZE); 	//calculate block number
				lseek(file_descriptor, totalBytes, SEEK_SET);
				write(file_descriptor, &nextnFree, sizeof(int));	//store nfree value in indirect block
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
		totalBytes = (storageBlockIdx * BLOCK_SIZE); 	//calculate block number
		lseek(file_descriptor, totalBytes, SEEK_SET);
		write(file_descriptor, &nextnFree, sizeof(int));	//store nfree value in indirect block
		write(file_descriptor, nextFree, sizeof(int)* FREE_ARRAY_SIZE);

		/*test */
		int testFree[10];
		lseek(file_descriptor, totalBytes, SEEK_SET);
		read(file_descriptor, testFree, sizeof(int) * 10);
		printf("test"); 
		/*end test*/
		return;
	}

}

int getFreeBlock(void) {

	//superBlock.nfree -= 1;		//get next free block from free array
	/*TEST */
	superBlock.nfree = 0; 
	//END TEST

	if (superBlock.nfree > 0) {  
		if (superBlock.free[superBlock.nfree] == 0) { //system is full, return error 

			superBlock.nfree++; 
			lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);					//find superblock
			write(file_descriptor, &superBlock, BLOCK_SIZE);			//update nfree in superblock

			return -1;
		}
		else {
			lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);					//find superblock
			write(file_descriptor, &superBlock, BLOCK_SIZE);			//update nfree in superblock
		

			/* TEST */
			superblock_type test;
			lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);					//find superblock
			read(file_descriptor, &test, BLOCK_SIZE);
			printf("Block # %d\n", superBlock.free[superBlock.nfree]);
			/* END TEST */
			return superBlock.free[superBlock.nfree];	//get free block from free array

		}
	}
	else {						//get new set of free blocks for free array
		int newBlock = superBlock.free[0];
		int totalBytes = (newBlock * BLOCK_SIZE);

		lseek(file_descriptor, totalBytes, SEEK_SET);				//find block from free array
		read(file_descriptor, &superBlock.nfree, sizeof(int)); //read in nfree into super block
		read(file_descriptor, superBlock.free, (FREE_ARRAY_SIZE) * sizeof(int)); // read 251 bytes to free array

		//write free array to superBlock
		lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);					//find superblock
		write(file_descriptor, &superBlock, BLOCK_SIZE);			//update nfree in superblock

		return superBlock.free[superBlock.nfree];				// get free block from free array
	}


}

void createRootDirectory(void) {
	inode_type rootDir;
	dir_type direc[16]; /*** new **/
	int freeBlock;
	int totalBytes;
	unsigned long writeBytes; /** new **/

	freeBlock = getFreeBlock();
	if (freeBlock == -1) {							// System was full
		printf("No Free Blocks Available\n");
		return;
	}

	direc[0].inode = 1;							//1st inode = root directory
	strcpy(direc[0].filename, ".");

	direc[1].inode = 1;							//1st node = parent
	strcpy(direc[1].filename, "..");

	/* new */
	for (int i = 2; i < 16; i++) {				//set rest of nodes to free
		direc[i].inode = 0;
	}
	

	// inode struct
	rootDir.flags = (INODE_ALLOC | DIREC_FILE); 		// I-node allocated + directory file 
	rootDir.nlinks = 1;
	rootDir.uid = 0; 
	rootDir.gid = 0;
	rootDir.addr[0] = freeBlock;
	rootDir.actime = time(NULL);
	rootDir.modtime = time(NULL);

	// directory size
	writeBytes = DIR_SIZE * 2;
	rootDir.size0 = (int)((writeBytes & 0xFFFFFFFF00000000) >> 32); 	// high 64 bit
	rootDir.size1 = (int)(writeBytes & 0x00000000FFFFFFFF); 		// low 64 bit

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
	superBlock.isize = total_inode_blocks; 		// total number of blocks for inode
	superBlock.fsize = total_blocks;		// total blocks for system

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

		write(file_descriptor, &nodeX, INODE_SIZE);		//write inode to block
	}


	return;
}


void quit() {
	if (superBlock.fsize > 0 && superBlock.isize > 0 ) {
		superBlock.time = time(NULL);							//update super block access time
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

				if ((n1 < (n2 + 2))) {			//number of blocks system size < inode blocks 
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
			args[1] = strtok(NULL, " ");	// system external file
			args[2] = strtok(NULL, "\n");	// file system v6 file
			int check = cpin(args[1], args[2]);
			if (check > 0) {
				printf("File transfer success\n"); 
			}
			else {
				printf("File transfer failed\n"); 
			}
		}
		else if ((strcmp(args[0], "cpout") == 0)) {
			args[1] = strtok(NULL, " ");	// system external file
			args[2] = strtok(NULL, "\n");	// file system v6 file
			cpout(args[1], args[2]);
		}
		else if ((strcmp(args[0], "rm") == 0)) {
			args[1] = strtok(NULL, "\n");	// file system v6 file
			rm(args[1]);
		}
		else if ((strcmp(args[0], "mkdir") == 0)) {
			args[1] = strtok(NULL, "");	// file system v6 directory
			mkdirv6(args[1]); 
   		}
		else if ((strcmp(args[0], "cd") == 0)) {
			args[1] = strtok(NULL, "");	// # nodes for i-nodes
			changeDirectoryV6(args[1]);
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
		modv6cmds(command);			//find modv6 command

	}
	return 0;
}
