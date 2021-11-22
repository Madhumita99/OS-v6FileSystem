#include "mod-v6.h"

inode_type getInode(unsigned int inodeNumber){
    
    inode_type inode;

    int totalBytes = (2 * BLOCK_SIZE) + (INODE_SIZE*(inodeNumber-1));    // calculates the where the inode is present
    lseek(file_descriptor, totalBytes, SEEK_SET);
    read(file_descriptor, &inode, INODE_SIZE);                  // reads the inode values into inode variable
    return inode;
}

void addFreeBlock (unsigned int block){

    if (superBlock.nfree == MAX_NFREE){                   // if the free array is full, store the free blocks in a new block
        lseek(file_descriptor, BLOCK_SIZE*block, SEEK_SET);
        write(file_descriptor, superBlock.nfree, sizeof(int));  
        write(file_descriptor, superBlock.free, (FREE_ARRAY_SIZE) * sizeof(int));
        superBlock.nfree = 0;
    }
    superBlock.free[superBlock.nfree] = block;                  // updating the free array with the new block
    superBlock.nfree++;
    lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);					//find superblock
    write(file_descriptor, &superBlock, BLOCK_SIZE);			//update nfree in superblock
}

void cpout(char* sourceFile, char* destinationFile) {
    
    inode_type node;
    int i, j; 
    unsigned int block;
    unsigned long inode_size;
    char buffer[BLOCK_SIZE] = {0};                              // temperoray storage to read the contents of block into
    dir_type directory[100];
    
    //check source file is open
    int check = fileSystemCheck();
    if (check == -1)  //issue with file system
    {
        return;
    }

    // external file is opened 
	int destination = open(destinationFile, O_RDWR | O_CREAT, 0600); /* Question - is it supposed to create a new file or overwrite old files? */
	if (destination == -1) {
		printf("File error\n");
		return;
	}

    //check if source file is in directory
    check = findDirectory(sourceFile, currentInode); 
    if (check == 0) {
        printf("File is not in directory\n");
        return;
    }
   
    // gets the inode of the current directory
	node = getInode(currentInode);
	
    // total size of the current directory
	inode_size = (node.size0 << 32) | (node.size1 );


    /** question - this is assuming directory  is continuous blocks **/
    block = node.addr[0];                                       // returns the first block stored in the addr of the inode
	lseek (file_descriptor, (block*BLOCK_SIZE), SEEK_SET);      // finding the address of the block of the inode
	read (file_descriptor, directory, inode_size);              // read the contents of the blocks of addr into buffer data structure
	
    // checks through each directory entry 
	for (i = 0; i < (inode_size / DIR_SIZE); i++){
	    
	    if ((strcmp(sourceFile, directory[i].filename) == 0) && (directory[i].inode>0)){    // if the file to be copied to external file has been found

	        inode_type fileInode = getInode(directory[i].inode);
                  // if the inode is allocated

	       	        
	        if(fileInode.flags == (INODE_ALLOC | PLAIN_FILE)){                   // if the inode is allocated & regular file
	            
                // calculates the size of file 
	            unsigned long fileInode_size = (fileInode.size0 << 32) | (fileInode.size1);

                // iterates through every block in the addr and stores the content in buffer
                lseek(destination, 0, SEEK_SET);

	            for (j =0; j < fileInode_size/BLOCK_SIZE; j++){

	                block = fileInode.addr[j];
	                lseek(file_descriptor, (block*BLOCK_SIZE), SEEK_SET);
	                read(file_descriptor, buffer, BLOCK_SIZE);
	                write(destination, buffer,BLOCK_SIZE);
	            }

                // stores the offset of the last block into the buffer
	            block = fileInode.addr[j];
	            lseek(file_descriptor, (block*BLOCK_SIZE), SEEK_SET);
	            read(file_descriptor, buffer, fileInode_size %BLOCK_SIZE);
	            write(destination, buffer,fileInode_size % BLOCK_SIZE);

                node.actime = time(NULL);

                // writing the updated inode into the block at it's correct position 

	            
                int totalBytes = (2*BLOCK_SIZE) + (INODE_SIZE * (currentInode - 1));
                lseek(file_descriptor, totalBytes, SEEK_SET);
                read(file_descriptor, node, INODE_SIZE); 

                printf("File written to system\n"); 
	        }
	        else{                                               // if the inode is unallocated, there is no file present
                printf("Error: File not found\n");
            }
	        return;
	    }
	}
}

void rm(char* filename){

    int i,j;
    unsigned long directory_size;
    dir_type directory[200];
    
    //check if file is opened
    int check = fileSystemCheck();
    if (check == -1)  //issue with file system
    {
        return;
    }

    // get the current inode
    inode_type node = getInode(currentInode);
   
    // total size of the current directory
    directory_size = (node.size0 << 32) | (node.size1);
       
    
    int block = node.addr[0];                                   // returns the first block stored in the addr of the inode
      
	
	lseek (file_descriptor, (block*BLOCK_SIZE), SEEK_SET);      // finding the address of the block of the inode

    /** question - this is assuming directory  is continuous blocks **/
	read (file_descriptor, directory, directory_size);              // read the contents of the blocks of addr into buffer data structure
	
    // checks through each directory entry
	for (i =0; i< (directory_size/DIR_SIZE); i++){
        

	    if ((strcmp(filename, directory[i].filename) == 0) && (directory[i].inode > 0)){      // if the file to be deleted has been found
	        
	        inode_type fileInode = getInode(directory[i].inode);
	        unsigned int* fileBlocks = fileInode.addr;          // storing the block numbers of the inode of the filename to be copied
	        
	        if (fileInode.flags == (INODE_ALLOC|PLAIN_FILE)){                     // if the inode is allocated & regular file
	            // calculates the size of file 
	            unsigned long fileInode_size = (fileInode.size0 << 32) | (fileInode.size1);
	            
	            // iterates through every block in the addr and add the blocks to the free array


	            for (j =0; j < (fileInode_size/BLOCK_SIZE); j++){
	                block = fileInode.addr[j];
	                
                    // add the block number to the free array
                    addFreeBlock(block);
	            }
	            if ((fileInode_size%BLOCK_SIZE) > 0){
                    block = fileInode.addr[j];
                   
                    // add the block number to the free array
                    addFreeBlock(block);
                }

                directory[i].inode = 0;				// this directory entry no longer exists so making inode 0
                node.flags = INODE_FREE;
                node.modtime = time(NULL);                      // updating the modifying time
                node.actime = time(NULL);                       // updating the accessing time
                node.nlinks = 0; 
                

                directory_size -= DIR_SIZE;

                node.size0 = (int)((directory_size & 0xFFFFFFFF00000000) >> 32);        // high 64 bit
                node.size1 = (int)(directory_size & 0x00000000FFFFFFFF);                 // low 64 bit

                // writing the updated directory into the block at it's correct position
                lseek(file_descriptor, (BLOCK_SIZE*node.addr[0]), SEEK_SET);
                write(file_descriptor, directory, directory_size);

                // writing the updated inode into the block at it's correct position 
                int inode_block = (currentInode*INODE_SIZE) / BLOCK_SIZE;
                int offset = (currentInode*INODE_SIZE) % BLOCK_SIZE;
                lseek(file_descriptor, (inode_block*BLOCK_SIZE) + offset, SEEK_SET);
                read(file_descriptor, node, INODE_SIZE);
	        }
            else {
                printf("Error: File not found\n");
            }
	        return;
	    }
	}

}
