int currentInode;                                               // saves the inode of the current directory 

inode_type getInode(int inodeNumber){
    
    inode_type inode;
    int block = 2 + ((INODE_SIZE*inodeNumber) / BLOCK_SIZE);    // calculates the block number where the inode is present
    int offset = (INODE_SIZE*inodeNumber) % BLOCK_SIZE;         // calculates the offset within the block
    lseek(file_descriptor, (BLOCK_SIZE*block), SEEK_SET);
    read(file_descriptor, &inode, INODE_SIZE);                  // reads the inode values into inode variable
    return inode;
}

void addFreeBlock (int block){

    if (superBlock.nfree == FREE_ARRAY_SIZE){                   // if the free array is full, store the free blocks in a new block
        lseek(file_descriptor, BLOCK_SIZE*block, SEEK_SET);
        write(file_descriptor, superBlock.free, sizeof(superBlock.free));
        superBlock.nfree = 0;
    }
    superBlock.free[superBlock.nfree] = block;                  // updating the free array with the new block
    superBlock.nfree++;
}

void cpout(char* sourceFile, char* destinationFile) {
    
    inode_type node;
    int i, j, block;
    long long inode_size;
    char buffer[BLOCK_SIZE] = {0};                              // temperoray storage to read the contents of block into
    dir_type directory[100];

    // external file is opened 
	int destination = open(destinationFile, O_RDWR | O_CREAT, 0600); 
	if (destination == -1) {
		printf("File is not opened/n");
		return;
	}
    
    // gets the inode of the current directory
	node = getInode(currentInode);
	block = node.addr[0];                                       // returns the first block stored in the addr of the inode
    // total size of the current directory
	inode_size = (node.size0 << 32) | (node.size1 );
	
	lseek (file_descriptor, (block*BLOCK_SIZE), SEEK_SET);      // finding the address of the block of the inode
	read (file_descriptor, directory, inode_size);              // read the contents of the blocks of addr into buffer data structure
	
    // checks through each directory entry 
	for (i = 0; i < (inode_size / sizeof(dir_type)); i++){
	    
	    if (strcmp(sourceFile, directory[i].filename) == 0){    // if the file to be copied to external file has been found

	        inode_type fileInode = getInode(directory[i].inode);
	        
	        if(fileInode.flags == (1 << 15)){                   // if the inode is allocated
	            
                // calculates the size of file 
	            long fileInode_size = (fileInode.size0 << 32) | (fileInode.size1);

                // iterates through every block in the addr and stores the content in buffer
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
                int inode_block = (currentInode*INODE_SIZE) / BLOCK_SIZE;
                int offset = currentInode*INODE_SIZE) % BLOCK_SIZE;
                lseek(file_descriptor, (inode_block*BLOCK_SIZE) + offset, SEEK_SET);
                read(file_descriptor, node, sizeof(inode_type));
	            
	        }
	        else{                                               // if the inode is unallocated, there is no file present
                printf("\n%s\n","There is no file");
            }
	        return;
	    }
	}
}

void rm(char* filename){

    int i,j;
    long long inode_size;
    // get the current inode
    inode_type node = getInode(currentInode);
    int block = node.addr[0];                                   // returns the first block stored in the addr of the inode
    dir_type directory[200];
    
    // total size of the current directory
    inode_size = (node.size0 << 32) | (node.size1 );
	
	lseek (file_descriptor, (block*BLOCK_SIZE), SEEK_SET);      // finding the address of the block of the inode
	read (file_descriptor, directory, inode_size);              // read the contents of the blocks of addr into buffer data structure
	
    // checks through each directory entry
	for (i =0; i< inode_size/sizeof(dir_type); i++){

	    if (strcmp(filename, directory[i].filename) == 0){      // if the file to be deleted has been found
	        
	        inode_type fileInode = getInode(directory[i].inode);
	        
	        if (fileInode.flags == (1<<15)){                    // to check if the inode is allocated or not
	            // calculates the size of file 
	            long fileInode_size = (fileInode.size0 << 32) | (fileInode.size1);
	            
	            // iterates through every block in the addr and add the blocks to the free array
	            for (j =0; j < fileInode_size/BLOCK_SIZE; j++){
	                block = addr[j];
	                // add the block number to the free array
                    addFreeBlock(block);
	            }
	            if ( 0<fileInode_size%BLOCK_SIZE ){
                    block = addr[j];
                    // add the block number to the free array
                    addFreeBlock(block);
                }
                node.flags = (1<<15);                           // make the inode free by setting the flag
                node.modtime = time(NULL);                      // updating the modifying time
                node.actime = time(NULL);                       // updating the accessing time
                
                directory[i]=directory[(inode_size/sizeof(dir_type))-1];
                inode_size -= sizeof(dir_type);

                node.size0 = (int)((inode_size & 0xFFFFFFFF00000000) >> 32);        // high 64 bit
                node.size1 = (int)(inode_size & 0x00000000FFFFFFFF);                 // low 64 bit

                // writing the updated directory into the block at it's correct position
                lseek(file_descriptor, (BLOCK_SIZE*node.addr[0]), SEEK_SET);
                write(file_descriptor, directory, inode_size);
                // writing the updated inode into the block at it's correct position 
                int inode_block = (currentInode*INODE_SIZE) / BLOCK_SIZE;
                int offset = currentInode*INODE_SIZE) % BLOCK_SIZE;
                lseek(file_descriptor, (inode_block*BLOCK_SIZE) + offset, SEEK_SET);
                read(file_descriptor, node, sizeof(inode_type));
	        }
            else {
                printf("\n%s\n","It is not a file");
            }
	        return;
	    }
	}
}
