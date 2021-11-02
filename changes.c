#define BLOCK_SIZE 1024
#define MAX_FILE_SIZE 4194304 // 4GB of file size
#define FREE_ARRAY_SIZE 248 // free and inode array size
#define INODE_SIZE 64

superblock_type superBlock;
int idx;
char filepath[100];
static struct inode_type root;
int currentInode;

void writeToBlock(int block, void* block, int bytes){

    lseek(idx, BLOCK_SIZE*block, SEEK_SET);
    write(idx, block, bytes);
}

int getFreeBlock() {

    if (superBlock.nfree == 0){
        int block = superBlock.free[0];
        lseek(idx, block* BLOCK_SIZE, SEEK_SET);
        read(idx, superBlock.free, FREE_ARRAY_SIZE*2);
        superBlock.nfree = 251;
        return block;
    }
    superBlock.nfree--;
    return superBlock.free[superBlock.nfree];
} 

void writeToBlockWithOffset(int blockNo, int offset, void * inode, int bytes){

    lseek(idx, (blockNo*BLOCK_SIZE) + offset, SEEK_SET);
    write(idx, inode, bytes);
}

void writeInodeToBlock(int inodeNo, inode_type inode){

    int blockNo = (inodeNo * INODE_SIZE) / BLOCK_SIZE;
    int offset = (inodeNo * INODE_SIZE) % BLOCK_SIZE;
    writeToBlockWithOffset(blockNo, offset, &inode, sizeOf(inode_type));
}
void createRootDirectory(){

    int block = getFreeBlock();
    directory direc[2];
    direc[0].inode = 0;
    strcpy(direc[0].filename, ".");

    direc[1].inode = 0;
    strcpy(direc[1].filename, "..");

    // write to new block

    root.flags = 1<<14 || 1<<15 // 15th bit is for allocated inode indication, 14th and 13th bit 10 shows that it is a directory
    root.nlinks = 1;
    root.uid = 0;
    root.gid = 0;
    root.size = 2*sizeof(direc);
    root.addr[0] = block;
    root.actime = time(NULL);
    root.modtime = time(NULL);

    writeInodeToBlock(0, root);
    currentInode = 0;
    
}
void addFreeBlockToFreeArray(int block){

    if (superBlock.nfree == FREE_ARRAY_SIZE){
        // write to new block
        writeToBlock(block, superBlock.free, FREE_ARRAY_SIZE*2);
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

    // defining other variables of the superblock
    superBlock.flock = 'f';
    superBlock.ilock = 'i';
    superBlock.fmod = 'f';
    superBlock.time = 0;

    // writing the super block
    writeToBlock(1, &superBlock, BLOCK_SIZE);
    // allocate empty space for inodes

    // create root directory for the first data block
    createRootDirectory();
}
