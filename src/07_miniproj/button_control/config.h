#ifndef CONFIG_H
#define CONFIG_H

#define SET_FREQ_PATH "/tmp/set_freq" // read-only for the module, write-only for the daemon
#define GET_FREQ_PATH "/tmp/get_freq" // read-write for the module, read-only for the daemon
#define AUTO_FREQ_PATH "/tmp/auto_freq" // read-only for the module, write-only for the daemon
#define GET_TEMP_PATH "/sys/class/thermal/thermal_zone0/temp" // read-only for both the module and the daemon

#define PID_FILE "/run/led_daemon.pid"

#endif // CONFIG_H