#include <stdio.h>  // For printf and perror
#include <stdlib.h> // For malloc and free
#include <string.h> // For memset
#include <unistd.h> // for sleep


int main() {
    // Keep trying to allocate 1MiB memory blocks until we hit the limit
    size_t block_size = 1024 * 1024; // 1 MiB
    size_t num_blocks = 0;
    while (1) {
        void *block = malloc(block_size);
        if (block == NULL) {
            perror("Memory allocation failed");
            break;
        }
        printf("Allocated %zu MiB\n", num_blocks + 1);
        memset(block, 0, block_size); // Fill the block with zeros
        num_blocks++;

        // Sleep a while to be able to monitor memory usage
        sleep(1);
    }
}
