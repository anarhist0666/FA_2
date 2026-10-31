#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#define INITIAL_FILE_CAPACITY 5
#define INITIAL_MATCH_CAPACITY 10
#define MAX_FILES 10

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_FILE_ERROR = 2,
    STATUS_MEMORY_ERROR = 3
} status_code;

typedef struct {
    size_t line_number;
    size_t char_position;
} Match;

typedef struct {
    Match *data;
    size_t count;
    size_t capacity;
} MatchArray;

typedef struct {
    char *filename;
    MatchArray matches;
} FileResult;

typedef struct {
    FileResult *data;
    size_t count;
    size_t capacity;
} SearchResults;

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

status_code init_match_array(MatchArray *const arr, const size_t initial_capacity)
{
    if (arr == NULL) {
        return STATUS_INVALID_ARGS;
    }

    size_t capacity = initial_capacity;
    if (capacity == 0) {
        capacity = INITIAL_MATCH_CAPACITY;
    }

    arr->data = (Match *)malloc(capacity * sizeof(Match));
    if (arr->data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    arr->count = 0;
    arr->capacity = capacity;

    return STATUS_OK;
}

void free_match_array(MatchArray *const arr)
{
    if (arr != NULL) {
        free(arr->data);
        arr->data = NULL;
        arr->count = 0;
        arr->capacity = 0;
    }
}

status_code expand_match_array(MatchArray *const arr)
{
    if (arr == NULL) {
        return STATUS_INVALID_ARGS;
    }

    size_t new_capacity = arr->capacity * 2;
    Match *new_data = (Match *)realloc(arr->data, new_capacity * sizeof(Match));

    if (new_data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    arr->data = new_data;
    arr->capacity = new_capacity;

    return STATUS_OK;
}

status_code add_match(MatchArray *const arr, const size_t line_number, const size_t char_position)
{
    if (arr == NULL) {
        return STATUS_INVALID_ARGS;
    }

    if (arr->count >= arr->capacity) {
        status_code expand_status = expand_match_array(arr);
        if (expand_status != STATUS_OK) {
            return expand_status;
        }
    }

    arr->data[arr->count].line_number = line_number;
    arr->data[arr->count].char_position = char_position;
    arr->count++;

    return STATUS_OK;
}

status_code init_search_results(SearchResults *const results, const size_t initial_capacity)
{
    if (results == NULL) {
        return STATUS_INVALID_ARGS;
    }

    size_t capacity = initial_capacity;
    if (capacity == 0) {
        capacity = INITIAL_FILE_CAPACITY;
    }

    results->data = (FileResult *)malloc(capacity * sizeof(FileResult));
    if (results->data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    results->count = 0;
    results->capacity = capacity;

    return STATUS_OK;
}

void free_search_results(SearchResults *const results)
{
    if (results != NULL) {
        for (size_t i = 0; i < results->count; i++) {
            free(results->data[i].filename);
            free_match_array(&results->data[i].matches);
        }
        free(results->data);
        results->data = NULL;
        results->count = 0;
        results->capacity = 0;
    }
}

status_code expand_search_results(SearchResults *const results)
{
    if (results == NULL) {
        return STATUS_INVALID_ARGS;
    }

    size_t new_capacity = results->capacity * 2;
    FileResult *new_data = (FileResult *)realloc(results->data, new_capacity * sizeof(FileResult));

    if (new_data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    results->data = new_data;
    results->capacity = new_capacity;

    return STATUS_OK;
}

char *read_entire_file(const char *const filename, size_t *const out_len)
{
    if (filename == NULL || out_len == NULL) {
        return NULL;
    }

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long fsize = ftell(file);
    if (fsize < 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *string = (char *)malloc((size_t)fsize + 1);
    if (string == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(string, 1, (size_t)fsize, file);
    if (bytes_read != (size_t)fsize) {
        free(string);
        fclose(file);
        return NULL;
    }

    string[fsize] = '\0';
    *out_len = (size_t)fsize;

    if (fclose(file) == EOF) {
        free(string);
        return NULL;
    }

    return string;
}

int substring_compare(const char *const text, const size_t text_pos,
                      const char *const substring, const size_t sub_len)
{
    if (text == NULL || substring == NULL) {
        return 0;
    }

    for (size_t i = 0; i < sub_len; i++) {
        if (text[text_pos + i] != substring[i]) {
            return 0;
        }
    }

    return 1;
}

status_code search_in_file(const char *const filename,
                           const char *const substring,
                           FileResult *const result)
{
    if (filename == NULL || substring == NULL || result == NULL) {
        return STATUS_INVALID_ARGS;
    }

    result->filename = allocate_and_copy_string(filename);
    if (result->filename == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    status_code init_status = init_match_array(&result->matches, INITIAL_MATCH_CAPACITY);
    if (init_status != STATUS_OK) {
        free(result->filename);
        result->filename = NULL;
        return init_status;
    }

    size_t text_len = 0;
    char *text = read_entire_file(filename, &text_len);
    if (text == NULL) {
        free(result->filename);
        result->filename = NULL;
        free_match_array(&result->matches);
        return STATUS_FILE_ERROR;
    }

    size_t sub_len = get_string_length(substring);

    if (sub_len == 0 || sub_len > text_len) {
        free(text);
        return STATUS_OK;
    }

    size_t line_number = 1;
    size_t char_in_line = 1;

    for (size_t i = 0; i <= text_len - sub_len; i++) {
        if (substring_compare(text, i, substring, sub_len)) {
            status_code match_status = add_match(&result->matches, line_number, char_in_line);
            if (match_status != STATUS_OK) {
                free(text);
                free(result->filename);
                result->filename = NULL;
                free_match_array(&result->matches);
                return match_status;
            }
        }

        if (text[i] == '\n') {
            line_number++;
            char_in_line = 1;
        } else {
            char_in_line++;
        }
    }

    free(text);
    return STATUS_OK;
}

status_code search_substring(const char *const substring, SearchResults *const results,
                             const int file_count, ...)
{
    if (substring == NULL || results == NULL || file_count <= 0) {
        return STATUS_INVALID_ARGS;
    }

    va_list args;
    va_start(args, file_count);

    for (int i = 0; i < file_count; i++) {
        const char *filename = va_arg(args, const char *);

        if (filename == NULL) {
            va_end(args);
            return STATUS_INVALID_ARGS;
        }

        if (results->count >= results->capacity) {
            status_code expand_status = expand_search_results(results);
            if (expand_status != STATUS_OK) {
                va_end(args);
                return expand_status;
            }
        }

        status_code search_status = search_in_file(filename, substring,
                                                    &results->data[results->count]);
        if (search_status == STATUS_OK) {
            results->count++;
        } else if (search_status != STATUS_FILE_ERROR) {
            va_end(args);
            return search_status;
        }
    }

    va_end(args);
    return STATUS_OK;
}

void print_search_results(const SearchResults *const results, const char *const substring)
{
    if (results == NULL || substring == NULL) {
        return;
    }

    printf("Search Results for substring: \"");
    for (size_t i = 0; substring[i] != '\0'; i++) {
        if (substring[i] == '\n') {
            printf("\\n");
        } else if (substring[i] == '\r') {
            printf("\\r");
        } else if (substring[i] == '\t') {
            printf("\\t");
        } else if (substring[i] == ' ') {
            printf("<space>");
        } else {
            printf("%c", substring[i]);
        }
    }
    printf("\"\n");

    for (size_t i = 0; i < results->count; i++) {
        printf("File: %s\n", results->data[i].filename);

        if (results->data[i].matches.count == 0) {
            printf("  No matches found\n");
        } else {
            printf("  Matches found: %zu\n", results->data[i].matches.count);
            for (size_t j = 0; j < results->data[i].matches.count; j++) {
                printf("    Line %zu, Position %zu\n",
                       results->data[i].matches.data[j].line_number,
                       results->data[i].matches.data[j].char_position);
            }
        }
    }
}

void print_error(const status_code code)
{
    switch (code) {
        case STATUS_INVALID_ARGS:
            fprintf(stderr, "Error: invalid arguments\n");
            break;
        case STATUS_FILE_ERROR:
            fprintf(stderr, "Error: file operation failed\n");
            break;
        case STATUS_MEMORY_ERROR:
            fprintf(stderr, "Error: memory allocation failed\n");
            break;
        case STATUS_OK:
            break;
    }
}

status_code validate_and_parse_args(const int argc, char *argv[],
                                     const char **const out_substring,
                                     int *const out_file_count,
                                     const char *const **const out_filenames)
{
    if (argc < 4) {
        return STATUS_INVALID_ARGS;
    }

    *out_substring = argv[1];
    if (*out_substring == NULL) {
        return STATUS_INVALID_ARGS;
    }

    char *endptr = NULL;
    long file_count_long = strtol(argv[2], &endptr, 10);

    if (endptr == argv[2] || *endptr != '\0') {
        return STATUS_INVALID_ARGS;
    }

    if (file_count_long <= 0 || file_count_long > MAX_FILES) {
        return STATUS_INVALID_ARGS;
    }

    if (argc - 3 != (int)file_count_long) {
        return STATUS_INVALID_ARGS;
    }

    *out_file_count = (int)file_count_long;
    *out_filenames = (const char *const *)argv + 3;

    return STATUS_OK;
}

status_code execute_search(const char *const substring, const int file_count,
                           const char *const *const filenames,
                           SearchResults *const results)
{
    if (substring == NULL || file_count <= 0 || file_count > MAX_FILES || 
        filenames == NULL || results == NULL) {
        return STATUS_INVALID_ARGS;
    }

    status_code status = STATUS_INVALID_ARGS;

    switch (file_count) {
        case 1:
            status = search_substring(substring, results, 1, filenames[0]);
            break;
        case 2:
            status = search_substring(substring, results, 2, filenames[0], filenames[1]);
            break;
        case 3:
            status = search_substring(substring, results, 3, filenames[0], filenames[1], filenames[2]);
            break;
        case 4:
            status = search_substring(substring, results, 4, filenames[0], filenames[1], filenames[2], filenames[3]);
            break;
        case 5:
            status = search_substring(substring, results, 5, filenames[0], filenames[1], filenames[2], filenames[3], filenames[4]);
            break;
        case 6:
            status = search_substring(substring, results, 6, filenames[0], filenames[1], filenames[2], filenames[3], filenames[4], filenames[5]);
            break;
        case 7:
            status = search_substring(substring, results, 7, filenames[0], filenames[1], filenames[2], filenames[3], filenames[4], filenames[5], filenames[6]);
            break;
        case 8:
            status = search_substring(substring, results, 8, filenames[0], filenames[1], filenames[2], filenames[3], filenames[4], filenames[5], filenames[6], filenames[7]);
            break;
        case 9:
            status = search_substring(substring, results, 9, filenames[0], filenames[1], filenames[2], filenames[3], filenames[4], filenames[5], filenames[6], filenames[7], filenames[8]);
            break;
        case 10:
            status = search_substring(substring, results, 10, filenames[0], filenames[1], filenames[2], filenames[3], filenames[4], filenames[5], filenames[6], filenames[7], filenames[8], filenames[9]);
            break;
        default:
            return STATUS_INVALID_ARGS;
    }

    return status;
}

int main(int argc, char *argv[])
{
    const char *substring = NULL;
    int file_count = 0;
    const char *const *filenames = NULL;

    status_code parse_status = validate_and_parse_args(argc, argv, &substring, &file_count, &filenames);
    if (parse_status != STATUS_OK) {
        print_error(parse_status);
        return (int)parse_status;
    }

    SearchResults results = {NULL, 0, 0};
    status_code init_status = init_search_results(&results, (size_t)file_count);
    if (init_status != STATUS_OK) {
        print_error(init_status);
        return (int)init_status;
    }

    status_code exec_status = execute_search(substring, file_count, filenames, &results);
    if (exec_status != STATUS_OK) {
        print_error(exec_status);
        free_search_results(&results);
        return (int)exec_status;
    }

    print_search_results(&results, substring);
    free_search_results(&results);

    return (int)STATUS_OK;
}