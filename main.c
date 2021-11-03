#include <stdio.h>
#include <fcntl.h>

void openfs(char* file_name) {
    int file_descriptor = open(file_name, O_RDWR | O_CREAT);
    if (file_descriptor == -1) {
        printf("file not found");
    } else {
        printf("success");
    }
}

void allocateBlocks(int isize, int fsize) {
    //int allocatedBlocks = 0;
    int unallocatedBlocks = fsize-isize;
    int blockIdx = isize + 1;
    int free[251];
    int nfree;
    int storageBlockIdx;
    int nextStorageBlockIdx;

    if (unallocatedBlocks < 251) {
        int totalDataBlocks = unallocatedBlocks;
        nfree = totalDataBlocks;
        for (int i = 0; i < totalDataBlocks; i++) {
            free[i] = blockIdx;
            blockIdx += 1;
        }
        superblock.free = free;
        superblock.nfree = nfree;
        return;
    }

    nfree = 251;
    for (int i = 0; i < 251; i++){
        free[i] = blockIdx;
        blockIdx += 1;
    }

    superblock.free = free;
    superblock.nfree = nfree;

    storageBlockIdx = free[0];
    while (blockIdx < fsize - 251) {
        //write 251 to storage block
        for (int i = 0; i < 251; i++) {
            if (i == 0) {
                nextStorageBlockIdx = blockIdx;
            }
            //write blockIdx to storage block
            blockIdx += 1;
        }
        storageBlockIdx = nextStorageBlockIdx;
    }

    int freeBlocks = blockIdx - fsize;
    //write freeBlocks to storage block
    for (blockIdx; blockIdx < fsize; blockIdx++) {
        //write blockIdx to storage block
    }


}