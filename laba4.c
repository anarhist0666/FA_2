#include <stdio.h>
#include <stdlib.h>

#define MAX_FILENAME_LEN 256
#define MAX_FILES 1000
#define MAX_CAPACITY 1000000

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_MEMORY_ERROR = 2,
    STATUS_INPUT_ERROR = 3
} status_code;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    unsigned long size;
    unsigned long importance;
} File;

typedef struct {
    char **filenames;
    size_t count;
    unsigned long total_size;
    unsigned long total_importance;
    status_code status;
} BackupResult;

void free_backup_result(BackupResult *const result)
{
    if (result != NULL) {
        if (result->filenames != NULL) {
            for (size_t i = 0; i < result->count; i++) {
                free(result->filenames[i]);
            }
            free(result->filenames);
        }
        result->filenames = NULL;
        result->count = 0;
        result->total_size = 0;
        result->total_importance = 0;
    }
}

size_t get_string_length(const char *const str)
{
    if (str == NULL) {
        return 0;
    }

    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }

    return len;
}

char *allocate_and_copy_string(const char *const str)
{
    if (str == NULL) {
        return NULL;
    }

    size_t len = get_string_length(str);
    char *copy = (char *)malloc(len + 1);

    if (copy == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        copy[i] = str[i];
    }
    copy[len] = '\0';

    return copy;
}

status_code read_input(File **const files, size_t *const file_count, unsigned long *const capacity)
{
    if (files == NULL || file_count == NULL || capacity == NULL) {
        return STATUS_INVALID_ARGS;
    }

    unsigned long n = 0;

    if (scanf("%lu %lu", &n, capacity) != 2) {
        return STATUS_INPUT_ERROR;
    }

    if (n <= 0 || n > MAX_FILES) {
        return STATUS_INPUT_ERROR;
    }

    if (*capacity <= 0 || *capacity > MAX_CAPACITY) {
        return STATUS_INPUT_ERROR;
    }

    *file_count = (size_t)n;
    *files = (File *)malloc(*file_count * sizeof(File));

    if (*files == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    for (size_t i = 0; i < *file_count; i++) {
        char filename[MAX_FILENAME_LEN];
        unsigned long size = 0;
        unsigned long importance = 0;

        if (scanf("%255s %lu %lu", filename, &size, &importance) != 3) {
            free(*files);
            *files = NULL;
            return STATUS_INPUT_ERROR;
        }

        if (size == 0) {
            free(*files);
            *files = NULL;
            return STATUS_INPUT_ERROR;
        }

        if (get_string_length(filename) == 0) {
            free(*files);
            *files = NULL;
            return STATUS_INPUT_ERROR;
        }

        size_t j;
        for (j = 0; j < MAX_FILENAME_LEN - 1 && filename[j] != '\0'; j++) {
            (*files)[i].filename[j] = filename[j];
        }
        (*files)[i].filename[j] = '\0';

        (*files)[i].size = size;
        (*files)[i].importance = importance;
    }

    return STATUS_OK;
}

BackupResult select_files_for_backup(const File *const files, const size_t file_count, 
                                     const unsigned long capacity)
{
    BackupResult result;
    result.filenames = NULL;
    result.count = 0;
    result.total_size = 0;
    result.total_importance = 0;
    result.status = STATUS_OK;

    if (file_count == 0 || files == NULL || capacity == 0) {
        return result;
    }

    unsigned long **dp = (unsigned long **)malloc((file_count + 1) * sizeof(unsigned long *));
    if (dp == NULL) {
        result.status = STATUS_MEMORY_ERROR;
        return result;
    }

    for (size_t i = 0; i <= file_count; i++) {
        dp[i] = (unsigned long *)malloc((capacity + 1) * sizeof(unsigned long));
        if (dp[i] == NULL) {
            for (size_t j = 0; j < i; j++) {
                free(dp[j]);
            }
            free(dp);
            result.status = STATUS_MEMORY_ERROR;
            return result;
        }
    }

    for (size_t i = 0; i <= file_count; i++) {
        for (unsigned long w = 0; w <= capacity; w++) {
            dp[i][w] = 0;
        }
    }

    for (size_t i = 1; i <= file_count; i++) {
        for (unsigned long w = 0; w <= capacity; w++) {
            dp[i][w] = dp[i - 1][w];

            if (files[i - 1].size > 0 && w >= files[i - 1].size) {
                unsigned long with_file = dp[i - 1][w - files[i - 1].size] + files[i - 1].importance;
                if (with_file > dp[i][w]) {
                    dp[i][w] = with_file;
                }
            }
        }
    }

    int *selected = (int *)malloc(file_count * sizeof(int));
    if (selected == NULL) {
        for (size_t i = 0; i <= file_count; i++) {
            free(dp[i]);
        }
        free(dp);
        result.status = STATUS_MEMORY_ERROR;
        return result;
    }

    for (size_t i = 0; i < file_count; i++) {
        selected[i] = 0;
    }

    size_t i = file_count;
    unsigned long w = capacity;

    while (i > 0) {
        if (dp[i][w] != dp[i - 1][w]) {
            selected[i - 1] = 1;
            w -= files[i - 1].size;
        }
        i--;
    }

    for (size_t i = 0; i <= file_count; i++) {
        free(dp[i]);
    }
    free(dp);

    size_t selected_count = 0;
    for (size_t i = 0; i < file_count; i++) {
        if (selected[i]) {
            selected_count++;
        }
    }

    if (selected_count > 0) {
        result.filenames = (char **)malloc(selected_count * sizeof(char *));
        if (result.filenames == NULL) {
            free(selected);
            result.status = STATUS_MEMORY_ERROR;
            return result;
        }

        for (size_t i = 0; i < file_count; i++) {
            if (selected[i]) {
                char *copy = allocate_and_copy_string(files[i].filename);
                if (copy == NULL) {
                    free_backup_result(&result);
                    result.filenames = NULL;
                    result.count = 0;
                    free(selected);
                    result.status = STATUS_MEMORY_ERROR;
                    return result;
                }
                result.filenames[result.count] = copy;
                result.total_size += files[i].size;
                result.total_importance += files[i].importance;
                result.count++;
            }
        }
    } else {
        result.filenames = NULL;
    }

    free(selected);

    return result;
}

void print_error(const status_code code)
{
    switch (code) {
        case STATUS_INVALID_ARGS:
            fprintf(stderr, "Error: invalid arguments\n");
            break;
        case STATUS_MEMORY_ERROR:
            fprintf(stderr, "Error: memory allocation failed\n");
            break;
        case STATUS_INPUT_ERROR:
            fprintf(stderr, "Error: invalid input format\n");
            break;
        case STATUS_OK:
            break;
    }
}

int main(void)
{
    File *files = NULL;
    size_t file_count = 0;
    unsigned long capacity = 0;

    status_code read_status = read_input(&files, &file_count, &capacity);
    if (read_status != STATUS_OK) {
        print_error(read_status);
        return (int)read_status;
    }

    BackupResult result = select_files_for_backup(files, file_count, capacity);

    if (result.status != STATUS_OK) {
        print_error(result.status);
        free_backup_result(&result);
        free(files);
        return (int)result.status;
    }

    printf("TOTAL_SIZE %lu\n", result.total_size);
    printf("TOTAL_IMPORTANCE %lu\n", result.total_importance);
    printf("FILES %zu\n", result.count);

    for (size_t i = 0; i < result.count; i++) {
        printf("%s\n", result.filenames[i]);
    }

    free_backup_result(&result);
    free(files);

    return (int)STATUS_OK;
}