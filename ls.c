#include "mod-v6.h"

void ls(void) {
    inode_type inode;
    int offset = (BLOCK_SIZE * 2) + (currentInode-1)*INODE_SIZE;
    lseek(file_descriptor, offset, SEEK_SET);
    read(file_descriptor, &inode, INODE_SIZE);
    int addr = inode.addr[0];
    //long long dir_size = (inode.size0 << 32) | (inode.size1 );
    int entries = BLOCK_SIZE/32;
    dir_type directory[entries];
    lseek(file_descriptor, addr*1024, SEEK_SET);
    read(file_descriptor, directory, BLOCK_SIZE);
    for(int i = 0; i < entries; i++){
        if (directory[i].inode > 0) {
            printf("%s\n", directory[i].filename);
        }
    }

}