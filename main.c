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
