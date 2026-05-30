// For standard input/output
#include <stdio.h>
// For file operations
#include <fcntl.h>
#include <unistd.h>
// For string operations
#include <string.h>
// For standard library functions
#include <stdlib.h>
// For epoll
#include <sys/epoll.h>

#define EXPORT_GPIO_PATH "/sys/class/gpio/export"
#define UNEXPORT_GPIO_PATH "/sys/class/gpio/unexport"

#define SET_FREQ_PATH "/tmp/set_freq" // TODO: add real path in class later
#define GET_FREQ_PATH "/tmp/set_freq" // TODO: add real path in class later (same as set_freq bc we don't have the sysmod yet)
#define AUTO_FREQ_PATH "/tmp/auto_freq" // TODO: add real path in class later

#define MIN_FREQ 1
#define MAX_FREQ 20

// We will nedd to define the GPIO pins for the following components:
// - The power LED to indicate when a button is pressed
// - The k1, k2 and k3 buttons to control the frequency and mode

const int POWER_LED_GPIO = 362; // GPIO pin for the power LED
const int K1_BUTTON_GPIO = 0; // GPIO pin for k1 button
const int K2_BUTTON_GPIO = 2; // GPIO pin for k2 button
const int K3_BUTTON_GPIO = 3; // GPIO pin for k3 button

// Update the frequency
static void update_frequency(int get_freq_fd, int set_freq_fd, int delta)
{
    // Read the current frequency from the get_freq interface
    char freq_str[16];
    lseek(get_freq_fd, 0, SEEK_SET);
    read(get_freq_fd, freq_str, sizeof(freq_str));

    // Convert the frequency string to an integer, apply the delta, and convert back to string
    int freq = atoi(freq_str);
    freq += delta;

    // Keep frequency between MIN_FREQ and MAX_FREQ
    if (freq < MIN_FREQ) {
        freq = MIN_FREQ;
    } else if (freq > MAX_FREQ) {
        freq = MAX_FREQ;
    }
    // Write the new frequency back to the set_freq interface
    snprintf(freq_str, sizeof(freq_str), "%d", freq);
    lseek(set_freq_fd, 0, SEEK_SET);
    write(set_freq_fd, freq_str, strlen(freq_str));

    // Debug log, TODO: remove this in the final version
    printf("Updated frequency to %d\n", freq);
}

static int switch_mode(int auto_freq_fd)
{
    // Read the current mode from the auto_freq interface
    char mode_str[16];
    lseek(auto_freq_fd, 0, SEEK_SET);
    read(auto_freq_fd, mode_str, sizeof(mode_str));

    // Toggle the mode (0/1 for manual/auto)
    const char *new_mode = (strcmp(mode_str, "0") == 0) ? "1" : "0";

    // Write the new mode back to the auto_freq interface
    lseek(auto_freq_fd, 0, SEEK_SET);
    write(auto_freq_fd, new_mode, strlen(new_mode));

    // Debug log, TODO: remove this in the final version
    printf("Switched mode to %s (%s)\n", new_mode, strcmp(new_mode, "0") == 0 ? "manual" : "auto");

    return 0;
}

// Auto-setup of one GPIO pin
static int setup_gpio(int gpio, const char *direction, const char *edge)
{
    char path[64];
    FILE *file;

    // Unexport the GPIO pin first (in case it was already exported)
    snprintf(path, sizeof(path), UNEXPORT_GPIO_PATH);
    file = fopen(path, "w");
    if (file) {
        fprintf(file, "%d", gpio);
        fclose(file);
    }

    // Export the GPIO pin to sysfs
    snprintf(path, sizeof(path), EXPORT_GPIO_PATH);
    file = fopen(path, "w");
    if (file) {
        fprintf(file, "%d", gpio);
        fclose(file);
    }

    // Set the direction
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    file = fopen(path, "w");
    if (file) {
        fprintf(file, "%s", direction);
        fclose(file);
    }

    // Set the edge (if applicable)
    if (edge) {
        snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", gpio);
        file = fopen(path, "w");
        if (file) {
            fprintf(file, "%s", edge);
            fclose(file);
        }
    }

    // Open and return the file descriptor for the value file
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(path, (strcmp(direction, "in") == 0) ? O_RDONLY : O_WRONLY);
    if (fd == -1) {
        perror("open gpio value");
        return -1;
    }
    return fd;
}

// ---
// Main function

int main(void)
{
    // Create the files for the get_freq, set_freq and auto_freq interfaces if they don't exist
    // TODO: remove this in the final version, we will have the sysmod to create these files for us
    int fd;
    fd = open(SET_FREQ_PATH, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open set_freq");
        return 1;
    }
    close(fd);
    fd = open(AUTO_FREQ_PATH, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open auto_freq");
        return 1;
    }
    close(fd);
    // ---

    // Prepare the get_freq interface
    int get_freq_fd = open(GET_FREQ_PATH, O_RDONLY);
    if (get_freq_fd == -1) {
        perror("open get_freq");
        return 1;
    }
    
    // Prepare the set_freq interface
    int set_freq_fd = open(SET_FREQ_PATH, O_WRONLY);
    if (set_freq_fd == -1) {
        perror("open set_freq");
        return 1;
    }

    // Prepare the auto_freq interface (read/write)
    int auto_freq_fd = open(AUTO_FREQ_PATH, O_RDWR);
    if (auto_freq_fd == -1) {
        perror("open auto_freq");
        return 1;
    }


    // Setup the GPIOs for the buttons and the power LED
    int k1_fd = setup_gpio(K1_BUTTON_GPIO, "in", "both");
    int k2_fd = setup_gpio(K2_BUTTON_GPIO, "in", "both");
    int k3_fd = setup_gpio(K3_BUTTON_GPIO, "in", "both");
    int power_led_fd = setup_gpio(POWER_LED_GPIO, "out", NULL);

    // Create the epoll context
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        return 1;
    }

    // Add each button's file descriptor to the epoll context
    struct epoll_event ev;
    ev.events = EPOLLPRI; // Trigger on urgent data (button state change)
    ev.data.fd = k1_fd;
    
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, k1_fd, &ev);
    if (ret == -1) {
        perror("epoll_ctl k1");
        return 1;
    }

    ev.data.fd = k2_fd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, k2_fd, &ev);
    if (ret == -1) {
        perror("epoll_ctl k2");
        return 1;
    }

    ev.data.fd = k3_fd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, k3_fd, &ev);
    if (ret == -1) {
        perror("epoll_ctl k3");
        return 1;
    }

    // Main loop to poll button states and control the LED
    char k1_value = '0', k2_value = '0', k3_value = '0';
    while (1) {
        // Wait for any button event using epoll
        struct epoll_event events[3];
        int nr = epoll_wait(epfd, events, 3, -1);
        if (nr == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nr; i++) {
            // Read fd to clear the event
            if (events[i].data.fd == k1_fd) {
                lseek(k1_fd, 0, SEEK_SET);
                read(k1_fd, &k1_value, sizeof(k1_value));
                // If k1 is pressed, increase the frequency
                if (k1_value == '1') {
                    // TODO: confirm increment value
                    update_frequency(get_freq_fd, set_freq_fd, 1);
                }
            } else if (events[i].data.fd == k2_fd) {
                lseek(k2_fd, 0, SEEK_SET);
                read(k2_fd, &k2_value, sizeof(k2_value));
                // If k2 is pressed, decrease the frequency
                if (k2_value == '1') {
                    update_frequency(get_freq_fd, set_freq_fd, -1);
                }
            } else if (events[i].data.fd == k3_fd) {
                lseek(k3_fd, 0, SEEK_SET);
                read(k3_fd, &k3_value, sizeof(k3_value));
                // If k3 is pressed, switch mode
                if (k3_value == '1') {
                    switch_mode(auto_freq_fd);
                }
            }
        }

        // Print state of the buttons, TODO: remove this debug print in the final version
        printf("%c %c %c\n", k1_value == '1' ? 'o' : 'x', k2_value == '1' ? 'o' : 'x', k3_value == '1' ? 'o' : 'x');

        // Blink the power LED if k1 or k2 are pressed
        write(power_led_fd, (k1_value == '1' || k2_value == '1') ? "1" : "0", 1);
    }

    return 0;
}