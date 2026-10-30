#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>

/* file path */
static char *join_paths(const char *dir, const char *name) {
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    if (name_len > SIZE_MAX - 2 ||
        dir_len > SIZE_MAX - name_len - 2) {return NULL;}

    size_t path_len = dir_len + name_len + 2;
    char *path = malloc(path_len);
    if (path == NULL) {return NULL;}

    int written = snprintf(path, path_len, "%s/%s", dir, name);
    if (written < 0 || (size_t)written >= path_len) {
        free(path);
        return NULL;
    }

    return path;
}

/* timestamped audit/log */
int log_event(const char *message) {
    FILE *log_file = fopen("sync_audit.log", "a");
    if (log_file == NULL) {
        perror("Could not open audit log");
        return -1;
    }

    time_t now = time(NULL);
    struct tm local_time;
    char timestamp[32];
    if (now != (time_t)-1 &&
        localtime_r(&now, &local_time) != NULL &&
        strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", &local_time) > 0) {
    } else {snprintf(timestamp, sizeof(timestamp), "time unavailable");}

    int written = fprintf(log_file, "[%s] %s\n", timestamp, message);
    int close_result = fclose(log_file);
    if (written < 0 || close_result != 0) {
        fprintf(stderr, "Could not write audit log\n");
        return -1;
    }

    return 0;
}

/* calculate file contents for comparison */
int calculate_checksum(const char *path, uint64_t *result) {
    FILE *file = fopen(path, "rb");
    unsigned char buffer[4096];
    size_t bytes_read;
    if (file == NULL) {return -1;}

    uint64_t hash = UINT64_C(14695981039346656037);
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            hash ^= buffer[i];
            hash *= UINT64_C(1099511628211);
        }
    }

    if (ferror(file)) {
        fclose(file);
        return -1;
    }

    if (fclose(file) != 0) {return -1;}

    *result = hash;
    return 0;
}

/* compare & log findings */
int deep_scan(const char *source, const char *destination) {
    DIR *directory = opendir(source);
    struct dirent *entry;
    struct stat source_stat;
    struct stat destination_stat;
    int result = EXIT_SUCCESS;
    if (directory == NULL) {
        perror("Could not open source directory");
        log_event("ERROR: Could not open source directory");
        return EXIT_FAILURE;
    }

    printf("V2: Source directory: %s\n", source);
    printf("V2: Destination directory: %s\n", destination);

    if (log_event("Started metadata and checksum comparison") != 0) {
        result = EXIT_FAILURE;
    }

    while (1) {
        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                perror("Error reading source directory");
                result = EXIT_FAILURE;
            }
            break;
        }

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {continue;}

        char *source_path = join_paths(source, entry->d_name);
        char *destination_path = join_paths(destination, entry->d_name);
        if (source_path == NULL || destination_path == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            free(source_path);
            free(destination_path);
            result = EXIT_FAILURE;
            continue;
        }

        if (stat(source_path, &source_stat) != 0) {
            perror("Could not inspect source entry");
            result = EXIT_FAILURE;
            goto cleanup;
        }

        if (!S_ISREG(source_stat.st_mode)) {goto cleanup;}
        if (stat(destination_path, &destination_stat) != 0) {
            if (errno == ENOENT) {
                printf("SYNC: %s is missing from destination\n",
			 entry->d_name);
                char message[1024];
                snprintf(message, sizeof(message),
                         "Missing destination file: %s",
                         entry->d_name);
                if (log_event(message) != 0) {result = EXIT_FAILURE;}
            } else {
                perror("Could not inspect destination entry");
                result = EXIT_FAILURE;
            }
            goto cleanup;
        }

        if (!S_ISREG(destination_stat.st_mode)) {
            printf("SYNC: %s has non-regular destination entry\n",
	    entry->d_name);
            goto cleanup;
        }

        if (source_stat.st_size != destination_stat.st_size) {
            printf("SYNC: %s has different file sizes\n", entry->d_name);
            char message[1024];
            snprintf(message, sizeof(message),
                     "Size mismatch: %s", entry->d_name);
            if (log_event(message) != 0) {result = EXIT_FAILURE;}
            goto cleanup;
        }

        uint64_t source_hash;
        uint64_t destination_hash;
        if (calculate_checksum(source_path, &source_hash) != 0 ||
            calculate_checksum(destination_path, &destination_hash) != 0) {
            fprintf(stderr, "Checksum calculation failed: %s\n",
                    entry->d_name);
            if (log_event("ERROR: Checksum calculation failed") != 0) {
                result = EXIT_FAILURE;
            }
            result = EXIT_FAILURE;
            goto cleanup;
        }

        char message[1024];
        if (source_hash != destination_hash) {
            printf("SYNC: %s has different content\n", entry->d_name);
            snprintf(message, sizeof(message),
                     "Checksum mismatch: %s", entry->d_name);
        } else {
            printf("OK: %s matches. Size: %jd bytes, "
                   "Checksum: %016" PRIx64 "\n",
                   entry->d_name,
                   (intmax_t)source_stat.st_size,
                   source_hash);
            snprintf(message, sizeof(message),
                     "Verified: %s | Size: %jd | Hash: %016"
                     PRIx64,
                     entry->d_name,
                     (intmax_t)source_stat.st_size,
                     source_hash);
        }

        if (log_event(message) != 0) {result = EXIT_FAILURE;}

        cleanup:
        free(source_path);
        free(destination_path);
    }

    if (closedir(directory) != 0) {
        perror("Could not close source directory");
        result = EXIT_FAILURE;
    }

    if (log_event("Metadata and checksum comparison completed") != 0) {
        result = EXIT_FAILURE;
    }

    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <source_directory> <destination_directory>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    return deep_scan(argv[1], argv[2]);
}

