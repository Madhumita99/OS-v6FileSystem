#include "mod-v6.h"
//returns byte offset of free inode from beginning of file system
/*int getFreeInodeOffset(void){
    int offset = 2*1024;
    int end = (superBlock.isize + 2) * 1024;
    inode_type inode;
    lseek(file_descriptor, offset, SEEK_SET);
    while(offset < end) {
        read(file_descriptor, inode, 64);
        offset += 64;
        if ((inode.flags & INODE_ALLOC) == 0) { //check flags to see if free?
            return offset;
        }
    }
    return -1; //free inode not found
}*/

/*      Creat a new file called v6-file in the v6 file system and fill the contents of the newly created file
        with the contents of the externalfile. The file externalfile is a file in the native unix machine, the
        system where your program is being run.*/
//return 1 on success, -1 on failure
//sizes are in bytes
int cpin (char* externalFile, char* internalFile) {
    //get length of external file

    unsigned long file_size;
    inode_type inode;
    int offset;
    unsigned long inode_size;


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
    int num_of_blocks = file_size/1024 + (file_size % 1024 != 0);
    int nfree_reset = superBlock.nfree;
    int free_reset[251];
    memcpy(free_reset, superBlock.free, sizeof(free_reset));



    //get data blocks

    int blocks[ num_of_blocks ];
    for (int i = 0; i < num_of_blocks; i++) {
        int free_block = getFreeBlock();
        if (free_block == -1){
            printf("Error: Not enough system memory\n");
            //reset superblock
            superBlock.nfree = nfree_reset;
            memcpy(superBlock.free, free_reset, sizeof(free_reset));
            
            //rewrite superblock
            lseek(file_descriptor, BLOCK_SIZE, SEEK_SET);					//find superblock
            write(file_descriptor, &superBlock, BLOCK_SIZE);			//update nfree in superblock

            superblock_type testSuper = superBlock;
            return -1;
        }
        blocks[i] = free_block;
    }

    //write to data blocks
    for (int i = 0; i < num_of_blocks; i++) {
        int bytes = 1024;
        if (i == num_of_blocks - 1) {
            bytes = file_size - i * 1024;
        }
        char buffer[bytes];
        offset = blocks[i] * 1024;
                
        lseek(file_descriptor, offset, SEEK_SET);
        read(source_fd, buffer, bytes);
        write(file_descriptor, buffer, bytes);

    }

    //test that file has been written
/*    for (int i = 0; i < num_of_blocks; i++) {
        int bytes = 1024;
        if (i == num_of_blocks - 1) {
            bytes = file_size - i * 1024;
        }
        char test_buffer[bytes];
        offset = blocks[i] * 1024;
        lseek(file_descriptor, offset, SEEK_SET);
        read(file_descriptor, test_buffer, bytes);
        printf("break");
    }*/

    //get inode for new file
    int inode_idx = getFreeInode();


    //set fields except address
    inode.flags = (INODE_ALLOC | PLAIN_FILE); 		// I-node allocated + directory file
    inode.nlinks = 1;
    inode.uid = 0;
    inode.gid = 0;
    inode.size0 = (int)((file_size & 0xFFFFFFFF00000000) >> 32);
    inode.size1 = (int)(file_size & 0x00000000FFFFFFFF);
    inode.actime = time(NULL);
    inode.modtime = time(NULL);
    //if small file:
    if ( num_of_blocks < 9 ) {
        for (int i = 0; i < num_of_blocks; i++)
            inode.addr[i] = blocks[i];
    }
        //if large file:
    else {
        for (int i = 0; i < num_of_blocks; i++) {
            //todo
        }
    }
    updateInodeEntry(0, inode_idx, inode);

    //test that inode has been written
/*    inode_type test_inode[1];
    lseek(file_descriptor, inode_offset, SEEK_SET);
    read(file_descriptor, test_inode, 64);*/

    //TODO TEST
    dir_type dir_entry;
    dir_entry.inode = inode_idx;
    strcpy(dir_entry.filename, internalFile);


    check= addNewFileDirectoryEntry(currentInode, dir_entry);
    if (check < 0) {		//error occurred
        return -1;
    }

    return 1;
}