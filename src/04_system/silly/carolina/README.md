# My notes

## Run the unoptimized program and check CPU usage

First we can compile the program like so:

```bash
make
```

Then, we connect to the target:

```bash
ssh root@192.168.53.14
```

And launch the app:

```bash
cd /workspace/src/04_system/silly/carolina
./build/silly_led_control
```

We then put the process in background with `CTRL+Z` and then the command `bg`.

Then, we can check the CPU usage with `top`. Here we can confirm that it is using 100% of the CPU.

![alt text](./images/unoptimized_top.png)

Bring the process back to foreground with `fg` and stop it with `CTRL+C`.

## Optimizing the program

The lab instruction suggests using `timerfd` to optimize the program. This is also very useful for multiplexing, which is required for the next step. We can use `select` to multiplex the timer and the buttons. For now, we only configure the timer for the LED blinking.

First, we need to include the header for `timerfd`:

```c
#include <sys/timerfd.h>
```

Then, we can create a new timer file descriptor with `timerfd_create`:

```c
int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
if (timer_fd == -1) {
    perror("timerfd_create");
    exit(EXIT_FAILURE);
}
```

Then we set the initial expiration and interval with `timerfd_settime`. The particularity of this function is that it takes the time in seconds and nanoseconds, the latter being the remaining time after extracting the seconds. So we need to convert our period in milliseconds to seconds and nanoseconds.

```c
// Get the seconds and nanoseconds components of the period
int period_sec = period_ms / 1000; // seconds part
long period_ns = (period_ms % 1000) * 1000000L; // nanoseconds part (remaining after extracting seconds)

// Configure the timer to fire every 'period_ms' milliseconds
struct itimerspec timer_spec;
timer_spec.it_value.tv_sec = period_sec;
timer_spec.it_value.tv_nsec = period_ns;
timer_spec.it_interval.tv_sec = period_sec;
timer_spec.it_interval.tv_nsec = period_ns;
```

Then we can start the timer with `timerfd_settime`:

```c
if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
    perror("timerfd_settime");
    exit(-1);
}
```

The next step is to monitor the file without blocking all resources and have the same 100% CPU usage problem. We inevitably need to have a `while true` loop, but using `select` will allow us to block until the timer expires, without consuming CPU resources.

```c
while (1) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(timer_fd, &read_fds);

    // Wait for the timer to expire
    int ret = select(timer_fd + 1, &read_fds, NULL, NULL, NULL);
    if (ret == -1) {
        perror("select");
        exit(-1);
    }

    if (FD_ISSET(timer_fd, &read_fds)) {
        // Timer expired, toggle the LED
        uint64_t expirations;
        read(timer_fd, &expirations, sizeof(expirations)); // Clear the timer event
        toggle_led();
    }
}
```

The LED will toogle (not blink!) every `period_ms` milliseconds. Let's take a look at this portion of the original code:

```c
long duty   = 2;     // %
long period = 1000;  // ms
if (argc >= 2) period = atoi(argv[1]);
period *= 1000000;  // in ns

// compute duty period...
long p1 = period / 100 * duty;
long p2 = period - p1;
```

If we want to have this "blinking" effect, we can use the `duty` variable to compute the time the LED is on and off. We can then use `timerfd_settime` to change the timer period dynamically:

```c
// Toggle the LED
toggle_led(led_fd);

// Compute the new timer period based on the duty cycle
long on_time_ns = period * duty / 100;
long off_time_ns = period - on_time_ns;

// Update the timer to toggle the LED on and off
if (toggle_state) {
    timer_spec.it_value.tv_sec = on_time_ns / 1000000000;
    timer_spec.it_value.tv_nsec = on_time_ns % 1000000000;
} else {
    timer_spec.it_value.tv_sec = off_time_ns / 1000000000;
    timer_spec.it_value.tv_nsec = off_time_ns % 1000000000;
}

if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
    perror("timerfd_settime");
    exit(-1);
}
```

TODO: implement and test this.


## Adding the required features

The lab instructions ask us do to the following:

1. At startup, the blinking frequency will be set to 2 Hz
2. Use the push buttons
    - k1 to increase the frequency
    - k2 to reset the frequency to its initial value
    - k3 to decrease the frequency
    - optional: a continuous press on a button will indicate an auto increment or decrement of the frequency.
3. All frequency changes will be logged with Linux syslog.
4. Multiplexing of inputs/outputs must be used.

