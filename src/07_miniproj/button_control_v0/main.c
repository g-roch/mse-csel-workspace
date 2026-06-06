#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>

#define EXPORT_GPIO_PATH   "/sys/class/gpio/export"
#define UNEXPORT_GPIO_PATH "/sys/class/gpio/unexport"

const int K1_BUTTON_GPIO = 0;
const int K2_BUTTON_GPIO = 2;
const int K3_BUTTON_GPIO = 3;

static int setup_gpio(int gpio, const char *direction, const char *edge)
{
    char path[64];
    FILE *file;

    snprintf(path, sizeof(path), UNEXPORT_GPIO_PATH);
    file = fopen(path, "w");
    if (file) { fprintf(file, "%d", gpio); fclose(file); }

    snprintf(path, sizeof(path), EXPORT_GPIO_PATH);
    file = fopen(path, "w");
    if (file) { fprintf(file, "%d", gpio); fclose(file); }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    file = fopen(path, "w");
    if (file) { fprintf(file, "%s", direction); fclose(file); }

    if (edge) {
        snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", gpio);
        file = fopen(path, "w");
        if (file) { fprintf(file, "%s", edge); fclose(file); }
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(path, O_RDONLY);
    if (fd == -1) { perror("open gpio value"); return -1; }
    return fd;
}

int main(void)
{
    int k1_fd = setup_gpio(K1_BUTTON_GPIO, "in", "both");
    int k2_fd = setup_gpio(K2_BUTTON_GPIO, "in", "both");
    int k3_fd = setup_gpio(K3_BUTTON_GPIO, "in", "both");

    int maxfd = (k1_fd > k2_fd ? (k1_fd > k3_fd ? k1_fd : k3_fd) : (k2_fd > k3_fd ? k2_fd : k3_fd));
    char value[2];

    while (1) {
        fd_set fd_in;
        FD_ZERO(&fd_in);
        FD_SET(k1_fd, &fd_in);
        FD_SET(k2_fd, &fd_in);
        FD_SET(k3_fd, &fd_in);

        int ret = select(maxfd + 1, &fd_in, NULL, NULL, NULL);
        if (ret == -1) { perror("select"); break; }

        if (FD_ISSET(k1_fd, &fd_in)) {
            lseek(k1_fd, 0, SEEK_SET);
            read(k1_fd, value, sizeof(value));
            if (value[0] == '1') printf("K1 pressed\n");
        }
        if (FD_ISSET(k2_fd, &fd_in)) {
            lseek(k2_fd, 0, SEEK_SET);
            read(k2_fd, value, sizeof(value));
            if (value[0] == '1') printf("K2 pressed\n");
        }
        if (FD_ISSET(k3_fd, &fd_in)) {
            lseek(k3_fd, 0, SEEK_SET);
            read(k3_fd, value, sizeof(value));
            if (value[0] == '1') printf("K3 pressed\n");
        }
    }

    return 0;
}
