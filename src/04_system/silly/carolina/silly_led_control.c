/**
 * Copyright 2018 University of Applied Sciences Western Switzerland / Fribourg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project: HEIA-FR / HES-SO MSE - MA-CSEL1 Laboratory
 *
 * Abstract: System programming -  file system
 *
 * Purpose: NanoPi silly status led control system
 *
 * Autĥor:  Daniel Gachet
 * Date:    07.11.2018
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
// timerfd_*
#include <sys/timerfd.h>
// uint64_t
#include <stdint.h>
// perror
#include <stdio.h>

/*
 * status led - gpioa.10 --> gpio10
 * power led  - gpiol.10 --> gpio362
 */
#define GPIO_EXPORT   "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"
#define GPIO_LED      "/sys/class/gpio/gpio10"
#define LED           "10"


static int open_led()
{
    // unexport pin out of sysfs (reinitialization)
    int f = open(GPIO_UNEXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // export pin to sysfs
    f = open(GPIO_EXPORT, O_WRONLY);
    write(f, LED, strlen(LED));
    close(f);

    // config pin
    f = open(GPIO_LED "/direction", O_WRONLY);
    write(f, "out", 3);
    close(f);

    // open gpio value attribute
    f = open(GPIO_LED "/value", O_RDWR);
    return f;
}

int toggle_led(int led_fd)
{
    static int state = 0;
    state = !state;
    lseek(led_fd, 0, SEEK_SET);
    write(led_fd, state ? "1" : "0", 1);

    return 0;
}

int main(int argc, char* argv[])
{
    // Manage command line arguments
     if (argc > 2) {
        fprintf(stderr, "Usage: %s [period_ms]\n", argv[0]);
        return -1;
    }

    long period_ms = 1000;  // ms
    if (argc >= 2) period_ms = atoi(argv[1]);

    // Get already a hold of the LED file descriptor
    int led_fd = open_led();

    // Create a timerfd to generate periodic events
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("timerfd_create");
        return -1;
    }

    // Get the seconds and nanoseconds components of the period
    int period_sec = period_ms / 1000; // seconds part
    long period_ns = (period_ms % 1000) * 1000000L; // nanoseconds part (remaining after extracting seconds)

    // Configure the timer to fire every 'period_ms' milliseconds
    struct itimerspec timer_spec;
    timer_spec.it_value.tv_sec = period_sec;
    timer_spec.it_value.tv_nsec = period_ns;
    timer_spec.it_interval.tv_sec = period_sec;
    timer_spec.it_interval.tv_nsec = period_ns;

    // Configure and start the timer
    if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        perror("timerfd_settime");
        exit(-1);
    }

    while (1) {
        // Define a set of file descriptors to monitor
        fd_set read_fds;
        // Init the fd list
        FD_ZERO(&read_fds);
        // Add the timer file descriptor to the set
        FD_SET(timer_fd, &read_fds);

        // Wait for the timer to expire
        int ret = select(timer_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret == -1) {
            perror("select");
            exit(-1);
        }

        // Check if the timer file descriptor is ready
        if (FD_ISSET(timer_fd, &read_fds)) {
            // Timer expired, toggle the LED
            uint64_t expirations;
            // Clear the timer event
            read(timer_fd, &expirations, sizeof(expirations));
            // Toggle the LED
            toggle_led(led_fd);
        }
    }

    return 0;
}