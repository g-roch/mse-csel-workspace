// For standard input/output
#include <stdio.h>
// For file operations
#include <fcntl.h>
#include <unistd.h>
// For string operations
#include <string.h>
// For standard library functions
#include <stdlib.h>
// For file permissions
#include <sys/stat.h>
// For epoll
#include <sys/epoll.h>
// OLED lib
#include "ssd1306.h"
// Config
#include "config.h"

#define EXPORT_GPIO_PATH "/sys/class/gpio/export"
#define UNEXPORT_GPIO_PATH "/sys/class/gpio/unexport"

#define MIN_FREQ 1
#define MAX_FREQ 20
#define FREQ_STEP 1

// We will nedd to define the GPIO pins for the following components:
// - The power LED to indicate when a button is pressed
// - The k1, k2 and k3 buttons to control the frequency and mode

const int POWER_LED_GPIO = 362; // GPIO pin for the power LED
const int K1_BUTTON_GPIO = 0; // GPIO pin for k1 button
const int K2_BUTTON_GPIO = 2; // GPIO pin for k2 button
const int K3_BUTTON_GPIO = 3; // GPIO pin for k3 button

int frequency = 10; // Initial frequency (Hz)
int temperature = 35; // Initial temperature (°C)

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
    sprintf(freq_str, "%d\n", freq);
    lseek(set_freq_fd, 0, SEEK_SET);
    write(set_freq_fd, freq_str, strlen(freq_str));

    // Update the frequency variable and display
    frequency = freq;
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

// Daemonize the process
void daemonize()
{
    // Fork the process
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE); // Fork failed
    if (pid > 0) exit(EXIT_SUCCESS); // Parent exits, child continues as daemon

    if (setsid() <0) exit(EXIT_FAILURE); // Create a new session

    // Fork again to ensure the daemon cannot acquire a controlling terminal
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE); // Fork failed
    if (pid > 0) exit(EXIT_SUCCESS); // Parent exits, child continues as daemon

    umask(0); // TODO: Set file permissions mask
    chdir("/"); // TODO: Change working directory to ... 

    // Close standard file descriptors (stdin, stdout, stderr)
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    FILE *fp = fopen(PID_FILE, "w");
    if (fp) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

void update_display()
{
    // ssd1306_clear_display();
    // Display initial message
    ssd1306_set_position(0, 0);
    ssd1306_puts("CSEL1");
    ssd1306_set_position(0, 1);
    ssd1306_puts("----------");
    ssd1306_set_position(0, 3);
    
    char buffer[20];
    sprintf(buffer, "Temp: %d'C  ", temperature);
    ssd1306_puts(buffer);

    ssd1306_set_position(0, 4);
    sprintf(buffer, "Freq: %dHz  ", frequency);
    ssd1306_puts(buffer);
}

int get_temperature()
{
    // Read the temperature from the system file
    char temp_str[16];
    int temp_fd = open(GET_TEMP_PATH, O_RDONLY);
    if (temp_fd == -1) {
        perror("open temperature");
        return -1;
    }
    read(temp_fd, temp_str, sizeof(temp_str));
    close(temp_fd);

    // Convert the temperature string to an integer (divide by 1000 to get °C)
    int temp = atoi(temp_str) / 1000;
    return temp;
}


// ---
// Main function

int main(void)
{
    // daemonize();

    // --------------------------------------------------------------------------------------------
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
    // --------------------------------------------------------------------------------------------

    // Initialize the OLED display
    ssd1306_init();
    temperature = get_temperature();
    update_display();

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
                    update_frequency(get_freq_fd, set_freq_fd, FREQ_STEP);
                    update_display();
                }
            } else if (events[i].data.fd == k2_fd) {
                lseek(k2_fd, 0, SEEK_SET);
                read(k2_fd, &k2_value, sizeof(k2_value));
                // If k2 is pressed, decrease the frequency
                if (k2_value == '1') {
                    update_frequency(get_freq_fd, set_freq_fd, -FREQ_STEP);
                    update_display();
                }
            } else if (events[i].data.fd == k3_fd) {
                lseek(k3_fd, 0, SEEK_SET);
                read(k3_fd, &k3_value, sizeof(k3_value));
                // If k3 is pressed, switch mode
                if (k3_value == '1') {
                    switch_mode(auto_freq_fd);
                    update_display();
                }
            }
        }

        // Blink the power LED if any button is pressed
        write(power_led_fd, (k1_value == '1' || k2_value == '1' || k3_value == '1') ? "1" : "0", 1);

        
    }

    return 0;
}