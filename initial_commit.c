int idx;
char filepath[100];

int getFreeBlock{
    if (superBlock.nfree == 0){
        int block = superBlock.free[0];
        lseek(idx, block* BLOCK_SIZE, SEEK_SET);
        read(idx, superBlock.free, FREE_ARRAY_SIZE*2);
        superBlock.nfree = 100;
        return block;
    }
    superBlock.nfree--;
    return superBlock.free[superBlock.nfree];
}

void addFreeBlockToFreeArray(int block){
    if (superBlock.nfree == FREE_ARRAY_SIZE){
        // write to new block
        
        superBlock.nfree = 0;;
    }
    superBlock.free[superBlock.nfree] = block;
    superBlock.nfree++;
}
void initfs (char* filePath, int total_blocks, int total_inode_block){
    int block;
    superBlock.isize = total_inode_block;
    superBlock.fsize = total_blocks;
    if((open(path,O_RDWR|O_CREAT,0600))== -1)
    {
        printf("\n file opening error [%s]\n",strerror(errno));
        return;
    }
    strcpy(filepath, filePath);

    // add all the blocks to the free array
    for (block = 1 + super.isize; block < total_blocks; block++)
        addFreeBlockToFreeArray(block);
    // add all the free inodes 

    // defining other variables of the superblock
    // writing the super block

    // allocate empty space for inodes

    // create root directory for the first data block
}
