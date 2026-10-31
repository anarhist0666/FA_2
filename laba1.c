#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGS = 1,
    STATUS_MEMORY_ERROR = 2,
    STATUS_INVALID_FLAG = 3,
    STATUS_INVALID_SEED = 4
} status_code;

typedef struct {
    char flag;
    unsigned int seed;
    const char **strings;
    size_t string_count;
    char *result;
    size_t length;
} ParsedArgs;

status_code get_string_length(const char *const str, size_t *const length)
{
    if (str == NULL || length == NULL) {
        return STATUS_INVALID_ARGS;
    }

    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }

    *length = len;
    return STATUS_OK;
}

status_code reverse_string(const char *const str, char **const result)
{
    if (str == NULL || result == NULL) {
        return STATUS_INVALID_ARGS;
    }

    *result = NULL;

    size_t len = 0;
    status_code len_status = get_string_length(str, &len);
    if (len_status != STATUS_OK) {
        return len_status;
    }

    *result = (char *)malloc(len + 1);
    if (*result == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    for (size_t i = 0; i < len; i++) {
        (*result)[i] = str[len - 1 - i];
    }
    (*result)[len] = '\0';

    return STATUS_OK;
}

status_code uppercase_odd_positions(const char *const str, char **const result)
{
    if (str == NULL || result == NULL) {
        return STATUS_INVALID_ARGS;
    }

    *result = NULL;

    size_t len = 0;
    status_code len_status = get_string_length(str, &len);
    if (len_status != STATUS_OK) {
        return len_status;
    }

    *result = (char *)malloc(len + 1);
    if (*result == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    for (size_t i = 0; i < len; i++) {
        if (i % 2 == 1) {
            (*result)[i] = (char)toupper((unsigned char)str[i]);
        } else {
            (*result)[i] = str[i];
        }
    }
    (*result)[len] = '\0';

    return STATUS_OK;
}

status_code sort_by_char_type(const char *const str, char **const result)
{
    if (str == NULL || result == NULL) {
        return STATUS_INVALID_ARGS;
    }

    *result = NULL;

    size_t len = 0;
    status_code len_status = get_string_length(str, &len);
    if (len_status != STATUS_OK) {
        return len_status;
    }

    *result = (char *)malloc(len + 1);
    if (*result == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    size_t pos = 0;

    for (size_t i = 0; i < len; i++) {
        if (isdigit((unsigned char)str[i])) {
            (*result)[pos++] = str[i];
        }
    }

    for (size_t i = 0; i < len; i++) {
        if (isalpha((unsigned char)str[i])) {
            (*result)[pos++] = str[i];
        }
    }

    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)str[i]) && !isalpha((unsigned char)str[i])) {
            (*result)[pos++] = str[i];
        }
    }

    (*result)[len] = '\0';

    return STATUS_OK;
}

status_code concatenate_random(const char **const strings, const size_t count, 
                               const unsigned int seed, char **const result)
{
    if (strings == NULL || result == NULL || count == 0) {
        return STATUS_INVALID_ARGS;
    }

    *result = NULL;

    size_t total_len = 0;
    for (size_t i = 0; i < count; i++) {
        if (strings[i] == NULL) {
            return STATUS_INVALID_ARGS;
        }
        size_t len = 0;
        status_code len_status = get_string_length(strings[i], &len);
        if (len_status != STATUS_OK) {
            return len_status;
        }
        total_len += len;
    }

    *result = (char *)malloc(total_len + 1);
    if (*result == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    size_t *indices = (size_t *)malloc(count * sizeof(size_t));
    if (indices == NULL) {
        free(*result);
        *result = NULL;
        return STATUS_MEMORY_ERROR;
    }

    for (size_t i = 0; i < count; i++) {
        indices[i] = i;
    }

    srand(seed);

    for (size_t i = count - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        size_t temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    size_t pos = 0;
    for (size_t i = 0; i < count; i++) {
        size_t idx = indices[i];
        size_t str_len = 0;
        (void)get_string_length(strings[idx], &str_len);
        for (size_t j = 0; j < str_len; j++) {
            (*result)[pos++] = strings[idx][j];
        }
    }
    (*result)[pos] = '\0';

    free(indices);
    return STATUS_OK;
}

status_code validate_seed(const char *const seed_str, unsigned int *const seed)
{
    if (seed_str == NULL || seed == NULL) {
        return STATUS_INVALID_ARGS;
    }

    if (seed_str[0] == '-') {
        return STATUS_INVALID_SEED;
    }

    char *endptr = NULL;
    unsigned long seed_val = strtoul(seed_str, &endptr, 10);

    if (*endptr != '\0' || seed_val > UINT_MAX) {
        return STATUS_INVALID_SEED;
    }

    *seed = (unsigned int)seed_val;
    return STATUS_OK;
}

status_code validate_and_parse_args(const int argc, char *const argv[], 
                                     char *const flag,
                                     unsigned int *const seed,
                                     const char ***const strings,
                                     size_t *const string_count)
{
    if (argc < 3 || argv == NULL || flag == NULL) {
        return STATUS_INVALID_ARGS;
    }

    if (argv[1][0] != '-' || argv[1][2] != '\0') {
        return STATUS_INVALID_FLAG;
    }

    *flag = argv[1][1];

    if (*flag == 'l' || *flag == 'r' || *flag == 'u' || *flag == 'n') {
        if (argc != 3) {
            return STATUS_INVALID_ARGS;
        }
        *string_count = 1;
        *strings = (const char **)malloc(sizeof(char *));
        if (*strings == NULL) {
            return STATUS_MEMORY_ERROR;
        }
        (*strings)[0] = argv[2];
        return STATUS_OK;
    } else if (*flag == 'c') {
        if (argc < 5) {
            return STATUS_INVALID_ARGS;
        }

        status_code seed_status = validate_seed(argv[3], seed);
        if (seed_status != STATUS_OK) {
            return seed_status;
        }

        int total_strings = argc - 3;
        *strings = (const char **)malloc(total_strings * sizeof(char *));
        if (*strings == NULL) {
            return STATUS_MEMORY_ERROR;
        }

        (*strings)[0] = argv[2];
        for (int i = 1, arg_idx = 4; arg_idx < argc; i++, arg_idx++) {
            (*strings)[i] = argv[arg_idx];
        }
        *string_count = (size_t)total_strings;

        return STATUS_OK;
    }

    return STATUS_INVALID_FLAG;
}

status_code execute_operation(ParsedArgs *const args)
{
    if (args == NULL) {
        return STATUS_INVALID_ARGS;
    }

    status_code exec_status = STATUS_OK;

    switch (args->flag) {
        case 'l': {
            exec_status = get_string_length(args->strings[0], &args->length);
            break;
        }

        case 'r': {
            exec_status = reverse_string(args->strings[0], &args->result);
            break;
        }

        case 'u': {
            exec_status = uppercase_odd_positions(args->strings[0], &args->result);
            break;
        }

        case 'n': {
            exec_status = sort_by_char_type(args->strings[0], &args->result);
            break;
        }

        case 'c': {
            exec_status = concatenate_random(args->strings, args->string_count, 
                                            args->seed, &args->result);
            break;
        }

        default:
            exec_status = STATUS_INVALID_FLAG;
    }

    return exec_status;
}

void free_parsed_args(ParsedArgs *const args)
{
    if (args == NULL) {
        return;
    }

    if (args->strings != NULL) {
        free(args->strings);
        args->strings = NULL;
    }

    if (args->result != NULL) {
        free(args->result);
        args->result = NULL;
    }

    args->string_count = 0;
    args->length = 0;
}

void print_error(const status_code code)
{
    switch (code) {
        case STATUS_INVALID_ARGS:
            fprintf(stderr, "Error: invalid command line arguments\n");
            break;
        case STATUS_MEMORY_ERROR:
            fprintf(stderr, "Error: memory allocation failed\n");
            break;
        case STATUS_INVALID_FLAG:
            fprintf(stderr, "Error: invalid flag. Use: -l, -r, -u, -n, -c\n");
            break;
        case STATUS_INVALID_SEED:
            fprintf(stderr, "Error: invalid seed value\n");
            break;
        case STATUS_OK:
            break;
    }
}

int main(const int argc, char *const argv[])
{
    ParsedArgs args = {
        .flag = '\0',
        .seed = 0,
        .strings = NULL,
        .string_count = 0,
        .result = NULL,
        .length = 0
    };

    status_code val_status = validate_and_parse_args(argc, argv, &args.flag, 
                                                     &args.seed, &args.strings, 
                                                     &args.string_count);
    if (val_status != STATUS_OK) {
        print_error(val_status);
        free_parsed_args(&args);
        return (int)val_status;
    }

    status_code exec_status = execute_operation(&args);
    if (exec_status != STATUS_OK) {
        print_error(exec_status);
        free_parsed_args(&args);
        return (int)exec_status;
    }

    switch (args.flag) {
        case 'l': {
            printf("Length: %zu\n", args.length);
            break;
        }

        case 'r': {
            printf("Reversed: %s\n", args.result);
            break;
        }

        case 'u': {
            printf("Uppercase odd positions: %s\n", args.result);
            break;
        }

        case 'n': {
            printf("Sorted by type: %s\n", args.result);
            break;
        }

        case 'c': {
            printf("Random concatenated: %s\n", args.result);
            break;
        }

        default:
            break;
    }

    free_parsed_args(&args);
    return (int)STATUS_OK;
}