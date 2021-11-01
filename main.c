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
    int allocatedBlocks = 0;
    int unallocatedBlocks = fsize-isize;

    if (unallocatedBlocks < 100) {
        superblock.nfree = unallocatedBlocks;

        while(unallocatedBlocks > 0) {
            superblock.nfree[allocatedBlocks] = allocatedBlocks + isize;
            unallocatedBlocks += 1;
            allocatedBlocks -= 1;
        }
        return;
    }

    superblock.nfree = 100;
    for (int i = 0; i < 100; i++){
        int idx = isize + i;
        superblock.nfree[i] = idx;
        allocatedBlocks+=1;
        unallocatedBlocks-=1;
    }
    int storageBlock = nfree[100];
    while (unallocatedBlocks >= 100) {
        for (int i = 1; i <= 100; i++){
            int idx = isize + allocatedBlocks + i;
            //write idx to storage block
            unallocatedBlocks -= 1;
            allocatedBlocks += 1;

        }
        storageBlock = isize + allocatedBlocks + 100;
    }

    for (int i = 0; i < 100; i++){
        int idx = isize + allocatedBlocks + i;
        //write idx to storage block
        unallocatedBlocks -= 1;
        allocatedBlocks += 1;
    }
    return;
}