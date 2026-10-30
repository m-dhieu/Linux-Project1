# System Call Summary Table

This table summarizes some system calls observed during successful and failed backup.

| System Call | Purpose | Relevant Arguments | Return Value or Error | Role in the Program |
|---|---|---|---|---|
| `mkdir()` | Creates a directory | `"testdata/backup_success"`, mode `0755` | `0` | Creates backup directory. The failed experiment also returned `0` when creating its directory. |
| `openat()` | Opens or creates a file | Source: `O_RDONLY`; destination: `O_WRONLY \| O_CREAT \| O_TRUNC`; log: `O_WRONLY \| O_CREAT \| O_APPEND` | Log: `3`; source: `4`; destination: `5`. Missing source: `-1 ENOENT`. | Opens log and source files, and creates destination file. |
| `read()` | Reads data from a file descriptor | File descriptor `4`, buffer size `4096` | `140` bytes read; subsequent call returns `0` (end-of-file). | Reads source file during successful backup. |
| `write()` | Writes data to a file descriptor | File descriptor `5`, data buffer, `140` bytes | `140` bytes written successfully. Failed experiment also shows writes to standard error and the log. | Copies source content and records program messages in the log. |
| `fstat()` | Retrieves information about an open file | File descriptor `4` | `0` | Checks source-file information before copying. |
| `close()` | Closes an open file descriptor | File descriptors `3`, `4`, and `5` | `0` | Releases log, source, and destination file descriptors. |
| `clone()` | Creates a new process or thread | Child-process flags include `SIGCHLD` | Returns child PID `870` in the successful experiment and `889` in the failed experiment. | Implements child-process creation requested through `fork()` in the program. |
| `wait4()` | Waits for a child process and retrieves its status | Child PID `870` or `889` | Returns the corresponding child PID. Child status is `0` for success and `1` for failure. | Allows parent process to wait for and check the backup operation. |
| `exit_group()` | Terminates the calling process group | Exit status `0` or `1` | Displayed as `?` by `strace`; termination is confirmed by `+++ exited with 0 +++` or `+++ exited with 1 +++`. | Terminates child and parent processes with appropriate status. |

**Note:** Some system-call names in the trace differ from the POSIX functions used in the C source. For example, `fork()` is represented by `clone()` and `waitpid()` by `wait4()` on this Linux system.

