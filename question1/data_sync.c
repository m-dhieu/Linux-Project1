#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

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

/* regular files in source dir vs destination */
int scan_and_compare(const char *source, const char *destination) {
    DIR *directory = opendir(source);
    struct dirent *entry;
    struct stat source_stat;
    struct stat destination_stat;
    int result = EXIT_SUCCESS;
    if (directory == NULL) {
        perror("Could not open source directory");
        return EXIT_FAILURE;
    }

    printf("Source directory: %s\n", source);
    printf("Destination directory: %s\n", destination);

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
        } else if (!S_ISREG(source_stat.st_mode)) {
            /* ignore dir & non-regular files */
        } else if (stat(destination_path, &destination_stat) != 0) {
            if (errno == ENOENT) {
                printf("SYNC: %s is missing from destination\n",
                       entry->d_name);
            } else {
                perror("Could not inspect destination entry");
                result = EXIT_FAILURE;
            }
        } else if (!S_ISREG(destination_stat.st_mode)) {
            printf("SYNC: %s has non-regular destination entry\n",
                   entry->d_name);
        } else {
            printf("OK: %s exists in both directories\n", entry->d_name);}

        free(source_path);
        free(destination_path);
    }

    if (closedir(directory) != 0) {
        perror("Could not close source directory");
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
    return scan_and_compare(argv[1], argv[2]);
}

