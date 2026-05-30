#define _GNU_SOURCE     // for CPU affinity
#include <sys/types.h>  // for fork()
#include <unistd.h>     // for fork()
#include <stdlib.h>     // for exit()
#include <sched.h>      // for CPU affinity
#include <stdio.h>      // for printf() and perror()
#include <sys/types.h>  // for socketpair()
#include <sys/socket.h> // for socketpair()
#include <string.h>     // for strlen()
#include <stdbool.h>    // for bool type

// This is the entrypoint of the program
int main() {
    // There are 2 processes, each running on its own CPU core
    // They communicate via socketpair
    int fd[2];
    // - Use DGRAM for well-defined message boundaries, so that the parent can read one message at a time
    int err = socketpair(AF_UNIX, SOCK_DGRAM, 0, fd);
    if (err == -1) {
        perror("Error creating socket pair");
        exit(err);
    }

    // Only after creating the socketpair can we fork
    int pid = fork();

    // Init the CPU set here
    cpu_set_t set;
    CPU_ZERO(&set);

    if (pid == 0) {
        // - Child process -

        // Close the child's socket as it will only use fd[1]
        close(fd[0]);

        // Affinity to core 1
        CPU_SET(1, &set);
        int ret = sched_setaffinity(0, sizeof(set), &set);
        if (ret == -1) {
            perror("Error setting affinity of child to CPU 1");
            exit(ret);
        }
        
        printf("Child process affinity set to CPU 1 successfully\n");

        // The child process will emmit text messages to it's parent
        const char* text = "Hello from the child process!\n";
        write(fd[1], text, strlen(text));

        text = "This is another message from the child process.\n";
        write(fd[1], text, strlen(text));

        text = "exit";
        write(fd[1], text, strlen(text));

        close(fd[1]); // Close the child's socket after sending messages
        
        exit(0);

    } else if (pid > 0) {
        // - Parent process -

        // Close the child's socket as the parent will only read from its own socket
        close(fd[1]);

        // Affinity to core 0
        CPU_SET(0, &set);
        int ret = sched_setaffinity(0, sizeof(set), &set);
        if (ret == -1) {
            perror("Error setting affinity of parent to CPU 0");
            exit(ret);
        }

        printf("Parent process affinity set to CPU 0 successfully\n");

        bool shouldExit = false;

        // The parent process will receive the text messages and print them on the console
        while (!shouldExit) {
            char buffer[256];
            ssize_t bytesRead = read(fd[0], buffer, sizeof(buffer) - 1);
            if (bytesRead == -1) {
                perror("Error reading from socket");
                exit(bytesRead);
            } else if (bytesRead == 0) {
                // End of file, the child has closed the socket
                break;
            }
            buffer[bytesRead] = '\0'; // Null-terminate the string
            
            // The message "exit" terminates the program (we do NOT print it)
            if (strcmp(buffer, "exit") == 0) {
                printf("Parent process received exit signal from child, preparing to terminate...\n");
                shouldExit = true; // Set the shouldExit flag if the message is "exit"
            } else {
                printf("Parent process received message: %s", buffer);
            }
        }

        close(fd[0]); // Close the parent's socket after finishing reading

        printf("Parent process received exit signal, terminating...\n");
        
        exit(0);

    } else {
        // TODO: Manage error
    }

    
    

    // We are required to capture and ignore these signals: SIGHUP, SIGINT, SIGQUIT, SIGABRT and SIGTERM

    // TODO: Implement the program here
    return 0;
}