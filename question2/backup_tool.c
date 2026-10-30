#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#define BUFFER_SIZE 4096

/* accounting for partial writes & interruptions */
int write_all(int fd, const char *buffer, size_t length) {
    size_t total = 0;
    while (total < length) {
        ssize_t n = write(fd, buffer + total, length - total);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }
        total += (size_t)n;
    }

    return 0;
}

/* log message */
void write_log(int fd, const char *message) {
    char entry[512];
    char timestamp[64];
    time_t now = time(NULL);
    struct tm tm_info;
    if (localtime_r(&now, &tm_info) != NULL)
        strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", &tm_info);
    else
        snprintf(timestamp, sizeof(timestamp), "UNKNOWN_TIME");

    int len = snprintf(entry, sizeof(entry),
                       "[%s] [PID:%d] %s\n",
                       timestamp, (int)getpid(), message);
    if (len < 0 || (size_t)len >= sizeof(entry)) {
        fprintf(stderr, "Log entry formatting failed\n");
        return;
    }
    if (write_all(fd, entry, (size_t)len) == -1)
        perror("Log write failed");
}

int copy_file(const char *source, const char *destination, int log_fd) {
    int src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        fprintf(stderr, "Cannot open source: %s\n", strerror(errno));
        write_log(log_fd, "ERROR: Source file couldn't be opened");
        return EXIT_FAILURE;
    }

    struct stat file_info;
    if (fstat(src_fd, &file_info) == -1 ||
        !S_ISREG(file_info.st_mode)) {
        fprintf(stderr, "Source invalid or can't be inspected\n");
        write_log(log_fd, "ERROR: Invalid source file");
        close(src_fd);
        return EXIT_FAILURE;
    }

    int dest_fd = open(destination,
                       O_WRONLY | O_CREAT | O_TRUNC,
                       file_info.st_mode & 0777);
    if (dest_fd == -1) {
        fprintf(stderr, "Cannot create destination: %s\n", strerror(errno));
        write_log(log_fd, "ERROR: Destination file creation failed");
        close(src_fd);
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int result = EXIT_SUCCESS;
    while (1) {
        bytes_read = read(src_fd, buffer, sizeof(buffer));
        if (bytes_read == 0) break;
        if (bytes_read == -1) {
            if (errno == EINTR) continue;
            perror("Source read failed");
            write_log(log_fd, "ERROR: Source file read failed");
            result = EXIT_FAILURE;
            break;
        }
        if (write_all(dest_fd, buffer, (size_t)bytes_read) == -1) {
            perror("Destination write failed");
            write_log(log_fd, "ERROR: Destination file write failed");
            result = EXIT_FAILURE;
            break;
        }
    }

    if (close(src_fd) == -1) {
        perror("Source close failed");
        result = EXIT_FAILURE;
    }
    if (close(dest_fd) == -1) {
        perror("Destination close failed");
        result = EXIT_FAILURE;
    }
    if (result == EXIT_SUCCESS)
        write_log(log_fd, "File copied successfully :)");

    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <backup_directory>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    const char *source = argv[1];
    const char *directory = argv[2];
    char log_path[4096];
    char destination[4096];
    const char *filename = strrchr(source, '/');
    filename = filename ? filename + 1 : source;
    if (*filename == '\0') {
        fprintf(stderr, "Invalid source filename\n");
        return EXIT_FAILURE;
    }

    int n = snprintf(log_path, sizeof(log_path), "%s/backup.log", directory);
    if (n < 0 || (size_t)n >= sizeof(log_path)) {
        fprintf(stderr, "Log path is too long\n");
        return EXIT_FAILURE;
    }

    n = snprintf(destination, sizeof(destination),
                 "%s/%s.bak", directory, filename);
    if (n < 0 || (size_t)n >= sizeof(destination)) {
        fprintf(stderr, "Destination path is too long\n");
        return EXIT_FAILURE;
    }

    /* create backup dir if it doesn't exist */
    if (mkdir(directory, 0755) == -1) {
        if (errno != EEXIST) {
            perror("Directory creation failed");
            return EXIT_FAILURE;
        }
        struct stat info;
        if (stat(directory, &info) == -1 || !S_ISDIR(info.st_mode)) {
            fprintf(stderr, "Backup path is not a directory\n");
            return EXIT_FAILURE;
        }
    }

    printf("Backup directory ready: %s\n", directory);

    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Log file creation failed");
        return EXIT_FAILURE;
    }

    write_log(log_fd, "Backup initialized");

    /* child to perform backup */
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        close(log_fd);
        return EXIT_FAILURE;
    }
    if (pid == 0) {
        write_log(log_fd, "Child process started backup");
        int result = copy_file(source, destination, log_fd);
        close(log_fd);
        _exit(result);
    }

    int status;
    pid_t waited;
    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid failed");
        close(log_fd);
        return EXIT_FAILURE;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
        printf("Backup complete\n");
        write_log(log_fd, "Backup complete *_*");
    } else {
        printf("Backup failed\n");
        write_log(log_fd, "Backup failed in child process");
    }

    close(log_fd);

    return (WIFEXITED(status) &&
            WEXITSTATUS(status) == EXIT_SUCCESS)
           ? EXIT_SUCCESS : EXIT_FAILURE;
}

