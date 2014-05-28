
## stdhack

**fair warning**: late night coding, comes with no guarantee, etc...

stdhack runs a child process with standard I/O descriptors redirected
through UNIX pipes.

### Motivation

For some reason, OpenVZ won't let chrooted processes access standard file
descriptors (`stdin`, `stdout` and `stderr`) from the `/proc` interface.

In particular, the following are unavailable:

    /dev/stdin  -> /proc/self/fd/0
    /dev/stdout -> /proc/self/fd/1
    /dev/stderr -> /proc/self/fd/2

Typical behaviour (from within a chroot environment on OpenVZ):

    % echo "test" > /dev/stdout
    bash: /dev/stdout: Permission denied

    % echo "test" | sudo tee /dev/stdout
    tee: /dev/stdout: Permission denied
    test

This causes all kinds of integration problems with various applications
(for instance, `/dev/stdout` is currently the only way to have nginx log
to its standard output, and *not* use log files).

### Examples

    % stdhack sh -c 'ls -l /proc/$$/fd'
    total 0
    lr-x------ 1 oprs oprs 64 May 28 02:21 0 -> pipe:[15847754]
    l-wx------ 1 oprs oprs 64 May 28 02:21 1 -> pipe:[15847755]
    l-wx------ 1 oprs oprs 64 May 28 02:21 2 -> pipe:[15847756]

    % stdhack ps
      PID TTY          TIME CMD
    11503 pts/13   00:00:01 bash
    19849 pts/13   00:00:00 stdhack
    19850 pts/13   00:00:00 ps

