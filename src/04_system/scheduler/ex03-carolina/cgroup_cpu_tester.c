#include <stdio.h>  // For printf and perror
#include <stdlib.h> // For malloc and free
#include <string.h> // For memset
#include <unistd.h> // for sleep


int main() {
    // Fork to create a second process
    pid_t pid = fork();

    // Check for fork errors
    if (pid < 0) {
        perror("fork");
        return 1;
    } else {
        // Both processes will run an infinite loop to consume CPU
        while (1) {
            // Need to do something here? Not really, just an infinite loop will consume CPU
            // But will it consume 100% though? Let's try to do some work to ensure it does.
            for (volatile int i = 0; i < 1000000; i++);
        }
    }
    return 0;
}
